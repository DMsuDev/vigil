// -----------------------------------------------------------------------------
//  Copyright (c) 2026 @DMsuDev. Licensed under the MIT License.
//  See LICENSE file in the project root for full license text.
// -----------------------------------------------------------------------------

#pragma once

#include "vigil/logging/log_level.h"

namespace vigil::detail {

class LoggerImpl;

/// @brief Applies @p level to @p impl and notifies OnLevelChange outside internal locks.
/// @note Calls from OnMessage or OnLevelChange callbacks update the level silently.
void ChangeLevel(LoggerImpl& impl, LogLevel level);

} // namespace vigil::detail
