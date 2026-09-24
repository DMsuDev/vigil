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

#include <stdexcept>

namespace vigil::crash_triggers {

[[noreturn]] inline void ThrowFromNoexcept() noexcept
{
    throw std::runtime_error("intentional throw from noexcept — triggers std::terminate()");
}

[[noreturn]] inline void TriggerTerminate()
{
    ThrowFromNoexcept();
}

} // namespace vigil::crash_triggers
