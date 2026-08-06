// -----------------------------------------------------------------------------
//  Copyright (c) 2026 @DMsuDev. Licensed under the MIT License.
//  See LICENSE file in the project root for full license text.
// -----------------------------------------------------------------------------

#pragma once

#include "vigil/detail/symbol_export.h"

/**
 * @file console.h
 * @brief Small utilities for configuring the host console, independent of logging.
 */

namespace vigil {

/**
 * @brief Configures the current console host to utilize UTF-8 encoding.
 *
 * @details Windows consoles default to legacy code pages, which prevents
 * Unicode text from rendering or being read correctly. This function switches
 * both the console input and output code pages to UTF-8 (`CP_UTF8`), ensuring
 * consistent Unicode character handling across log streams and standard input.
 *
 * @note For optimal results, this function should be invoked during application
 * initialization before the program performs any console I/O operations.
 *
 * @note This function is a silent no-op on non-Windows platforms.
 */
VIGIL_API bool EnableUTF8Console();

} // namespace vigil
