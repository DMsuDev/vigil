// -----------------------------------------------------------------------------
//  Copyright (c) 2026 @DMsuDev. Licensed under the MIT License.
//  See LICENSE file in the project root for full license text.
// -----------------------------------------------------------------------------

#include "vigil/logging/log_system.h"

#include "test_helpers.h"

#include <gtest/gtest.h>

#include <filesystem>

using namespace vigil;

// =============================================================================
// Initialization / Shutdown
// =============================================================================

TEST(LogSystemTest, IsInitializedReflectsInitAndShutdownCycles)
{
    LogSystem::Shutdown();
    ASSERT_FALSE(LogSystem::IsInitialized());

    LogSystemConfig config;
    config.Name         = "init_cycle_test";
    config.ConsoleLevel = LogLevel::Off;
    config.LogDir       = vigil::test::kLogDir.string();
    LogSystem::Init(config);
    ASSERT_TRUE(LogSystem::IsInitialized());

    LogSystem::Shutdown();
    ASSERT_FALSE(LogSystem::IsInitialized());
}

TEST(LogSystemTest, SecondInitCallIsIgnoredWhileAlreadyInitialized)
{
    LogSystemConfig first;
    first.Name = "reinit_first";
    vigil::test::ScopedRegistry registry(first);

    LogSystemConfig second;
    second.Name = "reinit_second";
    LogSystem::Init(second);

    // The first Init() call wins; the second is a documented no-op.
    EXPECT_EQ(LogSystem::Main().GetName(), "reinit_first");
}

TEST(LogSystemTest, ShutdownIsIdempotent)
{
    vigil::test::ScopedRegistry registry({});
    LogSystem::Shutdown();
    ASSERT_NO_THROW(LogSystem::Shutdown()); // second call must not throw or crash
    ASSERT_FALSE(LogSystem::IsInitialized());
}

// =============================================================================
// Pre-init guards
// =============================================================================

TEST(LogSystemTest, AccessingLoggersBeforeInitThrowsLogicError)
{
    LogSystem::Shutdown();

    ASSERT_THROW((void)LogSystem::Main(),            std::logic_error);
    ASSERT_THROW((void)LogSystem::Create("x"),       std::logic_error);
    ASSERT_THROW((void)LogSystem::Get("x"),          std::logic_error);
}

TEST(LogSystemTest, FindReturnsNullptrBeforeInit)
{
    LogSystem::Shutdown();
    // Find() is explicitly documented to never throw.
    ASSERT_EQ(LogSystem::Find("x"), nullptr);
}

// =============================================================================
// Named logger lifecycle
// =============================================================================

TEST(LogSystemTest, CreateReturnsSameInstanceForDuplicateName)
{
    vigil::test::ScopedRegistry registry({});

    auto& first  = LogSystem::Create("worker");
    auto& second = LogSystem::Create("worker");
    ASSERT_EQ(&first, &second);
}

TEST(LogSystemTest, GetThrowsForUnknownName)
{
    vigil::test::ScopedRegistry registry({});
    ASSERT_THROW((void)LogSystem::Get("does_not_exist"), std::runtime_error);
}

TEST(LogSystemTest, FindReturnsNullptrForUnknownNameAndValidPtrAfterCreate)
{
    vigil::test::ScopedRegistry registry({});

    ASSERT_EQ(LogSystem::Find("known"), nullptr);

    (void)LogSystem::Create("known");
    ASSERT_NE(LogSystem::Find("known"), nullptr);
    ASSERT_EQ(&LogSystem::Get("known"), LogSystem::Find("known"));
}

TEST(LogSystemTest, RemoveDropsNamedLoggerFromRegistry)
{
    vigil::test::ScopedRegistry registry({});

    LogSystem::Create("temporary");
    ASSERT_NE(LogSystem::Find("temporary"), nullptr);

    LogSystem::Remove("temporary");
    ASSERT_EQ(LogSystem::Find("temporary"), nullptr);
}

TEST(LogSystemTest, RemoveIsNoOpForNeverRegisteredName)
{
    vigil::test::ScopedRegistry registry({});

    // Must not throw and must not disturb the main logger.
    ASSERT_NO_THROW(LogSystem::Remove("never_existed"));
    ASSERT_TRUE(LogSystem::IsInitialized());
}

TEST(LogSystemTest, SetMainPromotesNamedLoggerAndRemovesItFromNamedMap)
{
    LogSystemConfig config;
    config.Name = "setmain_host";
    vigil::test::ScopedRegistry registry(config);

    LogSystem::Create("promoted");
    LogSystem::SetMain("promoted");

    EXPECT_EQ(LogSystem::Main().GetName(), "promoted");
    EXPECT_EQ(LogSystem::Find("promoted"), nullptr); // no longer in named map
}

TEST(LogSystemTest, SetMainIsNoOpForNeverRegisteredName)
{
    LogSystemConfig config;
    config.Name = "setmain_noop_host";
    vigil::test::ScopedRegistry registry(config);

    LogSystem::SetMain("never_existed");
    EXPECT_EQ(LogSystem::Main().GetName(), "setmain_noop_host");
}

// =============================================================================
// Global level control
// =============================================================================

TEST(LogSystemTest, SetGlobalLevelAppliesToMainAndAllNamedLoggers)
{
    vigil::test::ScopedRegistry registry({});

    auto& named = LogSystem::Create("named");

    LogSystem::SetGlobalLevel(LogLevel::Error);

    EXPECT_EQ(LogSystem::Main().GetLevel(), LogLevel::Error);
    EXPECT_EQ(named.GetLevel(), LogLevel::Error);
}

TEST(LogSystemTest, SetLevelOnlyAffectsTheTargetedLogger)
{
    vigil::test::ScopedRegistry registry({});

    auto& named = LogSystem::Create("targeted");
    LogSystem::SetLevel("targeted", LogLevel::Critical);

    EXPECT_EQ(named.GetLevel(), LogLevel::Critical);
    // Main logger must be untouched.
    EXPECT_NE(LogSystem::Main().GetLevel(), LogLevel::Critical);
}

// =============================================================================
// Flush
// =============================================================================

TEST(LogSystemTest, FlushAllFlushesMainAndEveryNamedLogger)
{
    vigil::test::ScopedRegistry registry({});

    auto& named    = LogSystem::Create("named");
    auto mainSink  = vigil::test::AttachTestSink(LogSystem::Main());
    auto namedSink = vigil::test::AttachTestSink(named);

    LogSystem::FlushAll();

    EXPECT_EQ(mainSink->flush_counter(),  1u);
    EXPECT_EQ(namedSink->flush_counter(), 1u);
}

TEST(LogSystemTest, FlushByNameOnlyFlushesTheRequestedLogger)
{
    vigil::test::ScopedRegistry registry({});

    auto& named    = LogSystem::Create("named");
    auto mainSink  = vigil::test::AttachTestSink(LogSystem::Main());
    auto namedSink = vigil::test::AttachTestSink(named);

    LogSystem::Flush("named");

    EXPECT_EQ(namedSink->flush_counter(), 1u);
    EXPECT_EQ(mainSink->flush_counter(),  0u);
}

TEST(LogSystemTest, FlushByNameIsNoOpForUnknownLogger)
{
    vigil::test::ScopedRegistry registry({});

    auto mainSink = vigil::test::AttachTestSink(LogSystem::Main());
    ASSERT_NO_THROW(LogSystem::Flush("never_existed"));
    EXPECT_EQ(mainSink->flush_counter(), 0u);
}

// =============================================================================
// Lazy file creation
// =============================================================================

TEST(LogSystemTest, LogFileIsNotCreatedWhenNothingIsLogged)
{
    const auto logPath = vigil::test::kLogDir / "lazy_empty.log";
    std::filesystem::remove(logPath);

    LogSystemConfig config;
    config.Name    = "lazy_empty";
    config.LogFile = "lazy_empty.log";
    vigil::test::ScopedRegistry registry(config);

    LogSystem::FlushAll(); // flush with nothing written must not create the file

    ASSERT_FALSE(std::filesystem::exists(logPath));
}

TEST(LogSystemTest, LogFileIsCreatedOnceFirstMessageIsLogged)
{
    const auto logPath = vigil::test::kLogDir / "lazy_create.log";
    std::filesystem::remove(logPath);

    LogSystemConfig config;
    config.Name    = "lazy_create";
    config.LogFile = "lazy_create.log";
    vigil::test::ScopedRegistry registry(config);

    ASSERT_FALSE(std::filesystem::exists(logPath));

    LogSystem::Main().Info("this message creates the file");

    ASSERT_TRUE(std::filesystem::exists(logPath));
}

TEST(LogSystemTest, MessagesBelowFileLevelDoNotCreateTheLogFile)
{
    const auto logPath = vigil::test::kLogDir / "lazy_filter.log";
    std::filesystem::remove(logPath);

    LogSystemConfig config;
    config.Name    = "lazy_filter";
    config.LogFile = "lazy_filter.log";
    vigil::test::ScopedRegistry registry(config);

    // File sink defaults to Trace, so set it to Error to filter Debug/Info.
    LogSystem::SetGlobalFileLevel(LogLevel::Error);

    LogSystem::Main().Debug("below filter — must not create file");
    LogSystem::Main().Info("below filter — must not create file");

    ASSERT_FALSE(std::filesystem::exists(logPath))
        << "File was created despite all messages being below the file-level filter";

    LogSystem::Main().Error("above filter — must create file");
    ASSERT_TRUE(std::filesystem::exists(logPath));
}
