// -----------------------------------------------------------------------------
//  Copyright (c) 2026 @DMsuDev. Licensed under the MIT License.
//  See LICENSE file in the project root for full license text.
// -----------------------------------------------------------------------------

#include "vigil/vigil.h"

// ScopedLogger is included transitively via "vigil/vigil.h".
// #include "vigil/logging/scoped_logger.h"

#include <iostream>
#include <thread>
#include <chrono>

// Demonstrates ScopedLogger: automatic entry/exit trace logging with elapsed
// time for both named scopes and function instrumentation.

// ============================================================================
// Demos
// ============================================================================

static void Demo_FunctionScope();
static void Demo_NestedScopes();
static void Demo_ExplicitLevel();
static void Demo_EarlyReturn();
static void Demo_ManualScope();

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

#ifndef VIGIL_ENABLE_SCOPED_LOG
    std::fprintf(stderr,
        "[Vigil] This example requires VIGIL_ENABLE_SCOPED_LOG to be defined.\n"
        "        Please rebuild with -DVIGIL_ENABLE_SCOPED_LOG=ON.\n");
    return EXIT_FAILURE;
#endif

    Demo_FunctionScope();
    Demo_NestedScopes();
    Demo_ExplicitLevel();
    Demo_EarlyReturn();
    Demo_ManualScope();

    return EXIT_SUCCESS;
}

// ============================================================================
// Demo: Function Scope
// ============================================================================

// Instruments an entire function using the compiler-provided signature.
// VIGIL_SCOPED_LOG_FUNCTION() captures and cleans the full function signature,
// emitting entry and exit messages with elapsed time automatically.
static void Demo_FunctionScope()
{
    std::cout << "\n========== Demo: Function Scope ==========\n";

    vigil::LogSystem::Init({
        .Name         = "ScopedExample",
        .ConsoleLevel = vigil::LogLevel::Trace
    });

    {
        VIGIL_SCOPED_LOG_FUNCTION();

        vigil::Info("Vigil Scoped Logger Example v{}.{}.{}",
            VIGIL_VERSION_MAJOR, VIGIL_VERSION_MINOR, VIGIL_VERSION_PATCH);

        std::this_thread::sleep_for(std::chrono::milliseconds(120));
        vigil::Info("Doing work inside Demo_FunctionScope.");
    }

    vigil::LogSystem::Shutdown();
}

// ============================================================================
// Demo: Nested Scopes
// ============================================================================

// Demonstrates hierarchical output from nested VIGIL_SCOPED_LOG blocks.
// Each inner scope reports its own elapsed time, letting you pinpoint
// which sub-step dominates the total duration.
static void Demo_NestedScopes()
{
    std::cout << "\n========== Demo: Nested Scopes ==========\n";

    vigil::LogSystem::Init({
        .Name         = "ScopedExample",
        .ConsoleLevel = vigil::LogLevel::Trace
    });

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

    vigil::LogSystem::Shutdown();
}

// ============================================================================
// Demo: Explicit Level
// ============================================================================

// Demonstrates scoped logging at non-default severity levels.
// VIGIL_SCOPED_LOG_FUNCTION_LEVEL(Warn) is useful for production builds where
// Trace is filtered out but scope boundaries on hot paths must remain visible.
static void Demo_ExplicitLevel()
{
    std::cout << "\n========== Demo: Explicit Level ==========\n";

    vigil::LogSystem::Init({
        .Name         = "ScopedExample",
        .ConsoleLevel = vigil::LogLevel::Trace
    });

    {
        VIGIL_SCOPED_LOG_FUNCTION_LEVEL(vigil::LogLevel::Warn);

        {
            VIGIL_SCOPED_LOG_LEVEL("Critical path", vigil::LogLevel::Error);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            vigil::Warn("Inside an Error-level scope.");
        }
    }

    vigil::LogSystem::Shutdown();
}

// ============================================================================
// Demo: Early Return
// ============================================================================

// Demonstrates the shutdown fallback: if the function returns before
// LogSystem::Shutdown() is reached, ScopedLogger writes the exit message
// directly to stderr so timing data is never silently lost.
static void Demo_EarlyReturn()
{
    std::cout << "\n========== Demo: Early Return ==========\n";

    vigil::LogSystem::Init({
        .Name         = "ScopedExample",
        .ConsoleLevel = vigil::LogLevel::Trace
    });

    const bool hasError = true;

    {
        VIGIL_SCOPED_LOG_FUNCTION();

        vigil::Info("Starting work...");
        std::this_thread::sleep_for(std::chrono::milliseconds(40));

        if (hasError)
        {
            vigil::Warn("Encountered early condition, exiting immediately.");
            return; // Shutdown() never runs - ScopedLogger falls back to stderr.
        }

        vigil::Info("This line is skipped due to early return.");
        std::this_thread::sleep_for(std::chrono::milliseconds(40));
    }

    vigil::LogSystem::Shutdown();
}

// ============================================================================
// Demo: Manual Scope
// ============================================================================

// Demonstrates VIGIL_SCOPE_BEGIN / VIGIL_SCOPE_END for cases where a named
// C++ block cannot cleanly express the desired scope boundary.
//
// Each BEGIN opens a real C++ block and constructs a ScopedLogger. The
// matching END closes it, triggering the destructor. Nesting is fully
// supported — __COUNTER__ ensures unique variable names per expansion.
static void Demo_ManualScope()
{
    std::cout << "\n========== Demo: Manual Scope ==========\n";

    vigil::LogSystem::Init({
        .Name         = "ScopedExample",
        .ConsoleLevel = vigil::LogLevel::Trace
    });

    VIGIL_SCOPE_BEGIN("Pipeline");

        vigil::Info("Starting pipeline.");

        VIGIL_SCOPE_BEGIN("Stage 1: Validation");
            vigil::Info("Validating input data.");
            std::this_thread::sleep_for(std::chrono::milliseconds(15));
        VIGIL_SCOPE_END();

        VIGIL_SCOPE_BEGIN("Stage 2: Processing");
            vigil::Info("Processing validated data.");
            std::this_thread::sleep_for(std::chrono::milliseconds(25));

            // Level override: remains visible even when Trace is filtered.
            VIGIL_SCOPE_BEGIN_LEVEL("Hot sub-step", vigil::LogLevel::Warn);
                vigil::Warn("Expensive operation running.");
                std::this_thread::sleep_for(std::chrono::milliseconds(30));
            VIGIL_SCOPE_END();

        VIGIL_SCOPE_END();

        VIGIL_SCOPE_BEGIN("Stage 3: Serialization");
            vigil::Info("Serializing results to disk.");
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        VIGIL_SCOPE_END();

        vigil::Info("Pipeline complete.");

    VIGIL_SCOPE_END();

    vigil::LogSystem::Shutdown();
}
