// -----------------------------------------------------------------------------
//  Copyright (c) 2026 @DMsuDev. Licensed under the MIT License.
//  See LICENSE file in the project root for full license text.
// -----------------------------------------------------------------------------

#include "vigil/vigil.h"

#include <filesystem>

#include <iostream>
#include <cstring>
#include <vector>

// Demonstrates Vigil's assertion macros: VIGIL_ASSERT, VIGIL_VERIFY,
// VIGIL_ASSERT_NOT_NULL, VIGIL_ASSERT_IN_RANGE, and VIGIL_UNREACHABLE_ASSERT.
// Safe cases run by default; failure cases must be requested explicitly and
// will abort the process by design.

static void DemoBasicAssert();
static void DemoVerify();
static void DemoNullCheck();
static void DemoRangeCheck();
static void DemoUnreachable();

static void FailBasicAssert();
static void FailNullCheck();
static void FailRangeCheck();
static void FailUnreachable();

static void PrintUsage(const char* program);

int main(int argc, char* argv[])
{
    if (!vigil::EnableUTF8Console())
    {
        std::fprintf(stderr,
            "[Vigil] Failed to enable UTF-8 console support. "
            "Unicode output may not display correctly.\n");
    }

    try
    {
        vigil::LogSystemConfig config;
        config.Name         = "AssertExample";
        config.ConsoleLevel = vigil::LogLevel::Trace;
        vigil::LogSystem::Init(config);

        vigil::Info("Vigil Assertion Example  v{}.{}.{}",
            VIGIL_VERSION_MAJOR, VIGIL_VERSION_MINOR, VIGIL_VERSION_PATCH);

#if !defined(VIGIL_ENABLE_ASSERTS)
        vigil::Warn("Assertions are DISABLED in this build.");
        vigil::Warn("Define VIGIL_ENABLE_ASSERTS to activate assertion checks.");
#endif

        vigil::Info("");

        std::string app_name = std::filesystem::path(argv[0]).filename().string();

        std::vector<const char*> args(argv + 1, argv + argc);

        for (const char* arg : args)
        {
            if (std::strcmp(arg, "--help") == 0 || std::strcmp(arg, "-h") == 0)
            {
                PrintUsage(app_name.c_str());
                vigil::LogSystem::Shutdown();
                return 0;
            }
        }

        using Fn = void(*)();
        struct Entry { const char* flag; Fn fn; };

        const Entry safe[] = {
            { "--basic",       DemoBasicAssert },
            { "--verify",      DemoVerify      },
            { "--null",        DemoNullCheck   },
            { "--range",       DemoRangeCheck  },
            { "--unreachable", DemoUnreachable },
        };

        const Entry failing[] = {
            { "--fail-basic",       FailBasicAssert  },
            { "--fail-null",        FailNullCheck    },
            { "--fail-range",       FailRangeCheck   },
            { "--fail-unreachable", FailUnreachable  },
        };

        auto dispatch = [&](const char* arg) -> bool
        {
            for (const auto& e : safe)
                if (std::strcmp(arg, e.flag) == 0) { e.fn(); vigil::Info(""); return true; }
            for (const auto& e : failing)
                if (std::strcmp(arg, e.flag) == 0) { e.fn(); return true; }
            return false;
        };

        if (args.empty() || (args.size() == 1 && std::strcmp(args[0], "--all") == 0))
        {
            vigil::Info("Running all safe cases...");
            vigil::Info("");
            for (const auto& e : safe) { e.fn(); vigil::Info(""); }
            vigil::Info("All safe cases completed successfully.");
        }
        else
        {
            for (const char* arg : args)
            {
                if (!dispatch(arg))
                {
                    vigil::Error("Unknown option: '{}'.", arg);
                    vigil::Info("");
                    PrintUsage(app_name.c_str());
                    vigil::LogSystem::Shutdown();
                    return 1;
                }
            }
        }

        vigil::LogSystem::Shutdown();
    }
    catch (const std::exception& ex)
    {
        std::fprintf(stderr, "[Vigil] Example failed: %s\n", ex.what());
        return 1;
    }

    return 0;
}

// ----------------------------------------------------------------------------
// Safe cases
// ----------------------------------------------------------------------------

static void DemoBasicAssert()
{
    vigil::Info("== Basic assertions =========================================");

    VIGIL_ASSERT(true, "Unconditional pass.");

    int x = 42;
    VIGIL_ASSERT(x == 42, "Expected x == 42, got {}", x);
    VIGIL_ASSERT(x > 0);

    vigil::Info("  All basic assertions passed  (x = {}).", x);
}

static void DemoVerify()
{
    vigil::Info("== VIGIL_VERIFY =============================================");
    vigil::Info("  Unlike VIGIL_ASSERT, the expression is ALWAYS evaluated,");
    vigil::Info("  even when assertions are compiled out.");

    int counter = 0;
    auto increment = [&counter]() { ++counter; return true; };

    VIGIL_VERIFY(increment(), "Side effect must execute.");
    VIGIL_ASSERT(counter == 1, "counter should be 1 after VIGIL_VERIFY, got {}", counter);

    vigil::Info("  Side effect confirmed: counter = {}.", counter);
}

static void DemoNullCheck()
{
    vigil::Info("== Null pointer checks ======================================");

    int value   = 100;
    int* valid  = &value;
    int* null_p = nullptr;

    VIGIL_ASSERT_NOT_NULL(valid);
    VIGIL_ASSERT(*valid == 100, "Dereferenced value should be 100, got {}", *valid);

    // Confirming the null is null (passing case — no abort).
    VIGIL_ASSERT(null_p == nullptr, "Null pointer correctly identified.");

    vigil::Info("  Pointer checks passed  (value = {}).", *valid);
}

static void DemoRangeCheck()
{
    vigil::Info("== Range validation =========================================");

    int    age   = 25;
    double temp  = 22.5;
    size_t index = 5;

    VIGIL_ASSERT_IN_RANGE(age,   0,    120);
    VIGIL_ASSERT_IN_RANGE(temp, -50.0, 50.0);
    VIGIL_ASSERT_IN_RANGE(index, 0u,   9u);

    vigil::Info("  All ranges valid  (age={}, temp={:.1f}, index={}).",
        age, temp, index);
}

static void DemoUnreachable()
{
    vigil::Info("== Unreachable code detection ================================");
    vigil::Info("  VIGIL_UNREACHABLE_ASSERT() guards switch default branches.");
    vigil::Info("  Adding a new enum value without a case triggers it at runtime.");

    enum class State { Starting, Running, Stopped };
    State state = State::Running;

    switch (state)
    {
        case State::Starting: vigil::Info("  State: Starting."); break;
        case State::Running:  vigil::Info("  State: Running."); break;
        case State::Stopped:  vigil::Info("  State: Stopped."); break;
        // default: VIGIL_UNREACHABLE_ASSERT();
    }

    vigil::Info("  No unreachable paths hit for State::Running.");
}

// ----------------------------------------------------------------------------
// Intentional failure cases (abort by design)
// ----------------------------------------------------------------------------

static void FailBasicAssert()
{
    vigil::Warn("Triggering intentional failure: basic assertion.");
    int x = 42;
    VIGIL_ASSERT(x == 0, "Expected x == 0, but x = {}.", x);
}

static void FailNullCheck()
{
    vigil::Warn("Triggering intentional failure: null pointer check.");
    int* p = nullptr;
    VIGIL_ASSERT_NOT_NULL(p);
}

static void FailRangeCheck()
{
    vigil::Warn("Triggering intentional failure: range validation.");
    int value = 150;
    VIGIL_ASSERT_IN_RANGE(value, 0, 100);
}

static void FailUnreachable()
{
    vigil::Warn("Triggering intentional failure: unreachable code path.");
    VIGIL_UNREACHABLE_ASSERT();
}

// ----------------------------------------------------------------------------
// Usage
// ----------------------------------------------------------------------------

static void PrintUsage(const char* program)
{
    std::cout << "Usage: " << program << " [option]\n\n";

    std::cout << "  (none) / --all       Run all safe cases (default).\n";
    std::cout << "  --basic              Basic VIGIL_ASSERT.\n";
    std::cout << "  --verify             VIGIL_VERIFY side-effect semantics.\n";
    std::cout << "  --null               VIGIL_ASSERT_NOT_NULL.\n";
    std::cout << "  --range              VIGIL_ASSERT_IN_RANGE.\n";
    std::cout << "  --unreachable        VIGIL_UNREACHABLE_ASSERT (safe path).\n\n";

    std::cout << "Failure cases — these abort the process:\n";
    std::cout << "  --fail-basic         Failing basic assertion.\n";
    std::cout << "  --fail-null          Failing null check.\n";
    std::cout << "  --fail-range         Failing range check.\n";
    std::cout << "  --fail-unreachable   Unreachable path executed.\n";
}
