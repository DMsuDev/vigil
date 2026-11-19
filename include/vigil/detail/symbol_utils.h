// -----------------------------------------------------------------------------
//  Copyright (c) 2026 @DMsuDev. Licensed under the MIT License.
//  See LICENSE file in the project root for full license text.
// -----------------------------------------------------------------------------

#pragma once

#include "vigil/detail/symbol_export.h"

#include <string>
#include <string_view>

namespace vigil::detail {

/// @brief Returns a path containing only the requested trailing components.
/// @param path   Source path (may use '/' or '\\' separators).
/// @param components Number of trailing components to keep.
///                   0 = full path; 1 = filename; 2 = parent/filename.
/// @return A view into @p path. The source storage must outlive the returned view.
[[nodiscard]] VIGIL_API
std::string_view FormatFilePath(std::string_view path, unsigned components = 1);

/// @brief Normalizes path separators to use '/' instead of '\\'.
/// @param path Source path (may use '/' or '\\' separators).
/// @return A new string with normalized path separators.
[[nodiscard]] VIGIL_API
std::string NormalizePathSeparators(std::string_view path);

/// @brief Removes compiler-specific noise from a function signature.
/// @param signature Raw function signature to process.
/// @return A cleaned, human-readable function signature.
[[nodiscard]] VIGIL_API
std::string CleanFunctionSignature(std::string_view signature);

} // namespace vigil::detail
