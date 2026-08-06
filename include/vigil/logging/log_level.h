// -----------------------------------------------------------------------------
//  Copyright (c) 2026 @DMsuDev. Licensed under the MIT License.
//  See LICENSE file in the project root for full license text.
// -----------------------------------------------------------------------------

#pragma once

#include <cstdint>

/**
 * @file log_level.h
 * @brief Severity levels, plain-macro form (preprocessor) and enum form (runtime).
 *
 * The preprocessor cannot evaluate scoped enum values in #if, so the
 * VIGIL_LOG_LEVEL_* macros exist purely for compile-time gating. LogLevel
 * is defined in terms of them so both stay in sync by construction.
 */

#define VIGIL_LOG_LEVEL_TRACE    0
#define VIGIL_LOG_LEVEL_DEBUG    1
#define VIGIL_LOG_LEVEL_INFO     2
#define VIGIL_LOG_LEVEL_WARN     3
#define VIGIL_LOG_LEVEL_ERROR    4
#define VIGIL_LOG_LEVEL_CRITICAL 5
#define VIGIL_LOG_LEVEL_OFF      6

#ifndef VIGIL_ACTIVE_LOG_LEVEL
    #define VIGIL_ACTIVE_LOG_LEVEL VIGIL_LOG_LEVEL_TRACE
#endif

namespace vigil {

/// @brief Severity level for a log message. Lower values are more verbose.
enum class LogLevel : uint8_t {
    Trace    = VIGIL_LOG_LEVEL_TRACE,    ///< Highly detailed, per-call diagnostic output.
    Debug    = VIGIL_LOG_LEVEL_DEBUG,    ///< Debug-level messages, typically for development.
    Info     = VIGIL_LOG_LEVEL_INFO,     ///< General informational messages.
    Warn     = VIGIL_LOG_LEVEL_WARN,     ///< Warnings about potential issues.
    Error    = VIGIL_LOG_LEVEL_ERROR,    ///< Error events that might still allow the application to continue.
    Critical = VIGIL_LOG_LEVEL_CRITICAL, ///< Critical errors causing premature termination.
    Off      = VIGIL_LOG_LEVEL_OFF,      ///< Logging disabled.
};

/**
 * @brief Checks whether a severity level survives the compile-time gate (@ref VIGIL_ACTIVE_LOG_LEVEL).
 *
 * @details Mirrors the condition used by the `VIGIL_*` macros, for call sites that build a message
 *          manually (e.g. to avoid expensive formatting) instead of going through a macro.
 *
 * @param level Severity level to check.
 * @return @c true if @p level is enabled by the active compile-time gate.
 */
constexpr bool IsLevelActive(LogLevel level) noexcept
{
    return static_cast<int>(level) >= VIGIL_ACTIVE_LOG_LEVEL;
}

} // namespace vigil
