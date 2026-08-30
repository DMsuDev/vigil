// -----------------------------------------------------------------------------
//  Copyright (c) 2026 @DMsuDev. Licensed under the MIT License.
//  See LICENSE file in the project root for full license text.
// -----------------------------------------------------------------------------

#include "stack_trace_backend.h"

#include <Windows.h>
#include <DbgHelp.h>

#include <atomic>
#include <mutex>

namespace vigil::detail {

//==============================================================================
// DbgHelp initialization
//==============================================================================

namespace {

// SymInitialize() must be called once per process before any Sym* functions.
// DbgHelp is not thread-safe; s_mutex serializes all resolution calls.

std::mutex        s_mutex;
std::once_flag    s_initFlag;
std::atomic<bool> s_initialized{false};

void EnsureDbgHelpReady()
{
    std::call_once(s_initFlag, []()
    {
        SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);
        const bool ok = (SymInitialize(GetCurrentProcess(), nullptr, TRUE) != FALSE);
        s_initialized.store(ok, std::memory_order_release);
    });
}

} // namespace

//==============================================================================
// Symbol name cleanup
//==============================================================================

namespace {

// Removes MSVC calling convention decorators that SymFromAddr occasionally
// prepends even when SYMOPT_UNDNAME is active (e.g. "__cdecl ").
void StripCallingConvention(std::string& name)
{
    for (std::string_view decorator : {
        "__cdecl ", "__stdcall ", "__fastcall ", "__vectorcall " })
    {
        if (const auto pos = name.find(decorator); pos != std::string::npos)
        {
            name.erase(pos, decorator.size());
            break; // at most one decorator per symbol
        }
    }
}

} // namespace

//==============================================================================
// Frame resolution
//==============================================================================

namespace {

// Resolves a single PC to one StackFrame via SymFromAddr + SymGetLineFromAddr64.
// Pre-conditions: DbgHelp initialized, s_mutex held by caller.
[[nodiscard]] StackFrame ResolvePhysical(HANDLE process, DWORD64 address)
{
    StackFrame frame;
    frame.address   = static_cast<std::uintptr_t>(address);
    frame.isInlined = false;

    // ---- Symbol name -------------------------------------------------------

    constexpr ULONG       kMaxNameLen = 512;
    constexpr std::size_t kBufSize    = sizeof(SYMBOL_INFO) + kMaxNameLen;

    alignas(SYMBOL_INFO) char buf[kBufSize] = {};
    auto* sym         = reinterpret_cast<SYMBOL_INFO*>(buf);
    sym->SizeOfStruct = sizeof(SYMBOL_INFO);
    sym->MaxNameLen   = kMaxNameLen;

    DWORD64 symDisp = 0;
    if (SymFromAddr(process, address, &symDisp, sym))
    {
        frame.symbolName.assign(sym->Name, sym->NameLen);
        StripCallingConvention(frame.symbolName);
    }

    // ---- Source location ---------------------------------------------------

    IMAGEHLP_LINE64 line{};
    line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
    DWORD lineDisp    = 0;
    if (SymGetLineFromAddr64(process, address, &lineDisp, &line))
    {
        frame.fileName = line.FileName;
        frame.line     = static_cast<std::uint32_t>(line.LineNumber);
        // DbgHelp does not expose column numbers; frame.column stays 0.
    }

    return frame;
}

// Resolves one PC into its physical frame plus any inlined call sites via
// SymFromInlineContext / SymGetLineFromInlineContext (DbgHelp 6.x / Win8+).
// Falls back to ResolvePhysical() when no inline frames are present.
[[nodiscard]] std::vector<StackFrame> ResolveWithInlines(
    HANDLE  process,
    DWORD64 address)
{
    std::vector<StackFrame> result;

    const DWORD inlineCount = SymAddrIncludeInlineTrace(process, address);
    if (inlineCount == 0)
    {
        result.push_back(ResolvePhysical(process, address));
        return result;
    }

    DWORD context  = 0;
    DWORD frameIdx = 0;
    if (!SymQueryInlineTrace(process, address, 0, address, address, &context, &frameIdx))
    {
        result.push_back(ResolvePhysical(process, address));
        return result;
    }

    result.reserve(static_cast<std::size_t>(inlineCount) + 1u);

    // Emit innermost inlined frames first — matches POSIX ordering where
    // inlined call sites precede their physical parent frame.
    for (DWORD i = 0; i < inlineCount; ++i)
    {
        StackFrame inlined;
        inlined.address   = static_cast<std::uintptr_t>(address);
        inlined.isInlined = true;

        constexpr ULONG kMaxNameLen = 512;
        alignas(SYMBOL_INFO) char buf[sizeof(SYMBOL_INFO) + kMaxNameLen] = {};
        auto* sym         = reinterpret_cast<SYMBOL_INFO*>(buf);
        sym->SizeOfStruct = sizeof(SYMBOL_INFO);
        sym->MaxNameLen   = kMaxNameLen;

        DWORD64 symDisp = 0;
        if (SymFromInlineContext(process, address, context, &symDisp, sym))
        {
            inlined.symbolName.assign(sym->Name, sym->NameLen);
            StripCallingConvention(inlined.symbolName);
        }

        IMAGEHLP_LINE64 line{};
        line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
        DWORD lineDisp    = 0;
        if (SymGetLineFromInlineContext(process, address, context, 0, &lineDisp, &line))
        {
            inlined.fileName = line.FileName;
            inlined.line     = static_cast<std::uint32_t>(line.LineNumber);
        }

        result.push_back(std::move(inlined));
        ++context;
    }

    result.push_back(ResolvePhysical(process, address));
    return result;
}

} // namespace

//==============================================================================
// Public Windows API
//==============================================================================

std::vector<StackFrame> CaptureFrames(unsigned framesToSkip, unsigned maxFrames)
{
    constexpr unsigned kOwnFrames   = 1u;  // CaptureFrames() itself adds one frame to skip
    constexpr unsigned kAbsoluteMax = 128u;

    const unsigned skip = (framesToSkip <= std::numeric_limits<unsigned>::max() - kOwnFrames)
                        ? framesToSkip + kOwnFrames
                        : framesToSkip;

    const DWORD toCapture = static_cast<DWORD>(std::min(maxFrames, kAbsoluteMax));

    // Store as const void* to avoid reinterpret_cast at the ResolveAddresses call site.
    std::vector<const void*> raw(toCapture);
    const USHORT captured = CaptureStackBackTrace(
        static_cast<DWORD>(skip),
        toCapture,
        const_cast<void**>(raw.data()),
        nullptr);

    return ResolveAddresses(raw.data(), static_cast<std::size_t>(captured));
}

std::vector<StackFrame> ResolveAddresses(
    const void* const* addresses,
    std::size_t        count)
{
    if (count == 0)
        return {};

    EnsureDbgHelpReady();
    if (!s_initialized.load(std::memory_order_acquire))
    {
        // DbgHelp failed to initialize — emit raw-address-only frames.
        std::vector<StackFrame> frames;
        frames.reserve(count);
        for (std::size_t i = 0; i < count; ++i)
        {
            StackFrame f;
            f.address = reinterpret_cast<std::uintptr_t>(addresses[i]);
            frames.push_back(std::move(f));
        }
        return frames;
    }

    HANDLE process = GetCurrentProcess();

    std::vector<StackFrame> frames;
    frames.reserve(count);

    std::lock_guard<std::mutex> lock(s_mutex);

    for (std::size_t i = 0; i < count; ++i)
    {
        const auto address  = reinterpret_cast<DWORD64>(addresses[i]);
        auto       expanded = ResolveWithInlines(process, address);
        for (auto& f : expanded)
            frames.push_back(std::move(f));
    }

    return frames;
}

} // namespace vigil::detail
