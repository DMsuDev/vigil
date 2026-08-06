// -----------------------------------------------------------------------------
//  Copyright (c) 2026 @DMsuDev. Licensed under the MIT License.
//  See LICENSE file in the project root for full license text.
// -----------------------------------------------------------------------------

#pragma once

#include "vigil/detail/symbol_export.h"

#include <cstdint>
#include <string>
#include <vector>

/**
 * @file stack_trace.h
 * @brief Cross-platform call stack capture and symbolication.
 *
 * Provides utilities to capture the current thread's call stack and resolve
 * stack frames into human-readable symbols and source locations when debug
 * information is available.
 *
 * Platform backends:
 *  - Windows: DbgHelp
 *  - Linux:   backtrace()/libunwind (implementation-dependent)
 *  - macOS:   backtrace()/libunwind (implementation-dependent)
 *
 * @warning Capturing a stack trace performs memory allocations and symbol
 * lookups. It is **not async-signal-safe** and must not be called directly
 * from a signal handler. See crash_handler.h for signal-safe crash reporting.
 */

namespace vigil {

/// @brief Represents a single frame in a captured stack trace.
struct StackFrame {
    std::uintptr_t address = 0;      ///< Raw instruction pointer for this frame.
    std::string    symbolName;       ///< Demangled function name, or "<unknown>".
    std::string    fileName;         ///< Source file, empty if unavailable (e.g. no debug info).
    std::uint32_t  line = 0;         ///< Source line, 0 if unavailable.
};

/// @brief Utilities for capturing and formatting stack traces.
class VIGIL_API StackTrace {
public:
    /**
     * @brief Captures the current call stack.
     * @param framesToSkip Number of innermost frames to omit (e.g. skip
     *                      Capture() itself and its immediate caller).
     * @param maxFrames    Maximum number of frames to capture.
     * @return The captured frames, outermost (main/entry point) last.
     */
    [[nodiscard]] static std::vector<StackFrame> Capture(
        unsigned framesToSkip = 0,
        unsigned maxFrames = 64);

    /**
     * @brief Formats a captured stack trace into a human-readable string.
     *
     * Example output:
     * @code
     * #0 Renderer::Draw()   renderer.cpp:142
     * #1 Scene::Render()    scene.cpp:81
     * #2 main()             main.cpp:37
     * @endcode
     *
     * @param frames Frames previously returned by Capture().
     * @return Multi-line formatted stack trace.
     */
    [[nodiscard]] static std::string Format(const std::vector<StackFrame>& frames);

    /**
     * @brief Captures and formats the current stack trace.
     *
     * @param framesToSkip Number of frames to omit, excluding this function.
     * @param maxFrames Maximum number of frames to capture.
     * @return Formatted multi-line stack trace.
     */
    [[nodiscard]] static std::string CaptureAndFormat(
        unsigned framesToSkip = 0,
        unsigned maxFrames = 64);
};

} // namespace vigil
