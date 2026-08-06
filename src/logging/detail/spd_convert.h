// -----------------------------------------------------------------------------
//  Copyright (c) 2026 @DMsuDev. Licensed under the MIT License.
//  See LICENSE file in the project root for full license text.
// -----------------------------------------------------------------------------

#pragma once

// NOTE: This header is internal to the Vigil implementation and is never installed
// or exposed to consumers. It is the only place in the codebase allowed to include
// spdlog directly.

#include "vigil/logging/log_level.h"
#include <spdlog/spdlog.h>

#include "vigil/detail/symbol_export.h"

namespace vigil::detail {

[[nodiscard]] constexpr spdlog::level::level_enum ToSpdLevel(LogLevel level) noexcept
{
    switch (level) {
        case LogLevel::Trace:    return spdlog::level::trace;
        case LogLevel::Debug:    return spdlog::level::debug;
        case LogLevel::Info:     return spdlog::level::info;
        case LogLevel::Warn:     return spdlog::level::warn;
        case LogLevel::Error:    return spdlog::level::err;
        case LogLevel::Critical: return spdlog::level::critical;
        case LogLevel::Off:      return spdlog::level::off;
    }
    return spdlog::level::info;
}

[[nodiscard]] constexpr LogLevel FromSpdLevel(spdlog::level::level_enum level) noexcept
{
    switch (level) {
        case spdlog::level::trace:    return LogLevel::Trace;
        case spdlog::level::debug:    return LogLevel::Debug;
        case spdlog::level::info:     return LogLevel::Info;
        case spdlog::level::warn:     return LogLevel::Warn;
        case spdlog::level::err:      return LogLevel::Error;
        case spdlog::level::critical: return LogLevel::Critical;
        default:                      return LogLevel::Off;
    }
}

} // namespace vigil::detail
