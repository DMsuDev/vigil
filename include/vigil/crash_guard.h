// -----------------------------------------------------------------------------
//   _    __ __ _____ ___ __
//  | |  / / _/ ____/ _/ /
//  | | / // // / __ / // /     Logging and Diagnostics for C++
//  | |/ // // /_/ // // /___   https://github.com/DMsuDev/vigil
//  |___/___/\____/___/_____/
//
//  Copyright (c) 2026 @DMsuDev. Licensed under the MIT License.
//  See LICENSE file in the project root for full license text.
// -----------------------------------------------------------------------------

#pragma once

#include "vigil/detail/symbol_export.h"

#include <filesystem>
#include <functional>

/**
 * @file crash_guard.h
 * @brief Cross-platform crash and fatal signal handler.
 *
 * Intercepts unhandled exceptions and fatal system signals to capture stack traces,
 * write a crash report file and invokes the user callback prior to process
 * termination.
 */

namespace vigil {

/// @brief Information passed to the user callback on a fatal crash.
struct CrashInfo
{
    const char* reason;      ///< Short description of the failure.
    const char* stackTrace;  ///< Pre-formatted stack trace, or nullptr.
    const char* reportPath;  ///< Path of the crash report file, or nullptr if it could not be written.
};

/// @brief User-supplied callback invoked once before process termination.
using CrashCallback = std::function<void(const CrashInfo&)>;

/**
 * @brief Cross-platform crash handler.
 *
 * Intercepts fatal system signals and unhandled exceptions, flushes the log,
 * writes a crash report file and invokes the user callback before process
 * termination.
 *
 * @note Call InstallThreadAltStack() on every worker thread created by the application.
 */
class VIGIL_API CrashHandler
{
public:
    CrashHandler()                               = delete;
    CrashHandler(const CrashHandler&)            = delete;
    CrashHandler& operator=(const CrashHandler&) = delete;

    /**
     * @brief Arms all platform crash hooks.
     *
     * Must be invoked from the main thread before creating any secondary threads.
     * Automatically registers the emergency stack for the calling thread.
     *
     * @param callback        Optional user callback executed before termination.
     * @param reportDirectory Preferred directory for crash reports. The system temporary
     *                        directory is used when empty or not writable.
     */
    static void Install(CrashCallback callback = nullptr,
                        const std::filesystem::path& reportDirectory = {});

    /**
     * @brief Prepares the current thread to safely handle a stack overflow.
     *
     * Allocates emergency stack memory so fatal error handlers can execute if the thread
     * runs out of stack space.
     *
     * @note Call this on every secondary thread after system initialization.
     *       The main thread is covered automatically.
     */
    static void InstallThreadAltStack() noexcept;

    /// @brief Returns true if Install() has been called at least once.
    [[nodiscard]] static bool IsInstalled() noexcept;
};

} // namespace vigil
