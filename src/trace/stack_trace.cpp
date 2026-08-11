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

#if defined(VIGIL_ENABLE_STACK_TRACE)
    #if defined(VIGIL_PLATFORM_WINDOWS)
        #include "detail/stack_trace_win.h"
    #else
        #include "detail/stack_trace_posix.h"
    #endif
#endif

namespace vigil {

namespace {

constexpr std::size_t kPointerWidth = sizeof(void*) * 2;

/// @brief Formats a memory address as a zero-padded uppercase hex string.
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

/// @brief Returns true for standard runtime entry-point frames that appear at
///        the tail of every stack trace and add no diagnostic value.
[[nodiscard]] bool IsRuntimeFrame(std::string_view symbol)
{
    return symbol == "invoke_main"            ||
           symbol == "__scrt_common_main"     ||
           symbol == "__scrt_common_main_seh" ||
           symbol == "mainCRTStartup"         ||
           symbol == "BaseThreadInitThunk"    ||
           symbol == "RtlUserThreadStart"     ||
           symbol == "start"                  ||
           symbol == "_start"                 ||
           symbol == "__libc_start_main";
}

} // namespace

// ============================================================================
// StackTrace — public implementation
// ============================================================================

#if defined(VIGIL_ENABLE_STACK_TRACE)

std::vector<StackFrame> StackTrace::Capture(unsigned framesToSkip, unsigned maxFrames)
{
    // +1 to exclude this function from the output. The backend applies its own
    // +1 for CaptureFrames() itself, so the total overhead is exactly 2 frames.
    const unsigned skip = (framesToSkip < std::numeric_limits<unsigned>::max())
                        ? framesToSkip + 1u
                        : framesToSkip;

    return detail::CaptureFrames(skip, maxFrames);
}

std::vector<StackFrame> StackTrace::CaptureFromAddresses(
    const void* const* addresses,
    std::size_t        count)
{
    if (!addresses || count == 0)
        return {};

#if defined(VIGIL_PLATFORM_WINDOWS)
    // Windows: ResolveAddresses accepts const void* const* directly.
    return detail::ResolveAddresses(addresses, count);
#else
    // POSIX: backtrace_symbols() (used in the fallback path) requires
    // void* const*, so copy into a local mutable buffer. libbacktrace itself
    // only needs uintptr_t values, but we pass through the same helper to
    // keep a single code path.
    std::vector<void*> mutable_ptrs(count);
    for (std::size_t i = 0; i < count; ++i)
        mutable_ptrs[i] = const_cast<void*>(addresses[i]);

    return detail::ResolveAddresses(mutable_ptrs.data(), count);
#endif
}

#else // !VIGIL_ENABLE_STACK_TRACE

//------------------------------------------------------------------------------
// Stub implementation — stack tracing disabled at compile time
//------------------------------------------------------------------------------

std::vector<StackFrame> StackTrace::Capture(unsigned, unsigned)       { return {}; }
std::vector<StackFrame> StackTrace::CaptureFromAddresses(const void* const*, std::size_t) { return {}; }

#endif // VIGIL_ENABLE_STACK_TRACE

//------------------------------------------------------------------------------
// Format() — shared across all platforms and both enabled/disabled builds
//------------------------------------------------------------------------------

std::string StackTrace::Format(const std::vector<StackFrame>& frames)
{
#if !defined(VIGIL_ENABLE_STACK_TRACE)
    (void)frames;
    return "Stack trace: <disabled by compile options>\n";
#else
    if (frames.empty())
        return "Stack trace: <empty>\n";

    // Collect pointers to visible frames, trimming the runtime boilerplate tail.
    std::vector<const StackFrame*> visible;
    visible.reserve(frames.size());

    for (const auto& frame : frames)
    {
        if (IsRuntimeFrame(frame.symbolName))
            break;
        visible.push_back(&frame);
    }

    if (visible.empty())
        return "Stack trace: <all frames are runtime boilerplate>\n";

    std::ostringstream out;

    out << "Stack trace (" << visible.size() << " frames):\n\n";

    const int digits = static_cast<int>(
        std::max<std::size_t>(2u, std::to_string(visible.size()).length()));

    for (std::size_t i = 0; i < visible.size(); ++i)
    {
        const StackFrame& frame = *visible[i];

        out << std::setw(digits)
            << i
            << "# "
            << FormatAddress(frame.address);

        if (!frame.symbolName.empty())
        {
            out << " in " << frame.symbolName;

            if (frame.isInlined)
                out << " (inlined)";
        }

        if (!frame.fileName.empty())
        {
            out << " at "
                << detail::FormatFilePath(frame.fileName, 2u)
                << ':'
                << frame.line;

            if (frame.column > 0)
                out << ':' << frame.column;
        }

        if (i + 1 < visible.size())
            out << '\n';
    }

    return out.str();
#endif
}

std::string StackTrace::CaptureAndFormat(unsigned framesToSkip, unsigned maxFrames)
{
    const unsigned adjusted = (framesToSkip < std::numeric_limits<unsigned>::max())
                            ? framesToSkip + 1u
                            : framesToSkip;
    return Format(Capture(adjusted, maxFrames));
}

} // namespace vigil
