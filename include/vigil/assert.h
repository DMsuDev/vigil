// -----------------------------------------------------------------------------
//  Copyright (c) 2026 @DMsuDev. Licensed under the MIT License.
//  See LICENSE file in the project root for full license text.
// -----------------------------------------------------------------------------

#pragma once

#include "vigil/detail/platform_detection.h"
#include "vigil/detail/compiler_attributes.h"
#include "vigil/detail/preprocessor_utils.h"
#include "vigil/core/source_location.h"
#include "vigil/logging/logger.h"

/**
 * @file assert.h
 * @brief Assertion macros with integrated logging and source-location capture.
 *
 * VIGIL_ASSERT(check, ...) logs a detailed failure message (expression text,
 * file, line, function) through Vigil's main logger, flushes it, and breaks
 * into the debugger if one is attached. Compiles to a no-op in builds where
 * VIGIL_ENABLE_ASSERTS is not defined.
 *
 * VIGIL_VERIFY behaves identically to VIGIL_ASSERT in builds where asserts
 * are enabled, but - unlike VIGIL_ASSERT - it still EVALUATES the condition
 * even when asserts are disabled (just without logging/breaking on failure).
 * Use VIGIL_VERIFY when the expression has a required side effect; use
 * VIGIL_ASSERT for pure validation checks with no side effects.
 */

#if defined(VIGIL_CPP20)
    // C++20 standard [[unlikely]] is a statement attribute. It must be attached
    // directly to the 'if' statement itself at the macro call site.
    #define VIGIL_INTERNAL_ASSERT_COND(check) (!(check))
    #define VIGIL_INTERNAL_ASSERT_ATTR        [[unlikely]]
#else
    // Pre-C++20 fallback uses the compiler intrinsic optimization hint wrapper
    // (__builtin_expect) or a plain pass-through on MSVC.
    #define VIGIL_INTERNAL_ASSERT_COND(check) (VIGIL_UNLIKELY(!(check)))
    #define VIGIL_INTERNAL_ASSERT_ATTR
#endif

#if defined(VIGIL_ENABLE_ASSERTS)

namespace vigil::detail {

/**
 * @brief Formats, logs, flushes, breaks into the debugger, and terminates the program.
 *
 * Defined out-of-line in assert.cpp to minimize code bloat at call sites,
 * break circular dependencies with logger.h, and allow [[noreturn]] optimizations.
 *
 * @param exprText Stringified form of the asserted expression.
 * @param loc      Source location of the assert call site.
 * @param message  Optional pre-formatted, user-supplied context message.
 */
[[noreturn]] VIGIL_API void ReportAssertFailure(
    std::string_view exprText,
    const ::vigil::SourceLocation& loc,
    std::string_view message = {}
);

} // namespace vigil::detail

/// @cond INTERNAL
#define VIGIL_INTERNAL_ASSERT_IMPL(check, loc, ...) \
    do { \
        if (VIGIL_INTERNAL_ASSERT_COND(check)) VIGIL_INTERNAL_ASSERT_ATTR { \
            ::vigil::detail::ReportAssertFailure(VIGIL_STRINGIFY(check), (loc) \
                __VA_OPT__(, fmt::format(__VA_ARGS__))); \
        } \
    } while (0)
/// @endcond

/// @def VIGIL_ASSERT(check, ...)
/// @brief Asserts @p check is true; logs and breaks into the debugger on failure.
/// @param check Boolean expression to evaluate. NOT evaluated when asserts
///              are disabled (VIGIL_ENABLE_ASSERTS not defined) - see VIGIL_VERIFY
///              if the expression has a required side effect.
/// @param ...   Optional format string and arguments (fmt syntax) describing
///              the current program state; omitted entirely from the log
///              output if not provided.
#define VIGIL_ASSERT(check, ...) \
    VIGIL_INTERNAL_ASSERT_IMPL((check), VIGIL_CURRENT_LOC(), __VA_ARGS__)

/// @def VIGIL_VERIFY(check, ...)
/// @copydetails VIGIL_ASSERT
/// @note Unlike VIGIL_ASSERT, @p check is ALWAYS evaluated, even in builds
/// where assertion logging/breaking is disabled. Use for expressions with a
/// required side effect (e.g. `VIGIL_VERIFY(file.close())`).
#define VIGIL_VERIFY(check, ...) VIGIL_ASSERT((check), __VA_ARGS__)

#else // !VIGIL_ENABLE_ASSERTS

#define VIGIL_ASSERT(check, ...)  ((void)0)
#define VIGIL_VERIFY(check, ...)  ((void)(check))

#endif

// ============================================================================
// Specialized Assertion Helpers
// ============================================================================

/// @def VIGIL_ASSERT_NOT_NULL(ptr)
/// @brief Asserts that @p ptr is not nullptr.
#define VIGIL_ASSERT_NOT_NULL(ptr) \
    VIGIL_ASSERT((ptr) != nullptr, "Pointer must not be null: {}", VIGIL_STRINGIFY(ptr))

/// @def VIGIL_ASSERT_IN_RANGE(val, min_val, max_val)
/// @brief Asserts that @p val lies within [min_val, max_val] inclusive.
#define VIGIL_ASSERT_IN_RANGE(val, min_val, max_val) \
    VIGIL_ASSERT(((val) >= (min_val)) && ((val) <= (max_val)), \
        "Value '{}' is out of range [{} - {}]", val, min_val, max_val)

/// @def VIGIL_UNREACHABLE_ASSERT()
/// @brief Marks a code path as one that should never execute. Unlike the raw
/// VIGIL_UNREACHABLE() (undefined behavior if reached), this version logs
/// and breaks first in builds with asserts enabled, then falls through to
/// VIGIL_UNREACHABLE() so release builds still get the optimization hint.
#define VIGIL_UNREACHABLE_ASSERT() \
    do { \
        VIGIL_ASSERT(false, "Unreachable code path executed"); \
        VIGIL_UNREACHABLE(); \
    } while (0)
