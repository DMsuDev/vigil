// -----------------------------------------------------------------------------
//  Copyright (c) 2026 @DMsuDev. Licensed under the MIT License.
//  See LICENSE file in the project root for full license text.
// -----------------------------------------------------------------------------

#pragma once

/**
 * @file preprocessor_utils.h
 * @brief Low-level preprocessor helpers used throughout Vigil.
 *
 * Provides small preprocessor utilities used throughout the library,
 * including helpers for compiler-specific macro expansion quirks and
 * common bit-flag definitions.
 *
 * These macros are expanded entirely by the preprocessor and introduce no
 * runtime overhead.
 */

// ============================================================================
// Macro Expansion
// ============================================================================

/**
 * @def VIGIL_EXPAND_MACRO(x)
 * @brief Forces a macro argument to fully expand before further processing.
 *
 * @details
 * Some preprocessor techniques require an additional expansion pass
 * to force macro arguments to expand before further processing.
 *
 * This is primarily needed to work around expansion-order differences
 * in MSVC's legacy preprocessor and to ensure consistent behavior across
 * all supported compilers.
 *
 * @param x Token or macro to force-expand.
 *
 * @code
 * #define INNER hello
 * #define OUTER(x) x
 *
 * VIGIL_STRINGIFY(VIGIL_EXPAND_MACRO(OUTER(INNER))) // -> "hello"
 * VIGIL_STRINGIFY(OUTER(INNER))                     // -> "INNER"
 * @endcode
 */
#define VIGIL_EXPAND_MACRO(x) x

// ============================================================================
// Stringification
// ============================================================================

/**
 * @def VIGIL_STRINGIFY_IMPL(x)
 * @brief Internal helper. Do not use directly, instead use VIGIL_STRINGIFY() instead.
 *
 * @details The preprocessor `#` stringification operator does not expand macro
 * arguments before converting them into string literals. This helper performs the raw
 * stringification; VIGIL_STRINGIFY() wraps it with one extra layer of macro
 * indirection so that, if `x` is itself a macro, it is expanded to its value
 * first.
 */
#define VIGIL_STRINGIFY_IMPL(x) #x

/**
 * @def VIGIL_STRINGIFY(x)
 * @brief Converts a macro argument to a string literal, expanding it first.
 *
 * @details Use this (not VIGIL_STRINGIFY_IMPL) whenever the argument may
 * itself be a macro (e.g. VIGIL_VERSION_MAJOR) rather than a literal token.
 * The extra level of macro indirection guarantees correct expansion on
 * every supported compiler, including MSVC.
 *
 * @param x Token or macro to stringify.
 *
 * @code
 * #define ANSWER 42
 * VIGIL_STRINGIFY(ANSWER)       // -> "42"
 * VIGIL_STRINGIFY_IMPL(ANSWER)  // -> "ANSWER"  (likely NOT what you want)
 * @endcode
 *
 * @see VIGIL_STRINGIFY_IMPL()
 */
#define VIGIL_STRINGIFY(x) VIGIL_STRINGIFY_IMPL(x)

// ============================================================================
// Token Concatenation
// ============================================================================

/**
 * @def VIGIL_CONCAT_IMPL(a, b)
 * @brief Internal helper. Do not use directly, use VIGIL_CONCAT() instead.
 *
 * @details The preprocessor ## operator suppresses macro expansion of its
 * operands. This helper performs the raw token paste; VIGIL_CONCAT() wraps
 * it with one extra level of macro indirection so that both @p a and @p b
 * are fully expanded before the paste occurs.
 *
 * @see VIGIL_CONCAT()
 */
#define VIGIL_CONCAT_IMPL(a, b) a##b

/**
 * @def VIGIL_CONCAT(a, b)
 * @brief Concatenates two tokens after fully expanding both macro arguments.
 *
 * @details Use this (not VIGIL_CONCAT_IMPL) whenever either argument may
 * itself be a macro. The extra level of indirection guarantees that @p a
 * and @p b are expanded to their final values before the ## operator pastes
 * them together, producing correct results on all supported compilers
 * including MSVC.
 *
 * @param a Left-hand token or macro to expand and concatenate.
 * @param b Right-hand token or macro to expand and concatenate.
 *
 * @code
 * #define PREFIX  vigil_scope_
 * #define COUNTER 42
 *
 * VIGIL_CONCAT(PREFIX, COUNTER)       // -> vigil_scope_42
 * VIGIL_CONCAT_IMPL(PREFIX, COUNTER)  // -> PREFIX##COUNTER (likely NOT what you want)
 * @endcode
 *
 * @see VIGIL_CONCAT_IMPL()
 */
#define VIGIL_CONCAT(a, b) VIGIL_CONCAT_IMPL(a, b)

// ============================================================================
// Bit-Flag Helpers
// ============================================================================

/**
 * @def VIGIL_BIT(n)
 * @brief Produces an unsigned bit mask with bit @p n set.
 *
 * Intended for defining flag-style enumerations where each enumerator
 * occupies a single distinct bit.
 *
 * @warning The bit index must be smaller than the number of bits in
 * @c unsigned int. Shifting by a value greater than or equal to the
 * type width results in undefined behavior.
 *
 * @param n Zero-based bit index.
 * @return Unsigned bit mask with bit @p n set.
 *
 * @code
 * enum LogFlags : unsigned {
 *     LogFlags_Timestamp = VIGIL_BIT(0),
 *     LogFlags_ThreadId  = VIGIL_BIT(1),
 *     LogFlags_Colorized = VIGIL_BIT(2),
 * };
 * @endcode
 *
 * @see VIGIL_BIT64()
 */
#define VIGIL_BIT(n) (1u << (n))

/**
 * @def VIGIL_BIT64(n)
 * @brief Produces a 64-bit unsigned bit mask with bit `n` set.
 *
 * @details Intended for defining flag-style enumerations or masks that
 * require more than 32 bits.
 *
 * @warning The bit index must be smaller than the number of bits in
 * a 64-bit unsigned integer.
 *
 * @param n Zero-based bit index.
 * @return Unsigned bit mask with bit @p n set.
 *
 * @see VIGIL_BIT()
 */
#define VIGIL_BIT64(n) (1ull << (n))
