// -----------------------------------------------------------------------------
//  Copyright (c) 2026 @DMsuDev. Licensed under the MIT License.
//  See LICENSE file in the project root for full license text.
// -----------------------------------------------------------------------------

// This translation unit is compiled only on Windows.
// It must never be added to the build on POSIX platforms.

#include "stack_trace_win.h"

#include <Windows.h>
#include <DbgHelp.h>

#include <atomic>
#include <mutex>

namespace vigil::detail {

namespace {

// ---------------------------------------------------------------------------
// DbgHelp initialization
//
// SymInitialize() must be called once per process before any Sym* functions.
// DbgHelp uses shared global state and is not thread-safe; s_mutex serializes
// all resolution calls across threads.
// ---------------------------------------------------------------------------

static std::mutex         s_mutex;
static std::once_flag     s_initFlag;
static std::atomic<bool>  s_initialized{false};

void EnsureDbgHelpReady()
{
    std::call_once(s_initFlag, []() {
        SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);
        const bool ok = (SymInitialize(GetCurrentProcess(), nullptr, TRUE) != FALSE);
        s_initialized.store(ok, std::memory_order_release);
    });
}

// ---------------------------------------------------------------------------
// Single-address resolution
//
// Pre-conditions: DbgHelp is initialized, s_mutex is held by the caller.
// ---------------------------------------------------------------------------

[[nodiscard]] StackFrame ResolveOne(HANDLE process, const void* address)
{
    StackFrame frame;
    frame.address = reinterpret_cast<std::uintptr_t>(address);

    // ---- Symbol name -------------------------------------------------------
    constexpr ULONG       kMaxNameLen = 512;
    constexpr std::size_t kBufSize    = sizeof(SYMBOL_INFO) + kMaxNameLen;

    alignas(SYMBOL_INFO) char symbolBuffer[kBufSize] = {};
    auto* sym         = reinterpret_cast<SYMBOL_INFO*>(symbolBuffer);
    sym->SizeOfStruct = sizeof(SYMBOL_INFO);
    sym->MaxNameLen   = kMaxNameLen;

    DWORD64 symDisplacement = 0;
    if (SymFromAddr(process, frame.address, &symDisplacement, sym))
    {
        frame.symbolName.assign(sym->Name, sym->NameLen);

        // Strip MSVC calling-convention decorators (e.g. "__cdecl ").
        constexpr std::string_view kCdecl = "__cdecl ";
        if (const auto pos = frame.symbolName.find(kCdecl); pos != std::string::npos)
            frame.symbolName.erase(pos, kCdecl.size());
    }

    // ---- Source location ---------------------------------------------------
    IMAGEHLP_LINE64 lineInfo{};
    lineInfo.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
    DWORD lineDisplacement = 0;
    if (SymGetLineFromAddr64(process, frame.address, &lineDisplacement, &lineInfo))
    {
        frame.fileName = lineInfo.FileName;
        frame.line     = static_cast<std::uint32_t>(lineInfo.LineNumber);
        // DbgHelp does not expose column numbers; leave frame.column = 0.
    }

    // DbgHelp does not natively expand inlined frames in the same way DWARF
    // does via backtrace_pcinfo(). SymAddrIncludeInlineTrace() / SymQueryInlineTrace()
    // exist on Windows 8+ and could be added here in the future.
    frame.isInlined = false;

    return frame;
}

} // namespace

// ---------------------------------------------------------------------------
// Public Windows API (declared in stack_trace_win.h)
// ---------------------------------------------------------------------------

std::vector<StackFrame> CaptureFrames(unsigned framesToSkip, unsigned maxFrames)
{
    // RtlCaptureStackBackTrace is documented to reliably return at most 62
    // frames when framesToSkip is non-zero on older Windows versions.
    constexpr unsigned kPlatformMax = 62u;
    const DWORD toCapture = static_cast<DWORD>(std::min(maxFrames, kPlatformMax));

    std::vector<void*> raw(toCapture);

    // +1 to exclude CaptureFrames() itself.
    const unsigned skip = (framesToSkip < std::numeric_limits<unsigned>::max())
                        ? framesToSkip + 1u
                        : framesToSkip;

    const USHORT captured = CaptureStackBackTrace(
        static_cast<DWORD>(skip),
        toCapture,
        raw.data(),
        nullptr);

    return ResolveAddresses(raw.data(), captured);
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
        // DbgHelp failed to initialize; return frames with raw addresses only.
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
        frames.push_back(ResolveOne(process, addresses[i]));

    return frames;
}

} // namespace vigil::detail
