// -----------------------------------------------------------------------------
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
 * @brief RAII utility for automatic scope entry/exit trace logging with elapsed time.
 *
 * Defines the @ref vigil::ScopedLogger class and a family of convenience
 * macros for scope instrumentation (RAII and manual BEGIN/END blocks).
 *
 * All facilities in this header are conditionally compiled via `VIGIL_ENABLE_SCOPED_LOG`.
 */

namespace vigil {

/**
 * @brief RAII guard that emits entry and exit messages for a named scope.
 *
 * On construction, logs `>> <scope>` at the configured severity level through
 * the main logger. On destruction, logs `<< <scope> (<elapsed> ms)`.
 *
 * ### Shutdown Fallback
 * If @ref LogSystem::Shutdown() is called before this guard's destructor runs,
 * the exit message is routed to `stderr` to ensure timing data is not lost.
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
    /// @brief Tag type used to opt into the owning-string constructor.
    struct OwnedTag {};

    /**
     * @brief Constructs a logger from a string literal or view.
     *
     * Performs zero allocations for statically named scopes.
     *
     * @param scope String literal identifying the instrumented scope.
     * @param level Severity level for entry and exit messages. Defaults to @ref LogLevel::Trace.
     */
    explicit ScopedLogger(std::string_view scope, LogLevel level = LogLevel::Trace)
        : m_Scope(scope)
        , m_Level(level)
        , m_Start(std::chrono::steady_clock::now())
    {
        LogSystem::Main().Log(m_Level, ">> {}", m_Scope);
    }

    /**
     * @brief Constructs a logger taking ownership of a dynamic string.
     *
     * Used internally by function-tracing macros to hold cleaned signature
     * produced by @ref vigil::detail::CleanFunctionSignature.
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
     * @brief Destroys the guard and emits exit trace with execution duration.
     *
     * Falls back to `stderr` if @ref LogSystem is uninitialized or shut down.
     */
    ~ScopedLogger() noexcept
    {
        const auto elapsed = std::chrono::steady_clock::now() - m_Start;
        const auto ms      = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();

        if (LogSystem::IsInitialized())
        {
            LogSystem::Main().Log(m_Level, "<< {} ({} ms)", m_Scope, ms);
        }
        else
        {
            std::fprintf(stderr,
                "[Vigil/ScopedLogger] << %s (%lld ms)  [LogSystem offline]\n",
                m_Scope.c_str(),
                static_cast<long long>(ms));
        }
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
#define VIGIL_SCOPED_LOG_FUNCTION()                                       \
    ::vigil::ScopedLogger VIGIL_CONCAT(_vigil_scope_fn_, __COUNTER__)(    \
        ::vigil::detail::CleanFunctionSignature(VIGIL_CURRENT_FUNCTION),  \
        ::vigil::ScopedLogger::OwnedTag{})

/**
 * @def VIGIL_SCOPED_LOG_FUNCTION_LEVEL(level)
 * @brief Instruments the current function signature at an explicit log level.
 *
 * @param level Log severity (@ref vigil::LogLevel).
 * @hideinitializer
 */
#define VIGIL_SCOPED_LOG_FUNCTION_LEVEL(level)                            \
    ::vigil::ScopedLogger VIGIL_CONCAT(_vigil_scope_fn_, __COUNTER__)(    \
        ::vigil::detail::CleanFunctionSignature(VIGIL_CURRENT_FUNCTION),  \
        ::vigil::ScopedLogger::OwnedTag{}, level)

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
