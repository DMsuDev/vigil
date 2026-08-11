// -----------------------------------------------------------------------------
//  Copyright (c) 2026 @DMsuDev. Licensed under the MIT License.
//  See LICENSE file in the project root for full license text.
// -----------------------------------------------------------------------------

#include "vigil/vigil.h"
#include "vigil/logging/scoped_logger.h"

#include <thread>
#include <chrono>

// Demonstrates ScopedLogger: automatic entry/exit trace logging with elapsed
// time for both named scopes and function instrumentation.

static void DemoScopedFunction();
static void DemoNestedScopes();
static void DemoExplicitLevel();
static void DemoEarlyReturn();

int main()
{
    if (!vigil::EnableUTF8Console())
    {
        std::fprintf(stderr,
            "[Vigil] Failed to enable UTF-8 console support. "
            "Unicode output may not display correctly.\n");
    }

#ifdef VIGIL_ENABLE_SCOPED_LOG
    try
    {
        vigil::LogSystemConfig config;
        config.Name         = "ScopedExample";
        config.ConsoleLevel = vigil::LogLevel::Trace;
        vigil::LogSystem::Init(config);

        vigil::Info("Vigil Scoped Logger Example  v{}.{}.{}",
            VIGIL_VERSION_MAJOR, VIGIL_VERSION_MINOR, VIGIL_VERSION_PATCH);


        DemoScopedFunction();
        DemoNestedScopes();
        DemoExplicitLevel();
        DemoEarlyReturn();

        vigil::LogSystem::Shutdown();
    }
    catch (const std::exception& ex)
    {
        std::fprintf(stderr, "[Vigil] Example failed: %s\n", ex.what());
        return EXIT_FAILURE;
    }

#else
    std::fprintf(stderr,
        "[Vigil] This example requires VIGIL_ENABLE_SCOPED_LOG to be defined.\n"
        "Please rebuild with -DVIGIL_ENABLE_SCOPED_LOG=ON\n");
    return EXIT_FAILURE;
#endif

    return EXIT_SUCCESS;
}

// Instruments an entire function using the compiler-provided signature.
static void DemoScopedFunction()
{
    VIGIL_SCOPED_LOG_FUNCTION();

    std::this_thread::sleep_for(std::chrono::milliseconds(120));
    vigil::Info("Doing work inside DemoScopedFunction.");
}

// Demonstrates hierarchical output from nested VIGIL_SCOPED_LOG blocks.
static void DemoNestedScopes()
{
    VIGIL_SCOPED_LOG_FUNCTION();

    {
        VIGIL_SCOPED_LOG("Loading assets");
        std::this_thread::sleep_for(std::chrono::milliseconds(40));

        {
            VIGIL_SCOPED_LOG("Parsing manifest");
            std::this_thread::sleep_for(std::chrono::milliseconds(15));
        }

        {
            VIGIL_SCOPED_LOG("Uploading textures");
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
    }

    {
        VIGIL_SCOPED_LOG("Initializing subsystems");
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
    }
}

static void DemoExplicitLevel()
{
    VIGIL_SCOPED_LOG_FUNCTION();

    {
        VIGIL_SCOPED_LOG("Critical path");
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        vigil::Warn("Inside a Warn-level scope.");
    }
}

// The destructor always fires — exit is logged even on early return.
static void DemoEarlyReturn()
{
    VIGIL_SCOPED_LOG_FUNCTION();

    vigil::Info("About to return early.");
    return;

    vigil::Info("This line is never reached.");
}
