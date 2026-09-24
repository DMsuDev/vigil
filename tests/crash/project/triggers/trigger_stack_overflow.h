// -----------------------------------------------------------------------------
//  Copyright (c) 2026 @DMsuDev. Licensed under the MIT License.
//  See LICENSE file in the project root for full license text.
// -----------------------------------------------------------------------------

#pragma once

#include "vigil/detail/compiler_attributes.h"

namespace vigil::crash_triggers {

[[noreturn]] VIGIL_NOINLINE inline void TriggerStackOverflow(int depth = 0)
{
    volatile char frame[512];
    frame[0] = static_cast<char>(depth);
    TriggerStackOverflow(depth + 1);
    (void)frame; // prevent the array from being optimized away before the call
}

} // namespace vigil::crash_triggers
