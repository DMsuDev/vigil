// -----------------------------------------------------------------------------
//  Copyright (c) 2026 @DMsuDev. Licensed under the MIT License.
//  See LICENSE file in the project root for full license text.
// -----------------------------------------------------------------------------

#include "vigil/logging/scoped_logger.h"

#include "test_helpers.h"

#include <gtest/gtest.h>

#include <thread>
#include <chrono>

using namespace vigil;

#ifdef VIGIL_ENABLE_SCOPED_LOG

TEST(ScopedLoggerTest, EmitsEntryAndExitMessagesWithElapsedTime)
{
    LogSystemConfig config;
    config.Name = "scoped_logger_test";
    config.LogFile = "test_scoped_logger.log";
    vigil::test::ScopedRegistry registry(config);

    auto sink = vigil::test::AttachTestSink(LogSystem::Main());

    {
        ScopedLogger scope("TestScope");
        // Simulate some work
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // Entry message + exit message = 2
    ASSERT_EQ(sink->msg_counter(), 2u);

    auto lines = sink->lines();
    ASSERT_EQ(lines.size(), 2u);

    // Entry message contains ">>"
    ASSERT_NE(lines[0].find(">>"), std::string::npos);
    ASSERT_NE(lines[0].find("TestScope"), std::string::npos);

    // Exit message contains "<<" and elapsed time in ms
    ASSERT_NE(lines[1].find("<<"), std::string::npos);
    ASSERT_NE(lines[1].find("TestScope"), std::string::npos);
    ASSERT_NE(lines[1].find("ms"), std::string::npos);
}

TEST(ScopedLoggerTest, RespectsCustomLogLevel)
{
    LogSystemConfig config;
    config.Name = "scoped_logger_level_test";
    config.LogFile = "test_scoped_logger_level.log";
    vigil::test::ScopedRegistry registry(config);

    auto& logger = LogSystem::Main();
    auto sink = vigil::test::AttachTestSink(logger);

    // Set logger level to Warn, so Trace messages should be filtered out
    logger.SetLevel(LogLevel::Warn);

    {
        // This should be filtered out (Trace < Warn)
        ScopedLogger scope("FilteredScope", LogLevel::Trace);
    }

    ASSERT_EQ(sink->msg_counter(), 0u);

    {
        // This should go through (Warn >= Warn)
        ScopedLogger scope("VisibleScope", LogLevel::Warn);
    }

    // Entry + exit = 2
    ASSERT_EQ(sink->msg_counter(), 2u);
}

TEST(ScopedLoggerTest, SupportsNestedScopes)
{
    LogSystemConfig config;
    config.Name = "scoped_logger_nested_test";
    config.LogFile = "test_scoped_logger_nested.log";
    vigil::test::ScopedRegistry registry(config);

    auto sink = vigil::test::AttachTestSink(LogSystem::Main());

    {
        ScopedLogger outer("OuterScope");
        {
            ScopedLogger inner("InnerScope");
        }
    }

    // Outer entry, inner entry, inner exit, outer exit = 4
    ASSERT_EQ(sink->msg_counter(), 4u);

    auto lines = sink->lines();
    ASSERT_EQ(lines.size(), 4u);

    // Verify ordering: outer entry -> inner entry -> inner exit -> outer exit
    ASSERT_NE(lines[0].find(">> OuterScope"), std::string::npos);
    ASSERT_NE(lines[1].find(">> InnerScope"), std::string::npos);
    ASSERT_NE(lines[2].find("<< InnerScope"), std::string::npos);
    ASSERT_NE(lines[3].find("<< OuterScope"), std::string::npos);
}

TEST(ScopedLoggerTest, MacroVIGIL_SCOPED_LOGWorks)
{
    LogSystemConfig config;
    config.Name = "scoped_logger_macro_test";
    config.LogFile = "test_scoped_logger_macro.log";
    vigil::test::ScopedRegistry registry(config);

    auto sink = vigil::test::AttachTestSink(LogSystem::Main());

    {
        VIGIL_SCOPED_LOG("MacroScope");
    }

    ASSERT_EQ(sink->msg_counter(), 2u);

    auto lines = sink->lines();
    ASSERT_NE(lines[0].find(">> MacroScope"), std::string::npos);
    ASSERT_NE(lines[1].find("<< MacroScope"), std::string::npos);
}

static void TestFunctionForScopedLog()
{
    VIGIL_SCOPED_LOG_FUNCTION();
}

TEST(ScopedLoggerTest, MacroVIGIL_SCOPED_LOG_FUNCTIONWorks)
{
    LogSystemConfig config;
    config.Name = "scoped_logger_func_macro_test";
    config.LogFile = "test_scoped_logger_func_macro.log";
    vigil::test::ScopedRegistry registry(config);

    auto sink = vigil::test::AttachTestSink(LogSystem::Main());

    TestFunctionForScopedLog();

    ASSERT_EQ(sink->msg_counter(), 2u);

    auto lines = sink->lines();
    // Function name should appear in both entry and exit
    ASSERT_NE(lines[0].find(">>"), std::string::npos);
    ASSERT_NE(lines[0].find("TestFunctionForScopedLog"), std::string::npos);
    ASSERT_NE(lines[1].find("<<"), std::string::npos);
    ASSERT_NE(lines[1].find("TestFunctionForScopedLog"), std::string::npos);
}

TEST(ScopedLoggerTest, OwningStringConstructorWorks)
{
    LogSystemConfig config;
    config.Name = "scoped_logger_owning_test";
    config.LogFile = "test_scoped_logger_owning.log";
    vigil::test::ScopedRegistry registry(config);

    auto sink = vigil::test::AttachTestSink(LogSystem::Main());

    {
        std::string scopeName = "DynamicScope";
        ScopedLogger scope(std::move(scopeName));
    }

    ASSERT_EQ(sink->msg_counter(), 2u);

    auto lines = sink->lines();
    ASSERT_NE(lines[0].find(">> DynamicScope"), std::string::npos);
    ASSERT_NE(lines[1].find("<< DynamicScope"), std::string::npos);
}

#else // !VIGIL_ENABLE_SCOPED_LOG

TEST(ScopedLoggerTest, DisabledWhenFeatureIsOff)
{
    // When VIGIL_ENABLE_SCOPED_LOG is not defined, the macros should expand to ((void)0)
    // This test just verifies the code compiles without the feature enabled.
    VIGIL_SCOPED_LOG("Should be no-op");
    VIGIL_SCOPED_LOG_FUNCTION();
    SUCCEED() << "Macros compile to no-op when VIGIL_ENABLE_SCOPED_LOG is disabled.";
}

#endif // VIGIL_ENABLE_SCOPED_LOG
