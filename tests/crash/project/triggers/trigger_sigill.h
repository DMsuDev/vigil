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

namespace vigil::crash_triggers {

[[noreturn]] inline void TriggerSigIll()
{
    VIGIL_DEBUGBREAK_IF_ATTACHED();
    VIGIL_UNREACHABLE();
}

} // namespace vigil::crash_triggers
