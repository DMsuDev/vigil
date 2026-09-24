// -----------------------------------------------------------------------------
//  Copyright (c) 2026 @DMsuDev. Licensed under the MIT License.
//  See LICENSE file in the project root for full license text.
// -----------------------------------------------------------------------------

#pragma once

#include <cstdlib>

namespace vigil::crash_triggers {

[[noreturn]] inline void TriggerSigAbrt()
{
    std::abort();
}

} // namespace vigil::crash_triggers
