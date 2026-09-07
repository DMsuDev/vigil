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

#include <string>
#include <vector>

using namespace vigil;

// =============================================================================
// OnMessage
// =============================================================================

TEST(HooksTest, OnMessageReceivesEveryLoggedMessageWithCorrectFields)
{
    LogSystemConfig config;
    config.Name = "hooks_message";
    vigil::test::ScopedRegistry registry(config);

    std::vector<LogMessageEvent> received;
    LogSystem::SetOnMessage([&](const LogMessageEvent& e) { received.push_back(e); });

    auto& named = LogSystem::Create("Network");
    LogSystem::Main().Info("main message {}", 1);
    named.Warn("network warning");

    ASSERT_EQ(received.size(), 2u);

    EXPECT_EQ(received[0].Level,      LogLevel::Info);
    EXPECT_EQ(received[0].Message,    "main message 1");
    EXPECT_EQ(received[0].LoggerName, "hooks_message");

    EXPECT_EQ(received[1].Level,      LogLevel::Warn);
    EXPECT_EQ(received[1].LoggerName, "Network");
}

TEST(HooksTest, OnMessageIsNotCalledForFilteredMessages)
{
    vigil::test::ScopedRegistry registry({});

    size_t callCount = 0;
    LogSystem::SetOnMessage([&](const LogMessageEvent&) { ++callCount; });

    LogSystem::Main().SetLevel(LogLevel::Error);
    LogSystem::Main().Info("below level — must not fire hook");

    EXPECT_EQ(callCount, 0u);
}

TEST(HooksTest, SetOnMessageReplacesThePreviousCallback)
{
    vigil::test::ScopedRegistry registry({});

    size_t firstCount  = 0;
    size_t secondCount = 0;

    LogSystem::SetOnMessage([&](const LogMessageEvent&) { ++firstCount; });
    LogSystem::Main().Info("first");

    LogSystem::SetOnMessage([&](const LogMessageEvent&) { ++secondCount; });
    LogSystem::Main().Info("second");

    LogSystem::SetOnMessage(nullptr);
    LogSystem::Main().Info("third — no hook");

    EXPECT_EQ(firstCount,  1u);
    EXPECT_EQ(secondCount, 1u);
}

TEST(HooksTest, OnMessageFiresForNamedLoggersCreatedBeforeAndAfterHookRegistration)
{
    vigil::test::ScopedRegistry registry({});

    size_t count = 0;
    auto& before = LogSystem::Create("before");
    LogSystem::SetOnMessage([&](const LogMessageEvent&) { ++count; });
    auto& after  = LogSystem::Create("after");

    before.Info("from before");
    after.Info("from after");

    EXPECT_EQ(count, 2u);
}

// =============================================================================
// OnLevelChange
// =============================================================================

TEST(HooksTest, OnLevelChangeReceivesOldAndNewLevelWithLoggerName)
{
    LogSystemConfig config;
    config.Name = "hooks_level";
    vigil::test::ScopedRegistry registry(config);

    size_t    changeCount = 0;
    LogLevel  capturedOld = LogLevel::Off;
    LogLevel  capturedNew = LogLevel::Off;
    std::string capturedName;

    LogSystem::SetOnLevelChange([&](const LevelChangeEvent& e) {
        ++changeCount;
        capturedOld  = e.OldLevel;
        capturedNew  = e.NewLevel;
        capturedName = e.LoggerName;
    });

    LogSystem::Create("Network");
    LogSystem::SetLevel("Network", LogLevel::Error);

    EXPECT_EQ(changeCount,   1u);
    EXPECT_EQ(capturedName,  "Network");
    EXPECT_EQ(capturedOld,   LogLevel::Trace); // default after Init
    EXPECT_EQ(capturedNew,   LogLevel::Error);
}

TEST(HooksTest, SetGlobalLevelFiresOnLevelChangeForEveryLogger)
{
    vigil::test::ScopedRegistry registry({});

    size_t   changeCount = 0;
    LogLevel capturedNew = LogLevel::Off;

    LogSystem::SetOnLevelChange([&](const LevelChangeEvent& e) {
        ++changeCount;
        capturedNew = e.NewLevel;
    });

    LogSystem::Create("A");
    LogSystem::Create("B");
    LogSystem::SetGlobalLevel(LogLevel::Warn);

    EXPECT_EQ(changeCount,  1u);
    EXPECT_EQ(capturedNew,  LogLevel::Warn);
}

// =============================================================================
// OnFlush
// =============================================================================

TEST(HooksTest, OnFlushIsCalledWithTheCorrectLoggerName)
{
    vigil::test::ScopedRegistry registry({});

    size_t      flushCount = 0;
    std::string flushedName;

    LogSystem::SetOnFlush([&](const FlushEvent& e) {
        ++flushCount;
        flushedName = e.LoggerName;
    });

    LogSystem::Create("Network");
    LogSystem::Flush("Network");

    EXPECT_EQ(flushCount,   1u);
    EXPECT_EQ(flushedName,  "Network");
}

TEST(HooksTest, FlushAllFiresOnFlushForEachLogger)
{
    vigil::test::ScopedRegistry registry({});

    size_t flushCount = 0;
    LogSystem::SetOnFlush([&](const FlushEvent&) { ++flushCount; });

    LogSystem::Create("A");
    LogSystem::Create("B");
    LogSystem::FlushAll(); // main + A + B = 3

    EXPECT_EQ(flushCount, 3u);
}

// =============================================================================
// OnShutdown
// =============================================================================

TEST(HooksTest, OnShutdownIsCalledExactlyOnce)
{
    vigil::test::ScopedRegistry registry({});

    size_t shutdownCount = 0;
    LogHooks hooks;
    hooks.OnShutdown = [&] { ++shutdownCount; };
    LogSystem::SetHooks(std::move(hooks));

    LogSystem::Shutdown();
    EXPECT_EQ(shutdownCount, 1u);

    // A second Shutdown() must be a no-op with respect to the callback.
    LogSystem::Shutdown();
    EXPECT_EQ(shutdownCount, 1u);
}

// =============================================================================
// ClearHooks
// =============================================================================

TEST(HooksTest, ClearHooksRemovesAllCallbacks)
{
    vigil::test::ScopedRegistry registry({});

    size_t count = 0;
    LogSystem::SetOnMessage([&](const LogMessageEvent&) { ++count; });
    LogSystem::ClearHooks();
    LogSystem::Main().Info("not observed");

    EXPECT_EQ(count, 0u);
}

// =============================================================================
// Aggregate SetHooks
// =============================================================================

TEST(HooksTest, SetHooksRegistersAllCallbacksAtOnce)
{
    LogSystemConfig config;
    config.Name = "hooks_aggregate";
    vigil::test::ScopedRegistry registry(config);

    size_t msgCount   = 0;
    bool   shutdown   = false;

    LogHooks hooks;
    hooks.OnMessage  = [&](const LogMessageEvent&) { ++msgCount; };
    hooks.OnShutdown = [&] { shutdown = true; };
    LogSystem::SetHooks(std::move(hooks));

    LogSystem::Main().Info("hello");
    EXPECT_EQ(msgCount, 1u);

    LogSystem::Shutdown();
    EXPECT_TRUE(shutdown);
}
