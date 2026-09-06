// -----------------------------------------------------------------------------
//  Copyright (c) 2026 @DMsuDev. Licensed under the MIT License.
//  See LICENSE file in the project root for full license text.
// -----------------------------------------------------------------------------

#pragma once

// This header is internal to the Vigil implementation and is never installed
// or exposed to consumers. It is the only place in the codebase allowed to
// include spdlog directly.

#include <spdlog/spdlog.h>

#include "vigil/logging/log_level.h"
#include "vigil/core/smart_pointers.h"
#include "vigil/detail/symbol_export.h"

#include <string_view>

namespace vigil::detail {

/// @brief Wraps a single @c spdlog::logger instance and its associated sinks.
class LoggerImpl {
public:
    LoggerImpl(Shared<spdlog::logger> logger,
               spdlog::sink_ptr       consoleSink,
               spdlog::sink_ptr       fileSink);

    LoggerImpl(const LoggerImpl&)            = delete;
    LoggerImpl& operator=(const LoggerImpl&) = delete;
    LoggerImpl(LoggerImpl&&)                 = delete;
    LoggerImpl& operator=(LoggerImpl&&)      = delete;

    /// @brief Logs a raw, already-formatted message at the given level.
    void Log(LogLevel level, std::string_view message);

    /// @brief Sets the runtime filtering level.
    void SetLevel(LogLevel level);

    /// @brief Returns the current runtime filtering level.
    [[nodiscard]] LogLevel GetLevel() const noexcept;

    /// @brief Flushes all sinks immediately.
    void Flush();

    /// @brief Attaches an arbitrary sink to the underlying logger.
    /// Intended for white-box tests only. Not for general use.
    VIGIL_API void AttachSink(spdlog::sink_ptr sink);

    [[nodiscard]] Shared<spdlog::logger> SpdLogger() const noexcept;
    [[nodiscard]] spdlog::sink_ptr FileSink() const noexcept;
    [[nodiscard]] spdlog::sink_ptr ConsoleSink() const noexcept;

    [[nodiscard]] VIGIL_API std::string_view Name() const noexcept;

private:
    Shared<spdlog::logger> m_Logger;
    spdlog::sink_ptr       m_ConsoleSink;
    spdlog::sink_ptr       m_FileSink;
};

} // namespace vigil::detail
