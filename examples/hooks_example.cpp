// -----------------------------------------------------------------------------
//  Copyright (c) 2026 @DMsuDev. Licensed under the MIT License.
//  See LICENSE file in the project root for full license text.
// -----------------------------------------------------------------------------

#include "vigil/vigil.h"

#include <iostream>
#include <vector>
#include <string>

// Demo of LogSystem hooks usage
// Shows how to register and use various hooks in the LogSystem.

// ============================================================================
// Example Initialization Macro
// ============================================================================

#define VIGIL_EXAMPLE_INIT(name)               \
    vigil::LogSystem::Init({                   \
        .Name         = name,                  \
        .LogDir       = "logs/hooks",          \
        .ConsoleLevel = vigil::LogLevel::Trace \
    });


// ============================================================================
// Helpers
// ============================================================================

static const char* LevelToString(vigil::LogLevel level)
{
    switch (level)
    {
        case vigil::LogLevel::Trace:    return "Trace";
        case vigil::LogLevel::Debug:    return "Debug";
        case vigil::LogLevel::Info:     return "Info";
        case vigil::LogLevel::Warn:     return "Warn";
        case vigil::LogLevel::Error:    return "Error";
        case vigil::LogLevel::Critical: return "Critical";
        default:                        return "Unknown";
    }
}

// Simulates an in-app console that receives log messages (e.g. a game editor).
struct AppConsole
{
    struct Entry {
        vigil::LogLevel Level;
        std::string     Message;
        std::string     LoggerName;
    };

    std::vector<Entry> Entries;

    void Push(const vigil::LogMessageEvent& e)
    {
        Entries.push_back({ e.Level, std::string(e.Message), std::string(e.LoggerName) });
        std::cout << "[AppConsole] [" << LevelToString(e.Level) << "] "
                  << "[" << e.LoggerName << "] " << e.Message << "\n";
    }

    void Print() const
    {
        std::cout << "\n--- AppConsole captured " << Entries.size() << " entries ---\n";
        for (const auto& entry : Entries)
        {
            std::cout << "  [" << LevelToString(entry.Level) << "] "
                      << entry.Message << "\n";
        }
    }
};

// ============================================================================
// Demo 1 — SetHooks (all at once)
// ============================================================================

static void Demo_SetHooks()
{
    std::cout << "\n========== Demo 1: SetHooks ==========\n";

    if (!vigil::EnableUTF8Console())
        std::cerr << "Failed to enable UTF-8 console\n";

    AppConsole console;

    VIGIL_EXAMPLE_INIT("HooksDemo");

    vigil::LogSystem::SetHooks({
        .OnMessage = [&console](const vigil::LogMessageEvent& e) {
            console.Push(e);
        },
        .OnLevelChange = [](const vigil::LevelChangeEvent& e) {
            std::cout << "[Hook] Level changed"
                      << (e.LoggerName.empty() ? " globally" : " on '" + std::string(e.LoggerName) + "'")
                      << ": " << LevelToString(e.OldLevel)
                      << " -> " << LevelToString(e.NewLevel) << "\n";
        },
        .OnFlush = [](const vigil::FlushEvent& e) {
            std::cout << "[Hook] Flush triggered"
                      << (e.LoggerName.empty() ? " (all loggers)" : " on '" + std::string(e.LoggerName) + "'")
                      << "\n";
        },
        .OnShutdown = [] {
            std::cout << "\n[Hook] LogSystem shutting down\n";
        },
    });

    // Basic logging — OnMessage should fire for each
    VIGIL_INFO("Application started");
    VIGIL_WARN("Low memory warning: {} MB remaining", 128);
    VIGIL_ERROR("Failed to load asset: {}", "texture_diffuse.png");

    // Named logger — OnMessage should still fire
    auto& net = vigil::LogSystem::Create("Network");
    net.Info("Connected to server");
    net.Warn("Packet loss detected: {}%", 12);
    VIGIL_LOG_NAMED("Network", vigil::LogLevel::Info, "Named logger message");

    // Level change — OnLevelChange should fire
    vigil::LogSystem::SetGlobalLevel(vigil::LogLevel::Warn);

    // These should be filtered out (below Warn)
    VIGIL_INFO("This should NOT appear");
    VIGIL_DEBUG("This should NOT appear either");

    // These should pass through
    VIGIL_WARN("This should appear");
    VIGIL_ERROR("This should appear too");

    // Flush — OnFlush should fire
    vigil::LogSystem::FlushAll();

    console.Print();
    vigil::LogSystem::Shutdown();
}

// ============================================================================
// Demo 2 — Individual setters
// ============================================================================

static void Demo_IndividualSetters()
{
    std::cout << "\n========== Demo 2: Individual setters ==========\n";

    VIGIL_EXAMPLE_INIT("IndividualDemo");

    // Register only what you need
    vigil::LogSystem::SetOnMessage([](const vigil::LogMessageEvent& e) {
        if (e.Level >= vigil::LogLevel::Error)
            std::cout << "[ALERT] Critical message from '" << e.LoggerName << "': " << e.Message << "\n";
    });

    VIGIL_INFO("This is info — no alert");
    VIGIL_WARN("This is a warning — no alert");
    VIGIL_ERROR("This is an error — ALERT should fire");
    VIGIL_CRITICAL("This is critical — ALERT should fire");

    // Replace OnMessage mid-run — now capture everything
    vigil::LogSystem::SetOnMessage([](const vigil::LogMessageEvent& e) {
        std::cout << "[NewCallback] " << LevelToString(e.Level) << ": " << e.Message << "\n";
    });

    VIGIL_INFO("Now captured by the new callback");
    VIGIL_WARN("This too");

    // Clear only OnMessage — other hooks remain (none registered here, but pattern is valid)
    vigil::LogSystem::SetOnMessage(nullptr);

    VIGIL_INFO("This fires no callback — silently logged to file only");

    vigil::LogSystem::Shutdown();
}

// ============================================================================
// Demo 3 — ClearHooks
// ============================================================================

static void Demo_ClearHooks()
{
    std::cout << "\n========== Demo 3: ClearHooks ==========\n";

    VIGIL_EXAMPLE_INIT("ClearDemo");

    vigil::LogSystem::SetOnMessage([](const vigil::LogMessageEvent& e) {
        std::cout << "[Before clear] " << e.Message << "\n";
    });

    VIGIL_INFO("Hook active — this should print");
    VIGIL_WARN("Hook active — this should print too");

    vigil::LogSystem::ClearHooks();

    VIGIL_INFO("Hook cleared — no callback output");
    VIGIL_ERROR("Hook cleared — no callback output");

    vigil::LogSystem::Shutdown();
}

// ============================================================================
// Demo 4 — Named logger level change hook
// ============================================================================

static void Demo_NamedLoggerLevelChange()
{
    std::cout << "\n========== Demo 4: Named logger level change ==========\n";

    VIGIL_EXAMPLE_INIT("NamedLevelDemo");

    vigil::LogSystem::SetOnLevelChange([](const vigil::LevelChangeEvent& e) {
        if (e.LoggerName.empty())
            std::cout << "[Hook] Global level: "
                      << LevelToString(e.OldLevel) << " -> " << LevelToString(e.NewLevel) << "\n";
        else
            std::cout << "[Hook] Logger '" << e.LoggerName << "' level: "
                      << LevelToString(e.OldLevel) << " -> " << LevelToString(e.NewLevel) << "\n";
    });

    vigil::LogSystem::Create("Physics");
    vigil::LogSystem::Create("Renderer");

    // Per-logger change
    vigil::LogSystem::SetLevel("Physics",  vigil::LogLevel::Debug);
    vigil::LogSystem::SetLevel("Renderer", vigil::LogLevel::Error);

    // Global change — fires once with empty LoggerName
    vigil::LogSystem::SetGlobalLevel(vigil::LogLevel::Warn);

    vigil::LogSystem::Shutdown();
}

// ============================================================================
// Entry point
// ============================================================================

int main()
{
    Demo_SetHooks();
    Demo_IndividualSetters();
    Demo_ClearHooks();
    Demo_NamedLoggerLevelChange();

    return 0;
}
