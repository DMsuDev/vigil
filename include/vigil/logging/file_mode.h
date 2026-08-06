// -----------------------------------------------------------------------------
//  Copyright (c) 2026 @DMsuDev. Licensed under the MIT License.
//  See LICENSE file in the project root for full license text.
// -----------------------------------------------------------------------------

#pragma once

namespace vigil {

/// @brief Controls whether a sink's log file is preserved or reset on startup.
enum class FileOpenMode {
    Append,   ///< Keep existing log content; new entries are appended.
    Truncate, ///< Clear the file's previous content on open.
};

} // namespace vigil
