// -----------------------------------------------------------------------------
//  Copyright (c) 2026 @DMsuDev. Licensed under the MIT License.
//  See LICENSE file in the project root for full license text.
// -----------------------------------------------------------------------------

#include "vigil/logging/log_system.h"

#include "test_helpers.h"

#include <gtest/gtest.h>

using namespace vigil;

TEST(LogSystemTest, IsInitializedReflectsInitShutdownCalls)
{
    LogSystem::Shutdown();
    ASSERT_FALSE(LogSystem::IsInitialized());

    LogSystemConfig config;
    config.Name = "registry_init_test";
    config.LogFile = (vigil::test::kLogDir / "test_registry_init.log").string();
    config.ConsoleLevel = LogLevel::Off;
    LogSystem::Init(config);
    ASSERT_TRUE(LogSystem::IsInitialized());

    LogSystem::Shutdown();
    ASSERT_FALSE(LogSystem::IsInitialized());
}

TEST(LogSystemTest, ASecondInitCallIsIgnoredWhileAlreadyInitialized)
{
    LogSystemConfig first;
    first.Name = "registry_reinit_first";
    first.LogFile = "test_registry_reinit_first.log";
    vigil::test::ScopedRegistry registry(first);

    LogSystemConfig second;
    second.Name = "registry_reinit_second";
    second.LogFile = "test_registry_reinit_second.log";
    LogSystem::Init(second);

    // The first Init() call wins; the second is a documented no-op.
    ASSERT_EQ(LogSystem::Main().GetName(), "registry_reinit_first");
}

TEST(LogSystemTest, AccessingLoggersBeforeInitThrows)
{
    LogSystem::Shutdown();

    ASSERT_THROW((void)LogSystem::Main(), std::logic_error);
    ASSERT_THROW((void)LogSystem::Create("whatever"), std::logic_error);
    ASSERT_THROW((void)LogSystem::Get("whatever"), std::logic_error);

    // Find() is explicitly documented to never throw.
    ASSERT_EQ(LogSystem::Find("whatever"), nullptr);
}

TEST(LogSystemTest, CreateReturnsTheSameInstanceForARepeatedName)
{
    LogSystemConfig config;
    config.Name = "registry_create_test";
    config.LogFile = "test_registry_create.log";
    vigil::test::ScopedRegistry registry(config);

    auto& first = LogSystem::Create("worker");
    auto& second = LogSystem::Create("worker");
    ASSERT_EQ(&first, &second);
}

TEST(LogSystemTest, GetThrowsForAnUnknownLoggerNameFindReturnsNullptrInstead)
{
    LogSystemConfig config;
    config.Name = "registry_get_find_test";
    config.LogFile = "test_registry_get_find.log";
    vigil::test::ScopedRegistry registry(config);

    ASSERT_THROW((void)LogSystem::Get("does_not_exist"), std::runtime_error);
    ASSERT_EQ(LogSystem::Find("does_not_exist"), nullptr);

    (void)LogSystem::Create("known");
    ASSERT_NE(LogSystem::Find("known"), nullptr);
    ASSERT_EQ(&LogSystem::Get("known"), LogSystem::Find("known"));
}

TEST(LogSystemTest, RemoveDropsANamedLoggerFromTheRegistry)
{
    LogSystemConfig config;
    config.Name = "registry_remove_test";
    config.LogFile = "test_registry_remove.log";
    vigil::test::ScopedRegistry registry(config);

    LogSystem::Create("temporary");
    ASSERT_NE(LogSystem::Find("temporary"), nullptr);

    LogSystem::Remove("temporary");
    ASSERT_EQ(LogSystem::Find("temporary"), nullptr);
}

TEST(LogSystemTest, RemoveIsANoOpForANameThatWasNeverRegistered)
{
    LogSystemConfig config;
    config.Name = "registry_remove_missing_test";
    config.LogFile = "test_registry_remove_missing.log";
    vigil::test::ScopedRegistry registry(config);

    // Should not throw, and should not disturb the main logger.
    LogSystem::Remove("never_existed");
    ASSERT_EQ(LogSystem::Main().GetName(), "registry_remove_missing_test");
}

TEST(LogSystemTest, SetMainPromotesANamedLoggerAndRemovesItFromTheNamedMap)
{
    LogSystemConfig config;
    config.Name = "registry_setmain_test";
    config.LogFile = "test_registry_setmain.log";
    vigil::test::ScopedRegistry registry(config);

    LogSystem::Create("promoted");
    LogSystem::SetMain("promoted");

    ASSERT_EQ(LogSystem::Main().GetName(), "promoted");
    // Promotion removes the logger from the named registry.
    ASSERT_EQ(LogSystem::Find("promoted"), nullptr);
}

TEST(LogSystemTest, SetMainIsANoOpForANameThatWasNeverRegistered)
{
    LogSystemConfig config;
    config.Name = "registry_setmain_missing_test";
    config.LogFile = "test_registry_setmain_missing.log";
    vigil::test::ScopedRegistry registry(config);

    LogSystem::SetMain("never_existed");
    ASSERT_EQ(LogSystem::Main().GetName(), "registry_setmain_missing_test");
}

TEST(LogSystemTest, SetGlobalLevelAppliesToTheMainLoggerAndEveryNamedLogger)
{
    LogSystemConfig config;
    config.Name = "registry_global_level_test";
    config.LogFile = "test_registry_global_level.log";
    vigil::test::ScopedRegistry registry(config);

    auto& named = LogSystem::Create("named");

    LogSystem::SetGlobalLevel(LogLevel::Error);

    ASSERT_EQ(LogSystem::Main().GetLevel(), LogLevel::Error);
    ASSERT_EQ(named.GetLevel(), LogLevel::Error);
}

TEST(LogSystemTest, FlushAllFlushesTheMainLoggerAndEveryNamedLogger)
{
    LogSystemConfig config;
    config.Name = "registry_flush_all_test";
    config.LogFile = "test_registry_flush_all.log";
    vigil::test::ScopedRegistry registry(config);

    auto& named = LogSystem::Create("named");
    auto mainSink = vigil::test::AttachTestSink(LogSystem::Main());
    auto namedSink = vigil::test::AttachTestSink(named);

    LogSystem::FlushAll();

    ASSERT_EQ(mainSink->flush_counter(), 1u);
    ASSERT_EQ(namedSink->flush_counter(), 1u);
}

TEST(LogSystemTest, FlushByNameOnlyFlushesTheRequestedNamedLogger)
{
    LogSystemConfig config;
    config.Name = "registry_flush_named_test";
    config.LogFile = "test_registry_flush_named.log";
    vigil::test::ScopedRegistry registry(config);

    auto& named = LogSystem::Create("named");
    auto mainSink = vigil::test::AttachTestSink(LogSystem::Main());
    auto namedSink = vigil::test::AttachTestSink(named);

    LogSystem::Flush("named");

    ASSERT_EQ(namedSink->flush_counter(), 1u);
    ASSERT_EQ(mainSink->flush_counter(), 0u);
}
