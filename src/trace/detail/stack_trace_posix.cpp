// -----------------------------------------------------------------------------
//  Copyright (c) 2026 @DMsuDev. Licensed under the MIT License.
//  See LICENSE file in the project root for full license text.
// -----------------------------------------------------------------------------

// This translation unit is compiled only on POSIX platforms (Linux / macOS).
// It must never be added to the build on Windows.

#include "stack_trace_posix.h"

#include <backtrace.h>    // libbacktrace — vendored via FetchContent
#include <cxxabi.h>       // abi::__cxa_demangle — part of libstdc++ / libc++
#include <execinfo.h>     // backtrace() — async-signal-safe raw capture
#include <cstdlib>
#include <cstring>
#include <memory>
#include <mutex>

namespace vigil::detail {

namespace {

// ---------------------------------------------------------------------------
// libbacktrace state
//
// backtrace_create_state() must be called once per process. The state object
// is not thread-safe for concurrent calls to backtrace_pcinfo(); we serialize
// all resolution calls with s_mutex.
//
// error_callback receives the error string and an integer error code. We
// intentionally swallow errors here: if libbacktrace cannot open the binary
// or locate DWARF data, the symbol will remain as "<unknown>" rather than
// crashing the crash handler.
// ---------------------------------------------------------------------------

void OnBacktraceError(void* /*data*/, const char* /*msg*/, int /*errnum*/) {}

struct BacktraceState {
    backtrace_state* state = nullptr;
    std::mutex       mutex;

    BacktraceState()
    {
        // nullptr program name → libbacktrace reads /proc/self/exe (Linux) or
        // _NSGetExecutablePath() (macOS) to locate the binary and its DWARF.
        // threaded = 0: we manage our own locking via s_mutex.
        state = backtrace_create_state(nullptr, /*threaded=*/0,
                                       OnBacktraceError, /*data=*/nullptr);
    }
};

BacktraceState& GetState()
{
    // Constructed once at first call; destructor never runs (intentional:
    // we may need the state right up until process termination).
    static BacktraceState s;
    return s;
}

// ---------------------------------------------------------------------------
// Itanium ABI demangling
// ---------------------------------------------------------------------------

[[nodiscard]] std::string Demangle(const char* mangled)
{
    if (!mangled || mangled[0] == '\0')
        return {};

    int status = 0;
    std::unique_ptr<char, decltype(&free)> demangled(
        abi::__cxa_demangle(mangled, nullptr, nullptr, &status), &free);

    return (status == 0 && demangled) ? std::string(demangled.get()) : std::string(mangled);
}

// ---------------------------------------------------------------------------
// libbacktrace callback state used during a single resolution pass
//
// backtrace_pcinfo() calls FullCallback once per logical frame (including
// inlined frames at the same PC). We collect them all into an output vector.
// ---------------------------------------------------------------------------

struct ResolutionContext {
    std::vector<StackFrame>* output;  // non-owning, points into caller's vector
    std::uintptr_t           address; // physical PC being resolved
};

// Called by libbacktrace once per logical frame at a given PC.
// The first call is the innermost (outermost logical) frame; subsequent calls
// are inlined call sites, in order from innermost to outermost.
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
    // The first frame emitted for this PC is the physical frame; all
    // subsequent ones at the same PC are inlined call sites.
    frame.isInlined  = !ctx->output->empty() &&
                       ctx->output->back().address == ctx->address;

    ctx->output->push_back(std::move(frame));
    return 0; // 0 = continue iteration
}

// Called by libbacktrace when it cannot resolve a PC through DWARF.
// We emit a minimal frame with just the raw address so the caller still
// has a record of the instruction pointer.
void ErrorCallback(void* data, const char* /*msg*/, int /*errnum*/)
{
    auto* ctx = static_cast<ResolutionContext*>(data);

    StackFrame frame;
    frame.address    = ctx->address;
    frame.isInlined  = false;
    ctx->output->push_back(std::move(frame));
}

// ---------------------------------------------------------------------------
// ResolveOne — resolves a single PC into one or more StackFrames.
//
// Appends to @p output. Must be called with the state mutex held.
// ---------------------------------------------------------------------------

void ResolveOne(
    backtrace_state*         state,
    uintptr_t                pc,
    std::vector<StackFrame>& output)
{
    ResolutionContext ctx{&output, static_cast<std::uintptr_t>(pc)};
    backtrace_pcinfo(state, pc, FullCallback, ErrorCallback, &ctx);
}

} // namespace

// ---------------------------------------------------------------------------
// Public POSIX API (declared in stack_trace_posix.h)
// ---------------------------------------------------------------------------

std::vector<StackFrame> CaptureFrames(unsigned framesToSkip, unsigned maxFrames)
{
    // Saturating add: skip CaptureFrames() itself (+1) without overflowing.
    const unsigned skip = (framesToSkip < std::numeric_limits<unsigned>::max())
                        ? framesToSkip + 1u
                        : framesToSkip;

    constexpr unsigned kAbsoluteMax = 256u;
    const unsigned totalToCapture = std::min(skip + maxFrames, kAbsoluteMax);

    std::vector<void*> raw(totalToCapture);
    const int captured = ::backtrace(raw.data(), static_cast<int>(raw.size()));
    if (captured <= 0 || static_cast<unsigned>(captured) <= skip)
        return {};

    const unsigned start = skip;
    const unsigned end   = std::min(static_cast<unsigned>(captured), start + maxFrames);

    return ResolveAddresses(raw.data() + start, end - start);
}

std::vector<StackFrame> ResolveAddresses(
    void* const* addresses,
    std::size_t  count)
{
    if (count == 0)
        return {};

    BacktraceState& bs = GetState();
    if (!bs.state)
    {
        // libbacktrace failed to initialize; fall back to raw addresses only.
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

    std::vector<StackFrame> frames;
    frames.reserve(count); // will grow when inlined frames are expanded

    // Serialize all resolution calls: backtrace_pcinfo() is not reentrant
    // when the state was created with threaded=0.
    std::lock_guard<std::mutex> lock(bs.mutex);

    for (std::size_t i = 0; i < count; ++i)
    {
        const auto pc = reinterpret_cast<uintptr_t>(addresses[i]);
        ResolveOne(bs.state, pc, frames);
    }

    return frames;
}

} // namespace vigil::detail
