// -----------------------------------------------------------------------------
//  Copyright (c) 2026 @DMsuDev. Licensed under the MIT License.
//  See LICENSE file in the project root for full license text.
// -----------------------------------------------------------------------------

#include "vigil/detail/debugger.h"
#include "vigil/detail/platform_detection.h"

#include <cstddef>

#if defined(VIGIL_PLATFORM_WINDOWS)
    #include <Windows.h>
#elif defined(VIGIL_PLATFORM_LINUX)
    #include <cstdio>
#elif defined(VIGIL_PLATFORM_MACOS)
    #include <sys/sysctl.h>
    #include <unistd.h>
#endif

namespace vigil::detail {

bool IsDebuggerAttached() noexcept
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
