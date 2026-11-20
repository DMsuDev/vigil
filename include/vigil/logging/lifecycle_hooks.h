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

#include "vigil/logging/log_level.h"

#include <functional>
#include <string_view>
#include <string>

/// @file lifecycle_hooks.h
/// @brief Callback types and event structs for LogSystem hook points.

namespace vigil {

// ============================================================================
// Event context structs
// ============================================================================

/// @brief Context passed to the message callback on every emitted log entry.
struct LogMessageEvent {
    LogLevel    Level{ LogLevel::Trace }; ///< Severity level of the emitted message.
    std::string Message;                  ///< Message text as logged. The sink output pattern is not applied.
    std::string LoggerName;               ///< Name of the logger that emitted the message.
};

/// @brief Context passed when a logger's severity level changes outside OnMessage and OnLevelChange callbacks.
struct LevelChangeEvent {
    std::string_view LoggerName;                  ///< Name of the affected logger. Empty when the change is global. Valid only during the callback.
    LogLevel         OldLevel{ LogLevel::Trace }; ///< Severity level before the change. For global changes, the main logger's previous level.
    LogLevel         NewLevel{ LogLevel::Trace }; ///< Severity level after the change.
};

/// @brief Context passed after a flush operation completes.
struct FlushEvent {
    /// Name of the flushed logger. @ref LogSystem::FlushAll emits one event per logger.
    /// Valid only during the callback: copy it if it must outlive the call.
    std::string_view LoggerName;
};

// ============================================================================
// Callback signatures
// ============================================================================

/// Invoked on every emitted log message. Called on the logging thread.
using LogMessageCallback  = std::function<void(const LogMessageEvent&)>;

/// Invoked when a logger's severity level changes outside OnMessage and OnLevelChange callbacks.
using LevelChangeCallback = std::function<void(const LevelChangeEvent&)>;

/// Invoked after each flush. Reentrant flushes do not trigger another callback on the same thread.
using FlushCallback = std::function<void(const FlushEvent&)>;

/// Invoked with no arguments at lifecycle transitions (Shutdown).
/// A nested Shutdown() from inside the callback returns immediately on the same thread.
using LifecycleCallback = std::function<void()>;

// ============================================================================
// Aggregate hook set
// ============================================================================

/**
 * @brief Aggregate of all optional LogSystem hooks.
 *
 * Pass an instance to @ref LogSystem::SetHooks to register all hooks at once.
 * Any field left default-constructed (nullptr) is considered unregistered.
 *
 * OnLevelChange, OnFlush and OnShutdown prevent recursive invocation
 * on the same thread. Hooks are independent of one another.
 */
struct LogHooks {
    /// @brief Fired on every emitted log message.
    /// @warning Called on the logging thread. Avoid blocking operations.
    LogMessageCallback  OnMessage;

    /// @brief Fired when a logger's level changes.
    /// @note Changes made inside OnMessage or OnLevelChange are not reported.
    LevelChangeCallback OnLevelChange;

    /// @brief Fired after each flush operation.
    FlushCallback       OnFlush;

    /// @brief Fired before LogSystem teardown begins.
    /// @note Reentrant calls to Shutdown() return immediately.
    LifecycleCallback   OnShutdown;
};

} // namespace vigil
