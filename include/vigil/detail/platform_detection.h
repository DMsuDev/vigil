// -----------------------------------------------------------------------------
//  Copyright (c) 2026 @DMsuDev. Licensed under the MIT License.
//  See LICENSE file in the project root for full license text.
// -----------------------------------------------------------------------------

#pragma once

/**
 * @file platform_detection.h
 * @brief Compile-time detection of the build environment.
 *
 * This header centralizes compile-time detection of the target operating
 * system, CPU architecture and compiler toolchain used to build Vigil.
 *
 * During a standard build, these macros are typically supplied by the build
 * system through CMake using `target_compile_definitions()`. When such
 * definitions are unavailable (for example, when Vigil is integrated directly
 * into another project), the required information is automatically derived
 * from the compiler's predefined macros.
 *
 * The macros exposed by this header provide a consistent abstraction layer
 * used throughout the library for compiler-specific attributes, symbol
 * visibility, platform-dependent implementations and optimization utilities.
 */

// ============================================================================
// Platform Detection
// ============================================================================

/**
 * @name Platform Detection
 * @brief Detects the target operating system at compile time.
 *
 * If no platform has been specified by the build system, this section derives
 * the target operating system from the compiler's predefined platform macros.
 *
 * Exactly one of the following macros is defined:
 * - `VIGIL_PLATFORM_WINDOWS`
 * - `VIGIL_PLATFORM_LINUX`
 * - `VIGIL_PLATFORM_MACOS`
 *
 * Compilation terminates with an error if the target platform is not
 * supported.
 *
 * @{
 */
#if !defined(VIGIL_PLATFORM_WINDOWS) && \
    !defined(VIGIL_PLATFORM_LINUX)   && \
    !defined(VIGIL_PLATFORM_MACOS)

    #if defined(_WIN32) || defined(__CYGWIN__)
        #define VIGIL_PLATFORM_WINDOWS 1

        // Reduce the amount of declarations pulled in by <Windows.h> and prevent
        // the min/max macros from polluting the global namespace.
        #ifndef WIN32_LEAN_AND_MEAN
            #define WIN32_LEAN_AND_MEAN
        #endif

        #ifndef NOMINMAX
            #define NOMINMAX
        #endif

    #elif defined(__APPLE__) && defined(__MACH__)
        #define VIGIL_PLATFORM_MACOS 1

    #elif defined(__linux__)
        #define VIGIL_PLATFORM_LINUX 1

    #else
        #error "[Vigil] Target platform is not supported. Vigil currently supports Windows, Linux and macOS only."
    #endif

#endif // !defined(VIGIL_PLATFORM_*)

/** @} */

// ============================================================================
// Platform Metadata
// ============================================================================

/// @brief Guards against conflicting platform macros supplied by the build
/// system (e.g. two VIGIL_PLATFORM_* defines passed simultaneously via CMake).
#if (defined(VIGIL_PLATFORM_WINDOWS) + \
     defined(VIGIL_PLATFORM_LINUX)   + \
     defined(VIGIL_PLATFORM_MACOS)) > 1
    #error "[Vigil] Multiple target platforms defined simultaneously."
#endif

/**
 * @def VIGIL_PLATFORM_NAME
 * @brief Exposes a human-readable platform identifier.
 *
 * Defines `VIGIL_PLATFORM_NAME`, a string literal describing the detected
 * target operating system. The macro is primarily intended for diagnostics,
 * logging and informational output.
 */
#if defined(VIGIL_PLATFORM_WINDOWS)
    #define VIGIL_PLATFORM_NAME "Windows"

#elif defined(VIGIL_PLATFORM_MACOS)
    #define VIGIL_PLATFORM_NAME "macOS"

#elif defined(VIGIL_PLATFORM_LINUX)
    #define VIGIL_PLATFORM_NAME "Linux"

#else
    #error "[Vigil] Target platform is not supported. Vigil currently supports Windows, Linux and macOS only."
#endif

// ============================================================================
// Architecture Detection
// ============================================================================

/// @brief Guards against conflicting architecture macros supplied by the build
/// system (e.g. two VIGIL_ARCH_* defines passed simultaneously via CMake).
#if (defined(VIGIL_ARCH_X64)    + \
     defined(VIGIL_ARCH_X86)    + \
     defined(VIGIL_ARCH_ARM64)) > 1
    #error "[Vigil] Multiple target architectures defined simultaneously."
#endif

/**
 * @name Architecture Detection
 * @brief Detects the target CPU architecture at compile time.
 *
 * Unless already defined by the build system, the target architecture is
 * determined using compiler predefined macros.
 *
 * Officially validated architectures:
 * - x86_64
 * - ARM64 / AArch64
 *
 * Experimental:
 * - x86 (32-bit)
 *
 * Compilation terminates with an error if the detected architecture is not
 * supported.
 *
 * @{
 */
#if !defined(VIGIL_ARCH_X64) && \
    !defined(VIGIL_ARCH_X86) && \
    !defined(VIGIL_ARCH_ARM64)

    #if defined(__x86_64__) || defined(_M_X64)
        #define VIGIL_ARCH_X64 1

    #elif defined(__i386__) || defined(_M_IX86)
        #define VIGIL_ARCH_X86 1

    #elif defined(__aarch64__) || defined(__arm64__) || defined(_M_ARM64)
        #define VIGIL_ARCH_ARM64 1

    #else
        #error "[Vigil] Unsupported CPU architecture. Vigil currently supports x86_64, x86 (32-bit) and ARM64 only."
    #endif

#endif // !defined(VIGIL_ARCH_*)

/** @} */

// ============================================================================
// Architecture Information
// ============================================================================

/**
 * @def VIGIL_ARCH_NAME
 * @brief Exposes a human-readable architecture identifier.
 *
 * Defines `VIGIL_ARCH_NAME`, a string literal describing the detected target
 * architecture. The macro is intended for diagnostics, logging and version
 * reporting.
 */
#if defined(VIGIL_ARCH_X64)
    #define VIGIL_ARCH_NAME "x86_64"

#elif defined(VIGIL_ARCH_ARM64)
    #define VIGIL_ARCH_NAME "ARM64/AArch64"

#elif defined(VIGIL_ARCH_X86)
    #define VIGIL_ARCH_NAME "x86"
#endif

// ============================================================================
// Compiler Detection
// ============================================================================

/**
 * @name Compiler Detection
 * @brief Detects the active C++ compiler toolchain.
 *
 * This section identifies the active compiler toolchain used to build Vigil
 * and exposes a set of macros describing the compiler name and version.
 *
 * The detected compiler information is used internally to select the
 * appropriate implementation of:
 * - compiler attributes
 * - optimization hints
 * - debugger intrinsics
 * - symbol visibility
 * - compiler-specific workarounds
 *
 * Supported compilers:
 * - Microsoft Visual C++ (MSVC)
 * - Clang-cl (Clang targeting MSVC ABI)
 * - Clang
 * - GNU Compiler (GCC)
 *
 * Compilation terminates with an error if the compiler is not supported.
 *
 * @{
 */
#if defined(__clang__)
    #define VIGIL_COMPILER_CLANG 1
    #if defined(_MSC_VER)
        #define VIGIL_COMPILER_CLANG_CL 1
        #define VIGIL_COMPILER_NAME "Clang-cl"
    #else
        #define VIGIL_COMPILER_NAME "Clang"
    #endif

#elif defined(_MSC_VER)
    #define VIGIL_COMPILER_MSVC 1
    #define VIGIL_COMPILER_NAME "MSVC"

#elif defined(__GNUC__)
    #define VIGIL_COMPILER_GCC 1
    #define VIGIL_COMPILER_NAME "GCC"

#else
    #error "[Vigil] Unsupported compiler. Vigil currently supports MSVC, Clang and GCC only."
#endif

/** @} */

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
#elif defined(VIGIL_COMPILER_CLANG) || \
      defined(VIGIL_COMPILER_CLANG_CL) || \
      defined(VIGIL_COMPILER_GCC)
    #define VIGIL_CURRENT_FUNCTION __PRETTY_FUNCTION__
#else
    #define VIGIL_CURRENT_FUNCTION __func__
#endif

// ============================================================================
// Language Standard Detection
// ============================================================================

/**
 * @name Language Standard Detection
 * @brief Detects the active C++ language standard.
 *
 * This section normalizes the language version reported by different
 * compilers and exposes a common set of macros describing the active
 * C++ standard.
 *
 * Macros are cumulative, not mutually exclusive: a C++23 build defines
 * `VIGIL_CPP23`, `VIGIL_CPP20` AND `VIGIL_CPP17`. This lets call sites test
 * "at least this standard" with a single `#if defined(VIGIL_CPP20)` check.
 *
 * The following macros are defined to indicate the active C++ standard:
 * - `VIGIL_CPP17`
 * - `VIGIL_CPP20`
 * - `VIGIL_CPP23`
 *
 * Compilation terminates with an error if the language standard is
 * older than C++17.
 *
 * @{
 */

/**
 * @def VIGIL_CPP_VERSION
 * @brief Numeric value representing the active C++ language standard.
 *
 * Expands to either __cplusplus or _MSVC_LANG depending on the compiler.
 */

#if defined(VIGIL_COMPILER_MSVC)
    #define VIGIL_CPP_VERSION _MSVC_LANG
#else
    #define VIGIL_CPP_VERSION __cplusplus
#endif

#if VIGIL_CPP_VERSION >= 202302L
    #define VIGIL_CPP23 1
    #define VIGIL_CPP20 1
    #define VIGIL_CPP17 1
#elif VIGIL_CPP_VERSION >= 202002L
    #define VIGIL_CPP20 1
    #define VIGIL_CPP17 1
#elif VIGIL_CPP_VERSION >= 201703L
    #define VIGIL_CPP17 1
#else
    #error "[Vigil] C++17 or newer is required."
#endif

/** @} */
