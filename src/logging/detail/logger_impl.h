// -----------------------------------------------------------------------------
//  Copyright (c) 2026 @DMsuDev. Licensed under the MIT License.
//  See LICENSE file in the project root for full license text.
// -----------------------------------------------------------------------------

#pragma once

// This header is internal to the Vigil implementation and is never installed
// or exposed to consumers. It is the only place in the codebase allowed to
// include spdlog directly.

#if defined(_MSC_VER)
    #pragma warning(push, 0)
#elif defined(__GNUC__) || defined(__clang__)
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wall"
    #pragma GCC diagnostic ignored "-Wextra"
#endif

#include <spdlog/spdlog.h>

#if defined(_MSC_VER)
    #pragma warning(pop)
#elif defined(__GNUC__) || defined(__clang__)
    #pragma GCC diagnostic pop
#endif

#include "vigil/logging/log_level.h"
#include "vigil/core/smart_pointers.h"

#include <string>

namespace vigil::detail {

/// @brief Wraps a single @c spdlog::logger instance and its associated sinks.
class LoggerImpl {
public:
    Shared<spdlog::logger> m_Logger;
    spdlog::sink_ptr m_ConsoleSink;
    spdlog::sink_ptr m_FileSink;

    LoggerImpl(Shared<spdlog::logger> logger,
               spdlog::sink_ptr consoleSink,
               spdlog::sink_ptr fileSink)
        : m_Logger(std::move(logger))
        , m_ConsoleSink(std::move(consoleSink))
        , m_FileSink(std::move(fileSink))
    {}

    LoggerImpl(const LoggerImpl&)            = delete;
    LoggerImpl& operator=(const LoggerImpl&) = delete;
    LoggerImpl(LoggerImpl&&)                 = delete;
    LoggerImpl& operator=(LoggerImpl&&)      = delete;

    /// @brief Logs a raw, already-formatted message at the given level.
    void Log(LogLevel level, const std::string_view & message);

    /// @brief Sets the runtime filtering level.
    void SetLevel(LogLevel level);

    /// @brief Returns the current runtime filtering level.
    [[nodiscard]] LogLevel GetLevel() const noexcept;

    /// @brief Flushes all sinks immediately.
    void Flush();
};

} // namespace vigil::detail
