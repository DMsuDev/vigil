// -----------------------------------------------------------------------------
//  Copyright (c) 2026 @DMsuDev. Licensed under the MIT License.
//  See LICENSE file in the project root for full license text.
// -----------------------------------------------------------------------------

#pragma once

#include "vigil/detail/platform_detection.h"

/**
 * @file symbol_export.h
 * @brief Shared library symbol visibility utilities.
 *
 * Provides a portable abstraction for controlling symbol visibility when
 * building or consuming Vigil as a shared library.
 *
 * Platform-specific mechanisms such as `__declspec(dllexport)` and
 * `__attribute__((visibility("default")))` are hidden behind a single
 * public macro.
 *
 * Build configuration macros are supplied by CMake:
 * - `VIGIL_BUILD_SHARED` indicates that Vigil is built or consumed as a shared library.
 * - `VIGIL_EXPORT_SYMBOLS` is defined only while building the Vigil library itself.
 *
 * When neither macro is defined, Vigil is assumed to be used as a static
 * library and no visibility attributes are emitted
 */

// ============================================================================
// Symbol Export
// ============================================================================

/**
 * @def VIGIL_API
 * @brief Declares a public symbol export or import for shared library builds.
 *
 * Apply this macro to every public class, function or global variable that
 * forms part of Vigil's shared library interface.
 *
 * Depending on the target platform and build configuration, the macro
 * expands to the appropriate import/export attribute or to an empty
 * definition for static library builds.
 *
 * @note This macro has no effect for static library builds.
 *
 * @code
 * class VIGIL_API Logger
 * {
 * public:
 *     void Log(std::string_view message);
 * };
 *
 * VIGIL_API void Initialize();
 * @endcode
 */
#if defined(VIGIL_BUILD_SHARED)

    // ---- Windows / Cygwin ------------------------------------------------
    #if defined(VIGIL_PLATFORM_WINDOWS)
        #if defined(VIGIL_EXPORT_SYMBOLS)
            // Export public symbols while building the DLL.
            #define VIGIL_API __declspec(dllexport)
        #else
            // Import public symbols when consuming the DLL.
            #define VIGIL_API __declspec(dllimport)
        #endif

    // ---- GCC/Clang visible platforms (Linux, macOS, etc.) ----------------
    #elif defined(VIGIL_PLATFORM_LINUX) || defined(VIGIL_PLATFORM_MACOS)
        #define VIGIL_API __attribute__((visibility("default")))

    // ---- Unknown platform ------------------------------------------------
    #else
        #error "[Vigil] Unsupported platform for shared library symbol visibility."
    #endif
#else
    // No import/export attributes are required for static libraries.
    #define VIGIL_API
#endif
