// -----------------------------------------------------------------------------
//  Copyright (c) 2026 @DMsuDev. Licensed under the MIT License.
//  See LICENSE file in the project root for full license text.
// -----------------------------------------------------------------------------

#include "vigil/logging/log_limiter.h"
#include "vigil/logging/logger_registry.h"

#include "test_helpers.h"

#include <gtest/gtest.h>

#include <chrono>
#include <thread>

using namespace vigil;

// NOTE: LogOncePolicy/LogTTLPolicy track keys in process-wide static state
// that is never reset between tests (there is no public Reset()), so every
// TEST below uses a key unique to itself to stay independent from the
// others regardless of run order.

TEST(LogLimiterTest, LogOnceShouldLogIsTrueOnlyTheFirstTimeAKeyIsSeen)
{
    const std::string key = "log_once_should_log_key";

    ASSERT_TRUE(LogOncePolicy::ShouldLog(key));
    ASSERT_FALSE(LogOncePolicy::ShouldLog(key));
    ASSERT_FALSE(LogOncePolicy::ShouldLog(key));
}

TEST(LogLimiterTest, LogOnceShouldLogTracksEachKeyIndependently)
{
    const std::string keyA = "log_once_independent_key_a";
    const std::string keyB = "log_once_independent_key_b";

    ASSERT_TRUE(LogOncePolicy::ShouldLog(keyA));
    ASSERT_TRUE(LogOncePolicy::ShouldLog(keyB));
    ASSERT_FALSE(LogOncePolicy::ShouldLog(keyA));
}

TEST(LogLimiterTest, LogOnceWritesThroughTheMainLoggerOnlyOncePerKey)
{
    LogSystemConfig config;
    config.Name = "log_once_write_test";
    config.LogFile = "test_log_once_write.log";
    vigil::test::ScopedRegistry registry(config);

    auto sink = vigil::test::AttachTestSink(LoggerRegistry::Main());
    const std::string key = "log_once_write_test_key";

    LogOncePolicy::LogOnce(key, LogLevel::Info, "first");
    LogOncePolicy::LogOnce(key, LogLevel::Info, "second");
    LogOncePolicy::LogOnce(key, LogLevel::Info, "third");

    ASSERT_EQ(sink->msg_counter(), 1u);
}

TEST(LogLimiterTest, LogTTLShouldLogBlocksRepeatsUntilTheTTLExpires)
{
    const std::string key = "log_ttl_should_log_key";

    // Large TTL relative to the test's own runtime, so it cannot expire mid-test.
    ASSERT_TRUE(LogTTLPolicy::ShouldLog(key, 100.0));
    ASSERT_FALSE(LogTTLPolicy::ShouldLog(key, 100.0));
}

TEST(LogLimiterTest, LogTTLShouldLogAllowsLoggingAgainOnceTheTTLHasElapsed)
{
    const std::string key = "log_ttl_expiry_key";
    constexpr double ttlSeconds = 0.05;

    ASSERT_TRUE(LogTTLPolicy::ShouldLog(key, ttlSeconds));
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    ASSERT_TRUE(LogTTLPolicy::ShouldLog(key, ttlSeconds));
}

TEST(LogLimiterTest, LogTTLWritesThroughTheMainLoggerOncePerTTLWindow)
{
    LogSystemConfig config;
    config.Name = "log_ttl_write_test";
    config.LogFile = "test_log_ttl_write.log";
    vigil::test::ScopedRegistry registry(config);

    auto sink = vigil::test::AttachTestSink(LoggerRegistry::Main());
    const std::string key = "log_ttl_write_test_key";

    LogTTLPolicy::LogTTL(key, 100.0, LogLevel::Warn, "first");
    LogTTLPolicy::LogTTL(key, 100.0, LogLevel::Warn, "second");

    ASSERT_EQ(sink->msg_counter(), 1u);
}
