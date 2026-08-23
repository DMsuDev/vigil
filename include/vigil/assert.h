// -----------------------------------------------------------------------------
//  Copyright (c) 2026 @DMsuDev. Licensed under the MIT License.
//  See LICENSE file in the project root for full license text.
// -----------------------------------------------------------------------------

#pragma once

#include "vigil/detail/compiler_attributes.h"
#include "vigil/detail/preprocessor_utils.h"
#include "vigil/detail/symbol_export.h"

#include "vigil/core/source_location.h"

#include <string_view>

/**
 * @file assert.h
 * @brief Provides assertion macros integrated with logging and source location capture.
 *
 * Evaluates conditions and logs failure details including expression text, file, line,
 * and function context. Triggers a debugger break on assertion failures when enabled.
 */

#if defined(VIGIL_CPP20)
    // C++20 standard [[unlikely]] is a statement attribute. It must be attached
    // directly to the 'if' statement itself at the macro call site.
    #define VIGIL_INTERNAL_ASSERT_COND(check) (!(check))
    #define VIGIL_INTERNAL_ASSERT_ATTR        [[unlikely]]
#else
    // C++17: fall back to the __builtin_expect wrapper defined in
    // compiler_attributes.h (no-op on MSVC without __has_builtin support).
    #define VIGIL_INTERNAL_ASSERT_COND(check) (VIGIL_UNLIKELY(!(check)))
    #define VIGIL_INTERNAL_ASSERT_ATTR
#endif

// ============================================================================
// Active build: ASSERTS ENABLED
// ============================================================================

#if defined(VIGIL_ENABLE_ASSERTS)

namespace vigil::detail {

/**
 * @brief Formats, logs, flushes, breaks into the debugger, and terminates the program.
 *
 * Defined out-of-line in assert.cpp to:
 *   - Minimize code bloat at every call site.
 *   - Break circular header dependencies with logger.h.
 *   - Allow [[noreturn]] to propagate through the whole call graph.
 *
 * @param exprText  Stringified form of the asserted expression.
 * @param loc       Source location captured at the assert call site.
 * @param message   Optional pre-formatted, user-supplied context string.
 *                  Defaults to an empty view when called from the no-message branch.
 */
[[noreturn]] VIGIL_API void ReportAssertFailure(
    std::string_view exprText,
    const ::vigil::SourceLocation& loc,
    std::string_view message = {}
);

} // namespace vigil::detail

/// @cond INTERNAL

// ----------------------------------------------------------------------------
// Internal macro implementations
//
// Two separate paths per C++ version to avoid passing an empty variadic pack
// to fmt::format in C++17, which is either ill-formed or implementation-defined
// depending on the fmt version.
// ----------------------------------------------------------------------------

#if defined(VIGIL_CPP20)

    // C++20: __VA_OPT__ cleanly detects whether a message argument was supplied.
    // When no message is provided the default empty string_view overload is used.
    #define VIGIL_INTERNAL_ASSERT_IMPL(check, loc, ...) \
        do { \
            if (VIGIL_INTERNAL_ASSERT_COND(check)) VIGIL_INTERNAL_ASSERT_ATTR { \
                ::vigil::detail::ReportAssertFailure(VIGIL_STRINGIFY(check), (loc) \
                    __VA_OPT__(, fmt::format(__VA_ARGS__))); \
            } \
        } while (0)

#else // C++17

    // Message variant: at least one variadic argument (format string + optional args).
    // Always receives a non-empty pack because VIGIL_ASSERT_MSG guarantees it.
    #define VIGIL_INTERNAL_ASSERT_IMPL(check, loc, ...) \
        do { \
            if (VIGIL_INTERNAL_ASSERT_COND(check)) VIGIL_INTERNAL_ASSERT_ATTR { \
                ::vigil::detail::ReportAssertFailure( \
                    VIGIL_STRINGIFY(check), \
                    (loc), \
                    fmt::format(__VA_ARGS__) \
                ); \
            } \
        } while (0)

    // No-message variant: calls ReportAssertFailure without a message argument,
    // relying on the default empty string_view parameter. Avoids an empty
    // fmt::format() call which is ill-formed in some fmt versions.
    #define VIGIL_INTERNAL_ASSERT_IMPL_NO_MSG(check, loc) \
        do { \
            if (VIGIL_INTERNAL_ASSERT_COND(check)) VIGIL_INTERNAL_ASSERT_ATTR { \
                ::vigil::detail::ReportAssertFailure( \
                    VIGIL_STRINGIFY(check), \
                    (loc) \
                ); \
            } \
        } while (0)

#endif // VIGIL_CPP20

/// @endcond

// ============================================================================
// Public assertion macros
// ============================================================================

/**
 * @def VIGIL_ASSERT_MSG(check, ...)
 * @brief Evaluates an expression and triggers an assertion failure with a custom message if false.
 *
 * Logs failure details alongside pre-formatted context and halts execution in the debugger.
 * The condition is ignored entirely when assertions are disabled (VIGIL_ENABLE_ASSERTS undefined).
 *
 * @param check Boolean expression to evaluate.
 * @param ...   Format string and arguments (fmt syntax) providing additional context.
 */
#define VIGIL_ASSERT_MSG(check, ...) \
    VIGIL_INTERNAL_ASSERT_IMPL((check), VIGIL_CURRENT_LOC(), __VA_ARGS__)

/**
 * @def VIGIL_ASSERT(check)
 * @brief Evaluates an expression and triggers an assertion failure if it resolves to false.
 *
 * Logs expression failure details and halts execution in the debugger when enabled.
 * The condition is ignored entirely when assertions are disabled (VIGIL_ENABLE_ASSERTS undefined).
 *
 * @param check Boolean expression to evaluate.
 */
#if defined(VIGIL_CPP20)
    #define VIGIL_ASSERT(check) \
        VIGIL_INTERNAL_ASSERT_IMPL((check), VIGIL_CURRENT_LOC())
#else
    // C++17: use the no-message variant to avoid an empty fmt::format() call.
    #define VIGIL_ASSERT(check) \
        VIGIL_INTERNAL_ASSERT_IMPL_NO_MSG((check), VIGIL_CURRENT_LOC())
#endif

/**
 * @def VIGIL_VERIFY_MSG(check, ...)
 * @brief Like VIGIL_ASSERT_MSG, but **always evaluates @p check**.
 *
 * Use when the expression has an observable side effect that must execute even when
 * assertion logging and breaking are disabled in non-assert builds.
 *
 * @param check Boolean expression to evaluate. Always evaluated.
 * @param ...   Format string and arguments (fmt syntax) providing additional context.
 */
#define VIGIL_VERIFY_MSG(check, ...) \
    VIGIL_ASSERT_MSG((check), __VA_ARGS__)

/**
 * @def VIGIL_VERIFY(check)
 * @brief Like VIGIL_ASSERT, but **always evaluates @p check**.
 *
 * Use when the expression has an observable side effect that must execute even when
 * assertion logging and breaking are disabled in non-assert builds (e.g. `VIGIL_VERIFY(file.close())`).
 *
 * Defined independently of VIGIL_ASSERT to avoid double-wrapping @p check in parentheses,
 * which would cause VIGIL_STRINGIFY to capture the expression as "(check)" instead of "check".
 *
 * @param check Boolean expression to evaluate. Always evaluated.
 */
#if defined(VIGIL_CPP20)
    #define VIGIL_VERIFY(check) \
        VIGIL_INTERNAL_ASSERT_IMPL((check), VIGIL_CURRENT_LOC())
#else
    #define VIGIL_VERIFY(check) \
        VIGIL_INTERNAL_ASSERT_IMPL_NO_MSG((check), VIGIL_CURRENT_LOC())
#endif

#else // !VIGIL_ENABLE_ASSERTS

// ============================================================================
// Disabled build: no-ops
// ============================================================================

/// No-op when assertions are disabled. @p check is NOT evaluated.
#define VIGIL_ASSERT_MSG(check, ...) ((void)0)

/// No-op when assertions are disabled. @p check is NOT evaluated.
#define VIGIL_ASSERT(check)          ((void)0)

/// Evaluates @p check for side effects; logging and breaking are omitted.
#define VIGIL_VERIFY_MSG(check, ...) ((void)(check))

/// Evaluates @p check for side effects; logging and breaking are omitted.
#define VIGIL_VERIFY(check)          ((void)(check))

#endif // VIGIL_ENABLE_ASSERTS

// ============================================================================
// Specialized Assertion Helpers
// ============================================================================

/**
 * @def VIGIL_ASSERT_NOT_NULL(ptr)
 * @brief Asserts that @p ptr is not nullptr.
 *
 * Generates a message of the form:
 * @code{.text}
 * Pointer must not be null: <ptr-expression>
 * @endcode
 *
 * @param ptr Pointer expression to validate.
 */
#define VIGIL_ASSERT_NOT_NULL(ptr) \
    VIGIL_ASSERT_MSG((ptr) != nullptr, "Pointer must not be null: " VIGIL_STRINGIFY(ptr))

/**
 * @def VIGIL_ASSERT_IN_RANGE(val, min_val, max_val)
 * @brief Verifies that a value falls within [@p min_val, @p max_val] inclusive.
 *
 * Generates a message of the form:
 * @code{.text}
 * Value '<val>' is out of range [<min_val> - <max_val>]
 * @endcode
 *
 * @param val     Value to evaluate.
 * @param min_val Inclusive lower bound.
 * @param max_val Inclusive upper bound.
 */
#define VIGIL_ASSERT_IN_RANGE(val, min_val, max_val) \
    VIGIL_ASSERT_MSG(((val) >= (min_val)) && ((val) <= (max_val)), \
        "Value '{}' is out of range [{} - {}]", (val), (min_val), (max_val))

/**
 * @def VIGIL_UNREACHABLE_ASSERT()
 * @brief Asserts an unreachable code path before executing UB optimization hints.
 *
 * Logs and breaks into the debugger when assertions are enabled, then invokes
 * VIGIL_UNREACHABLE() to preserve release-mode dead-code elimination.
 */
#define VIGIL_UNREACHABLE_ASSERT() \
    do { \
        VIGIL_ASSERT_MSG(false, "Unreachable code path executed"); \
        VIGIL_UNREACHABLE(); \
    } while (0)
