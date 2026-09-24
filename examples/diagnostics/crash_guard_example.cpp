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

#include <cstdio>
#include <iostream>
#include <thread>

[[noreturn]] inline void TriggerSigSegV()
{
    volatile int* p = nullptr;
    *p = 42;
}

int main()
{
    // CrashGuard can be installed independently of the logging system.
    vigil::CrashHandler::Install(
        [](const vigil::CrashInfo& info)
        {
            if (info.reportPath)
                std::fprintf(stderr, "Crash report: %s\n", info.reportPath);
        },
        "logs/crash_guard");

    std::cout << "CrashGuard installed.\n";

    // Vigil logging can be initialized independently.
    vigil::LogSystem::Init({
        .Name = "crash_guard_example",
        .LogDir = "logs/crash_guard",
        .ConsoleLevel = vigil::LogLevel::Trace
    });

    // Repeated installation is safe; the first installation wins.
    vigil::CrashHandler::Install();

    std::thread worker([] {
        vigil::CrashHandler::InstallThreadAltStack();
    });

    worker.join();

    // Uncomment to trigger a crash and test CrashGuard.
    // TriggerSigSegV();

    std::cout << "CrashGuard setup completed successfully.\n";
}
