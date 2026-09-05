// -----------------------------------------------------------------------------
//  Copyright (c) 2026 @DMsuDev. Licensed under the MIT License.
//  See LICENSE file in the project root for full license text.
// -----------------------------------------------------------------------------

#pragma once

#include <cstdint>

#include "vigil/detail/compiler_attributes.h"

/**
 * @file log_level.h
 * @brief Severity levels in preprocessor form (compile-time) and enum form (runtime).
 *
 * The preprocessor cannot evaluate scoped enum values in `#if` directives, so
 * the `VIGIL_LEVEL_*` integer macros exist solely for compile-time gating.
 * `LogLevel` is defined in terms of them so both representations stay in sync
 * by construction.
 */

// Preprocessor-only severity constants (for #if gating).
#define VIGIL_LEVEL_TRACE    0
#define VIGIL_LEVEL_DEBUG    1
#define VIGIL_LEVEL_INFO     2
#define VIGIL_LEVEL_WARN     3
#define VIGIL_LEVEL_ERROR    4
#define VIGIL_LEVEL_CRITICAL 5
#define VIGIL_LEVEL_OFF      6

#ifndef VIGIL_ACTIVE_LOG_LEVEL
    #define VIGIL_ACTIVE_LOG_LEVEL VIGIL_LEVEL_TRACE
#endif

namespace vigil {

/// @brief Severity level for a log message. Lower values are more verbose.
enum class LogLevel : uint8_t {
    Trace    = VIGIL_LEVEL_TRACE,    ///< Highly detailed, per-call diagnostic output.
    Debug    = VIGIL_LEVEL_DEBUG,    ///< Development-time diagnostics.
    Info     = VIGIL_LEVEL_INFO,     ///< General informational messages.
    Warn     = VIGIL_LEVEL_WARN,     ///< Warnings about potential issues.
    Error    = VIGIL_LEVEL_ERROR,    ///< Recoverable errors.
    Critical = VIGIL_LEVEL_CRITICAL, ///< Fatal errors causing premature termination.
    Off      = VIGIL_LEVEL_OFF,      ///< Logging disabled.
};

VIGIL_PRAGMA_PUSH_WARNING
VIGIL_DISABLE_WARNING_MSVC(4296)
VIGIL_DISABLE_WARNING_GNU("-Wtype-limits")

/**
 * @brief Returns @c true if @p level survives the compile-time gate.
 *
 * Mirrors the condition used by the `VIGIL_LOG_*` macros for call sites that
 * construct a message manually to avoid expensive formatting.
 */
[[nodiscard]] constexpr bool IsLevelActive(LogLevel level) noexcept
{
    return static_cast<uint8_t>(level) >= static_cast<uint8_t>(VIGIL_ACTIVE_LOG_LEVEL);
}

VIGIL_PRAGMA_POP_WARNING

} // namespace vigil
