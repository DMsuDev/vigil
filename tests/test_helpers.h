// -----------------------------------------------------------------------------
//  Copyright (c) 2026 @DMsuDev. Licensed under the MIT License.
//  See LICENSE file in the project root for full license text.
// -----------------------------------------------------------------------------

#pragma once

// White-box test helpers. LogSystem is a process-wide singleton (Init()
// after the first successful call is a no-op), so every test that needs its
// own configuration must force a Shutdown()/Init() cycle around itself.

#include "vigil/logging/log_system.h"
#include "logging/detail/logger_impl.h"

#include "test_sink.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <memory>
#include <string>

namespace vigil::test {

inline const std::filesystem::path kLogDir = "test_logs";

class LogDirEnvironment : public ::testing::Environment {
public:
    void SetUp() override
    {
        std::error_code ec;
        std::filesystem::remove_all(kLogDir, ec);
        std::filesystem::create_directories(kLogDir, ec);
    }
};

// Registered once at static-init time (before RUN_ALL_TESTS() runs in
// gtest_main); `kLogDir` being an inline variable keeps this a single
// definition across every test .cpp that includes this header.
inline ::testing::Environment* const g_LogDirEnvironment =
    ::testing::AddGlobalTestEnvironment(new LogDirEnvironment);

/// @brief Returns a log file path scoped to the current test case and name,
///        guaranteeing no collisions between tests even when run in parallel.
///
/// Example: "test_logs/LogSystemTest_IsInitialized.log"
inline std::filesystem::path UniqueLogPath()
{
    const auto* info = ::testing::UnitTest::GetInstance()->current_test_info();
    std::string name = info ? (std::string{info->test_suite_name()} + "_" + info->name())
                            : "unknown_test";
    return kLogDir / (name + ".log");
}

/// @brief RAII guard that provides each test with an isolated LogSystem state.
///
/// Console output is suppressed by default. Log files are always written into
/// @ref kLogDir unless @p config.LogDir is already set. If config.LogFile is
/// empty it is derived automatically from the running test name via
/// @ref UniqueLogPath(), eliminating the need for per-test manual file names.
struct ScopedRegistry {
    explicit ScopedRegistry(vigil::LogSystemConfig config = {})
    {
        if (!config.ConsoleLevel.has_value())
            config.ConsoleLevel = vigil::LogLevel::Off;

        if (config.LogDir.empty())
            config.LogDir = kLogDir.string();

        if (config.LogFile.empty())
            config.LogFile = UniqueLogPath().filename().string();

        vigil::LogSystem::Shutdown();
        vigil::LogSystem::Init(config);
    }

    ~ScopedRegistry() { vigil::LogSystem::Shutdown(); }

    ScopedRegistry(const ScopedRegistry&)            = delete;
    ScopedRegistry& operator=(const ScopedRegistry&) = delete;
};

/// @brief Attaches a fresh TestSinkMt to @p logger and returns it for inspection.
inline std::shared_ptr<TestSinkMt> AttachTestSink(vigil::Logger& logger)
{
    auto sink = std::make_shared<TestSinkMt>();
    logger.Impl().AttachSink(sink);
    return sink;
}

} // namespace vigil::test
