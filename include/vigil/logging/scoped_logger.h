// -----------------------------------------------------------------------------
//  Copyright (c) 2026 @DMsuDev. Licensed under the MIT License.
//  See LICENSE file in the project root for full license text.
// -----------------------------------------------------------------------------

#pragma once

#include "vigil/logging/log_system.h"
#include "vigil/detail/symbol_export.h"

#include "vigil/detail/symbol_utils.h"        // For CleanFunctionSignature()
#include "vigil/detail/compiler_attributes.h" // For VIGIL_CURRENT_FUNCTION()
#include "vigil/detail/preprocessor_utils.h"  // For VIGIL_CONCAT() and VIGIL_STRINGIFY()

#include <chrono>
#include <string>
#include <string_view>

/**
 * @file scoped_logger.h
 * @brief RAII utility for automatic scope entry/exit trace logging with elapsed time.
 *
 * Provides @ref vigil::ScopedLogger for explicit scope instrumentation and the
 * @ref VIGIL_SCOPED_LOG / @ref VIGIL_SCOPED_LOG_FUNCTION convenience macros.
 *
 * All declarations in this file are conditionally compiled under
 * `VIGIL_ENABLE_SCOPED_LOG`. When the option is disabled, the macros expand
 * to `((void)0)` and no overhead is incurred.
 *
 * ### Example
 *
 * @code{.cpp}
 * void Application::Run()
 * {
 *     VIGIL_SCOPED_LOG_FUNCTION();
 *
 *     {
 *         VIGIL_SCOPED_LOG("Loading configuration");
 *         // ...
 *     }
 * }
 * @endcode
 *
 * Produces:
 *
 * @code{.text}
 * [TRACE] >> void Application::Run()
 * [TRACE] >> Loading configuration
 * [TRACE] << Loading configuration (4 ms)
 * [TRACE] << void Application::Run() (197 ms)
 * @endcode
 */

namespace vigil {

/**
 * @brief RAII guard that emits entry and exit trace messages for a named scope.
 *
 * On construction, logs `>> <scope>` at the configured severity level through
 * the main logger. On destruction, logs `<< <scope> (<elapsed> ms)` with the
 * wall-clock time elapsed since construction.
 *
 * Non-copyable and non-movable to prevent accidental lifetime extension or
 * duplication of log entries.
 *
 * @note Controlled at compile time via `VIGIL_ENABLE_SCOPED_LOG`.
 *
 * @see VIGIL_SCOPED_LOG
 * @see VIGIL_SCOPED_LOG_FUNCTION
 */
class ScopedLogger {
public:
    /// @brief Tag type used to opt into the owning-string constructor.
    struct OwnedTag {};

    /**
     * @brief Constructs from a string literal or any `string_view`-compatible scope name.
     *
     * Use this overload with @ref VIGIL_SCOPED_LOG for zero-allocation instrumentation
     * of explicitly named scopes.
     *
     * @param scope String literal identifying the instrumented scope.
     * @param level Severity level for entry and exit messages. Defaults to @ref LogLevel::Trace.
     */
    explicit ScopedLogger(std::string_view scope, LogLevel level = LogLevel::Trace)
        : m_Scope(scope), m_Level(level), m_Start(std::chrono::steady_clock::now())
    {
        LogSystem::Main().Log(m_Level, ">> {}", m_Scope);
    }

    /**
     * @brief Constructs from an owning string, taking ownership via move.
     *
     * Used internally by @ref VIGIL_SCOPED_LOG_FUNCTION to safely own the
     * cleaned function signature produced by @ref vigil::detail::CleanFunctionSignature.
     *
     * @param scope  Cleaned function signature string to take ownership of.
     * @param        OwnedTag marker to explicitly select this owning overload.
     * @param level  Severity level for entry and exit messages. Defaults to @ref LogLevel::Trace.
     */
    explicit ScopedLogger(std::string&& scope, OwnedTag, LogLevel level = LogLevel::Trace)
        : m_Scope(std::move(scope))
        , m_Level(level)
        , m_Start(std::chrono::steady_clock::now())
    {
        LogSystem::Main().Log(m_Level, ">> {}", m_Scope);
    }

    /**
     * @brief Destroys the guard and emits the exit message with elapsed time.
     */
    ~ScopedLogger()
    {
        const auto elapsed = std::chrono::steady_clock::now() - m_Start;
        const auto ms      = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
        LogSystem::Main().Log(m_Level, "<< {} ({} ms)", m_Scope, ms);
    }

    ScopedLogger(const ScopedLogger&)            = delete;
    ScopedLogger& operator=(const ScopedLogger&) = delete;
    ScopedLogger(ScopedLogger&&)                 = delete;
    ScopedLogger& operator=(ScopedLogger&&)      = delete;

private:
    std::string                           m_Scope;
    LogLevel                              m_Level;
    std::chrono::steady_clock::time_point m_Start;
};

} // namespace vigil

// ============================================================================
// Scoped Logger Macros
// ============================================================================

#ifdef VIGIL_ENABLE_SCOPED_LOG

/**
 * @name Scoped Logger Macros
 * @{
 */

/**
 * @def VIGIL_SCOPED_LOG(scope)
 * @brief Instruments a named scope with entry/exit trace logging and elapsed time.
 * Expands to `((void)0)` when `VIGIL_ENABLE_SCOPED_LOG` is not defined.
 */
#define VIGIL_SCOPED_LOG(scope) \
    ::vigil::ScopedLogger VIGIL_CONCAT(_vigil_scope_, __LINE__)(scope)

/**
 * @def VIGIL_SCOPED_LOG_FUNCTION()
 * @brief Instruments the enclosing function using the compiler-provided signature.
 * The signature is cleaned of calling conventions via @ref vigil::detail::CleanFunctionSignature.
 * Expands to `((void)0)` when `VIGIL_ENABLE_SCOPED_LOG` is not defined.
 */
#define VIGIL_SCOPED_LOG_FUNCTION()                                 \
    ::vigil::ScopedLogger VIGIL_CONCAT(_vigil_scope_fn_, __LINE__)( \
        ::vigil::detail::CleanFunctionSignature(VIGIL_CURRENT_FUNCTION),    \
        ::vigil::ScopedLogger::OwnedTag{})

/** @} */

#else // !VIGIL_ENABLE_SCOPED_LOG

#define VIGIL_SCOPED_LOG(name)     ((void)0)
#define VIGIL_SCOPED_LOG_FUNCTION() ((void)0)

#endif // VIGIL_ENABLE_SCOPED_LOG
