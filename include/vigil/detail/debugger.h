// -----------------------------------------------------------------------------
//  Copyright (c) 2026 @DMsuDev. Licensed under the MIT License.
//  See LICENSE file in the project root for full license text.
// -----------------------------------------------------------------------------

#pragma once

#include "vigil/detail/platform_detection.h"

#if defined(VIGIL_PLATFORM_WINDOWS)
    #include <Windows.h>
#elif defined(VIGIL_PLATFORM_LINUX)
    #include <cstdio>
#elif defined(VIGIL_PLATFORM_MACOS)
    #include <sys/sysctl.h>
    #include <unistd.h>
#endif

namespace vigil::detail {

// ============================================================================
// IsDebuggerAttached
// ============================================================================

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
[[nodiscard]] inline bool IsDebuggerAttached() noexcept
{
#if defined(VIGIL_PLATFORM_WINDOWS)

    return ::IsDebuggerPresent() != FALSE;

#elif defined(VIGIL_PLATFORM_LINUX)

    int tracerPid = 0;
    if (FILE* f = std::fopen("/proc/self/status", "r"))
    {
        char line[256];
        while (std::fgets(line, sizeof(line), f))
        {
            if (std::sscanf(line, "TracerPid: %d", &tracerPid) == 1)
                break;
        }
        std::fclose(f);
    }
    return tracerPid > 0;

#elif defined(VIGIL_PLATFORM_MACOS)

    int mib[4] = { CTL_KERN, KERN_PROC, KERN_PROC_PID, ::getpid() };
    struct kinfo_proc info{};
    std::size_t size = sizeof(info);
    ::sysctl(mib, 4, &info, &size, nullptr, 0);
    return (info.kp_proc.p_flag & P_TRACED) != 0;

#else

    return false;

#endif
}

} // namespace vigil::detail
