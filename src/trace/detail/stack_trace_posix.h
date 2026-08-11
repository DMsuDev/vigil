// -----------------------------------------------------------------------------
//  Copyright (c) 2026 @DMsuDev. Licensed under the MIT License.
//  See LICENSE file in the project root for full license text.
// -----------------------------------------------------------------------------

#pragma once

// Internal header — must never be included from any public Vigil header.
// Included only by stack_trace.cpp (the dispatcher) on POSIX builds.

#include "trace/stack_trace.h"

#include <cstddef>
#include <limits>
#include <vector>

namespace vigil::detail {

/**
 * @brief Captures the current call stack and resolves it via libbacktrace.
 *
 * Async-signal-safe capture (backtrace()) + DWARF resolution (libbacktrace).
 * The +1 skip for this function itself is applied internally.
 *
 * @param framesToSkip Frames to omit above this function.
 * @param maxFrames    Maximum number of frames to capture.
 */
[[nodiscard]] std::vector<StackFrame> CaptureFrames(
    unsigned framesToSkip,
    unsigned maxFrames);

/**
 * @brief Resolves an array of raw instruction pointers into StackFrames.
 *
 * Not async-signal-safe. Designed for the watchdog thread. Inlined frames
 * are expanded inline after their physical parent frame.
 *
 * @param addresses Non-const pointer array (backtrace_symbols() requirement).
 * @param count     Number of entries in @p addresses.
 */
[[nodiscard]] std::vector<StackFrame> ResolveAddresses(
    void* const* addresses,
    std::size_t  count);

} // namespace vigil::detail
