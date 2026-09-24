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

#include <new>
#include <cstddef>
#include <limits>

namespace vigil::crash_triggers {

[[noreturn]] inline void TriggerBadAlloc()
{
    // Requesting SIZE_MAX bytes is guaranteed to fail on every platform.
    // The volatile cast prevents the compiler from constant-folding this
    // into a no-op or an unconditional throw at compile time.
    volatile std::size_t size = std::numeric_limits<std::size_t>::max();
    void* p = ::operator new(size);
    (void)p;
    VIGIL_UNREACHABLE();
}

} // namespace vigil::crash_triggers
