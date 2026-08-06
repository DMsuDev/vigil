// -----------------------------------------------------------------------------
//  Copyright (c) 2026 @DMsuDev. Licensed under the MIT License.
//  See LICENSE file in the project root for full license text.
// -----------------------------------------------------------------------------

#include "vigil/logging/logger_registry.h"

#include "test_helpers.h"

#include <gtest/gtest.h>

using namespace vigil;

TEST(LoggerRegistryTest, IsInitializedReflectsInitShutdownCalls)
{
    LoggerRegistry::Shutdown();
    ASSERT_FALSE(LoggerRegistry::IsInitialized());

    LogSystemConfig config;
    config.Name = "registry_init_test";
    config.LogFile = (vigil::test::kLogDir / "test_registry_init.log").string();
    config.ConsoleLevel = LogLevel::Off;
    LoggerRegistry::Init(config);
    ASSERT_TRUE(LoggerRegistry::IsInitialized());

    LoggerRegistry::Shutdown();
    ASSERT_FALSE(LoggerRegistry::IsInitialized());
}

TEST(LoggerRegistryTest, ASecondInitCallIsIgnoredWhileAlreadyInitialized)
{
    LogSystemConfig first;
    first.Name = "registry_reinit_first";
    first.LogFile = "test_registry_reinit_first.log";
    vigil::test::ScopedRegistry registry(first);

    LogSystemConfig second;
    second.Name = "registry_reinit_second";
    second.LogFile = "test_registry_reinit_second.log";
    LoggerRegistry::Init(second);

    // The first Init() call wins; the second is a documented no-op.
    ASSERT_EQ(LoggerRegistry::Main().GetName(), "registry_reinit_first");
}

TEST(LoggerRegistryTest, AccessingLoggersBeforeInitThrows)
{
    LoggerRegistry::Shutdown();

    ASSERT_THROW((void)LoggerRegistry::Main(), std::logic_error);
    ASSERT_THROW((void)LoggerRegistry::Create("whatever"), std::logic_error);
    ASSERT_THROW((void)LoggerRegistry::Get("whatever"), std::logic_error);

    // Find() is explicitly documented to never throw.
    ASSERT_EQ(LoggerRegistry::Find("whatever"), nullptr);
}

TEST(LoggerRegistryTest, CreateReturnsTheSameInstanceForARepeatedName)
{
    LogSystemConfig config;
    config.Name = "registry_create_test";
    config.LogFile = "test_registry_create.log";
    vigil::test::ScopedRegistry registry(config);

    auto& first = LoggerRegistry::Create("worker");
    auto& second = LoggerRegistry::Create("worker");
    ASSERT_EQ(&first, &second);
}

TEST(LoggerRegistryTest, GetThrowsForAnUnknownLoggerNameFindReturnsNullptrInstead)
{
    LogSystemConfig config;
    config.Name = "registry_get_find_test";
    config.LogFile = "test_registry_get_find.log";
    vigil::test::ScopedRegistry registry(config);

    ASSERT_THROW((void)LoggerRegistry::Get("does_not_exist"), std::runtime_error);
    ASSERT_EQ(LoggerRegistry::Find("does_not_exist"), nullptr);

    (void)LoggerRegistry::Create("known");
    ASSERT_NE(LoggerRegistry::Find("known"), nullptr);
    ASSERT_EQ(&LoggerRegistry::Get("known"), LoggerRegistry::Find("known"));
}

TEST(LoggerRegistryTest, RemoveDropsANamedLoggerFromTheRegistry)
{
    LogSystemConfig config;
    config.Name = "registry_remove_test";
    config.LogFile = "test_registry_remove.log";
    vigil::test::ScopedRegistry registry(config);

    LoggerRegistry::Create("temporary");
    ASSERT_NE(LoggerRegistry::Find("temporary"), nullptr);

    LoggerRegistry::Remove("temporary");
    ASSERT_EQ(LoggerRegistry::Find("temporary"), nullptr);
}

TEST(LoggerRegistryTest, RemoveIsANoOpForANameThatWasNeverRegistered)
{
    LogSystemConfig config;
    config.Name = "registry_remove_missing_test";
    config.LogFile = "test_registry_remove_missing.log";
    vigil::test::ScopedRegistry registry(config);

    // Should not throw, and should not disturb the main logger.
    LoggerRegistry::Remove("never_existed");
    ASSERT_EQ(LoggerRegistry::Main().GetName(), "registry_remove_missing_test");
}

TEST(LoggerRegistryTest, SetMainPromotesANamedLoggerAndRemovesItFromTheNamedMap)
{
    LogSystemConfig config;
    config.Name = "registry_setmain_test";
    config.LogFile = "test_registry_setmain.log";
    vigil::test::ScopedRegistry registry(config);

    LoggerRegistry::Create("promoted");
    LoggerRegistry::SetMain("promoted");

    ASSERT_EQ(LoggerRegistry::Main().GetName(), "promoted");
    // Promotion removes the logger from the named registry.
    ASSERT_EQ(LoggerRegistry::Find("promoted"), nullptr);
}

TEST(LoggerRegistryTest, SetMainIsANoOpForANameThatWasNeverRegistered)
{
    LogSystemConfig config;
    config.Name = "registry_setmain_missing_test";
    config.LogFile = "test_registry_setmain_missing.log";
    vigil::test::ScopedRegistry registry(config);

    LoggerRegistry::SetMain("never_existed");
    ASSERT_EQ(LoggerRegistry::Main().GetName(), "registry_setmain_missing_test");
}

TEST(LoggerRegistryTest, SetGlobalLevelAppliesToTheMainLoggerAndEveryNamedLogger)
{
    LogSystemConfig config;
    config.Name = "registry_global_level_test";
    config.LogFile = "test_registry_global_level.log";
    vigil::test::ScopedRegistry registry(config);

    auto& named = LoggerRegistry::Create("named");

    LoggerRegistry::SetGlobalLevel(LogLevel::Error);

    ASSERT_EQ(LoggerRegistry::Main().GetLevel(), LogLevel::Error);
    ASSERT_EQ(named.GetLevel(), LogLevel::Error);
}

TEST(LoggerRegistryTest, FlushAllFlushesTheMainLoggerAndEveryNamedLogger)
{
    LogSystemConfig config;
    config.Name = "registry_flush_all_test";
    config.LogFile = "test_registry_flush_all.log";
    vigil::test::ScopedRegistry registry(config);

    auto& named = LoggerRegistry::Create("named");
    auto mainSink = vigil::test::AttachTestSink(LoggerRegistry::Main());
    auto namedSink = vigil::test::AttachTestSink(named);

    LoggerRegistry::FlushAll();

    ASSERT_EQ(mainSink->flush_counter(), 1u);
    ASSERT_EQ(namedSink->flush_counter(), 1u);
}

TEST(LoggerRegistryTest, FlushByNameOnlyFlushesTheRequestedNamedLogger)
{
    LogSystemConfig config;
    config.Name = "registry_flush_named_test";
    config.LogFile = "test_registry_flush_named.log";
    vigil::test::ScopedRegistry registry(config);

    auto& named = LoggerRegistry::Create("named");
    auto mainSink = vigil::test::AttachTestSink(LoggerRegistry::Main());
    auto namedSink = vigil::test::AttachTestSink(named);

    LoggerRegistry::Flush("named");

    ASSERT_EQ(namedSink->flush_counter(), 1u);
    ASSERT_EQ(mainSink->flush_counter(), 0u);
}
