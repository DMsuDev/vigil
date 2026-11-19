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

#include "vigil/detail/platform_detection.h"

#include <csignal>
#include <cstdlib>

namespace vigil::crash_triggers {

[[noreturn]] inline void TriggerSigFpe()
{
#if defined(VIGIL_PLATFORM_WINDOWS) || defined(VIGIL_ARCH_X64) || defined(VIGIL_ARCH_X86)
    volatile int numerator   = 1;
    volatile int denominator = 0;
    volatile int result      = numerator / denominator;
    (void)result;
#else
    std::raise(SIGFPE); // AArch64 does not trap on integer division by zero, so we raise SIGFPE manually.
#endif
    std::abort();
}

} // namespace vigil::crash_triggers
