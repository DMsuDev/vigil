// -----------------------------------------------------------------------------
//   _    __ __ _____ ___ __
//  | |  / /  _/ ____/  _/ /
//  | | / // // / __ / // /     Logging and Diagnostics for C++
//  | |/ // // /_/ // // /___   https://github.com/DMsuDev/vigil
//  |___/___/\____/___/_____/
//
//  Copyright (c) 2026 @DMsuDev. Licensed under the MIT License.
//  See LICENSE file in the project root for full license text.
// -----------------------------------------------------------------------------

#include "vigil/vigil.h"

#include "triggers/trigger_sigsegv.h"
#include "triggers/trigger_sigabrt.h"
#include "triggers/trigger_sigfpe.h"
#include "triggers/trigger_sigill.h"
#include "triggers/trigger_stack_overflow.h"
#include "triggers/trigger_terminate.h"
#include "triggers/trigger_pure_virtual.h"
#include "triggers/trigger_bad_alloc.h"
#include "triggers/trigger_invalid_param.h"

#include <cstring>
#include <cstdio>

// ============================================================================
// Helpers
// ============================================================================

static void PrintUsage()
{
    std::fprintf(stderr,
        "Usage: vigil_crash_test <trigger>\n\n"
        "Triggers:\n"
        "  sigsegv        Null pointer dereference  (SIGSEGV)\n"
        "  sigabrt        Explicit abort()          (SIGABRT)\n"
        "  sigfpe         Integer divide by zero    (SIGFPE)\n"
        "  sigill         Illegal instruction       (SIGILL)\n"
        "  stack_overflow Unbounded recursion\n"
        "  terminate      noexcept violation        (std::terminate)\n"
        "  pure_virtual   Pure virtual call\n"
        "  bad_alloc      Exhaustive allocation     (std::bad_alloc)\n"
#if defined(VIGIL_PLATFORM_WINDOWS)
        "  invalid_param  CRT invalid parameter     (Windows only)\n"
#endif
        "\n");
}

// ============================================================================
// Entry point
// ============================================================================

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        PrintUsage();
        return 1;
    }

    // Install the crash handler before triggering any fault.
    vigil::CrashHandler::Install([](const vigil::CrashInfo& info) {
        std::fprintf(stderr, "[CrashConsumer] crash callback received\n");
    });

    const char* trigger = argv[1];

    if      (std::strcmp(trigger, "sigsegv")        == 0) vigil::crash_triggers::TriggerSigSegV();
    else if (std::strcmp(trigger, "sigabrt")        == 0) vigil::crash_triggers::TriggerSigAbrt();
    else if (std::strcmp(trigger, "sigfpe")         == 0) vigil::crash_triggers::TriggerSigFpe();
    else if (std::strcmp(trigger, "sigill")         == 0) vigil::crash_triggers::TriggerSigIll();
    else if (std::strcmp(trigger, "stack_overflow") == 0) vigil::crash_triggers::TriggerStackOverflow();
    else if (std::strcmp(trigger, "terminate")      == 0) vigil::crash_triggers::TriggerTerminate();
    else if (std::strcmp(trigger, "pure_virtual")   == 0) vigil::crash_triggers::TriggerPureVirtual();
    else if (std::strcmp(trigger, "bad_alloc")      == 0) vigil::crash_triggers::TriggerBadAlloc();
#if defined(VIGIL_PLATFORM_WINDOWS)
    else if (std::strcmp(trigger, "invalid_param")  == 0) vigil::crash_triggers::TriggerInvalidParam();
#endif
    else
    {
        std::fprintf(stderr, "Unknown trigger: '%s'\n\n", trigger);
        PrintUsage();
        return 1;
    }

    // Unreachable: every trigger path terminates the process.
    return 0;
}
