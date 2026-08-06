// -----------------------------------------------------------------------------
//  Copyright (c) 2026 @DMsuDev. Licensed under the MIT License.
//  See LICENSE file in the project root for full license text.
// -----------------------------------------------------------------------------

#pragma once

#include "vigil/detail/symbol_export.h"

#include <string>
#include <string_view>

namespace vigil::detail {

enum class FormatPathMode {
    FullPath,   ///< Return the full path as-is (e.g., "C:\path\to\file.cpp").
    TailPath,   ///< Return the last two components of the path (e.g., "to\file.cpp").
    FileName,   ///< Extract and return only the file name component (e.g., "file.cpp").
};

/// @brief Formats a file path down to a configurable number of trailing
///        directory components, always keeping the file name.
/// @param path   Source path (may use '/' or '\\' separators).
/// @param levels How many trailing directory components to keep in addition
///               to the file name. 0 = return the full, unmodified path.
///               If @p levels exceeds the number of components available,
///               the full path is returned (clamped, not an error).
/// @return A view into @p path's storage - callers must ensure @p path
///         outlives the returned view.
[[nodiscard]] VIGIL_API
std::string_view FormatFilePath(std::string_view path, unsigned levels = 1);

/// @brief Sanitizes a raw function signature by stripping qualifiers, calling conventions, and noise.
/// @param signature Raw function signature to process.
/// @return Cleaned, human-readable function signature string.
[[nodiscard]] VIGIL_API
std::string CleanFunctionSignature(std::string_view signature);

} // namespace vigil::detail
