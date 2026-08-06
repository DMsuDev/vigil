// -----------------------------------------------------------------------------
//  Copyright (c) 2026 @DMsuDev. Licensed under the MIT License.
//  See LICENSE file in the project root for full license text.
// -----------------------------------------------------------------------------

#include "vigil/core/console.h"
#include "vigil/detail/platform_detection.h"

#if defined(VIGIL_PLATFORM_WINDOWS)
    #include <Windows.h>
#endif

namespace vigil {

bool EnableUTF8Console()
{
#if defined(VIGIL_PLATFORM_WINDOWS)
    bool success = true;

    if (GetConsoleOutputCP() != CP_UTF8)
        success &= (SetConsoleOutputCP(CP_UTF8) != FALSE);

    if (GetConsoleCP() != CP_UTF8)
        success &= (SetConsoleCP(CP_UTF8) != FALSE);

    return success;
#else
    return true;
#endif
}

} // namespace vigil
