// -----------------------------------------------------------------------------
//  Copyright (c) 2026 @DMsuDev. Licensed under the MIT License.
//  See LICENSE file in the project root for full license text.
// -----------------------------------------------------------------------------

#pragma once

// NOTE: adapted from spdlog's own vendor/spdlog/tests/test_sink.h. Vigil hides
// spdlog from its public API, but its tests are white-box: they reach into
// Logger::Impl() to attach this sink directly to the underlying spdlog::logger,
// the same way spdlog's own test suite does with spdlog::sinks::test_sink.

#include <spdlog/details/null_mutex.h>
#include <spdlog/details/os.h>
#include <spdlog/sinks/base_sink.h>

#include <chrono>
#include <cstring>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace vigil::test {

template <class Mutex>
class TestSink : public spdlog::sinks::base_sink<Mutex> {
    const size_t lines_to_save = 100;

public:
    size_t msg_counter()
    {
        std::lock_guard<Mutex> lock(spdlog::sinks::base_sink<Mutex>::mutex_);
        return msg_counter_;
    }

    size_t flush_counter()
    {
        std::lock_guard<Mutex> lock(spdlog::sinks::base_sink<Mutex>::mutex_);
        return flush_counter_;
    }

    void set_delay(std::chrono::milliseconds delay)
    {
        std::lock_guard<Mutex> lock(spdlog::sinks::base_sink<Mutex>::mutex_);
        delay_ = delay;
    }

    // Returns every saved line, without the trailing eol.
    std::vector<std::string> lines()
    {
        std::lock_guard<Mutex> lock(spdlog::sinks::base_sink<Mutex>::mutex_);
        return lines_;
    }

protected:
    void sink_it_(const spdlog::details::log_msg& msg) override
    {
        spdlog::memory_buf_t formatted;
        spdlog::sinks::base_sink<Mutex>::formatter_->format(msg, formatted);
        auto eol_len = strlen(spdlog::details::os::default_eol);
        using diff_t = typename std::iterator_traits<decltype(formatted.end())>::difference_type;
        if (lines_.size() < lines_to_save) {
            lines_.emplace_back(formatted.begin(), formatted.end() - static_cast<diff_t>(eol_len));
        }
        msg_counter_++;
        std::this_thread::sleep_for(delay_);
    }

    void flush_() override { flush_counter_++; }

    size_t msg_counter_{0};
    size_t flush_counter_{0};
    std::chrono::milliseconds delay_{std::chrono::milliseconds::zero()};
    std::vector<std::string> lines_;
};

using TestSinkMt = TestSink<std::mutex>;
using TestSinkSt = TestSink<spdlog::details::null_mutex>;

} // namespace vigil::test
