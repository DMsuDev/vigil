// -----------------------------------------------------------------------------
//  Copyright (c) 2026 @DMsuDev. Licensed under the MIT License.
//  See LICENSE file in the project root for full license text.
// -----------------------------------------------------------------------------

#include "stack_trace.h"

#include "vigil/detail/platform_detection.h"
#include "vigil/detail/symbol_utils.h"

#include <algorithm>
#include <iomanip>
#include <limits>
#include <sstream>
#include <unordered_set>

#if defined(VIGIL_ENABLE_STACK_TRACE)
    #include "detail/stack_trace_backend.h"
#endif

namespace vigil {

//==============================================================================
// Internal helpers
//==============================================================================

namespace {

constexpr std::size_t kPointerWidth = sizeof(void*) * 2;

// Formats a raw address as zero-padded uppercase hex: "0x00007FF7F6E67B6B".
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

// Cleans a demangled symbol name for human-readable output — GDB style:
//   "<lambda_N>::operator()" → "<lambda>()"
//   "(anonymous namespace)::"  → ""
//   Appends "()" if the symbol doesn't already end with one.
[[nodiscard]] std::string SimplifySymbol(std::string_view symbol)
{
    std::string result{symbol};

    // "<lambda_N>::operator()" → "<lambda>"
    constexpr std::string_view kLambdaOp = ">::operator()";
    std::size_t pos = result.find(kLambdaOp);
    while (pos != std::string::npos)
    {
        const std::size_t open = result.rfind('<', pos);
        if (open == std::string::npos) break;
        result.replace(open, pos + kLambdaOp.size() - open, "<lambda>");
        pos = result.find(kLambdaOp, open + 8u); // 8 = len("<lambda>")
    }

    // "(anonymous namespace)::" → ""
    constexpr std::string_view kAnonNs = "(anonymous namespace)::";
    pos = result.find(kAnonNs);
    while (pos != std::string::npos)
    {
        result.erase(pos, kAnonNs.size());
        pos = result.find(kAnonNs, pos);
    }

    // Append "()" — matches GDB convention for function symbols.
    if (!result.empty() && !result.ends_with(')'))
        result += "()";

    return result;
}

// Returns true for standard runtime entry-point symbols that appear at the
// tail of every stack trace and carry no diagnostic value.
//
// NOTE: stops at the first match — runtime frames are always a contiguous
// block at the bottom. A user symbol colliding with a runtime name would be
// incorrectly trimmed; accepted trade-off given the near-zero probability.
[[nodiscard]] bool IsRuntimeFrame(std::string_view symbol)
{
    static const std::unordered_set<std::string_view> kRuntimeFrames = {
        // Windows CRT / Win32
        "invoke_main",
        "__scrt_common_main",
        "__scrt_common_main_seh",
        "mainCRTStartup",
        "BaseThreadInitThunk",
        "RtlUserThreadStart",
        // POSIX / glibc
        "start",
        "_start",
        "__libc_start_main",
    };
    return kRuntimeFrames.count(symbol) != 0;
}

} // namespace

//==============================================================================
// StackTrace::Capture / CaptureFromAddresses
//==============================================================================

#if defined(VIGIL_ENABLE_STACK_TRACE)

std::vector<StackFrame> StackTrace::Capture(unsigned framesToSkip, unsigned maxFrames)
{
    constexpr unsigned kOwnFrames = 1u; // Capture() itself adds one frame/level to skip
    const unsigned skip = (framesToSkip <= std::numeric_limits<unsigned>::max() - kOwnFrames)
                        ? framesToSkip + kOwnFrames
                        : framesToSkip;

    return detail::CaptureFrames(skip, maxFrames);
}

std::vector<StackFrame> StackTrace::CaptureFromAddresses(
    const void* const* addresses,
    std::size_t        count)
{
    if (!addresses || count == 0)
        return {};

    return detail::ResolveAddresses(addresses, count);
}

#else // !VIGIL_ENABLE_STACK_TRACE

std::vector<StackFrame> StackTrace::Capture(unsigned, unsigned)                           { return {}; }
std::vector<StackFrame> StackTrace::CaptureFromAddresses(const void* const*, std::size_t) { return {}; }

#endif // VIGIL_ENABLE_STACK_TRACE

//==============================================================================
// StackTrace::Format
//==============================================================================

std::string StackTrace::Format(const std::vector<StackFrame>& frames)
{
#if !defined(VIGIL_ENABLE_STACK_TRACE)
    (void)frames;
    return "Stack trace: <disabled by compile options>\n";
#else

    if (frames.empty())
        return "Stack trace: <empty>\n";

    // -------------------------------------------------------------------------
    // Trim runtime boilerplate from the tail
    // -------------------------------------------------------------------------

    std::vector<const StackFrame*> visible;
    visible.reserve(frames.size());

    for (const auto& frame : frames)
    {
        if (IsRuntimeFrame(frame.symbolName))
            break;
        visible.push_back(&frame);
    }

    // Also trim trailing unresolved frames (no symbol, no file)
    while (!visible.empty() &&
           visible.back()->symbolName.empty() &&
           visible.back()->fileName.empty())
    {
        visible.pop_back();
    }

    if (visible.empty())
        return "Stack trace: <all frames are runtime boilerplate>\n";

    // -------------------------------------------------------------------------
    // Render: GDB style: "#N addr in symbol() at file:line"
    // -------------------------------------------------------------------------

    std::ostringstream out;
    out << "Stack trace (" << visible.size() << " frames):\n\n";

    // Index field width grows naturally with frame count — no forced minimum.
    const int digits = static_cast<int>(
        std::to_string(visible.size() > 0u ? visible.size() - 1u : 0u).length());

    for (std::size_t i = 0; i < visible.size(); ++i)
    {
        const StackFrame& frame = *visible[i];

        out << '#' << std::setw(digits) << std::setfill(' ') << i
            << ' '
            << FormatAddress(frame.address);

        if (!frame.symbolName.empty())
        {
            out << " in " << SimplifySymbol(frame.symbolName);
            if (frame.isInlined)
                out << " [inlined]";
        }

        if (!frame.fileName.empty())
        {
            out << " at "
                << detail::FormatFilePath(frame.fileName, 2u)
                << ':' << frame.line;

            if (frame.column > 0)
                out << ':' << frame.column;
        }

        if (i + 1 < visible.size())
            out << '\n';
    }

    return out.str();

#endif // VIGIL_ENABLE_STACK_TRACE
}

//==============================================================================
// StackTrace::CaptureAndFormat
//==============================================================================

std::string StackTrace::CaptureAndFormat(unsigned framesToSkip, unsigned maxFrames)
{
    constexpr unsigned kOwnFrames = 1u; // CaptureAndFormat() itself adds one frame/level to skip
    const unsigned adjusted = (framesToSkip <= std::numeric_limits<unsigned>::max() - kOwnFrames)
                            ? framesToSkip + kOwnFrames
                            : framesToSkip;

    return Format(Capture(adjusted, maxFrames));
}

} // namespace vigil
