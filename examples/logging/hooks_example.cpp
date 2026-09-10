// -----------------------------------------------------------------------------
//   _    __ __ _____ ___ __
//  | |  / / _/ ____/ _/ /
//  | | / // // / __ / // /     Logging and Diagnostics for C++
//  | |/ // // /_/ // // /___   https://github.com/DMsuDev/vigil
//  |___/___/\____/___/_____/
//
//  Copyright (c) 2026 @DMsuDev. Licensed under the MIT License.
//  See LICENSE file in the project root for full license text.
// -----------------------------------------------------------------------------

#include "vigil/vigil.h"

#include <iostream>
#include <vector>
#include <string>

// ============================================================================
// Example Initialization Macro
// ============================================================================

#define VIGIL_EXAMPLE_INIT(name)               \
    vigil::LogSystem::Init({                   \
        .Name         = name,                  \
        .LogDir       = "logs/hooks",          \
        .ConsoleLevel = vigil::LogLevel::Trace \
    });


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
        std::cout << "[AppConsole] [" << vigil::ToString(e.Level) << "] "
                  << "[" << e.LoggerName << "] " << e.Message << "\n";
    }

    void Print() const
    {
        std::cout << "\n--- AppConsole captured " << Entries.size() << " entries ---\n";
        for (const auto& entry : Entries)
        {
            std::cout << "  [" << vigil::ToString(entry.Level) << "] "
                      << entry.Message << "\n";
        }
    }
};

// ============================================================================
// Demos
// ============================================================================

static void Demo_SetHooks();
static void Demo_IndividualSetters();
static void Demo_ClearHooks();
static void Demo_NamedLoggerLevelChange();

// ============================================================================
// Entry point
// ============================================================================

int main()
{
    std::cout << "==========================================\n";
    std::cout << "       Vigil LogSystem Hooks Examples     \n";
    std::cout << "==========================================\n";

    Demo_SetHooks();
    Demo_IndividualSetters();
    Demo_ClearHooks();
    Demo_NamedLoggerLevelChange();

    return 0;
}

// ============================================================================
// Demo 1: SetHooks (All-in-one registration)
// ============================================================================

static void Demo_SetHooks()
{
    std::cout << "\n--- Demo 1: SetHooks (All at once) ---\n";

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
                      << ": " << vigil::ToString(e.OldLevel)
                      << " -> " << vigil::ToString(e.NewLevel) << "\n";
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
    VIGIL_LOG_INFO("Application started");
    VIGIL_LOG_WARN("Low memory warning: {} MB remaining", 128);
    VIGIL_LOG_ERROR("Failed to load asset: {}", "texture_diffuse.png");

    // Named logger — OnMessage should still fire
    auto& net = vigil::LogSystem::Create("Network");
    net.Info("Connected to server");
    net.Warn("Packet loss detected: {}%", 12);
    VIGIL_LOG_NAMED("Network", vigil::LogLevel::Info, "Named logger message");

    // Level change — OnLevelChange should fire
    vigil::LogSystem::SetGlobalLevel(vigil::LogLevel::Warn);

    // These should be filtered out (below Warn)
    VIGIL_LOG_INFO("This should NOT appear");
    VIGIL_LOG_DEBUG("This should NOT appear either");

    // These should pass through
    VIGIL_LOG_WARN("This should appear");
    VIGIL_LOG_ERROR("This should appear too");

    // Flush — OnFlush should fire
    vigil::LogSystem::FlushAll();

    console.Print();
    vigil::LogSystem::Shutdown();
}

// ============================================================================
// Demo 2: Individual Setters
// ============================================================================

static void Demo_IndividualSetters()
{
    std::cout << "\n--- Demo 2: Individual Setters ---\n";

    VIGIL_EXAMPLE_INIT("IndividualDemo");

    vigil::Info("Vigil Individual Setters Demo v{}!", VIGIL_VERSION_FULL);

    // Register only what you need
    vigil::LogSystem::SetOnMessage([](const vigil::LogMessageEvent& e) {
        if (e.Level >= vigil::LogLevel::Error)
            std::cout << "[ALERT] Critical message from '" << e.LoggerName << "': " << e.Message << "\n";
    });

    VIGIL_LOG_INFO("This is info — no alert");
    VIGIL_LOG_WARN("This is a warning — no alert");
    VIGIL_LOG_ERROR("This is an error — ALERT should fire");
    VIGIL_LOG_CRITICAL("This is critical — ALERT should fire");

    // Replace OnMessage mid-run — now capture everything
    vigil::LogSystem::SetOnMessage([](const vigil::LogMessageEvent& e) {
        std::cout << "[NewCallback] " << vigil::ToString(e.Level) << ": " << e.Message << "\n";
    });

    VIGIL_LOG_INFO("Now captured by the new callback");
    VIGIL_LOG_WARN("This too");

    // Clear only OnMessage
    vigil::LogSystem::SetOnMessage(nullptr);

    VIGIL_LOG_INFO("This fires no callback — silently logged to file only");

    vigil::LogSystem::Shutdown();
}

// ============================================================================
// Demo 3: ClearHooks
// ============================================================================

static void Demo_ClearHooks()
{
    std::cout << "\n--- Demo 3: ClearHooks ---\n";

    VIGIL_EXAMPLE_INIT("ClearDemo");

    vigil::LogSystem::SetOnMessage([](const vigil::LogMessageEvent& e) {
        std::cout << "[Before clear] " << e.Message << "\n";
    });

    VIGIL_LOG_INFO("Hook active — this should print");
    VIGIL_LOG_WARN("Hook active — this should print too");

    vigil::LogSystem::ClearHooks();

    VIGIL_LOG_INFO("Hook cleared — no callback output");
    VIGIL_LOG_ERROR("Hook cleared — no callback output");

    vigil::LogSystem::Shutdown();
}

// ============================================================================
// Demo 4: Named Logger Level Change
// ============================================================================

static void Demo_NamedLoggerLevelChange()
{
    std::cout << "\n--- Demo 4: Named Logger Level Change ---\n";

    VIGIL_EXAMPLE_INIT("NamedLevelDemo");

    vigil::LogSystem::SetOnLevelChange([](const vigil::LevelChangeEvent& e) {
        if (e.LoggerName.empty())
            std::cout << "[Hook] Global level: "
                      << vigil::ToString(e.OldLevel) << " -> " << vigil::ToString(e.NewLevel) << "\n";
        else
            std::cout << "[Hook] Logger '" << e.LoggerName << "' level: "
                      << vigil::ToString(e.OldLevel) << " -> " << vigil::ToString(e.NewLevel) << "\n";
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
