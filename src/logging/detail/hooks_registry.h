// -----------------------------------------------------------------------------
//  Copyright (c) 2026 @DMsuDev. Licensed under the MIT License.
//  See LICENSE file in the project root for full license text.
// -----------------------------------------------------------------------------

#pragma once

#include "vigil/logging/lifecycle_hooks.h"
#include "logging/detail/callback_sink.h"

#include <algorithm>
#include <memory>

/**
 * @file hooks_registry.h
 * @brief Internal storage and dispatch for all active LogSystem hooks.
 */

namespace vigil::detail {

/// @brief Owns all registered hooks and the shared @ref CallbackSink instance.
struct HooksRegistry {

    LogHooks Hooks;

    /// Shared sink instance injected into every active spdlog logger.
    /// Kept alive here so spdlog doesn't drop it between logger creations.
    std::shared_ptr<CallbackSink> MessageSink;

    // ------------------------------------------------------------------ //
    //  Dispatch helpers                                                   //
    // ------------------------------------------------------------------ //

    void FireLevelChange(const LevelChangeEvent& e) const
    {
        if (Hooks.OnLevelChange)
            Hooks.OnLevelChange(e);
    }

    void FireFlush(const FlushEvent& e) const
    {
        if (Hooks.OnFlush)
            Hooks.OnFlush(e);
    }

    void FireShutdown() const
    {
        if (Hooks.OnShutdown)
            Hooks.OnShutdown();
    }

    // ------------------------------------------------------------------ //
    //  Sink management                                                    //
    // ------------------------------------------------------------------ //

    /// @brief Attaches the message sink to @p spdLogger if a callback is active.
    void AttachSinkTo(const std::shared_ptr<spdlog::logger>& spdLogger) const
    {
        if (!MessageSink || !spdLogger)
            return;

        auto& sinks = spdLogger->sinks();

        // Avoid duplicates if called more than once for the same logger.
        const auto it = std::find(sinks.begin(), sinks.end(), MessageSink);

        if (it == sinks.end())
            sinks.push_back(MessageSink);
    }

    /// @brief Detaches the message sink from @p spdLogger.
    void DetachSinkFrom(const std::shared_ptr<spdlog::logger>& spdLogger) const
    {
        if (!MessageSink || !spdLogger)
            return;

        auto& sinks = spdLogger->sinks();

        // std::remove takes the value to remove by const-ref. Storing the cast result in a
        // named variable guarantees its lifetime extends across the entire std::remove call.
        std::shared_ptr<spdlog::sinks::sink> target = MessageSink;
        sinks.erase(
            std::remove(sinks.begin(), sinks.end(), target),
            sinks.end());
    }

    /**
     * @brief Updates or creates the shared message sink and attaches it to
     *        every logger in @p range.
     *
     * If an OnMessage callback is registered, the existing sink is reused
     * whenever possible. If no callback is registered, the sink is detached
     * from all loggers and destroyed.
     */
    template <typename LoggerRange>
    void UpdateMessageSink(LoggerRange&& range)
    {
        if (Hooks.OnMessage)
        {
            if (!MessageSink)
            {
                MessageSink = std::make_shared<CallbackSink>(Hooks.OnMessage);
                for (auto& spdLogger : range)
                    AttachSinkTo(spdLogger);
            } else {
                MessageSink->SetCallback(Hooks.OnMessage);
            }

            return;

        }

        if (!MessageSink) return;

        for (auto& spdLogger : range)
            DetachSinkFrom(spdLogger);

        MessageSink.reset();
    }

    /// @brief Resets all hooks and detaches the sink from every logger in @p range.
    template <typename LoggerRange>
    void Clear(LoggerRange&& range)
    {
        Hooks = {};

        if (!MessageSink) return;

        for (auto& spdLogger : range)
            DetachSinkFrom(spdLogger);

        MessageSink.reset();
    }
};

} // namespace vigil::detail
