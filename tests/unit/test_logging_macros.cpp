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

TEST(LoggingMacroTest, AllMainLevelMacrosForwardCorrectLevel)
{
    vigil::test::ScopedRegistry registry({});
    auto sink = vigil::test::AttachTestSink(LogSystem::Main());

    VIGIL_LOG_TRACE("trace");
    VIGIL_LOG_DEBUG("debug");
    VIGIL_LOG_INFO("info");
    VIGIL_LOG_WARN("warn");
    VIGIL_LOG_ERROR("error");
    VIGIL_LOG_CRITICAL("critical");

    ASSERT_EQ(sink->msg_counter(), 6u);

    const auto lines = sink->lines();
    // Assert message content and ordering — not the surrounding spdlog pattern.
    EXPECT_NE(lines[0].find("trace"),    std::string::npos);
    EXPECT_NE(lines[1].find("debug"),    std::string::npos);
    EXPECT_NE(lines[2].find("info"),     std::string::npos);
    EXPECT_NE(lines[3].find("warn"),     std::string::npos);
    EXPECT_NE(lines[4].find("error"),    std::string::npos);
    EXPECT_NE(lines[5].find("critical"), std::string::npos);
}

TEST(LoggingMacroTest, MacrosRespectActiveLogLevel)
{
    vigil::test::ScopedRegistry registry({});
    auto sink = vigil::test::AttachTestSink(LogSystem::Main());

    LogSystem::Main().SetLevel(LogLevel::Error);

    VIGIL_LOG_TRACE("dropped");
    VIGIL_LOG_DEBUG("dropped");
    VIGIL_LOG_INFO("dropped");
    VIGIL_LOG_WARN("dropped");
    VIGIL_LOG_ERROR("kept");
    VIGIL_LOG_CRITICAL("kept");

    EXPECT_EQ(sink->msg_counter(), 2u);
}

TEST(LoggingMacroTest, NamedMacroRoutesToTheCorrectLogger)
{
    vigil::test::ScopedRegistry registry({});

    auto& named    = LogSystem::Create("MacroNetwork");
    auto  mainSink = vigil::test::AttachTestSink(LogSystem::Main());
    auto  namedSink = vigil::test::AttachTestSink(named);

    VIGIL_LOG_NAMED("MacroNetwork", LogLevel::Error, "connection failed: {}", 7);

    // Message must reach the named sink only.
    EXPECT_EQ(namedSink->msg_counter(), 1u);
    EXPECT_EQ(mainSink->msg_counter(),  0u);
    EXPECT_NE(namedSink->lines()[0].find("connection failed: 7"), std::string::npos);
}

TEST(LoggingMacroTest, IsLevelActiveMatchesCompileTimeSeverityOrdering)
{
    static_assert(IsLevelActive(LogLevel::Trace) ==
                  (VIGIL_LEVEL_TRACE >= VIGIL_ACTIVE_LOG_LEVEL));
    static_assert(IsLevelActive(LogLevel::Info) ==
                  (VIGIL_LEVEL_INFO >= VIGIL_ACTIVE_LOG_LEVEL));
    static_assert(IsLevelActive(LogLevel::Critical) ==
                  (VIGIL_LEVEL_CRITICAL >= VIGIL_ACTIVE_LOG_LEVEL));

    // Runtime sanity: Error is always active in any sane build.
    EXPECT_TRUE(IsLevelActive(LogLevel::Error));
}

TEST(LoggingMacroTest, FormattedArgumentsAreInterpolatedCorrectly)
{
    vigil::test::ScopedRegistry registry({});
    auto sink = vigil::test::AttachTestSink(LogSystem::Main());

    VIGIL_LOG_INFO("x={} y={} z={}", 1, 2.5f, "hello");

    ASSERT_EQ(sink->msg_counter(), 1u);
    EXPECT_NE(sink->lines()[0].find("x=1"), std::string::npos);
    EXPECT_NE(sink->lines()[0].find("y=2.5"), std::string::npos);
    EXPECT_NE(sink->lines()[0].find("z=hello"), std::string::npos);
}
