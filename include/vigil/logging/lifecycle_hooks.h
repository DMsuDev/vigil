// -----------------------------------------------------------------------------
//  Copyright (c) 2026 @DMsuDev. Licensed under the MIT License.
//  See LICENSE file in the project root for full license text.
// -----------------------------------------------------------------------------

#pragma once

#include "vigil/logging/log_level.h"

#include <functional>
#include <string_view>

/**
 * @file lifecycle_hooks.h
 * @brief Callback types and event structs for LogSystem hook points.
 */

namespace vigil {

// ============================================================================
// Event context structs
// ============================================================================

/// @brief Context passed to the message callback on every emitted log entry.
struct LogMessageEvent {
    LogLevel    Level;      ///< Severity level of the emitted message.
    std::string Message;    ///< Fully formatted message string (pattern already applied).
    std::string LoggerName; ///< Name of the logger that emitted the message.
};

/// @brief Context passed when any logger's severity level changes.
struct LevelChangeEvent {
    std::string_view LoggerName; ///< Name of the affected logger. Empty when the change is global.
    LogLevel         OldLevel;   ///< Severity level before the change.
    LogLevel         NewLevel;   ///< Severity level after the change.
};

/// @brief Context passed after a flush operation completes.
struct FlushEvent {
    /// Name of the flushed logger. Empty when @ref LogSystem::FlushAll was called.
    std::string_view LoggerName;
};

// ============================================================================
// Callback signatures
// ============================================================================

/// Invoked on every emitted log message. Called on the logging thread.
using LogMessageCallback  = std::function<void(const LogMessageEvent&)>;

/// Invoked whenever a logger's severity level changes.
using LevelChangeCallback = std::function<void(const LevelChangeEvent&)>;

/// Invoked after any flush operation.
using FlushCallback       = std::function<void(const FlushEvent&)>;

/// Invoked with no arguments at lifecycle transitions (Shutdown).
using LifecycleCallback   = std::function<void()>;

// ============================================================================
// Aggregate hook set
// ============================================================================

/**
 * @brief Aggregate of all optional LogSystem hook points.
 *
 * Pass an instance to @ref LogSystem::SetHooks to register all hooks at once.
 * Any field left default-constructed (nullptr) is treated as "not registered".
 *
 * ### C++20
 * @code{.cpp}
 * vigil::LogSystem::SetHooks({
 *     .OnMessage  = [&](const vigil::LogMessageEvent& e) { ui.Push(e.Message); },
 *     .OnShutdown = [] { RS_INFO("Vigil shutting down"); },
 * });
 * @endcode
 *
 * ### C++17
 * @code{.cpp}
 * vigil::LogHooks hooks;
 * hooks.OnMessage  = [&](const vigil::LogMessageEvent& e) { ui.Push(e.Message); };
 * hooks.OnShutdown = [] { RS_INFO("Vigil shutting down"); };
 * vigil::LogSystem::SetHooks(std::move(hooks));
 * @endcode
 */
struct LogHooks {
    /// @brief Fired on every emitted log message.
    /// @warning Called on the logging thread. Avoid blocking operations.
    LogMessageCallback  OnMessage;

    /// @brief Fired when any logger's severity level changes.
    LevelChangeCallback OnLevelChange;

    /// @brief Fired after any flush operation completes.
    FlushCallback       OnFlush;

    /// @brief Fired at the start of @ref LogSystem::Shutdown, before teardown.
    LifecycleCallback   OnShutdown;
};

} // namespace vigil
