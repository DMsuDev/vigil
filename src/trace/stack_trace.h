// -----------------------------------------------------------------------------
//  Copyright (c) 2026 @DMsuDev. Licensed under the MIT License.
//  See LICENSE file in the project root for full license text.
// -----------------------------------------------------------------------------

#pragma once

#include "vigil/detail/symbol_export.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

/**
 * @file stack_trace.h
 * @brief Cross-platform call stack capture and symbolication.
 *
 * Provides utilities to capture the current thread's call stack and resolve
 * stack frames into human-readable symbols, source locations, and inlined
 * call sites when debug information is available.
 *
 * Platform backends (all PRIVATE — no backend header leaks through this API):
 *  - Windows: CaptureStackBackTrace + DbgHelp (PDB)
 *  - Linux:   backtrace() + libbacktrace (DWARF)
 *  - macOS:   backtrace() + libbacktrace (DWARF)
 *
 * @warning Stack trace capture performs memory allocations and symbol lookups.
 * It is **not async-signal-safe** and must never be called directly from a
 * signal handler. CaptureFromAddresses() is designed for use from a watchdog
 * thread that runs outside signal-handler context; see its documentation.
 */

namespace vigil {

/// @brief Represents a single frame in a captured stack trace.
///
/// Inlined call sites are expanded as consecutive StackFrame entries that
/// share the same @p address as their physical (non-inlined) parent frame.
struct StackFrame {
    std::uintptr_t address   = 0;     ///< Raw instruction pointer for this frame.
    std::string    symbolName;        ///< Demangled function name, or raw address if unknown.
    std::string    fileName;          ///< Source file path; empty if unavailable.
    std::uint32_t  line      = 0;     ///< Source line number; 0 if unavailable.
    std::uint32_t  column    = 0;     ///< Source column number; 0 if unavailable.
    bool           isInlined = false; ///< True if this frame is an inlined call site.
};

/// @brief Utilities for capturing and formatting stack traces.
class VIGIL_API StackTrace {
public:

    /**
     * @brief Captures the current call stack.
     *
     * @param framesToSkip Number of innermost frames to omit (e.g. skip
     *                     Capture() itself and its immediate caller). Overflow
     *                     is handled by saturating addition.
     * @param maxFrames    Maximum number of frames to capture (clamped to a
     *                     platform-specific upper bound).
     * @return The captured frames, innermost first. Inlined call sites appear
     *         as consecutive entries sharing the same @p address.
     */
    [[nodiscard]] static std::vector<StackFrame> Capture(
        unsigned framesToSkip = 0,
        unsigned maxFrames    = 64);

    /**
     * @brief Formats a captured stack trace into a human-readable string.
     *
     * Standard runtime entry frames (__libc_start_main, mainCRTStartup, etc.)
     * are trimmed from the tail. Inlined frames are marked with "(inlined)".
     *
     * Example output:
     * @code
     * Stack trace (3 frames):
     *
     * 0# 0x00007FF7F6E67B6B in FailRangeCheck at examples\assert_example.cpp:239
     * 1# 0x00007FF7F6E6891A in `main'::`6'::<lambda_1>::operator() at examples\assert_example.cpp:95
     * 2# 0x00007FF7F6E68653 in main at examples\assert_example.cpp:110
     * @endcode
     *
     * @param frames Frames from Capture() or CaptureFromAddresses().
     * @return Multi-line formatted stack trace string.
     */
    [[nodiscard]] static std::string Format(const std::vector<StackFrame>& frames);

    /**
     * @brief Captures and formats the current stack trace in a single call.
     *
     * Equivalent to Format(Capture(framesToSkip + 1, maxFrames)).
     *
     * @param framesToSkip Frames to omit, not counting this function itself.
     * @param maxFrames    Maximum number of frames to capture.
     * @return Formatted multi-line stack trace string.
     */
    [[nodiscard]] static std::string CaptureAndFormat(
        unsigned framesToSkip = 0,
        unsigned maxFrames    = 64);

    /**
     * @brief Resolves raw instruction addresses into stack frames.
     *
     * Designed for the watchdog thread pattern: the signal handler captures
     * raw instruction pointers async-signal-safely (via backtrace() or
     * CaptureStackBackTrace()), then posts to a semaphore. The watchdog thread
     * wakes up and calls this function to perform the full DWARF/PDB resolution
     * outside restricted signal-handler context.
     *
     * @note Not async-signal-safe: performs heap allocations and symbol lookups.
     *
     * @param addresses Raw instruction pointers from a prior stack unwind.
     * @param count     Number of entries in @p addresses.
     * @return Resolved frames in the same order as @p addresses. Inlined call
     *         sites are expanded inline after their physical parent frame.
     */
    [[nodiscard]] static std::vector<StackFrame> CaptureFromAddresses(
        const void* const* addresses,
        std::size_t        count);
};

} // namespace vigil
