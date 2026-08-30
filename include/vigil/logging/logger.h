// -----------------------------------------------------------------------------
//  Copyright (c) 2026 @DMsuDev. Licensed under the MIT License.
//  See LICENSE file in the project root for full license text.
// -----------------------------------------------------------------------------

#pragma once

#include "vigil/detail/symbol_export.h"
#include "vigil/core/smart_pointers.h"

#include "vigil/logging/log_level.h"

#include <fmt/format.h>

#include <string>
#include <string_view>
#include <utility>

namespace vigil {

namespace detail {

/// @brief Alias shortening `fmt::format_string<Args...>` at every logging call site.
template <typename... Args>
using FormatString = fmt::format_string<Args...>;

// Forward declaration of the internal logger implementation class.
class LoggerImpl;

} // namespace detail

/**
 * @brief Lightweight handle to a logger managed by the Logger registry.
 *
 * Logger provides the public interface for interacting with an
 * underlying logger instance, including emitting log messages, configuring
 * its severity threshold, and flushing its sinks. Instances are created and
 * owned by the Logger registry and are obtained through @ref Logger::Main,
 * @ref Logger::Create, or @ref Logger::Get.
 */
class VIGIL_API Logger {
public:
    /// @brief Name assigned to this logger instance.
    [[nodiscard]] std::string_view GetName() const noexcept;

    /// @brief Sets the minimum severity level for this logger.
    void SetLevel(LogLevel level);

    /// @brief Retrieves the current minimum severity level.
    [[nodiscard]] LogLevel GetLevel() const noexcept;

    /// @brief Flushes all sinks associated with this logger immediately.
    void Flush();

    /// @brief Logs @p message verbatim (no formatting) at Trace severity.
    void Trace(std::string_view message);

    /// @brief Logs @p message verbatim (no formatting) at Debug severity.
    void Debug(std::string_view message);

    /// @brief Logs @p message verbatim (no formatting) at Info severity.
    void Info(std::string_view message);

    /// @brief Logs @p message verbatim (no formatting) at Warn severity.
    void Warn(std::string_view message);

    /// @brief Logs @p message verbatim (no formatting) at Error severity.
    void Error(std::string_view message);

    /// @brief Logs @p message verbatim (no formatting) at Critical severity.
    void Critical(std::string_view message);

    /// @brief Logs @p message verbatim (no formatting) at the given @p level.
    void Log(LogLevel level, std::string_view message);

    /// @brief Formats @p message with @p args using `fmt` syntax and logs it at Trace severity.
    template <typename... Args>
    void Trace(detail::FormatString<Args...> message, Args&&... args)
    {
        Log(LogLevel::Trace, message, std::forward<Args>(args)...);
    }

    /// @brief Formats @p message with @p args using `fmt` syntax and logs it at Debug severity.
    template <typename... Args>
    void Debug(detail::FormatString<Args...> message, Args&&... args)
    {
        Log(LogLevel::Debug, message, std::forward<Args>(args)...);
    }

    /// @brief Formats @p message with @p args using `fmt` syntax and logs it at Info severity.
    template <typename... Args>
    void Info(detail::FormatString<Args...> message, Args&&... args)
    {
        Log(LogLevel::Info, message, std::forward<Args>(args)...);
    }

    /// @brief Formats @p message with @p args using `fmt` syntax and logs it at Warn severity.
    template <typename... Args>
    void Warn(detail::FormatString<Args...> message, Args&&... args)
    {
        Log(LogLevel::Warn, message, std::forward<Args>(args)...);
    }

    /// @brief Formats @p message with @p args using `fmt` syntax and logs it at Error severity.
    template <typename... Args>
    void Error(detail::FormatString<Args...> message, Args&&... args)
    {
        Log(LogLevel::Error, message, std::forward<Args>(args)...);
    }

    /// @brief Formats @p message with @p args using `fmt` syntax and logs it at Critical severity.
    template <typename... Args>
    void Critical(detail::FormatString<Args...> message, Args&&... args)
    {
        Log(LogLevel::Critical, message, std::forward<Args>(args)...);
    }

    /// @brief Formats @p fmt with @p args using `fmt` syntax and logs the result at @p level.
    template <typename... Args>
    void Log(LogLevel level, detail::FormatString<Args...> fmt, Args&&... args)
    {
        LogImpl(level, fmt::format(fmt, std::forward<Args>(args)...));
    }

    /// @cond INTERNAL
    explicit Logger(Shared<detail::LoggerImpl> impl) noexcept;
    detail::LoggerImpl& Impl() noexcept { return *m_Impl; }
    /// @endcond

private:
    /// @cond INTERNAL
    void LogImpl(LogLevel level, const std::string& formatted);
    /// @endcond

    #if defined(VIGIL_COMPILER_MSVC)
        #pragma warning(push)
        #pragma warning(disable : 4251)
    #endif

    Shared<detail::LoggerImpl> m_Impl;

    #if defined(VIGIL_COMPILER_MSVC)
        #pragma warning(pop)
    #endif
};

} // namespace vigil
