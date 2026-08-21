// -----------------------------------------------------------------------------
//  Copyright (c) 2026 @DMsuDev. Licensed under the MIT License.
//  See LICENSE file in the project root for full license text.
// -----------------------------------------------------------------------------

#pragma once

#include "vigil/detail/platform_detection.h"
#include "vigil/detail/compiler_features.h"

// MSVC-specific intrinsics (e.g., __assume, __debugbreak)
#if defined(VIGIL_COMPILER_MSVC)
    #include <intrin.h>
#endif

#include <cstdlib>

/**
 * @file compiler_attributes.h
 * @brief Compiler abstraction layer for attributes, intrinsics and optimization hints.
 *
 * This header provides a unified set of compiler-independent macros that wrap
 * standard C++ attributes, compiler-specific extensions and optimization
 * intrinsics behind a consistent interface.
 *
 * Whenever possible, standard language attributes such as `[[nodiscard]]` or
 * `[[fallthrough]]` are used directly. When platform- or compiler-specific
 * functionality is required, the implementation transparently selects the
 * appropriate intrinsic for the active toolchain (MSVC, Clang or GCC).
 *
 * These macros allow the rest of the library to remain portable, readable and
 * free from compiler-specific preprocessor directives.
 *
 * @note
 * Unless otherwise stated:
 *
 * - These macros expand directly to standard C++ attributes or
 *   compiler-specific intrinsics.
 *
 * - Most macros introduce no additional runtime overhead.
 *
 * - Macros providing optimization hints (e.g. force inlining, branch prediction
 *   or assumptions) affect code generation and should only be used when
 *   profiling demonstrates a measurable benefit.
 */

// ==============================================================================
// Standard Language Attributes
// ==============================================================================

/**
 * @brief Indicates that the return value of a function should not be ignored.
 *
 * Ignoring the returned value usually represents a programming error.
 * Typical examples include factory functions, error codes and RAII guard
 * objects returned by value.
 *
 * @code
 * VIGIL_NODISCARD bool initialize();
 * @endcode
 */
#define VIGIL_NODISCARD [[nodiscard]]

/**
 * @brief  Suppresses warnings for intentionally unused entities.
 *
 * Useful for variables, parameters, functions or types that are only
 * referenced under specific build configurations (e.g. debug-only code).
 */
#define VIGIL_MAYBE_UNUSED [[maybe_unused]]

/**
 * @brief Explicitly marks an expression as intentionally unused.
 *
 * Useful when declaration-level attributes such as @c [[maybe_unused]]
 * cannot be applied.
 *
 * @param x Expression to discard.
 */
#define VIGIL_UNUSED(x) (void)(x)

/**
 * @brief Marks a function, class, or entity as deprecated.
 *
 * Causes the compiler to emit a diagnostic whenever the annotated entity is used.
 */
#define VIGIL_DEPRECATED [[deprecated]]

/**
 * @brief Marks an entity as deprecated with a custom compiler message.
 *
 * Use this to briefly explain to the developer which alternative they should use instead.
 *
 * @param msg A literal string explaining the migration path or reason for deprecation.
 *
 * @note Prefer this macro over @c VIGIL_DEPRECATED when a migration path or
 * replacement API can be provided.
 *
 * @code
 * VIGIL_DEPRECATED_MSG("Use decompress_v2() instead") void decompress();
 * @endcode
 */
#define VIGIL_DEPRECATED_MSG(msg) [[deprecated(msg)]]

/**
 * @brief Explicitly documents an intentional fallthrough between @c switch cases.
 *
 * @details Silences "implicit fallthrough" compiler warnings. It must be placed
 * as the last statement in a @c case block right before the next case label.
 *
 * @code
 * switch (state) {
 *     case State::Initializing:
 *         setup_hardware();
 *         VIGIL_FALLTHROUGH;
 *     case State::Ready:
 *         run();
 *         break;
 * }
 * @endcode
 */
#define VIGIL_FALLTHROUGH [[fallthrough]]

// ==============================================================================
//  Layout Attributes
// ==============================================================================

/**
 * @brief Enables empty member optimization for data members.
 *
 * Allows empty objects (such as stateless allocators or policy classes)
 * to occupy no storage when permitted by the implementation, reducing the
 * overall size of enclosing types.
 *
 * Uses the standard C++20 attribute whenever available and falls back to
 * compiler-specific equivalents where necessary.
 *
 * @note Requires C++20 support or an equivalent compiler-specific extension.
 */
#if defined(VIGIL_COMPILER_MSVC) && VIGIL_HAS_CPP_ATTRIBUTE(msvc::no_unique_address)
    #define VIGIL_NO_UNIQUE_ADDRESS [[msvc::no_unique_address]]
#elif VIGIL_HAS_CPP_ATTRIBUTE(no_unique_address)
    #define VIGIL_NO_UNIQUE_ADDRESS [[no_unique_address]]
#else
    #define VIGIL_NO_UNIQUE_ADDRESS
#endif

// ==============================================================================
// Inlining Control
// ==============================================================================

/**
 * @brief Requests aggressive function inlining.
 *
 * Instructs the compiler to inline the annotated function even when its
 * normal heuristics would choose not to.
 *
 * @warning This macro is only a strong optimization hint. The compiler may still
 * ignore the request depending on language rules, optimization settings
 * or implementation-specific limitations.
 *
 * Excessive use can increase binary size and negatively affect instruction
 * cache locality. Always validate benefits through profiling.
 *
 * @code
 * VIGIL_FORCE_INLINE
 * int square(int x)
 * {
 *     return x * x;
 * }
 * @endcode
 */
#if defined(VIGIL_COMPILER_MSVC)
    #define VIGIL_FORCE_INLINE __forceinline
#elif defined(VIGIL_COMPILER_CLANG) || defined(VIGIL_COMPILER_GCC)
    #define VIGIL_FORCE_INLINE __attribute__((always_inline)) inline
#else
    #define VIGIL_FORCE_INLINE inline
#endif

/**
 * @brief Strictly prevents the compiler from inlining the annotated function.
 *
 * Useful for isolating cold paths, preserving stack frames for debugging
 * or keeping frequently executed code compact.
 *
 * @code
 * VIGIL_NOINLINE
 * void report_fatal_error();
 * @endcode
 */
#if defined(VIGIL_COMPILER_MSVC)
    #define VIGIL_NOINLINE __declspec(noinline)
#elif defined(VIGIL_COMPILER_CLANG) || defined(VIGIL_COMPILER_GCC)
    #define VIGIL_NOINLINE __attribute__((noinline))
#else
    #define VIGIL_NOINLINE
#endif

// ==============================================================================
// Branch Prediction
// ==============================================================================

/**
 * @brief Provides a branch prediction hint indicating that an expression
 * is likely to evaluate to @c true.
 *
 * Useful for hot execution paths where the condition is expected to hold
 * most of the time.
 *
 * @note Modern compilers can often infer branch probabilities
 * automatically. Use this macro only when profiling demonstrates a
 * measurable performance benefit.
 *
 * @warning Incorrect branch prediction hints may reduce performance.
 * Always validate their impact through profiling.
 *
 * @param x The boolean expression to evaluate.
 * @return The evaluated value of @p x.
 *
 * @code
 * if (VIGIL_LIKELY(ptr != nullptr))
 * {
 *     ptr->do_work();
 * }
 * @endcode
 */
#if VIGIL_HAS_BUILTIN(__builtin_expect)
    #define VIGIL_LIKELY(x) __builtin_expect(!!(x), 1)
#else
    #define VIGIL_LIKELY(x) (x)
#endif

/**
 * @brief Provides a branch prediction hint indicating that an expression is
 * highly unlikely to evaluate to @c true.
 *
 * Useful for guarding rare execution paths such as catastrophic failures,
 * assertion checks or exceptional conditions.
 *
 * @note Modern compilers can often infer branch probabilities automatically.
 * Use this macro only when profiling demonstrates a measurable benefit.
 *
 * @warning Incorrect branch prediction hints may reduce performance rather
 * than improve it.
 *
 * @param x The boolean expression to evaluate.
 * @return The evaluated value of @p x.
 *
 * @code
 * if (VIGIL_UNLIKELY(error_occurred))
 * {
 *     handle_disaster();
 * }
 * @endcode
 */
#if VIGIL_HAS_BUILTIN(__builtin_expect)
    #define VIGIL_UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
    #define VIGIL_UNLIKELY(x) (x)
#endif

// ==============================================================================
// Optimization Assumptions
// ==============================================================================

/**
 * @brief Allows the optimizer to assume that a condition is guaranteed to hold.
 *
 * Enables the compiler to eliminate redundant checks and generate more
 * efficient code.
 *
 * @warning Undefined Behavior (UB): If the assumption is violated during
 * execution, the program has undefined behavior. Use this macro only for
 * invariants that are guaranteed by other means.
 *
 * @note The supplied expression must be free of observable side effects. Depending on
 * the compiler implementation, the expression may or may not be evaluated,
 * even though this macro does not perform runtime assumption checking.
 * Expressions such as @c i++, assignments or function calls with side
 * effects should therefore never be used.
 *
 * @param cond A condition that is guaranteed to hold whenever execution
 * reaches this point.
 */
#if defined(VIGIL_COMPILER_MSVC)
    #define VIGIL_ASSUME(cond) __assume(cond)
#elif VIGIL_HAS_BUILTIN(__builtin_assume)
    #define VIGIL_ASSUME(cond) __builtin_assume(cond)
#elif defined(VIGIL_COMPILER_GCC)
    #define VIGIL_ASSUME(cond) \
        do { if (!(cond)) __builtin_unreachable(); } while (0)
#else
    #define VIGIL_ASSUME(cond) ((void)0)
#endif

/**
 * @brief Marks a code path as logically unreachable.
 *
 * Indicates that execution can never reach this point.
 *
 * This allows the compiler to generate more efficient machine code and eliminate dead code.
 * (e.g., the @c default case of a @c switch statement that exhaustively covers an entire @c enum).
 *
 * @warning **Undefined Behavior (UB):** Execution reaching this point causes
 * undefined behavior. In development builds, consider validating the assumption
 * with an assertion macro before invoking this point.
 *
 * @code
 * default:
 *     VIGIL_UNREACHABLE();
 * @endcode
 */
#if defined(VIGIL_COMPILER_MSVC)
    #define VIGIL_UNREACHABLE() __assume(0)
#elif VIGIL_HAS_BUILTIN(__builtin_unreachable)
    #define VIGIL_UNREACHABLE() __builtin_unreachable()
#else
    #define VIGIL_UNREACHABLE() ::std::abort()
#endif

// ==============================================================================
// Debugging Utilities
// ==============================================================================

/**
 * @brief Triggers a debugger breakpoint.
 *
 * If a debugger is attached, execution stops at the current instruction.
 * Otherwise, the behavior is implementation-defined and typically results
 * in abnormal program termination.
 *
 * @warning Intended exclusively for debugging and diagnostic code.
 * Never use as part of normal control flow.
 *
 * @code
 * if (error_detected)
 * {
 *     VIGIL_DEBUGBREAK();
 * }
 * @endcode
 */
#if defined(VIGIL_COMPILER_MSVC)
    #define VIGIL_DEBUGBREAK() __debugbreak()
#elif VIGIL_HAS_BUILTIN(__builtin_debugtrap)
    #define VIGIL_DEBUGBREAK() __builtin_debugtrap()
#elif defined(VIGIL_COMPILER_CLANG) || defined(VIGIL_COMPILER_GCC)
    #define VIGIL_DEBUGBREAK() __builtin_trap()
#else
    #define VIGIL_DEBUGBREAK() ::std::abort()
#endif

// ============================================================================
// Current Function Information
// ============================================================================

/**
 * @def VIGIL_CURRENT_FUNCTION
 * @brief Expands to the name or signature of the current function.
 *
 * Defines `VIGIL_CURRENT_FUNCTION`, a compiler-independent macro that expands
 * to the most descriptive function identifier supported by the active
 * compiler.
 *
 * Depending on the compiler, this may be:
 * - `__FUNCSIG__` (MSVC)
 * - `__PRETTY_FUNCTION__` (Clang/GCC)
 * - `__func__` (fallback)
 *
 * The exact contents are implementation-defined and should be treated as diagnostic information only.
 *
 * The macro expands to a null-terminated character string describing the
 * current function and is primarily intended for diagnostics, assertions,
 * logging and crash reporting.
 */
#if defined(VIGIL_COMPILER_MSVC)
    #define VIGIL_CURRENT_FUNCTION __FUNCSIG__
#elif defined(VIGIL_COMPILER_CLANG) || defined(VIGIL_COMPILER_GCC)
    #define VIGIL_CURRENT_FUNCTION __PRETTY_FUNCTION__
#else
    #define VIGIL_CURRENT_FUNCTION __func__
#endif
