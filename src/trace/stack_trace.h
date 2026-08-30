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
 * Platform backends (all PRIVATE):
 *  - Windows: CaptureStackBackTrace + DbgHelp (PDB + inline frames)
 *  - Linux:   backtrace() + libbacktrace (DWARF + inline frames)
 *  - macOS:   backtrace() + libbacktrace (DWARF + inline frames)
 *
 * @warning Stack trace capture performs memory allocations and symbol lookups.
 *          It is not async-signal-safe and must not be called from a signal handler.
 */

namespace vigil {

// ============================================================================
// StackFrame
// ============================================================================

/**
 * @brief Represents a single frame in a captured stack trace.
 *
 * Inlined call sites are expanded as consecutive entries sharing the same
 * @ref address as their physical parent frame, innermost first.
 */
struct StackFrame
{
    std::uintptr_t address   = 0;     ///< Raw instruction pointer.
    std::string    symbolName;        ///< Demangled function name; empty if unknown.
    std::string    fileName;          ///< Source file path; empty if unavailable.
    std::uint32_t  line      = 0;     ///< Source line number; 0 if unavailable.
    std::uint32_t  column    = 0;     ///< Source column number; 0 if unavailable.
    bool           isInlined = false; ///< True if this frame is an inlined call site.
};

// ============================================================================
// StackTrace
// ============================================================================

/// @brief Utilities for capturing and formatting stack traces.
class VIGIL_API StackTrace
{
public:

    /**
     * @brief Captures the current call stack.
     *
     * @param framesToSkip Innermost frames to omit beyond this function itself.
     * @param maxFrames    Maximum frames to return, capped internally for safety.
     *
     * @return Captured frames, innermost first. Empty if stack tracing is
     *         disabled at compile time (@c VIGIL_ENABLE_STACK_TRACE).
     */
    [[nodiscard]] static std::vector<StackFrame> Capture(
        unsigned framesToSkip = 0,
        unsigned maxFrames    = 64);

    /**
     * @brief Resolves raw instruction pointers into StackFrames.
     *
     * Useful when addresses were captured separately under restricted conditions
     * and full symbol resolution is deferred to a safer execution context.
     *
     * @note Not async-signal-safe — performs heap allocations and symbol lookups.
     *
     * @param addresses Raw instruction pointers to resolve.
     * @param count     Number of entries in @p addresses.
     *
     * @return Resolved frames in the same order as @p addresses. Inlined call
     *         sites are expanded after their physical parent frame.
     */
    [[nodiscard]] static std::vector<StackFrame> CaptureFromAddresses(
        const void* const* addresses,
        std::size_t        count);

    /**
     * @brief Formats a captured stack trace into a human-readable string.
     *
     * Produces GDB-style output with one frame per line. Runtime boilerplate
     * and unresolved frames are trimmed automatically.
     *
     * @param frames Frames produced by @ref Capture or @ref CaptureFromAddresses.
     * @return Formatted multi-line string. Never empty.
     */
    [[nodiscard]] static std::string Format(const std::vector<StackFrame>& frames);

    /**
     * @brief Captures and immediately formats the current stack trace.
     *
     * Convenience wrapper around `Format(Capture(framesToSkip + 1, maxFrames))`.
     *
     * @param framesToSkip Frames to omit beyond this function itself.
     * @param maxFrames    Maximum number of frames to capture.
     *
     * @return Formatted multi-line stack trace string, always non-empty.
     */
    [[nodiscard]] static std::string CaptureAndFormat(
        unsigned framesToSkip = 0,
        unsigned maxFrames    = 64);
};

} // namespace vigil
