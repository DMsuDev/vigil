// -----------------------------------------------------------------------------
//  Copyright (c) 2026 @DMsuDev. Licensed under the MIT License.
//  See LICENSE file in the project root for full license text.
// -----------------------------------------------------------------------------

#include "vigil/logging/scoped_logger.h"

#include "test_helpers.h"

#include <gtest/gtest.h>

#include <chrono>
#include <thread>

using namespace vigil;

#ifdef VIGIL_ENABLE_SCOPED_LOG

// =============================================================================
// Basic behaviour
// =============================================================================

TEST(ScopedLoggerTest, EmitsEntryAndExitMessages)
{
    vigil::test::ScopedRegistry registry({});
    auto sink = vigil::test::AttachTestSink(LogSystem::Main());

    { ScopedLogger scope("BasicScope"); }

    ASSERT_EQ(sink->msg_counter(), 2u);
    const auto lines = sink->lines();
    EXPECT_NE(lines[0].find(">>"),          std::string::npos);
    EXPECT_NE(lines[0].find("BasicScope"),  std::string::npos);
    EXPECT_NE(lines[1].find("<<"),          std::string::npos);
    EXPECT_NE(lines[1].find("BasicScope"),  std::string::npos);
}

TEST(ScopedLoggerTest, ExitMessageContainsElapsedTimeInMs)
{
    vigil::test::ScopedRegistry registry({});
    auto sink = vigil::test::AttachTestSink(LogSystem::Main());

    {
        ScopedLogger scope("TimedScope");
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    const auto lines = sink->lines();
    ASSERT_EQ(lines.size(), 2u);
    EXPECT_NE(lines[1].find("ms"), std::string::npos);
}

// =============================================================================
// Level filtering
// =============================================================================

TEST(ScopedLoggerTest, ScopeBelowActiveLevelProducesNoMessages)
{
    vigil::test::ScopedRegistry registry({});
    auto& logger = LogSystem::Main();
    auto  sink   = vigil::test::AttachTestSink(logger);

    logger.SetLevel(LogLevel::Warn);
    { ScopedLogger scope("FilteredScope", LogLevel::Trace); } // Trace < Warn

    EXPECT_EQ(sink->msg_counter(), 0u);
}

TEST(ScopedLoggerTest, ScopeAtActiveLevelProducesBothMessages)
{
    vigil::test::ScopedRegistry registry({});
    auto& logger = LogSystem::Main();
    auto  sink   = vigil::test::AttachTestSink(logger);

    logger.SetLevel(LogLevel::Warn);
    { ScopedLogger scope("VisibleScope", LogLevel::Warn); } // Warn >= Warn

    EXPECT_EQ(sink->msg_counter(), 2u);
}

// =============================================================================
// Nesting
// =============================================================================

TEST(ScopedLoggerTest, NestedScopesProduceMessagesInCorrectOrder)
{
    vigil::test::ScopedRegistry registry({});
    auto sink = vigil::test::AttachTestSink(LogSystem::Main());

    {
        ScopedLogger outer("OuterScope");
        { ScopedLogger inner("InnerScope"); }
    }

    // Expected order: outer>>, inner>>, inner<<, outer<<
    ASSERT_EQ(sink->msg_counter(), 4u);
    const auto lines = sink->lines();
    EXPECT_NE(lines[0].find(">> OuterScope"), std::string::npos);
    EXPECT_NE(lines[1].find(">> InnerScope"), std::string::npos);
    EXPECT_NE(lines[2].find("<< InnerScope"), std::string::npos);
    EXPECT_NE(lines[3].find("<< OuterScope"), std::string::npos);
}

// =============================================================================
// Macros
// =============================================================================

TEST(ScopedLoggerTest, VIGIL_SCOPED_LOGEmitsEntryAndExit)
{
    vigil::test::ScopedRegistry registry({});
    auto sink = vigil::test::AttachTestSink(LogSystem::Main());

    { VIGIL_SCOPED_LOG("MacroScope"); }

    ASSERT_EQ(sink->msg_counter(), 2u);
    const auto lines = sink->lines();
    EXPECT_NE(lines[0].find(">> MacroScope"), std::string::npos);
    EXPECT_NE(lines[1].find("<< MacroScope"), std::string::npos);
}

static void HelperForFunctionMacroTest()
{
    VIGIL_SCOPED_LOG_FUNCTION();
}

TEST(ScopedLoggerTest, VIGIL_SCOPED_LOG_FUNCTIONIncludesFunctionName)
{
    vigil::test::ScopedRegistry registry({});
    auto sink = vigil::test::AttachTestSink(LogSystem::Main());

    HelperForFunctionMacroTest();

    ASSERT_EQ(sink->msg_counter(), 2u);
    const auto lines = sink->lines();
    EXPECT_NE(lines[0].find("HelperForFunctionMacroTest"), std::string::npos);
    EXPECT_NE(lines[1].find("HelperForFunctionMacroTest"), std::string::npos);
    EXPECT_NE(lines[0].find(">>"), std::string::npos);
    EXPECT_NE(lines[1].find("<<"), std::string::npos);
}

TEST(ScopedLoggerTest, VIGIL_SCOPED_LOG_LEVELUsesExplicitLevel)
{
    vigil::test::ScopedRegistry registry({});
    auto& logger = LogSystem::Main();
    auto  sink   = vigil::test::AttachTestSink(logger);

    logger.SetLevel(LogLevel::Warn);

    { VIGIL_SCOPED_LOG_LEVEL("Filtered",  LogLevel::Debug); } // Debug < Warn — dropped
    { VIGIL_SCOPED_LOG_LEVEL("Visible",   LogLevel::Warn);  } // Warn >= Warn — kept

    ASSERT_EQ(sink->msg_counter(), 2u); // only the second scope's two messages
    const auto lines = sink->lines();
    EXPECT_NE(lines[0].find("Visible"), std::string::npos);
}

TEST(ScopedLoggerTest, VIGIL_SCOPE_BEGIN_ENDPairBehavesLikeTheObjectForm)
{
    vigil::test::ScopedRegistry registry({});
    auto sink = vigil::test::AttachTestSink(LogSystem::Main());

    {
        VIGIL_SCOPE_BEGIN_LEVEL("ManualScope", LogLevel::Info);
        VIGIL_SCOPE_END();
    }

    ASSERT_EQ(sink->msg_counter(), 2u);
    const auto lines = sink->lines();
    EXPECT_NE(lines[0].find(">> ManualScope"), std::string::npos);
    EXPECT_NE(lines[1].find("<< ManualScope"), std::string::npos);
}

// =============================================================================
// Owning string constructor
// =============================================================================

TEST(ScopedLoggerTest, OwningStringConstructorPreservesName)
{
    vigil::test::ScopedRegistry registry({});
    auto sink = vigil::test::AttachTestSink(LogSystem::Main());

    {
        std::string name = "DynamicScope";
        ScopedLogger scope(std::move(name));
    }

    ASSERT_EQ(sink->msg_counter(), 2u);
    const auto lines = sink->lines();
    EXPECT_NE(lines[0].find(">> DynamicScope"), std::string::npos);
    EXPECT_NE(lines[1].find("<< DynamicScope"), std::string::npos);
}

#else // !VIGIL_ENABLE_SCOPED_LOG

TEST(ScopedLoggerTest, MacrosAreNoOpsWhenFeatureIsDisabled)
{
    VIGIL_SCOPED_LOG("noop");
    VIGIL_SCOPED_LOG_FUNCTION();
    SUCCEED() << "Macros compile to no-op when VIGIL_ENABLE_SCOPED_LOG is not defined.";
}

#endif // VIGIL_ENABLE_SCOPED_LOG
