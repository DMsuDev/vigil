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

#include "vigil/detail/preprocessor_utils.h"
#include "vigil/detail/compiler_attributes.h"

#include <csignal>
#include <cstdlib>

#if defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))
    #include <intrin.h>
#endif

namespace vigil::crash_triggers {

[[noreturn]] inline void TriggerSigIll()
{
#if defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))
    __ud2();
#elif VIGIL_HAS_BUILTIN(__builtin_trap)
    __builtin_trap();
#else
    std::raise(SIGILL);
#endif
    std::abort();
}

} // namespace vigil::crash_triggers
