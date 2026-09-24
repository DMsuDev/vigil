// -----------------------------------------------------------------------------
//   _    __ __ _____ ___ __
//  | |  / /  _/ ____/  _/ /
//  | | / // // / __ / // /     Logging and Diagnostics for C++
//  | |/ // // /_/ // // /___   https://github.com/DMsuDev/vigil
//  |___/___/\____/___/_____/
//
//  Copyright (c) 2026 @DMsuDev. Licensed under the MIT License.
//  See LICENSE file in the project root for full license text.
// -----------------------------------------------------------------------------

#pragma once

#if defined(VIGIL_PLATFORM_WINDOWS)

#include "vigil/detail/preprocessor_utils.h"

#include <cstdio>

namespace vigil::crash_triggers {

[[noreturn]] inline void TriggerInvalidParam()
{
    // printf_s with a null format string is the canonical way to trigger
    // the CRT invalid parameter handler without relying on internal symbols.
    printf_s(nullptr);
    VIGIL_UNREACHABLE();
}

} // namespace vigil::crash_triggers

#endif // VIGIL_PLATFORM_WINDOWS
