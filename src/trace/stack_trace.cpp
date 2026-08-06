// -----------------------------------------------------------------------------
//  Copyright (c) 2026 @DMsuDev. Licensed under the MIT License.
//  See LICENSE file in the project root for full license text.
// -----------------------------------------------------------------------------

#include "stack_trace.h"

#include "vigil/detail/platform_detection.h"
#include "vigil/detail/symbol_utils.h"

#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cstdint>

#if defined(VIGIL_ENABLE_STACK_TRACE)
    #if defined(VIGIL_PLATFORM_WINDOWS)
        #include <Windows.h>
        #include <DbgHelp.h>
        #include <mutex>
    #else
        #include <execinfo.h>
        #include <cxxabi.h>
        #include <string_view>
        #include <cstdlib>
        #include <memory>
    #endif
#endif

namespace vigil {

namespace {

constexpr std::size_t kPointerWidth = sizeof(void*) * 2;

/// @brief Formats a memory address into a standardized hexadecimal string representation.
[[nodiscard]] std::string FormatAddress(std::uintptr_t address)
{
    std::ostringstream oss;

    oss << "0x"
        << std::uppercase
        << std::hex
        << std::setw(static_cast<int>(kPointerWidth))
        << std::setfill('0')
        << address;

    return oss.str();
}

/// @brief Checks if a symbol belongs to the standard C/C++ runtime entry point frames.
[[nodiscard]] bool IsRuntimeFrame(std::string_view symbol)
{
    return symbol == "invoke_main" ||
           symbol == "__scrt_common_main" ||
           symbol == "__scrt_common_main_seh" ||
           symbol == "mainCRTStartup" ||
           symbol == "BaseThreadInitThunk" ||
           symbol == "RtlUserThreadStart" ||
           symbol == "start" ||
           symbol == "_start" ||
           symbol == "__libc_start_main";
}

#if defined(VIGIL_ENABLE_STACK_TRACE) && !defined(VIGIL_PLATFORM_WINDOWS)

/// @brief Demangles an Itanium-ABI mangled C++ symbol name; falls back to the
///        raw mangled string if demangling fails or the name is not mangled.
[[nodiscard]] std::string Demangle(const std::string& mangled)
{
    if (mangled.empty())
        return mangled;

    int status = 0;
    std::unique_ptr<char, decltype(&free)> demangled(
        abi::__cxa_demangle(mangled.c_str(), nullptr, nullptr, &status), &free);

    return (status == 0 && demangled) ? std::string(demangled.get()) : mangled;
}

#if defined(VIGIL_PLATFORM_MACOS)

/// @brief Parses one line of macOS backtrace_symbols() output.
/// Format: "<idx>  <module>  0x<address>  <symbol> + <offset>"
/// (no parentheses, unlike the glibc format used on Linux).
[[nodiscard]] std::string ParseSymbolMacOS(std::string_view raw, std::uintptr_t address)
{
    // Locate the address token, then take everything after it as "symbol + offset".
    const auto addrHex = FormatAddress(address);
    auto afterAddr = raw.find("0x");
    if (afterAddr == std::string_view::npos)
        return std::string(addrHex);

    auto rest = raw.substr(afterAddr);
    auto spacePos = rest.find(' ');
    if (spacePos == std::string_view::npos)
        return std::string(addrHex);
    rest = rest.substr(spacePos + 1);

    // Trim leading whitespace and strip the trailing " + <offset>".
    while (!rest.empty() && rest.front() == ' ')
        rest.remove_prefix(1);

    auto plusPos = rest.rfind(" + ");
    std::string symbolPart(plusPos != std::string_view::npos ? rest.substr(0, plusPos) : rest);

    return symbolPart.empty() ? std::string(addrHex) : Demangle(symbolPart);
}

#else // Linux / other glibc-style POSIX

/// @brief Parses one line of glibc backtrace_symbols() output.
/// Format: "module(mangled_symbol+offset) [address]"
[[nodiscard]] std::string ParseSymbolLinux(std::string_view raw, std::uintptr_t address)
{
    const auto parenPos = raw.find('(');
    const auto plusPos  = raw.find('+', parenPos);

    if (parenPos == std::string_view::npos ||
        plusPos  == std::string_view::npos ||
        plusPos  <= parenPos)
        return FormatAddress(address);

    std::string mangled(raw.substr(parenPos + 1, plusPos - parenPos - 1));
    return mangled.empty() ? FormatAddress(address) : Demangle(mangled);
}

#endif // VIGIL_PLATFORM_MACOS

#endif // VIGIL_ENABLE_STACK_TRACE && !VIGIL_PLATFORM_WINDOWS

} // namespace

#if defined(VIGIL_ENABLE_STACK_TRACE)

#if defined(VIGIL_PLATFORM_WINDOWS)

//==============================================================================
// Windows implementation (DbgHelp)
//==============================================================================

static std::mutex s_dbgHelpMutex;
static std::once_flag s_dbgHelpInit;
static bool s_dbgHelpInitialized = false;

std::vector<StackFrame> StackTrace::Capture(unsigned framesToSkip, unsigned maxFrames)
{
    std::vector<StackFrame> frames;
    frames.reserve(maxFrames);

    HANDLE process = GetCurrentProcess();

    // Initializes the DbgHelp symbol engine exactly once per process execution.
    std::call_once(s_dbgHelpInit, [process]() {
        SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_UNDNAME);
        s_dbgHelpInitialized = SymInitialize(process, nullptr, TRUE) != FALSE;
    });

    if (!s_dbgHelpInitialized) {
        return frames;
    }

    // RtlCaptureStackBackTrace cannot return more than 62 frames on older
    // Windows versions when framesToSkip is non-zero; clamp defensively.
    constexpr DWORD kMaxCapturable = 62;
    const DWORD framesToCapture = std::min<DWORD>(maxFrames, kMaxCapturable);

    std::vector<void*> addresses(framesToCapture);

    // Captures the raw instruction pointers, skipping the frame of Capture() itself.
    USHORT captured = CaptureStackBackTrace(framesToSkip + 1, framesToCapture, addresses.data(), nullptr);

    constexpr std::size_t kMaxNameLen = 256;
    alignas(SYMBOL_INFO) char symbolBuffer[sizeof(SYMBOL_INFO) + kMaxNameLen] = {};
    auto* symbol = reinterpret_cast<SYMBOL_INFO*>(symbolBuffer);
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
    symbol->MaxNameLen   = kMaxNameLen;

    // DbgHelp uses shared global state and is not fully thread-safe.
    // Synchronizes access to the DbgHelp symbol resolution functions.
    std::lock_guard<std::mutex> lock(s_dbgHelpMutex);

    for (USHORT i = 0; i < captured; ++i) {
        StackFrame frame;
        frame.address = reinterpret_cast<std::uintptr_t>(addresses[i]);

        // Resolves the function name corresponding to the instruction pointer.
        DWORD64 displacement = 0;
        symbol->MaxNameLen = kMaxNameLen; // SymFromAddr may mutate this; reset each iteration.
        if (SymFromAddr(process, frame.address, &displacement, symbol)) {
            frame.symbolName.assign(symbol->Name, symbol->NameLen);

            // Clean explicit calling convention noise from MSVC symbols
            constexpr std::string_view kCallingConvention = "__cdecl ";
            if (const auto pos = frame.symbolName.find(kCallingConvention); pos != std::string::npos) {
                frame.symbolName.erase(pos, kCallingConvention.size());
            }
        } else {
            frame.symbolName = FormatAddress(frame.address);
        }

        // Resolves the source file name and line number corresponding to the instruction pointer.
        IMAGEHLP_LINE64 line{};
        line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
        DWORD lineDisplacement = 0;
        if (SymGetLineFromAddr64(process, frame.address, &lineDisplacement, &line)) {
            frame.fileName = line.FileName;
            frame.line = line.LineNumber;
        }

        frames.push_back(std::move(frame));
    }

    return frames;
}

#else // !defined(VIGIL_PLATFORM_WINDOWS)

//==============================================================================
// Linux / macOS implementation (backtrace + demangle)
//==============================================================================

std::vector<StackFrame> StackTrace::Capture(unsigned framesToSkip, unsigned maxFrames)
{
    std::vector<StackFrame> frames;
    frames.reserve(maxFrames);

    std::vector<void*> addresses(maxFrames + framesToSkip + 1);
    int captured = backtrace(addresses.data(), static_cast<int>(addresses.size()));
    if (captured <= 0) {
        return frames;
    }

    // backtrace_symbols() allocates memory with malloc(); release it with free().
    std::unique_ptr<char*, decltype(&free)> symbols(
        backtrace_symbols(addresses.data(), captured), &free);

    if (!symbols) {
        return frames;
    }

    // Skip internal frames and clamp the number of frames returned.
    unsigned start = framesToSkip + 1;
    unsigned end = std::min(static_cast<unsigned>(captured), start + maxFrames);

    for (unsigned i = start; i < end; ++i) {
        StackFrame frame;
        frame.address = reinterpret_cast<std::uintptr_t>(addresses[i]);

        std::string_view raw(symbols.get()[i]);

#if defined(VIGIL_PLATFORM_MACOS)
        frame.symbolName = ParseSymbolMacOS(raw, frame.address);
#else
        frame.symbolName = ParseSymbolLinux(raw, frame.address);
#endif

        frames.push_back(std::move(frame));
    }

    return frames;
}

#endif // defined(VIGIL_PLATFORM_WINDOWS)

#else // !defined(VIGIL_ENABLE_STACK_TRACE)

//==============================================================================
// Stub implementation when stack tracing is disabled via CMake
//==============================================================================

std::vector<StackFrame> StackTrace::Capture(unsigned /*framesToSkip*/, unsigned /*maxFrames*/)
{
    // Returns an empty vector immediately, performing zero allocations or lookups.
    return {};
}

#endif // VIGIL_ENABLE_STACK_TRACE

//==============================================================================
// Shared formatting
//==============================================================================

std::string StackTrace::Format(const std::vector<StackFrame>& frames)
{
#if !defined(VIGIL_ENABLE_STACK_TRACE)
    (void)frames;
    return "Stack trace: <disabled by compile options>\n";
#else
    if (frames.empty()) {
        return "Stack trace: <empty>\n";
    }

    std::ostringstream out;

    std::vector<const StackFrame*> visibleFrames;
    visibleFrames.reserve(frames.size());

    for (const auto& frame : frames) {
        if (IsRuntimeFrame(frame.symbolName)) {
            break;
        }
        visibleFrames.push_back(&frame);
    }

    out << "------------------------------------------------------------\n";
    out << "Stack trace (" << visibleFrames.size() << " frames)\n";
    out << "------------------------------------------------------------\n\n";

    const auto digits = std::max<std::size_t>(2, std::to_string(visibleFrames.size()).length());

    for (std::size_t i = 0; i < visibleFrames.size(); ++i) {
        const StackFrame& frame = *visibleFrames[i];

        out << '#'
            << std::setw(static_cast<int>(digits))
            << std::setfill('0')
            << i
            << ' '
            << (!frame.symbolName.empty() ? frame.symbolName : "<unknown>")
            << '\n';

        out << "    Address : "
            << FormatAddress(frame.address)
            << '\n';

        if (!frame.fileName.empty()) {
            out << "    Location: "
                << detail::FormatFilePath(frame.fileName, 2U)
                << ':'
                << frame.line
                << '\n';
        }

        if (i + 1 < visibleFrames.size()) {
            out << '\n';
        }
    }

    return out.str();
#endif
}

[[nodiscard]] std::string StackTrace::CaptureAndFormat(unsigned framesToSkip, unsigned maxFrames)
{
    // Adjusts the depth parameter to omit the CaptureAndFormat wrapper layer.
    return Format(Capture(framesToSkip + 1, maxFrames));
}

} // namespace vigil
