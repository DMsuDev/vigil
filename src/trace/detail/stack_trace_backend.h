// -----------------------------------------------------------------------------
//  Copyright (c) 2026 @DMsuDev. Licensed under the MIT License.
//  See LICENSE file in the project root for full license text.
// -----------------------------------------------------------------------------

#pragma once

// The matching .cpp is selected at build time by CMake based on platform.

#include "trace/stack_trace.h"

#include <cstddef>
#include <limits>
#include <vector>

namespace vigil::detail {

/**
 * @brief Captures the current call stack using the platform-native backend.
 *
 * - Windows: CaptureStackBackTrace() + DbgHelp (PDB + inline frames)
 * - Linux:   backtrace() + libbacktrace (DWARF + inline frames)
 * - macOS:   backtrace() + libbacktrace (DWARF + inline frames)
 *
 * The +1 skip for this function itself is applied internally.
 *
 * @param framesToSkip Frames to omit above this function.
 * @param maxFrames    Maximum frames to capture, capped to an internal safety ceiling.
 */
[[nodiscard]] std::vector<StackFrame> CaptureFrames(
    unsigned framesToSkip,
    unsigned maxFrames);

/**
 * @brief Resolves an array of raw instruction pointers into StackFrames.
 *
 * Not async-signal-safe. Designed for the watchdog thread pattern where a
 * signal handler captures raw PCs and posts them for resolution here.
 *
 * Inlined frames are expanded inline after their physical parent frame.
 *
 * @param addresses Raw instruction pointers to resolve.
 * @param count     Number of entries in @p addresses.
 */
[[nodiscard]] std::vector<StackFrame> ResolveAddresses(
    const void* const* addresses,
    std::size_t        count);

} // namespace vigil::detail
