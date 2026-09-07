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

#include "vigil/logging/log_system.h"

#include "test_helpers.h"

#include <gtest/gtest.h>

using namespace vigil;

TEST(LoggerTest, ReportsTheNameItWasCreatedWith)
{
    LogSystemConfig config;
    config.Name = "logger_name_test";
    vigil::test::ScopedRegistry registry(config);

    ASSERT_EQ(LogSystem::Main().GetName(), "logger_name_test");
}

TEST(LoggerTest, SetLevelGetLevelRoundTrip)
{
    vigil::test::ScopedRegistry registry({});

    auto& logger = LogSystem::Main();

    logger.SetLevel(LogLevel::Warn);
    EXPECT_EQ(logger.GetLevel(), LogLevel::Warn);

    logger.SetLevel(LogLevel::Trace);
    EXPECT_EQ(logger.GetLevel(), LogLevel::Trace);
}

TEST(LoggerTest, MessagesBelowActiveLevelAreDropped)
{
    vigil::test::ScopedRegistry registry({});

    auto& logger = LogSystem::Main();
    auto  sink   = vigil::test::AttachTestSink(logger);

    logger.SetLevel(LogLevel::Warn);
    logger.Trace("dropped");
    logger.Debug("dropped");
    logger.Info("dropped");
    EXPECT_EQ(sink->msg_counter(), 0u);

    logger.Warn("kept");
    logger.Error("kept");
    EXPECT_EQ(sink->msg_counter(), 2u);
}

TEST(LoggerTest, MessagesAtOrAboveActiveLevelAreDelivered)
{
    vigil::test::ScopedRegistry registry({});

    auto& logger = LogSystem::Main();
    auto  sink   = vigil::test::AttachTestSink(logger);

    // All levels from Trace upward must pass when level is Trace.
    logger.SetLevel(LogLevel::Trace);
    logger.Trace("trace");
    logger.Debug("debug");
    logger.Info("info");
    logger.Warn("warn");
    logger.Error("error");
    logger.Critical("critical");
    EXPECT_EQ(sink->msg_counter(), 6u);
}

TEST(LoggerTest, FormattedLoggingInterpolatesFmtArguments)
{
    vigil::test::ScopedRegistry registry({});

    auto& logger = LogSystem::Main();
    auto  sink   = vigil::test::AttachTestSink(logger);

    logger.Info("value is {}", 42);
    logger.Warn("name={} id={}", "Alice", 7);

    const auto lines = sink->lines();
    ASSERT_EQ(lines.size(), 2u);
    // Only the formatted content is asserted; surrounding pattern
    // (timestamp, level, logger name) is spdlog's responsibility.
    EXPECT_NE(lines[0].find("value is 42"),      std::string::npos);
    EXPECT_NE(lines[1].find("name=Alice id=7"),  std::string::npos);
}

TEST(LoggerTest, FlushIsForwardedToEveryAttachedSink)
{
    vigil::test::ScopedRegistry registry({});

    auto& logger = LogSystem::Main();
    auto  sinkA  = vigil::test::AttachTestSink(logger);
    auto  sinkB  = vigil::test::AttachTestSink(logger);

    logger.Flush();
    logger.Flush();

    EXPECT_EQ(sinkA->flush_counter(), 2u);
    EXPECT_EQ(sinkB->flush_counter(), 2u);
}

TEST(LoggerTest, ChangingLevelDoesNotAffectMessagesAlreadyInFlight)
{
    // Synchronous logger only: no async queue involved.
    vigil::test::ScopedRegistry registry({});

    auto& logger = LogSystem::Main();
    auto  sink   = vigil::test::AttachTestSink(logger);

    logger.SetLevel(LogLevel::Trace);
    logger.Info("before level change");     // must be counted

    logger.SetLevel(LogLevel::Error);
    logger.Info("after level change");      // must be dropped

    EXPECT_EQ(sink->msg_counter(), 1u);
}
