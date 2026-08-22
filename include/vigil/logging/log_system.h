// -----------------------------------------------------------------------------
//  Copyright (c) 2026 @DMsuDev. Licensed under the MIT License.
//  See LICENSE file in the project root for full license text.
// -----------------------------------------------------------------------------

#pragma once

#include "vigil/logging/logger.h"
#include "vigil/detail/symbol_export.h"

#include <optional>
#include <utility>

/**
 * @file log_system.h
 * @brief Main facade and entry point for Vigil's logging system.
 *
 * Provides thread-safe access to a centralized main logger and to dynamically
 * created named loggers. By default, all loggers share the same sink
 * configuration unless explicitly customized.
 *
 * ### Example
 *
 * @code{.cpp}
 * vigil::LogSystem::Init({
 *     .Name = "Editor",
 * });
 *
 * VIGIL_INFO("Application initialized successfully");
 *
 * auto& netLogger = vigil::LogSystem::Create("Network");
 * VIGIL_LOG_NAMED("Network", vigil::LogLevel::Warn, "Connection retry attempt #{}", attempt);
 * @endcode
 */

namespace vigil {

struct LogSystemConfig
{
    /// @brief Name assigned to the main logger.
    std::string Name = "Main";

    /// @brief Path to the main log file. Defaults to `Name + ".log"` if left empty.
    std::string LogFile;

    /// @brief Minimum severity threshold required for messages to be printed to the console.
    std::optional<LogLevel> ConsoleLevel;

    /// @brief Enables asynchronous logging using a background thread pool to minimize call-site latency.
    bool Async = false;

    /// @brief Size of the internal queue used for asynchronous logging.
    uint32_t AsyncQueueSize = 8192;
};

struct LogConfig
{
    /// @brief Name assigned to the logger.
    std::string Name = "New";

    /// @brief Path to the output log file. Defaults to `Name + ".log"` if left empty.
    std::string LogFile;

    /// @brief File opening mode (`Append` to retain prior contents, `Truncate` to overwrite).
    FileOpenMode FileMode = FileOpenMode::Append;

    /// @brief Minimum severity threshold required for messages to be written to the log file.
    std::optional<LogLevel> FileLevel;
};

 /**
 * @brief Centralized registry and entry point for Vigil's loggers.
 *
 * `LogSystem` acts as a static registry controlling the lifecycle, setup, and
 * retrieval of named logger instances. A default "Main" logger is created upon
 * invoking @ref Init.
 *
 * Subsystem-specific loggers can be created dynamically via @ref LogSystem::Create. By default,
 * all loggers share the main file sink to consolidate output, but can be configured
 * with dedicated files or custom severity thresholds as needed.
 */
class VIGIL_API LogSystem {
public:
    /**
     * @brief Initializes the primary logging subsystem and creates the main logger.
     *
     * Configures the main logger, background processing threads (if asynchronous),
     * and default file/console sinks.
     *
     * @param config Setup configuration options. Defaults to standard settings.
     *
     * @note Must be called before issuing log operations. Subsequent invocations
     *       after initial setup are ignored.
     */
    static void Init(const LogSystemConfig& config = {});

    /// @brief Flushes all pending log messages and releases every registered logger.
    static void Shutdown();

    /// @brief True after Init() has completed successfully.
    [[nodiscard]] static bool IsInitialized() noexcept;

    /**
     * @brief Returns the named logger associated with @p name, creating it on
     *        first use if necessary.
     *
     * If a logger with @p name already exists, the cached instance is returned.
     * Otherwise, a new logger is created sharing the main logger's file sink.
     *
     * @param name Unique identifier for this logger.
     *
     * @return Reference to the existing or newly created logger.
     *
     * @throws std::logic_error if called before @ref Init.
     */
    static Logger& Create(std::string_view name);

    /**
     * @brief Creates or retrieves a named logger using an explicit configuration.
     *
     * Unlike the simpler @ref LogSystem::Create overload, this version allows per-logger
     * customization of file mode, sink severity levels, asynchronous behavior,
     * and other configuration options.
     *
     * If a logger with the same name already exists, the existing instance is
     * returned.
     *
     * @param config Full configuration options applied to the named logger.
     *
     * @return Reference to the requested @ref Logger instance.
     *
     * @throws std::logic_error if called before @ref Init.
     */
    static Logger& Create(const LogConfig& config);

    // ============================================================ //
    //                  LOGGER LIFECYCLE MANAGEMENT                 //
    // ============================================================ //

    /**
     * @brief Removes a named logger from the registry, releasing its sinks.
     *
     * Has no effect if no logger is registered under @p name. The main logger
     * cannot be removed through this function; use @ref SetMain to replace it.
     *
     * @param name Name of the logger to remove.
     */
    static void Remove(std::string_view name);

    /**
     * @brief Replaces the current main logger with an existing named logger.
     *
     * The named logger identified by @p name is promoted to become the result
     * of @ref Main and is removed from the named logger registry. The logger
     * previously returned by @ref Main is discarded.
     *
     * @param name Name of a previously created named logger (see @ref Create).
     *
     * @note Has no effect if no logger is registered under @p name.
     */
    static void SetMain(std::string_view name);

    // ============================================================ //
    //                     LOGGER GETTERS                           //
    // ============================================================ //

    /**
     * @brief Returns the default logger instance.
     *
     * Retrieves the main logger registered at startup. This instance is created
     * during @ref Init and is always valid for the lifetime of the registry.
     *
     * @throws std::logic_error if called before @ref Init.
     */
    [[nodiscard]] static Logger& Main();

    /**
     * @brief Returns a named logger with the given @p name.
     *
     * Retrieves the logger registered under @p name. This overload throws
     * if no logger with the given name exists; callers should use @ref Find()
     * when existence is not guaranteed.
     *
     * @param name The registered name of the logger.
     * @return The requested Logger associated with @p name.
     *
     * @throws std::logic_error if called before @ref Init.
     * @throws std::runtime_error if no logger is registered under @p name.
     */
    [[nodiscard]] static Logger& Get(std::string_view name);

    /**
     * @brief Finds a registered logger by @p name.
     *
     * Searches the registry for a logger associated with @p name. Unlike @ref Get(),
     * this function never asserts or throws if the logger does not exist.
     *
     * @param name Name of the registered logger.
     * @return A pointer to the logger if found, or @c nullptr if no matching logger exists.
     */
    [[nodiscard]] static Logger* Find(std::string_view name);

    // =============================================================================
    // Global log level control
    // =============================================================================

    /**
     * @brief Sets the global minimum severity level for all loggers.
     *
     * This function overrides any per-logger level settings, effectively
     * filtering out log messages below the specified level across the entire
     * logging system.
     *
     * @param level The new global minimum severity level.
     */
    static void SetGlobalLevel(LogLevel level);

    /**
     * @brief Sets the global minimum severity level for all loggers' file sinks.
     *
     * This function overrides any per-logger file level settings, effectively
     * filtering out log messages below the specified level for all file outputs.
     *
     * @param level The new global minimum severity level for file sinks.
     */
    static void SetGlobalFileLevel(LogLevel level);

    /**
     * @brief Sets the log level for the shared console sink.
     *
     * Changes the minimum severity level of the global console sink. The new
     * level is applied to all loggers using this sink.
     *
     * @param level The minimum severity level to be displayed in the console.
     */
    static void SetConsoleLevel(LogLevel level);

    // =============================================================================
    // Flush control
    // =============================================================================

    /**
     * @brief Flushes all registered loggers.
     *
     * Forces every logger to immediately write any buffered log messages to
     * their associated sinks.
     */
    static void FlushAll();

    /**
     * @brief Flushes a specific named logger.
     * @param name Name of the logger to flush.
     */
    static void Flush(std::string_view name);
};

// ============================================================================
// Free-function logging API
// ============================================================================

/**
 * @name Free-Function Logging API
 * @brief `spdlog`-style free functions that log through @ref LogSystem::Main.
 *
 * Equivalent in behavior to the @ref VIGIL_TRACE "main logger macros", provided
 * as plain functions for call sites that prefer to avoid macros. Both forms are
 * fully supported and interchangeable.
 * @{
 */

/// @copydoc Logger::Trace(std::string_view)
inline void Trace(std::string_view message) { LogSystem::Main().Trace(message); }

/// @copydoc Logger::Debug(std::string_view)
inline void Debug(std::string_view message) { LogSystem::Main().Debug(message); }

/// @copydoc Logger::Info(std::string_view)
inline void Info(std::string_view message) { LogSystem::Main().Info(message); }

/// @copydoc Logger::Warn(std::string_view)
inline void Warn(std::string_view message) { LogSystem::Main().Warn(message); }

/// @copydoc Logger::Error(std::string_view)
inline void Error(std::string_view message) { LogSystem::Main().Error(message); }

/// @copydoc Logger::Critical(std::string_view)
inline void Critical(std::string_view message) { LogSystem::Main().Critical(message); }

/// @copydoc Logger::Log(LogLevel, std::string_view)
inline void Log(LogLevel level, std::string_view message) { LogSystem::Main().Log(level, message); }

/// @copydoc Logger::Trace(detail::FormatString<Args...>, Args&&...)
template <typename... Args>
void Trace(detail::FormatString<Args...> message, Args&&... args)
{
    LogSystem::Main().Trace(message, std::forward<Args>(args)...);
}

/// @copydoc Logger::Debug(detail::FormatString<Args...>, Args&&...)
template <typename... Args>
void Debug(detail::FormatString<Args...> message, Args&&... args)
{
    LogSystem::Main().Debug(message, std::forward<Args>(args)...);
}

/// @copydoc Logger::Info(detail::FormatString<Args...>, Args&&...)
template <typename... Args>
void Info(detail::FormatString<Args...> message, Args&&... args)
{
    LogSystem::Main().Info(message, std::forward<Args>(args)...);
}

/// @copydoc Logger::Warn(detail::FormatString<Args...>, Args&&...)
template <typename... Args>
void Warn(detail::FormatString<Args...> message, Args&&... args)
{
    LogSystem::Main().Warn(message, std::forward<Args>(args)...);
}

/// @copydoc Logger::Error(detail::FormatString<Args...>, Args&&...)
template <typename... Args>
void Error(detail::FormatString<Args...> message, Args&&... args)
{
    LogSystem::Main().Error(message, std::forward<Args>(args)...);
}

/// @copydoc Logger::Critical(detail::FormatString<Args...>, Args&&...)
template <typename... Args>
void Critical(detail::FormatString<Args...> message, Args&&... args)
{
    LogSystem::Main().Critical(message, std::forward<Args>(args)...);
}

/// @copydoc Logger::Log(LogLevel, detail::FormatString<Args...>, Args&&...)
template <typename... Args>
void Log(LogLevel level, detail::FormatString<Args...> message, Args&&... args)
{
    LogSystem::Main().Log(level, message, std::forward<Args>(args)...);
}

/** @} */

} // namespace vigil


// ============================================================================
// Main Logger Macros
// ============================================================================

/**
 * @name Main Logger Macros
 * @brief Convenience macros for logging through the application's main logger.
 *
 * compile-time filtering based on @ref VIGIL_ACTIVE_LOG_LEVEL.
 *
 * @note The macros assume the main logger is accessed via @ref LogSystem::Main.
 * compile-time filtering based on @ref VIGIL_ACTIVE_LOG_LEVEL.
 *
 * Disabled log statements are completely eliminated during compilation.
 * @{
 */

 /**
 * @internal
 * @def VIGIL_LOG_COMMON
 * @brief Logs a message at the associated severity level with compile-time filtering.
 *
 * @details Forwards all arguments to the corresponding vigil::Trace/Debug/Info/Warn/Error/Critical
 *          free function, which logs through the main logger instance. Accepts either a plain
 *          message or a format string with arguments.
 *
 * @note When the severity level associated with this macro is disabled by
 *       @ref VIGIL_ACTIVE_LOG_LEVEL, the macro expands to @c ((void)0) and
 *       none of its arguments are evaluated.
 *
 * @param ... The message to log, optionally followed by arguments to interpolate.
 * @endinternal
 */

/// @def VIGIL_TRACE
/// @copydoc VIGIL_LOG_COMMON
/// @brief Logs a Trace severity message using the main logger.
#if VIGIL_ACTIVE_LOG_LEVEL <= VIGIL_LOG_LEVEL_TRACE
    #define VIGIL_TRACE(...) ::vigil::Trace(__VA_ARGS__)
#else
    #define VIGIL_TRACE(...) ((void)0)
#endif

/// @def VIGIL_DEBUG
/// @copydoc VIGIL_LOG_COMMON
/// @brief Logs a Debug severity message using the main logger.
#if VIGIL_ACTIVE_LOG_LEVEL <= VIGIL_LOG_LEVEL_DEBUG
    #define VIGIL_DEBUG(...) ::vigil::Debug(__VA_ARGS__)
#else
    #define VIGIL_DEBUG(...) ((void)0)
#endif

/// @def VIGIL_INFO
/// @copydoc VIGIL_LOG_COMMON
/// @brief Logs an Info severity message using the main logger.
#if VIGIL_ACTIVE_LOG_LEVEL <= VIGIL_LOG_LEVEL_INFO
    #define VIGIL_INFO(...) ::vigil::Info(__VA_ARGS__)
#else
    #define VIGIL_INFO(...) ((void)0)
#endif

/// @def VIGIL_WARN
/// @copydoc VIGIL_LOG_COMMON
/// @brief Logs a Warn severity message using the main logger.
#if VIGIL_ACTIVE_LOG_LEVEL <= VIGIL_LOG_LEVEL_WARN
    #define VIGIL_WARN(...) ::vigil::Warn(__VA_ARGS__)
#else
    #define VIGIL_WARN(...) ((void)0)
#endif

/// @def VIGIL_ERROR
/// @copydoc VIGIL_LOG_COMMON
/// @brief Logs an Error severity message using the main logger.
#if VIGIL_ACTIVE_LOG_LEVEL <= VIGIL_LOG_LEVEL_ERROR
    #define VIGIL_ERROR(...) ::vigil::Error(__VA_ARGS__)
#else
    #define VIGIL_ERROR(...) ((void)0)
#endif

/// @def VIGIL_CRITICAL
/// @copydoc VIGIL_LOG_COMMON
/// @brief Logs a Critical severity message using the main logger.
#if VIGIL_ACTIVE_LOG_LEVEL <= VIGIL_LOG_LEVEL_CRITICAL
    #define VIGIL_CRITICAL(...) ::vigil::Critical(__VA_ARGS__)
#else
    #define VIGIL_CRITICAL(...) ((void)0)
#endif

/** @} */

/**
 * @name Named Logger Macros
 * @brief Convenience macros for logging through named loggers.
 *
 * These macros retrieve (or create) a logger identified by @p name and
 * forward the message using the supplied runtime severity level.
 *
 * Unlike the main logger macros, the log level is supplied at runtime,
 * allowing a single call site to emit messages at different severities.
 * @{
 */

/**
 * @def VIGIL_LOG_NAMED(name, level, ...)
 * @brief Emits a log message through a specific named logger instance.
 *
 * Retrieves (or creates) a named logger via @ref LogSystem::Create and logs
 * the payload if the specified @p level satisfies the compile-time gate (`VIGIL_ACTIVE_LOG_LEVEL`).
 *
 * @param name  Unique string identifier for the target subsystem logger.
 * @param level Runtime severity level (@ref vigil::LogLevel) for this entry.
 * @param ...   Format string (`fmt` syntax) followed by formatting arguments.
 *
 * @note Because @p level is evaluated at runtime, plain runtime checks are used instead of
 *       `if constexpr` to support dynamic severity levels.
 */
#define VIGIL_LOG_NAMED(name, level, ...)                           \
    do {                                                            \
        if (::vigil::IsLevelActive(level)) {                        \
            ::vigil::LogSystem::Get(name).Log(level, __VA_ARGS__);  \
        }                                                           \
    } while (0)

/** @} */
