// -----------------------------------------------------------------------------
//  Copyright (c) 2026 @DMsuDev. Licensed under the MIT License.
//  See LICENSE file in the project root for full license text.
// -----------------------------------------------------------------------------

#pragma once

/**
 * @file compiler_features.h
 * @brief Compiler feature detection abstraction layer.
 *
 * This header provides a compiler-independent interface for querying support
 * for language features, attributes, compiler intrinsics and header
 * availability.
 *
 * Not every compiler implements the same feature detection macros
 * (e.g. @c __has_builtin or @c __has_cpp_attribute). Missing detection
 * macros are therefore defined as permissive fallbacks that evaluate to
 * @c 0, allowing the VIGIL_HAS_* wrappers to be used unconditionally
 * without additional preprocessor guards.
 *
 * @note Feature detection is inherently compiler-specific rather than
 * platform-specific. Consequently, this header intentionally has no
 * dependency on platform detection facilities.
 *
 * @warning
 * Some wrappers (notably @ref VIGIL_HAS_INCLUDE and
 * @ref VIGIL_HAS_CPP_ATTRIBUTE) ultimately expand to language-defined
 * preprocessor operators whose behavior is only guaranteed inside
 * @c #if and @c #elif expressions. Always invoke these wrappers directly
 * within the conditional rather than storing their result for later use.
 */

// ============================================================================
// Fallback Definitions
// ----------------------------------------------------------------------------
// Some compilers do not provide one or more of the standard feature detection
// macros. Defining missing macros to expand to 0 allows the wrappers below
// to be used unconditionally across all supported toolchains.
// ============================================================================

#ifndef __has_builtin
    #define __has_builtin(x) 0
#endif

#ifndef __has_attribute
    #define __has_attribute(x) 0
#endif

#ifndef __has_cpp_attribute
    #define __has_cpp_attribute(x) 0
#endif

#ifndef __has_declspec_attribute
    #define __has_declspec_attribute(x) 0
#endif

#ifndef __has_include
    #define __has_include(x) 0
#endif

#ifndef __has_feature
    #define __has_feature(x) 0
#endif

#ifndef __has_extension
    #define __has_extension(x) 0
#endif

#ifndef __has_warning
    #define __has_warning(x) 0
#endif

// ============================================================================
// Compiler Feature Detection Wrappers
// ============================================================================

/**
 * @name Compiler Feature Detection
 * @brief Portable wrappers around compiler feature detection facilities.
 *
 * This module provides a compiler-independent interface for querying language features,
 * builtins, attributes, header availability and other implementation-specific
 * capabilities.
 *
 * These wrappers should be preferred over using the underlying compiler
 * detection macros directly, as they provide consistent behavior across
 * supported toolchains and gracefully fall back when a particular detection
 * facility is unavailable.
 *
 * Unless otherwise specified, each wrapper expands to an integer constant
 * expression suitable for use in @c #if and @c #elif preprocessing directives.
 *
 * @note
 * These wrappers are intended exclusively for compile-time feature detection.
 * They must not be used as runtime expressions.
 * @{
 */

/**
 * @def VIGIL_HAS_BUILTIN(x)
 * @brief Detects support for a compiler builtin or intrinsic.
 *
 * @param x Builtin identifier (e.g. @c __builtin_expect).
 *
 * @return Non-zero if the builtin is supported; otherwise @c 0.
 *
 * @note GCC only provides @c __has_builtin starting with GCC 10.
 * Earlier versions always evaluate this wrapper to @c 0, even for
 * long-standing builtins. When supporting older GCC releases, combine
 * this wrapper with explicit compiler version checks where appropriate.
 *
 * @code
 * #if VIGIL_HAS_BUILTIN(__builtin_assume)
 * #endif
 * @endcode
 */
#define VIGIL_HAS_BUILTIN(x) __has_builtin(x)

/**
 * @def VIGIL_HAS_ATTRIBUTE(x)
 * @brief Detects support for a GNU-style compiler attribute.
 *
 * @param x Attribute identifier (e.g. @c always_inline).
 *
 * @code
 * #if VIGIL_HAS_ATTRIBUTE(always_inline)
 * #endif
 * @endcode
 */
#define VIGIL_HAS_ATTRIBUTE(x) __has_attribute(x)

/**
 * @def VIGIL_HAS_CPP_ATTRIBUTE(x)
 * @brief Detects support for a standard or vendor-specific C++ attribute.
 *
 * @param x Attribute identifier (e.g. @c nodiscard or
 * @c gnu::always_inline).
 *
 * @return
 * Returns @c 0 if unsupported; otherwise returns the SD-6 feature-test
 * value describing the supported revision of the attribute.
 *
 * @note
 * When testing for specific language revisions, prefer relational
 * comparisons over simple truth-value checks.
 *
 * @code
 * #if VIGIL_HAS_CPP_ATTRIBUTE(nodiscard) >= 201907L
 * #endif
 * @endcode
 */
#define VIGIL_HAS_CPP_ATTRIBUTE(x) __has_cpp_attribute(x)

/**
 * @def VIGIL_HAS_DECLSPEC_ATTRIBUTE(x)
 * @brief Detects support for a Microsoft @c __declspec attribute.
 *
 * @param x Declspec attribute identifier (e.g. @c dllexport).
 *
 * @code
 * #if VIGIL_HAS_DECLSPEC_ATTRIBUTE(noinline)
 * #endif
 * @endcode
 */
#define VIGIL_HAS_DECLSPEC_ATTRIBUTE(x) __has_declspec_attribute(x)

/**
 * @def VIGIL_HAS_INCLUDE(x)
 * @brief Detects whether a header is available for inclusion.
 *
 * @param x Header name using its normal include syntax.
 *
 * @warning
 * This macro must be used directly inside a @c #if or @c #elif
 * preprocessor conditional.
 *
 * @code
 * #if VIGIL_HAS_INCLUDE(<version>)
 *     #include <version>
 * #endif
 * @endcode
 */
#define VIGIL_HAS_INCLUDE(x) __has_include(x)

/**
 * @def VIGIL_HAS_FEATURE(x)
 * @brief Detects Clang-specific language or runtime features.
 *
 * @param x Feature identifier (e.g. @c address_sanitizer).
 *
 * @code
 * #if VIGIL_HAS_FEATURE(address_sanitizer)
 * #endif
 * @endcode
 */
#define VIGIL_HAS_FEATURE(x) __has_feature(x)

/**
 * @def VIGIL_HAS_EXTENSION(x)
 * @brief Detects Clang language extensions available independently of the
 * selected language standard.
 *
 * @param x Extension identifier.
 */
#define VIGIL_HAS_EXTENSION(x) __has_extension(x)

/**
 * @def VIGIL_HAS_WARNING(x)
 * @brief Detects support for a Clang warning flag.
 *
 * @param x Warning flag as a string literal
 * (e.g. @c "-Wshadow").
 *
 * @note
 * This wrapper is Clang-specific and always evaluates to @c 0 on
 * other compilers.
 *
 * @code
 * #if VIGIL_HAS_WARNING("-Wshadow-field")
 *     #pragma clang diagnostic ignored "-Wshadow-field"
 * #endif
 * @endcode
 */
#define VIGIL_HAS_WARNING(x) __has_warning(x)

/** @} */
