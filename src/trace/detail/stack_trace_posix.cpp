// -----------------------------------------------------------------------------
//  Copyright (c) 2026 @DMsuDev. Licensed under the MIT License.
//  See LICENSE file in the project root for full license text.
// -----------------------------------------------------------------------------

#include "stack_trace_backend.h"

#include <backtrace.h>    // libbacktrace
#include <cxxabi.h>       // abi::__cxa_demangle
#include <execinfo.h>     // backtrace()
#include <cstdlib>
#include <cstring>
#include <memory>
#include <mutex>

namespace vigil::detail {

//==============================================================================
// libbacktrace state
//==============================================================================

namespace {

// backtrace_create_state() must be called once per process. The resulting
// state is not thread-safe for concurrent backtrace_pcinfo() calls with
// threaded=0; all resolution is serialized via BacktraceState::mutex.
//
// Errors are swallowed intentionally: if libbacktrace cannot locate the binary
// or its DWARF data, symbols remain empty rather than aborting.

void OnBacktraceError(void* /*data*/, const char* /*msg*/, int /*errnum*/) {}

struct BacktraceState
{
    backtrace_state* state = nullptr;
    std::mutex       mutex;

    BacktraceState()
    {
        // nullptr → libbacktrace locates the binary automatically:
        //   Linux: /proc/self/exe
        //   macOS: _NSGetExecutablePath()
        // threaded=0: we serialize access ourselves via mutex above.
        state = backtrace_create_state(
            nullptr, /*threaded=*/0, OnBacktraceError, /*data=*/nullptr);
    }

    // Intentional leak — destructor never runs so the state remains valid
    // through process termination. Suppress with .lsan-suppressions when
    // running under LSan / ASan if needed.
};

BacktraceState& GetState()
{
    static BacktraceState s;
    return s;
}

} // namespace

//==============================================================================
// Itanium ABI demangling
//==============================================================================

namespace {

[[nodiscard]] std::string Demangle(const char* mangled)
{
    if (!mangled || mangled[0] == '\0')
        return {};

    int status = 0;
    std::unique_ptr<char, decltype(&free)> demangled(
        abi::__cxa_demangle(mangled, nullptr, nullptr, &status), &free);

    return (status == 0 && demangled)
        ? std::string(demangled.get())
        : std::string(mangled);
}

} // namespace

//==============================================================================
// libbacktrace callbacks
//==============================================================================

namespace {

// backtrace_pcinfo() fires FullCallback once per logical frame at a given PC,
// including inlined frames. ErrorCallback fires when DWARF resolution fails —
// we still emit a minimal frame carrying the raw address.

struct ResolutionContext
{
    std::vector<StackFrame>* output;     // non-owning — points into caller's vector
    std::uintptr_t           address;    // physical PC being resolved
    bool                     firstFrame; // true for the physical frame, false for inlined
};

int FullCallback(
    void*       data,
    uintptr_t   /*pc*/,
    const char* filename,
    int         lineno,
    const char* function)
{
    auto* ctx = static_cast<ResolutionContext*>(data);

    StackFrame frame;
    frame.address    = ctx->address;
    frame.symbolName = Demangle(function);
    frame.fileName   = filename ? filename : "";
    frame.line       = lineno > 0 ? static_cast<std::uint32_t>(lineno) : 0u;
    frame.column     = 0u; // libbacktrace does not provide column numbers

    // First callback for this PC = physical frame; all subsequent = inlined.
    frame.isInlined = !ctx->firstFrame;
    ctx->firstFrame = false;

    ctx->output->push_back(std::move(frame));
    return 0; // 0 = continue iteration
}

void ErrorCallback(void* data, const char* /*msg*/, int /*errnum*/)
{
    auto* ctx = static_cast<ResolutionContext*>(data);

    StackFrame frame;
    frame.address   = ctx->address;
    frame.isInlined = false;
    ctx->output->push_back(std::move(frame));
}

} // namespace

//==============================================================================
// Internal resolution helpers
//==============================================================================

namespace {

// Resolves a single PC into one or more StackFrames via libbacktrace.
// Must be called with BacktraceState::mutex held.
void ResolveOne(
    backtrace_state*         state,
    uintptr_t                pc,
    std::vector<StackFrame>& output)
{
    ResolutionContext ctx{&output, static_cast<std::uintptr_t>(pc), /*firstFrame=*/true};
    backtrace_pcinfo(state, pc, FullCallback, ErrorCallback, &ctx);
}

// Fallback when libbacktrace failed to initialize. Uses backtrace_symbols()
// for best-effort symbol names; falls back to raw addresses if that fails too.
[[nodiscard]] std::vector<StackFrame> FallbackResolve(
    const void* const* addresses,
    std::size_t        count)
{
    char** syms = ::backtrace_symbols(
        const_cast<void* const*>(addresses),
        static_cast<int>(count));

    std::vector<StackFrame> frames;
    frames.reserve(count);

    for (std::size_t i = 0; i < count; ++i)
    {
        StackFrame f;
        f.address    = reinterpret_cast<std::uintptr_t>(addresses[i]);
        // syms[i]: "module(mangled+offset) [address]" — demangle if present.
        f.symbolName = syms ? Demangle(syms[i]) : "";
        frames.push_back(std::move(f));
    }

    if (syms)
        free(syms);

    return frames;
}

} // namespace

//==============================================================================
// Public POSIX API
//==============================================================================

std::vector<StackFrame> CaptureFrames(unsigned framesToSkip, unsigned maxFrames)
{
    constexpr unsigned kOwnFrames    = 1u; // CaptureFrames() itself adds one frame to skip
    constexpr unsigned kAbsoluteMax  = 128u;

    const unsigned skip = (framesToSkip <= std::numeric_limits<unsigned>::max() - kOwnFrames)
                        ? framesToSkip + kOwnFrames
                        : framesToSkip;

    const unsigned totalToCapture = std::min(skip + maxFrames, kAbsoluteMax);

    std::vector<void*> raw(totalToCapture);
    const int captured = ::backtrace(raw.data(), static_cast<int>(raw.size()));

    if (captured <= 0 || static_cast<unsigned>(captured) <= skip)
        return {};

    const unsigned start = skip;
    const unsigned end   = std::min(static_cast<unsigned>(captured), start + maxFrames);

    return ResolveAddresses(
        const_cast<const void* const*>(raw.data() + start),
        end - start);
}

std::vector<StackFrame> ResolveAddresses(
    const void* const* addresses,
    std::size_t        count)
{
    if (count == 0)
        return {};

    BacktraceState& bs = GetState();

    if (!bs.state)
        return FallbackResolve(addresses, count);

    std::vector<StackFrame> frames;
    frames.reserve(count); // may grow as inlined frames are expanded

    // Serialize: backtrace_pcinfo() is not reentrant with threaded=0.
    std::lock_guard<std::mutex> lock(bs.mutex);

    for (std::size_t i = 0; i < count; ++i)
    {
        const auto pc = reinterpret_cast<uintptr_t>(addresses[i]);
        ResolveOne(bs.state, pc, frames);
    }

    return frames;
}

} // namespace vigil::detail
