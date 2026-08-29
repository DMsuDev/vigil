// -----------------------------------------------------------------------------
//  Copyright (c) 2026 @DMsuDev. Licensed under the MIT License.
//  See LICENSE file in the project root for full license text.
// -----------------------------------------------------------------------------

#include "vigil/vigil.h"

#include <iostream>
#include <thread>
#include <chrono>

// Demonstrates Vigil's logging API: initialization, formatting, runtime level
// control, named loggers, rate limiting, lifecycle hooks, and flush control.

// ============================================================================
// Helpers
// ============================================================================

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

// ============================================================================
// Example Initialization Macro
// ============================================================================

#define VIGIL_EXAMPLE_INIT                     \
    vigil::LogSystem::Init({                   \
        .Name         = "Example",             \
        .LogDir       = "logs/named",          \
        .ConsoleLevel = vigil::LogLevel::Trace \
    });

// ============================================================================
// Demos for various logging features
// ============================================================================

static void Demo_BasicLogging();
static void Demo_NamedLoggers();
static void Demo_RateLimiting();
static void Demo_LevelControl();
static void Demo_FlushControl();
static void Demo_LifecycleHooks();
static void Demo_LoggerLifecycle();

// ============================================================================
// Entry point
// ============================================================================

int main()
{
    if (!vigil::EnableUTF8Console())
    {
        std::fprintf(stderr,
            "[Vigil] Failed to enable UTF-8 console support. "
            "Unicode output may not display correctly.\n");
    }

    Demo_BasicLogging();
    Demo_NamedLoggers();
    Demo_RateLimiting();
    Demo_LevelControl();
    Demo_FlushControl();
    Demo_LifecycleHooks();
    Demo_LoggerLifecycle();

    return 0;
}

// ============================================================================
// Demo: Basic Logging
// ============================================================================

static void Demo_BasicLogging()
{
    std::cout << "\n========== Demo Basic Logging ==========\n";

    VIGIL_EXAMPLE_INIT;

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

    // Check whether a log level is enabled before paying the formatting cost.
    if (vigil::IsLevelActive(vigil::LogLevel::Debug))
        vigil::Debug("This level survives compile-time filtering.");

    vigil::LogSystem::Shutdown();
}

// ============================================================================
// Demo: Named Loggers
// ============================================================================

static void Demo_NamedLoggers()
{
    std::cout << "\n========== Demo Named Loggers ==========\n";

    VIGIL_EXAMPLE_INIT;

    // Simple named logger sharing the main file sink.
    auto& net = vigil::LogSystem::Create("Network");
    net.Info("Connected to server.");

    // Using the named logger through the macro.
    VIGIL_LOG_NAMED("Network", vigil::LogLevel::Warn, "Connection retry attempt #{}", 1);

    // Named logger with a dedicated file sink and custom file level.
    vigil::LogSystem::Create({
        .Name      = "Physics",
        .LogFile   = "physics.log",
        .FileMode  = vigil::FileOpenMode::Truncate,
        .FileLevel = vigil::LogLevel::Debug,
    });

    vigil::LogSystem::Get("Physics").Debug("Simulation step complete.");

    // Find() returns nullptr instead of throwing when a logger does not exist.
    if (auto* audio = vigil::LogSystem::Find("Audio"))
        audio->Info("Audio subsystem ready.");
    else
        VIGIL_WARN("No 'Audio' logger registered yet.");

    vigil::LogSystem::Shutdown();
}

// ============================================================================
// Demo: Rate Limiting
// ============================================================================

static void Demo_RateLimiting()
{
    std::cout << "\n========== Demo Rate Limiting ==========\n";

    VIGIL_EXAMPLE_INIT;

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

    vigil::LogSystem::Shutdown();
}

// ============================================================================
// Demo: Level Control
// ============================================================================

static void Demo_LevelControl()
{
    std::cout << "\n========== Demo Level Control ==========\n";

    VIGIL_EXAMPLE_INIT;

    auto& logger = vigil::LogSystem::Main();
    vigil::Info("Current level: {}", LevelName(logger.GetLevel()));

    // Per-logger level: raise the bar so Debug is filtered, then restore.
    logger.SetLevel(vigil::LogLevel::Info);
    VIGIL_DEBUG("This message should not be displayed!");
    logger.SetLevel(vigil::LogLevel::Trace);
    VIGIL_DEBUG("This message should be displayed.");

    // SetGlobalLevel() applies the same change to every registered logger at once.
    vigil::LogSystem::SetGlobalLevel(vigil::LogLevel::Info);
    VIGIL_DEBUG("This message should not be displayed either!");
    vigil::LogSystem::SetGlobalLevel(vigil::LogLevel::Trace);
    VIGIL_DEBUG("This message should be displayed again.");

    vigil::LogSystem::Shutdown();
}

// ============================================================================
// Demo: Flush Control
// ============================================================================

static void Demo_FlushControl()
{
    std::cout << "\n========== Demo Flush Control ==========\n";

    VIGIL_EXAMPLE_INIT;

    auto& net = vigil::LogSystem::Create("Network");
    net.Info("Network logger ready.");

    // Per-sink level filtering: file sink raised to Warn, console to Info.
    vigil::LogSystem::SetGlobalFileLevel(vigil::LogLevel::Warn);
    vigil::LogSystem::SetConsoleLevel(vigil::LogLevel::Info);
    VIGIL_TRACE("Dropped by both sinks.");
    VIGIL_INFO("Shown on the console; dropped by the file sink.");
    VIGIL_WARN("Shown on console and written to file.");

    // Restore levels and explicitly flush individual and all loggers.
    vigil::LogSystem::SetGlobalLevel(vigil::LogLevel::Trace);
    vigil::LogSystem::Flush("Network");
    vigil::LogSystem::FlushAll();

    vigil::LogSystem::Shutdown();
}

// ============================================================================
// Demo: Lifecycle Hooks
// ============================================================================

static void Demo_LifecycleHooks()
{
    std::cout << "\n========== Demo Lifecycle Hooks ==========\n";

    VIGIL_EXAMPLE_INIT;

    // Register hooks to observe logging events without modifying the pipeline.
    vigil::LogSystem::SetHooks({
        .OnMessage = [](const vigil::LogMessageEvent& e)
        {
            std::cout << "[Hook] Message captured at level "
                      << LevelName(e.Level) << ": " << e.Message << "\n";
        },
        .OnLevelChange = [](const vigil::LevelChangeEvent& e)
        {
            std::cout << "[Hook] Logger '" << e.LoggerName << "' level changed from "
                      << LevelName(e.OldLevel) << " to " << LevelName(e.NewLevel) << ".\n";
        },
        .OnFlush = [](const vigil::FlushEvent& e)
        {
            std::cout << "[Hook] Flush triggered on logger '" << e.LoggerName << "'.\n";
        },
    });

    vigil::Info("This message will be intercepted by the OnMessage hook.");

    vigil::LogSystem::Main().SetLevel(vigil::LogLevel::Warn);
    vigil::LogSystem::Main().SetLevel(vigil::LogLevel::Trace);

    vigil::LogSystem::Create("AnotherLogger");

    vigil::LogSystem::FlushAll();

    // Hooks can be cleared at any point without affecting the logger state.
    vigil::LogSystem::ClearHooks();
    vigil::Info("This message is no longer intercepted.");

    vigil::LogSystem::Shutdown();
}

// ============================================================================
// Demo: Logger Lifecycle
// ============================================================================

static void Demo_LoggerLifecycle()
{
    std::cout << "\n========== Demo Logger Lifecycle ==========\n";

    VIGIL_EXAMPLE_INIT;

    auto& tmp = vigil::LogSystem::Create("Temporary");
    tmp.Info("Created a temporary logger.");

    // Promote a named logger to replace Main().
    vigil::LogSystem::SetMain("Temporary");
    VIGIL_INFO("This now goes through the promoted 'Temporary' logger.");

    // Remove loggers whose sinks are no longer needed.
    // Find() returning nullptr confirms removal was effective.
    vigil::LogSystem::Remove("Temporary");
    vigil::LogSystem::Remove("Audio"); // no-op: never created in this demo

    if (!vigil::LogSystem::Find("Temporary"))
        std::cout << "[OK] 'Temporary' logger was removed successfully.\n";

    vigil::LogSystem::Shutdown();
}
