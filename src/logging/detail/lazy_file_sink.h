// -----------------------------------------------------------------------------
//  Copyright (c) 2026 @DMsuDev. Licensed under the MIT License.
//  See LICENSE file in the project root for full license text.
// -----------------------------------------------------------------------------

#pragma once

#include <spdlog/sinks/base_sink.h>
#include <spdlog/details/null_mutex.h>

#include <functional>
#include <mutex>

namespace vigil::detail {

/**
 * @brief File sink that defers file creation until the first message is logged.
 *
 * Prevents empty `.log` files when the application exits without writing
 * anything. If the factory throws on first use, subsequent messages are
 * silently dropped, and the application keeps running normally
 *
 * @tparam Mutex Use @c std::mutex (via @c LazyFileSink_mt) for multi-threaded
 *               loggers, or @c spdlog::details::null_mutex (via
 *               @c LazyFileSink_st) for single-threaded ones.
 */
template <typename Mutex>
class LazyFileSink final : public spdlog::sinks::base_sink<Mutex> {
public:
    /// Callable that constructs and returns the real file sink on first use.
    using Factory = std::function<spdlog::sink_ptr()>;

    /// @param factory Called once on the first logged message. Must return a
    ///                valid sink. Released immediately after to free resources.
    explicit LazyFileSink(Factory factory) : m_Factory(std::move(factory)) {}

protected:
    /// Initialises the real sink on first call, then forwards the message.
    /// The base class holds its mutex before calling this, so initialisation
    /// and forwarding are fully serialised without additional locking.
    void sink_it_(const spdlog::details::log_msg& msg) override
    {
        if (!m_RealSink && !m_InitFailed)
            try_init_();

        if (m_RealSink)
            m_RealSink->log(msg);
    }

    /// Flushes the real sink if it has been initialised.
    void flush_() override
    {
        if (m_RealSink)
            m_RealSink->flush();
    }

private:
    /// Attempts to create the underlying sink exactly once, and prevents future
    /// retries after a failed construction. This keeps the logger alive even if
    /// the file cannot be opened or the path is invalid.
    void try_init_()
    {
        try
        {
            m_RealSink = m_Factory();
        }
        catch (...)
        {
            m_InitFailed = true;
        }

        // Release the factory closure immediately after first-use creation;
        // we do not want to keep a lambda capturing large state around.
        m_Factory = nullptr;
    }

    Factory          m_Factory;
    spdlog::sink_ptr m_RealSink;
    bool             m_InitFailed = false;
};

/// Thread-safe variant. Suitable for the vast majority of cases.
using LazyFileSink_mt = LazyFileSink<std::mutex>;

/// Single-threaded variant. Only use when the owning logger runs on one thread.
using LazyFileSink_st = LazyFileSink<spdlog::details::null_mutex>;

} // namespace vigil::detail
