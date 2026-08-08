// -----------------------------------------------------------------------------
//  Copyright (c) 2026 @DMsuDev. Licensed under the MIT License.
//  See LICENSE file in the project root for full license text.
// -----------------------------------------------------------------------------

#include "vigil/vigil.h"

#include <thread>
#include <chrono>

// Demonstrates Vigil's logging API: initialization, formatting, runtime level
// control, named loggers, rate limiting, and lifecycle management.

static void DemoNamedLoggers();
static void DemoRateLimiting();
static void DemoLevelControl();
static void DemoFlushControl();
static void DemoLoggerLifecycle();

static const char* LevelName(vigil::LogLevel level)
{
    switch (level)
    {
        case vigil::LogLevel::Trace:    return "Trace";
        case vigil::LogLevel::Debug:    return "Debug";
        case vigil::LogLevel::Info:     return "Info";
        case vigil::LogLevel::Warn:     return "Warn";
        case vigil::LogLevel::Error:    return "Error";
        case vigil::LogLevel::Critical: return "Critical";
        case vigil::LogLevel::Off:      return "Off";
    }
    return "Unknown";
}

int main()
{
    if (!vigil::EnableUTF8Console())
    {
        std::fprintf(stderr,
            "[Vigil] Failed to enable UTF-8 console support. "
            "Unicode output may not display correctly.\n");
    }

    try
    {
        // Initialize the logging subsystem with a custom configuration.
        vigil::LogSystemConfig config;
        config.Name         = "Example";
        config.ConsoleLevel = vigil::LogLevel::Trace;
        vigil::LogSystem::Init(config);

        vigil::Info("Thanks for trying Vigil version {}.{}.{} !",
            VIGIL_VERSION_MAJOR, VIGIL_VERSION_MINOR, VIGIL_VERSION_PATCH);

        vigil::Warn("Easy padding in numbers like {:08d}", 42);
        vigil::Error("Support for int: {0:d};  hex: {0:x};  oct: {0:o}; bin: {0:b}", 42);
        vigil::Info("Support for floats {:03.2f}", 1.23456);
        vigil::Info("Positional args are {1} {0}..", "too", "supported");
        vigil::Info("{:>8} aligned, {:<8} aligned", "right", "left");

        // Convenience macros that forward all calls to the main logger.
        VIGIL_INFO("Application '{}' started successfully.", "Vigil Demo");
        VIGIL_WARN("Configuration file '{}' was not found. Using default settings.", "config.toml");
        VIGIL_ERROR("Failed to connect to '{}:{}'.", "127.0.0.1", 5432);

        VIGIL_INFO("Hex: 0x{:X}, Binary: {:b}", 255, 255);
        VIGIL_INFO("Pi ≈ {:.3f}", 3.1415926535);
        VIGIL_INFO("Build number: {:06}", 42);

        // Check whether a log level is enabled at compile time.
        if (vigil::IsLevelActive(vigil::LogLevel::Debug))
            vigil::Debug("This level survives compile-time filtering.");

        DemoNamedLoggers();
        DemoRateLimiting();
        DemoLevelControl();
        DemoFlushControl();
        DemoLoggerLifecycle();

        vigil::LogSystem::Shutdown();
    }
    catch (const std::exception& ex)
    {
        std::fprintf(stderr, "[Vigil] Example failed: %s\n", ex.what());
        return 1;
    }

    return 0;
}

static void DemoNamedLoggers()
{
    // Simple named logger sharing the main file sink.
    auto& net = vigil::LogSystem::Create("Network");
    net.Info("Connected to server.");
    VIGIL_LOG_NAMED("Network", vigil::LogLevel::Warn, "Connection retry attempt #{}", 1);

    // Named logger with a dedicated file sink and custom file level.
    vigil::LogConfig physicsCfg;
    physicsCfg.Name      = "Physics";
    physicsCfg.LogFile   = "physics.log";
    physicsCfg.FileMode  = vigil::FileOpenMode::Truncate;
    physicsCfg.FileLevel = vigil::LogLevel::Debug;
    vigil::LogSystem::Create(physicsCfg);
    vigil::LogSystem::Get("Physics").Debug("Simulation step complete.");

    // Find() returns nullptr instead of throwing when a logger does not exist.
    if (auto* audio = vigil::LogSystem::Find("Audio"))
        audio->Info("Audio subsystem ready.");
    else
        VIGIL_WARN("No 'Audio' logger registered yet.");
}

static void DemoRateLimiting()
{
    for (int i = 0; i < 5; ++i)
    {
        // LogOnce: only the first call with this key ever reaches the sink.
        vigil::LogOncePolicy::LogOnce("startup-warning", vigil::LogLevel::Warn,
            "This appears only once no matter how many times it's called.");

        // LogTTL: re-logs only after the TTL window (0.2 s) has elapsed.
        vigil::LogTTLPolicy::LogTTL("poll-tick", 0.2, vigil::LogLevel::Trace,
            "Polling subsystem status...");

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

static void DemoLevelControl()
{
    auto& logger = vigil::LogSystem::Main();
    vigil::Info("Current level: {}", LevelName(logger.GetLevel()));

    // Per-logger level: raise the bar, then restore.
    logger.SetLevel(vigil::LogLevel::Info);
    VIGIL_DEBUG("This message should not be displayed!");
    logger.SetLevel(vigil::LogLevel::Trace);
    VIGIL_DEBUG("This message should be displayed.");

    // SetGlobalLevel() applies the same change to every registered logger at once.
    vigil::LogSystem::SetGlobalLevel(vigil::LogLevel::Info);
    VIGIL_DEBUG("This message should not be displayed either!");
    vigil::LogSystem::SetGlobalLevel(vigil::LogLevel::Trace);
    VIGIL_DEBUG("This message should be displayed again.");
}

static void DemoFlushControl()
{
    auto& logger = vigil::LogSystem::Main();

    // Per-logger level filtering.
    logger.SetLevel(vigil::LogLevel::Debug);
    VIGIL_TRACE("Filtered out: logger level is now Debug.");
    VIGIL_DEBUG("This still gets through.");
    logger.SetLevel(vigil::LogLevel::Trace);

    // Per-sink level filtering: file sink raised to Warn, console to Info.
    vigil::LogSystem::SetGlobalFileLevel(vigil::LogLevel::Warn);
    vigil::LogSystem::SetConsoleLevel(vigil::LogLevel::Info);
    VIGIL_TRACE("Dropped by the file sink.");
    VIGIL_INFO("Shown on the console; dropped by the file sink.");

    // Restore and flush.
    vigil::LogSystem::SetGlobalLevel(vigil::LogLevel::Trace);
    vigil::LogSystem::Flush("Network");
    vigil::LogSystem::FlushAll();
}

static void DemoLoggerLifecycle()
{
    vigil::LogSystem::Create("Temporary").Info("Created a temporary logger.");

    // Promote a named logger to replace Main().
    vigil::LogSystem::SetMain("Temporary");
    VIGIL_INFO("This now goes through the promoted 'Temporary' logger.");

    // Remove loggers whose sinks are no longer needed.
    vigil::LogSystem::Remove("Physics");
    vigil::LogSystem::Remove("Audio"); // no-op: never created
}
