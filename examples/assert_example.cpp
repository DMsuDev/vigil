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
//
// Safe cases run by default; failure cases must be requested explicitly and
// will abort the process by design — do not combine them with other flags.


// ============================================================================
// Safe demos
// ============================================================================

static void Demo_BasicAssert();
static void Demo_Verify();
static void Demo_NullCheck();
static void Demo_RangeCheck();
static void Demo_Unreachable();

// ============================================================================
// Intentional failure cases — these abort the process
// ============================================================================

static void Fail_BasicAssert();
static void Fail_NullCheck();
static void Fail_RangeCheck();
static void Fail_Unreachable();

// ============================================================================
// Helpers
// ============================================================================

static void PrintUsage(const char* program);

// ============================================================================
// Entry point
// ============================================================================

int main(int argc, char* argv[])
{
    if (!vigil::EnableUTF8Console())
    {
        std::fprintf(stderr,
            "[Vigil] Failed to enable UTF-8 console support. "
            "Unicode output may not display correctly.\n");
    }

    vigil::LogSystem::Init({
        .Name         = "AssertExample",
        .LogDir       = "logs/asserts",
        .ConsoleLevel = vigil::LogLevel::Trace
    });

    vigil::Info("Vigil Assertion Example v{}.{}.{}",
        VIGIL_VERSION_MAJOR, VIGIL_VERSION_MINOR, VIGIL_VERSION_PATCH);

#if !defined(VIGIL_ENABLE_ASSERTS)
    vigil::Warn("Assertions are DISABLED in this build.");
    vigil::Warn("Rebuild with -DVIGIL_ENABLE_ASSERTS=ON to activate them.");
#endif

    vigil::Info("");

    const std::string appName = std::filesystem::path(argv[0]).filename().string();
    const std::vector<const char*> args(argv + 1, argv + argc);

    // -------------------------------------------------------------------------
    // Help
    // -------------------------------------------------------------------------

    if (args.size() == 1 &&
        (std::strcmp(args[0], "--help") == 0 || std::strcmp(args[0], "-h") == 0))
    {
        PrintUsage(appName.c_str());
        vigil::LogSystem::Shutdown();
        return 0;
    }

    // -------------------------------------------------------------------------
    // Dispatch table
    // -------------------------------------------------------------------------

    using Fn = void(*)();

    struct Entry
    {
        const char* flag;
        Fn          fn;
        bool        terminal; // true = aborts the process by design
    };

    const Entry entries[] = {
        { "--basic",             Demo_BasicAssert,  false },
        { "--verify",            Demo_Verify,       false },
        { "--null",              Demo_NullCheck,    false },
        { "--range",             Demo_RangeCheck,   false },
        { "--unreachable",       Demo_Unreachable,  false },
        { "--fail-basic",        Fail_BasicAssert,  true  },
        { "--fail-null",         Fail_NullCheck,    true  },
        { "--fail-range",        Fail_RangeCheck,   true  },
        { "--fail-unreachable",  Fail_Unreachable,  true  },
    };

    // -------------------------------------------------------------------------
    // Default: run all safe cases
    // -------------------------------------------------------------------------

    if (args.empty() || (args.size() == 1 && std::strcmp(args[0], "--all") == 0))
    {
        vigil::Info("Running all safe cases...");
        vigil::Info("");

        for (const auto& e : entries)
        {
            if (!e.terminal)
            {
                e.fn();
                vigil::Info("");
            }
        }

        vigil::Info("All safe cases completed successfully.");
        vigil::LogSystem::Shutdown();
        return 0;
    }

    // -------------------------------------------------------------------------
    // Explicit flags
    // -------------------------------------------------------------------------

    // Validate all flags before executing any, so the user gets a clean error
    // rather than a partial run followed by an unknown-option message.
    for (const char* arg : args)
    {
        bool known = false;
        for (const auto& e : entries)
            if (std::strcmp(arg, e.flag) == 0) { known = true; break; }

        if (!known)
        {
            vigil::Error("Unknown option: '{}'.", arg);
            vigil::Info("");
            PrintUsage(appName.c_str());
            vigil::LogSystem::Shutdown();
            return 1;
        }
    }

    // Warn if the user combined a terminal case with other flags — it will
    // abort before the rest can run.
    bool hasTerminal = false;
    for (const char* arg : args)
        for (const auto& e : entries)
            if (std::strcmp(arg, e.flag) == 0 && e.terminal) { hasTerminal = true; break; }

    if (hasTerminal && args.size() > 1)
    {
        vigil::Warn("A failure case was combined with other flags.");
        vigil::Warn("The process will abort on the first failure — subsequent flags will not run.");
        vigil::Info("");
    }

    for (const char* arg : args)
    {
        for (const auto& e : entries)
        {
            if (std::strcmp(arg, e.flag) == 0)
            {
                e.fn();
                if (!e.terminal) vigil::Info("");
                break;
            }
        }
    }

    vigil::LogSystem::Shutdown();
    return 0;
}

// ============================================================================
// Safe demos
// ============================================================================

static void Demo_BasicAssert()
{
    vigil::Info("== Basic assertions =========================================");

    VIGIL_ASSERT_MSG(true, "Unconditional pass.");

    int x = 42;
    VIGIL_ASSERT_MSG(x == 42, "Expected x == 42, got {}", x);
    VIGIL_ASSERT(x > 0);

    vigil::Info("  All basic assertions passed  (x = {}).", x);
}

static void Demo_Verify()
{
    vigil::Info("== VIGIL_VERIFY =============================================");
    vigil::Info("  Unlike VIGIL_ASSERT, the expression is ALWAYS evaluated,");
    vigil::Info("  even when assertions are compiled out.");

    int counter = 0;
    auto increment = [&counter]() { ++counter; return true; };

    VIGIL_VERIFY_MSG(increment(), "Side effect must execute.");
    VIGIL_ASSERT_MSG(counter == 1,
        "counter should be 1 after VIGIL_VERIFY, got {}", counter);

    vigil::Info("  Side effect confirmed: counter = {}.", counter);
}

static void Demo_NullCheck()
{
    vigil::Info("== Null pointer checks ======================================");

    int  value  = 100;
    int* valid  = &value;
    int* null_p = nullptr;

    VIGIL_ASSERT_NOT_NULL(valid);
    VIGIL_ASSERT_MSG(*valid == 100,
        "Dereferenced value should be 100, got {}", *valid);

    // Confirming the null is null (passing case — no abort).
    VIGIL_ASSERT_MSG(null_p == nullptr, "Null pointer correctly identified.");

    vigil::Info("  Pointer checks passed  (value = {}).", *valid);
}

static void Demo_RangeCheck()
{
    vigil::Info("== Range validation =========================================");

    int    age   = 25;
    double temp  = 22.5;
    int    index = 5;

    VIGIL_ASSERT_IN_RANGE(age,   0,    120);
    VIGIL_ASSERT_IN_RANGE(temp, -50.0, 50.0);
    VIGIL_ASSERT_IN_RANGE(index, 0,    9);

    vigil::Info("  All ranges valid  (age={}, temp={:.1f}, index={}).",
        age, temp, index);
}

static void Demo_Unreachable()
{
    vigil::Info("== Unreachable code detection ================================");
    vigil::Info("  VIGIL_UNREACHABLE_ASSERT() guards switch default branches.");
    vigil::Info("  Adding a new enum value without a case triggers it at runtime.");

    enum class State { Starting, Running, Stopped };
    const State state = State::Running;

    switch (state)
    {
        case State::Starting: vigil::Info("  State: Starting."); break;
        case State::Running:  vigil::Info("  State: Running.");  break;
        case State::Stopped:  vigil::Info("  State: Stopped.");  break;
        // default: VIGIL_UNREACHABLE_ASSERT();
    }

    vigil::Info("  No unreachable paths hit for State::Running.");
}

// ============================================================================
// Intentional failure cases (abort by design)
// ============================================================================

static void Fail_BasicAssert()
{
    vigil::Warn("Triggering intentional failure: basic assertion.");
    int x = 42;
    VIGIL_ASSERT_MSG(x == 0, "Expected x == 0, but x = {}.", x);
}

static void Fail_NullCheck()
{
    vigil::Warn("Triggering intentional failure: null pointer check.");
    int* p = nullptr;
    VIGIL_ASSERT_NOT_NULL(p);
}

static void Fail_RangeCheck()
{
    vigil::Warn("Triggering intentional failure: range validation.");
    int value = 150;
    VIGIL_ASSERT_IN_RANGE(value, 0, 100);
}

static void Fail_Unreachable()
{
    vigil::Warn("Triggering intentional failure: unreachable code path.");
    VIGIL_UNREACHABLE_ASSERT();
}

// ============================================================================
// Usage
// ============================================================================

static void PrintUsage(const char* program)
{
    std::cout << "Usage: " << program << " [option...]\n\n";

    std::cout << "Safe cases (combinable):\n";
    std::cout << "  (none) / --all       Run all safe cases (default).\n";
    std::cout << "  --basic              VIGIL_ASSERT / VIGIL_ASSERT_MSG.\n";
    std::cout << "  --verify             VIGIL_VERIFY side-effect semantics.\n";
    std::cout << "  --null               VIGIL_ASSERT_NOT_NULL.\n";
    std::cout << "  --range              VIGIL_ASSERT_IN_RANGE.\n";
    std::cout << "  --unreachable        VIGIL_UNREACHABLE_ASSERT (safe path).\n\n";

    std::cout << "Failure cases (abort the process — do not combine):\n";
    std::cout << "  --fail-basic         Failing VIGIL_ASSERT_MSG.\n";
    std::cout << "  --fail-null          Failing VIGIL_ASSERT_NOT_NULL.\n";
    std::cout << "  --fail-range         Failing VIGIL_ASSERT_IN_RANGE.\n";
    std::cout << "  --fail-unreachable   VIGIL_UNREACHABLE_ASSERT triggered.\n";
}
