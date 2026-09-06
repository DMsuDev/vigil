// -----------------------------------------------------------------------------
//  Copyright (c) 2026 @DMsuDev. Licensed under the MIT License.
//  See LICENSE file in the project root for full license text.
// -----------------------------------------------------------------------------

#include "vigil/logging/log_limiter.h"
#include "vigil/logging/log_system.h"

#include "test_helpers.h"

#include <gtest/gtest.h>

#include <chrono>
#include <thread>

using namespace vigil;

// NOTE: LogOncePolicy and LogTTLPolicy store state in process-wide statics
// with no public Reset(). Every test therefore uses a key derived from the
// test name (via __func__) to stay independent of run order.

// =============================================================================
// LogOncePolicy
// =============================================================================

TEST(LogLimiterTest, LogOnceShouldLogReturnsTrueOnlyOnFirstCall)
{
    const std::string key = "LogOnceShouldLogReturnsTrueOnlyOnFirstCall";

    EXPECT_TRUE(LogOncePolicy::ShouldLog(key));
    EXPECT_FALSE(LogOncePolicy::ShouldLog(key));
    EXPECT_FALSE(LogOncePolicy::ShouldLog(key));
}

TEST(LogLimiterTest, LogOnceTracksEachKeyIndependently)
{
    const std::string keyA = "LogOnceTracksEachKeyIndependently_A";
    const std::string keyB = "LogOnceTracksEachKeyIndependently_B";

    EXPECT_TRUE(LogOncePolicy::ShouldLog(keyA));
    EXPECT_TRUE(LogOncePolicy::ShouldLog(keyB));
    EXPECT_FALSE(LogOncePolicy::ShouldLog(keyA));
    EXPECT_FALSE(LogOncePolicy::ShouldLog(keyB));
}

TEST(LogLimiterTest, LogOnceWritesExactlyOneMessageRegardlessOfCallCount)
{
    vigil::test::ScopedRegistry registry({});
    auto sink = vigil::test::AttachTestSink(LogSystem::Main());

    const std::string key = "LogOnceWritesExactlyOneMessage";
    LogOncePolicy::LogOnce(key, LogLevel::Info, "first");
    LogOncePolicy::LogOnce(key, LogLevel::Info, "second");
    LogOncePolicy::LogOnce(key, LogLevel::Info, "third");

    EXPECT_EQ(sink->msg_counter(), 1u);
}

TEST(LogLimiterTest, LogOnceMessageContentIsPreservedOnFirstWrite)
{
    vigil::test::ScopedRegistry registry({});
    auto sink = vigil::test::AttachTestSink(LogSystem::Main());

    const std::string key = "LogOnceMessageContentIsPreserved";
    LogOncePolicy::LogOnce(key, LogLevel::Info, "expected content");
    LogOncePolicy::LogOnce(key, LogLevel::Info, "must not appear");

    const auto lines = sink->lines();
    ASSERT_EQ(lines.size(), 1u);
    EXPECT_NE(lines[0].find("expected content"), std::string::npos);
}

// =============================================================================
// LogTTLPolicy
// =============================================================================

TEST(LogLimiterTest, LogTTLShouldLogBlocksRepeatsWithinTTLWindow)
{
    const std::string key = "LogTTLShouldLogBlocksRepeats";

    EXPECT_TRUE(LogTTLPolicy::ShouldLog(key, 100.0)); // large TTL, cannot expire
    EXPECT_FALSE(LogTTLPolicy::ShouldLog(key, 100.0));
}

TEST(LogLimiterTest, LogTTLShouldLogAllowsLoggingAfterTTLExpires)
{
    const std::string key       = "LogTTLShouldLogAllowsAfterExpiry";
    constexpr double  kTTL      = 0.05; // 50 ms
    constexpr auto    kWait     = std::chrono::milliseconds{150};

    EXPECT_TRUE(LogTTLPolicy::ShouldLog(key, kTTL));
    std::this_thread::sleep_for(kWait);
    EXPECT_TRUE(LogTTLPolicy::ShouldLog(key, kTTL));
}

TEST(LogLimiterTest, LogTTLWritesExactlyOneMessagePerWindow)
{
    vigil::test::ScopedRegistry registry({});
    auto sink = vigil::test::AttachTestSink(LogSystem::Main());

    const std::string key = "LogTTLWritesExactlyOneMessagePerWindow";
    LogTTLPolicy::LogTTL(key, 100.0, LogLevel::Warn, "first");
    LogTTLPolicy::LogTTL(key, 100.0, LogLevel::Warn, "second");

    EXPECT_EQ(sink->msg_counter(), 1u);
}

TEST(LogLimiterTest, LogTTLTracksEachKeyIndependently)
{
    const std::string keyA = "LogTTLTracksEachKeyIndependently_A";
    const std::string keyB = "LogTTLTracksEachKeyIndependently_B";

    EXPECT_TRUE(LogTTLPolicy::ShouldLog(keyA, 100.0));
    EXPECT_TRUE(LogTTLPolicy::ShouldLog(keyB, 100.0));
    EXPECT_FALSE(LogTTLPolicy::ShouldLog(keyA, 100.0));
    EXPECT_FALSE(LogTTLPolicy::ShouldLog(keyB, 100.0));
}

TEST(LogLimiterTest, LogTTLWritesTwoMessagesAcrossTwoWindows)
{
    vigil::test::ScopedRegistry registry({});
    auto sink = vigil::test::AttachTestSink(LogSystem::Main());

    const std::string key   = "LogTTLWritesTwoMessagesAcrossTwoWindows";
    constexpr double  kTTL  = 0.05;
    constexpr auto    kWait = std::chrono::milliseconds{150};

    LogTTLPolicy::LogTTL(key, kTTL, LogLevel::Info, "window 1");
    std::this_thread::sleep_for(kWait);
    LogTTLPolicy::LogTTL(key, kTTL, LogLevel::Info, "window 2");

    EXPECT_EQ(sink->msg_counter(), 2u);
}
