// -----------------------------------------------------------------------------
//  Copyright (c) 2026 @DMsuDev. Licensed under the MIT License.
//  See LICENSE file in the project root for full license text.
// -----------------------------------------------------------------------------

#pragma once

#include "vigil/logging/lifecycle_hooks.h"
#include "logging/detail/spd_convert.h"

#include <spdlog/sinks/base_sink.h>
#include <mutex>
#include <string>

/**
 * @file callback_sink.h
 * @brief Internal spdlog sink that forwards formatted messages to a LogMessageCallback.
 */

namespace vigil::detail {

/**
 * @brief spdlog sink implementation that dispatches to a @ref LogMessageCallback.
 *
 * Registered as an additional sink on every active logger when a message
 * callback is set via @ref LogSystem::SetOnMessage or @ref LogSystem::SetHooks.
 * Removed from all loggers when the callback is cleared.
 *
 * @warning The callback is invoked on the logging thread while the sink lock
 *          is held. Keep it fast and non-blocking. Avoid calling any
 *          LogSystem functions from within the callback.
 */
class CallbackSink final : public spdlog::sinks::base_sink<std::mutex>
{
public:
    explicit CallbackSink(LogMessageCallback callback)
        : m_Callback(std::move(callback))
    {}

    /**
     * @brief Replaces the active callback without rebuilding the sink.
     * @note Thread-safe: acquires the sink's internal mutex before replacing the callback.
     */
    void SetCallback(LogMessageCallback callback)
    {
        std::scoped_lock lock(mutex_);
        m_Callback = std::move(callback);
    }

    /**
     * @brief Returns true if a callback is currently registered.
     * @note Thread-safe: acquires the sink's internal mutex before checking the callback.
     */
    [[nodiscard]] bool HasCallback() noexcept
    {
        std::scoped_lock lock(mutex_);
        return static_cast<bool>(m_Callback);
    }

protected:
    /**
     * @brief Invoked by spdlog for every log message that passes the sink's level filter.
     *
     * Called with @c mutex_ already held by @c base_sink, do NOT acquire
     * @c mutex_ again inside this function.
     *
     * @note @p msg.payload and @p msg.logger_name are string_views into spdlog's
     *       internal buffers. They are valid only for the duration of this call,
     *       so the event strings are copied into owned storage before dispatch.
     */
    void sink_it_(const spdlog::details::log_msg& msg) override
    {
        if (!m_Callback) return;

        // msg.payload and msg.logger_name are views into spdlog's internal
        // buffers, copy into owned strings before dispatch since the
        // callback may outlive this stack frame in deferred scenarios.
        LogMessageEvent event;
        event.Level      = detail::FromSpdLevel(msg.level);
        event.Message    = std::string(msg.payload.data(),     msg.payload.size());
        event.LoggerName = std::string(msg.logger_name.data(), msg.logger_name.size());

        m_Callback(event);
    }

    /// @brief No-op flush implementation.
    void flush_() override {}

private:
    LogMessageCallback m_Callback;
};

} // namespace vigil::detail
