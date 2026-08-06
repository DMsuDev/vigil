// -----------------------------------------------------------------------------
//  Copyright (c) 2026 @DMsuDev. Licensed under the MIT License.
//  See LICENSE file in the project root for full license text.
// -----------------------------------------------------------------------------

#include "vigil/logging/logger_registry.h"

#include "test_helpers.h"

#include <gtest/gtest.h>

using namespace vigil;

TEST(LoggerTest, ReportsTheNameItWasCreatedWith)
{
    LogSystemConfig config;
    config.Name = "logger_name_test";
    config.LogFile = "test_logger_name.log";
    vigil::test::ScopedRegistry registry(config);

    ASSERT_EQ(LoggerRegistry::Main().GetName(), "logger_name_test");
}

TEST(LoggerTest, SetLevelGetLevelRoundTrip)
{
    LogSystemConfig config;
    config.Name = "logger_level_test";
    config.LogFile = "test_logger_level.log";
    vigil::test::ScopedRegistry registry(config);

    auto& logger = LoggerRegistry::Main();

    logger.SetLevel(LogLevel::Warn);
    ASSERT_EQ(logger.GetLevel(), LogLevel::Warn);

    logger.SetLevel(LogLevel::Trace);
    ASSERT_EQ(logger.GetLevel(), LogLevel::Trace);
}

TEST(LoggerTest, MessagesBelowTheActiveLevelNeverReachTheSinks)
{
    LogSystemConfig config;
    config.Name = "logger_filter_test";
    config.LogFile = "test_logger_filter.log";
    vigil::test::ScopedRegistry registry(config);

    auto& logger = LoggerRegistry::Main();
    auto sink = vigil::test::AttachTestSink(logger);

    logger.SetLevel(LogLevel::Warn);
    logger.Trace("dropped: trace");
    logger.Debug("dropped: debug");
    logger.Info("dropped: info");
    ASSERT_EQ(sink->msg_counter(), 0u);

    logger.Warn("kept: warn");
    logger.Error("kept: error");
    ASSERT_EQ(sink->msg_counter(), 2u);
}

TEST(LoggerTest, FormattedLoggingInterpolatesFmtStyleArguments)
{
    LogSystemConfig config;
    config.Name = "logger_format_test";
    config.LogFile = "test_logger_format.log";
    vigil::test::ScopedRegistry registry(config);

    auto& logger = LoggerRegistry::Main();
    auto sink = vigil::test::AttachTestSink(logger);

    logger.Info("value is {}", 42);
    ASSERT_EQ(sink->msg_counter(), 1u);

    auto lines = sink->lines();
    ASSERT_EQ(lines.size(), 1u);
    // NOTE: this only checks the formatted argument made it through; the
    // surrounding pattern (timestamp, logger name, level) is Vigil's/spdlog's
    // concern, not this test's.
    ASSERT_NE(lines[0].find("value is 42"), std::string::npos);
}

TEST(LoggerTest, FlushForwardsToEveryAttachedSink)
{
    LogSystemConfig config;
    config.Name = "logger_flush_test";
    config.LogFile = "test_logger_flush.log";
    vigil::test::ScopedRegistry registry(config);

    auto& logger = LoggerRegistry::Main();
    auto sinkA = vigil::test::AttachTestSink(logger);
    auto sinkB = vigil::test::AttachTestSink(logger);

    logger.Flush();
    logger.Flush();

    ASSERT_EQ(sinkA->flush_counter(), 2u);
    ASSERT_EQ(sinkB->flush_counter(), 2u);
}
