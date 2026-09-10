// -----------------------------------------------------------------------------
//   _    __ __ _____ ___ __
//  | |  / /  _/ ____/  _/ /
//  | | / // // / __ / // /     Logging and Diagnostics for C++
//  | |/ // // /_/ // // /___   https://github.com/DMsuDev/vigil
//  |___/___/\____/___/_____/
//
//  Copyright (c) 2026 @DMsuDev. Licensed under the MIT License.
//  See LICENSE file in the project root for full license text.
// -----------------------------------------------------------------------------

#pragma once

#include "vigil/logging/log_system.h"

#include "vigil/detail/symbol_utils.h"        // CleanFunctionSignature()
#include "vigil/detail/compiler_attributes.h" // VIGIL_CURRENT_FUNCTION
#include "vigil/detail/preprocessor_utils.h"  // VIGIL_CONCAT(), VIGIL_STRINGIFY()

#include <chrono>
#include <cstdio>
#include <string>
#include <string_view>

/**
 * @file scoped_logger.h
 * @brief RAII instrumentation guard and macros for scope timing and trace logging.
 *
 * Provides automatic entering (`>>`) and exiting (`<<`) diagnostics with execution
 * duration measurements. Entire facility is conditionally compiled via `VIGIL_ENABLE_SCOPED_LOG`.
 */

namespace vigil {

/**
 * @brief RAII guard emitting entry and exit trace messages for a named block or function.
 *
 * On construction, logs `>> <scope>` at the specified severity level.
 * On destruction, logs `<< <scope> (<elapsed>)` with adaptive time units.
 *
 * @note If @ref LogSystem::Shutdown() is invoked while a guard is active,
 *       the exit message automatically routes to `stderr` to guarantee
 *       timing data delivery.
 *
 * @note Controlled at compile time via `VIGIL_ENABLE_SCOPED_LOG`.
 *
 * @see VIGIL_SCOPED_LOG
 * @see VIGIL_SCOPED_LOG_LEVEL
 * @see VIGIL_SCOPED_LOG_FUNCTION
 * @see VIGIL_SCOPED_LOG_FUNCTION_LEVEL
 * @see VIGIL_SCOPE_BEGIN
 * @see VIGIL_SCOPE_END
 */
class ScopedLogger {
public:

    /**
     * @brief Constructs a logger from a raw string literal.
     *
     * Provides an exact match for `const char[N]` to prevent overload ambiguity
     * while avoiding heap allocations.
     *
     * @param scope Pointer to a null-terminated string literal. Must outlive the guard scope.
     * @param level Severity level for entry and exit messages (defaults to @ref LogLevel::Trace).
     */
    explicit ScopedLogger(const char* scope, LogLevel level = LogLevel::Trace)
        : m_ScopeView(scope)
        , m_Level(level)
        , m_Start(std::chrono::steady_clock::now())
    {
        LogSystem::Main().Log(m_Level, ">> {}", m_ScopeView);
    }

    /**
     * @brief Constructs a logger from a string view.
     *
     * @param scope Non-owning view of the instrumented scope name.
     * @param level Severity level for entry and exit messages (defaults to @ref LogLevel::Trace).
     */
    explicit ScopedLogger(std::string_view scope, LogLevel level = LogLevel::Trace)
        : m_ScopeView(scope)
        , m_Level(level)
        , m_Start(std::chrono::steady_clock::now())
    {
        LogSystem::Main().Log(m_Level, ">> {}", m_ScopeView);
    }

    /**
     * @brief Constructs a logger taking ownership of a dynamic string.
     *
     * Designed for function-tracing macros where cleaned signatures are generated
     * dynamically at runtime. Moves string storage into the guard instance.
     *
     * @param scope Rvalue reference to a dynamic string to take ownership of.
     * @param level Severity level for entry and exit messages (defaults to @ref LogLevel::Trace).
     */
    explicit ScopedLogger(std::string&& scope, LogLevel level = LogLevel::Trace)
        : m_ScopeStorage(std::move(scope))
        , m_ScopeView(m_ScopeStorage)
        , m_Level(level)
        , m_Start(std::chrono::steady_clock::now())
    {
        LogSystem::Main().Log(m_Level, ">> {}", m_ScopeView);
    }

    /// @brief Destroys the guard and emits exit trace with execution duration.
    /// Falls back to `stderr` if @ref LogSystem is uninitialized or shut down.
    ~ScopedLogger() noexcept
    {
        try
        {
            const auto elapsed = std::chrono::steady_clock::now() - m_Start;
            const auto us = std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();

            // Adaptive formatting: avoid "0 ms" for sub-millisecond scopes.
            char time_buf[32];
            if (us < 1000)
            {
                std::snprintf(time_buf, sizeof(time_buf), "%lld \xc2\xb5s", static_cast<long long>(us));
            }
            else
            {
                // Express as milliseconds with two decimal places.
                const long long ms_int  = us / 1000;
                const long long ms_frac = (us % 1000) / 10; // hundredths
                std::snprintf(time_buf, sizeof(time_buf), "%lld.%02lld ms",
                    static_cast<long long>(ms_int),
                    static_cast<long long>(ms_frac));
            }

            if (LogSystem::IsInitialized())
            {
                LogSystem::Main().Log(m_Level, "<< {} ({})", m_ScopeView, time_buf);
            }
            else
            {
                std::fprintf(stderr,
                    "[Vigil/ScopedLogger] << %.*s (%s)  [LogSystem offline]\n",
                    static_cast<int>(m_ScopeView.size()),
                    m_ScopeView.data(),
                    time_buf);
            }
        }
        catch (...)
        {
            // Swallow all exceptions to ensure the destructor never throws.
        }
    }

    ScopedLogger(const ScopedLogger&)            = delete;
    ScopedLogger& operator=(const ScopedLogger&) = delete;
    ScopedLogger(ScopedLogger&&)                 = delete;
    ScopedLogger& operator=(ScopedLogger&&)      = delete;

private:
    std::string                           m_ScopeStorage;
    std::string_view                      m_ScopeView;
    LogLevel                              m_Level;
    std::chrono::steady_clock::time_point m_Start;
};

} // namespace vigil

#ifdef VIGIL_ENABLE_SCOPED_LOG

// ============================================================================
// Scoped Logger Macros: RAII
// ============================================================================

/**
 * @name RAII Scope Logging Macros (Preferred)
 * @{
 */

/**
 * @def VIGIL_SCOPED_LOG(scope)
 * @brief Instruments a named scope with entry/exit trace logging and elapsed time.
 *
 * @param scope String literal identifying the instrumented scope.
 * @hideinitializer
 */
#define VIGIL_SCOPED_LOG(scope) \
    ::vigil::ScopedLogger VIGIL_CONCAT(_vigil_scope_, __COUNTER__)(scope)

/**
 * @def VIGIL_SCOPED_LOG_LEVEL(scope, level)
 * @brief Instruments a named scope at an explicit log severity level.
 *
 * @param scope String literal identifying the scope.
 * @param level Log severity (@ref vigil::LogLevel).
 * @hideinitializer
 */
#define VIGIL_SCOPED_LOG_LEVEL(scope, level) \
    ::vigil::ScopedLogger VIGIL_CONCAT(_vigil_scope_, __COUNTER__)(scope, level)

/**
 * @def VIGIL_SCOPED_LOG_FUNCTION()
 * @brief Instruments the current function signature at Trace level.
 * @hideinitializer
 */
#define VIGIL_SCOPED_LOG_FUNCTION()                                      \
    ::vigil::ScopedLogger VIGIL_CONCAT(_vigil_scope_fn_, __COUNTER__)(   \
        ::vigil::detail::CleanFunctionSignature(VIGIL_CURRENT_FUNCTION))

/**
 * @def VIGIL_SCOPED_LOG_FUNCTION_LEVEL(level)
 * @brief Instruments the current function signature at an explicit log level.
 *
 * @param level Log severity (@ref vigil::LogLevel).
 * @hideinitializer
 */
#define VIGIL_SCOPED_LOG_FUNCTION_LEVEL(level)                                  \
    ::vigil::ScopedLogger VIGIL_CONCAT(_vigil_scope_fn_, __COUNTER__)(          \
        ::vigil::detail::CleanFunctionSignature(VIGIL_CURRENT_FUNCTION), level)
/** @} */

// ============================================================================
// Scoped Logger Macros: Manual
// ============================================================================

/**
 * @name Manual Block Scope Macros
 *
 * Each BEGIN opens a real C++ block `{` and constructs a `ScopedLogger`
 * inside it. The matching END closes the block `}`, which triggers the
 * destructor and emits the exit message.
 *
 * @code{.cpp}
 * VIGIL_SCOPE_BEGIN("Outer work");
 *     DoSomething();
 *     VIGIL_SCOPE_BEGIN("Inner step");
 *         DoInnerWork();
 *     VIGIL_SCOPE_END();
 * VIGIL_SCOPE_END();
 * @endcode
 *
 * @{
 */

/**
 * @def VIGIL_SCOPE_BEGIN(scope)
 * @brief Opens a manual block scope with trace logging.
 *
 * @param scope Scope label string literal.
 * @warning Must be paired with a matching @ref VIGIL_SCOPE_END() in the same block.
 * @hideinitializer
 */
#define VIGIL_SCOPE_BEGIN(scope) \
    {                            \
    ::vigil::ScopedLogger VIGIL_CONCAT(_vigil_manual_scope_, __COUNTER__)(scope)

/**
 * @def VIGIL_SCOPE_BEGIN_LEVEL(scope, level)
 * @brief Opens a manual block scope at an explicit log severity level.
 *
 * @param scope Scope label string literal.
 * @param level Log severity (@ref vigil::LogLevel).
 * @warning Must be paired with a matching @ref VIGIL_SCOPE_END() in the same block.
 * @hideinitializer
 */
#define VIGIL_SCOPE_BEGIN_LEVEL(scope, level) \
    {                                         \
    ::vigil::ScopedLogger VIGIL_CONCAT(_vigil_manual_scope_, __COUNTER__)(scope, level)

/**
 * @def VIGIL_SCOPE_END()
 * @brief Closes a manual logging block opened with @ref VIGIL_SCOPE_BEGIN.
 * @hideinitializer
 */
#define VIGIL_SCOPE_END() \
    }

/** @} */

#else // !VIGIL_ENABLE_SCOPED_LOG

#define VIGIL_SCOPED_LOG(scope)                ((void)0)
#define VIGIL_SCOPED_LOG_LEVEL(scope, level)   ((void)0)
#define VIGIL_SCOPED_LOG_FUNCTION()            ((void)0)
#define VIGIL_SCOPED_LOG_FUNCTION_LEVEL(level) ((void)0)

#define VIGIL_SCOPE_BEGIN(scope)               {
#define VIGIL_SCOPE_BEGIN_LEVEL(scope, level)  {
#define VIGIL_SCOPE_END()                      }

#endif // VIGIL_ENABLE_SCOPED_LOG
