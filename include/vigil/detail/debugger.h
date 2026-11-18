// -----------------------------------------------------------------------------
//  Copyright (c) 2026 @DMsuDev. Licensed under the MIT License.
//  See LICENSE file in the project root for full license text.
// -----------------------------------------------------------------------------

#pragma once

#include "vigil/detail/symbol_export.h"

namespace vigil::detail {

/**
 * @brief Returns true if a debugger is currently attached to this process.
 *
 * - Windows: queries IsDebuggerPresent() via the Win32 API.
 * - Linux:   reads TracerPid from /proc/self/status — non-zero means a
 *            debugger is attached via ptrace (gdb, lldb, strace, etc.).
 * - macOS:   queries the P_TRACED flag via sysctl KERN_PROC_PID.
 *
 * @note This function is not async-signal-safe and must not be called from
 *       a signal handler.
 */
[[nodiscard]] VIGIL_API bool IsDebuggerAttached() noexcept;

} // namespace vigil::detail
