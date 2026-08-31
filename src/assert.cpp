// -----------------------------------------------------------------------------
//  Copyright (c) 2026 @DMsuDev. Licensed under the MIT License.
//  See LICENSE file in the project root for full license text.
// -----------------------------------------------------------------------------

#include "vigil/assert.h"
#include "vigil/logging/log_system.h"
#include "trace/stack_trace.h"

#include "vigil/detail/symbol_utils.h"

#include <iostream>
#include <cstdlib>

namespace vigil::detail {

[[noreturn]] VIGIL_API void ReportAssertFailure(
    std::string_view exprText,
    const ::vigil::SourceLocation& loc,
    std::string_view message)
{
    // Capture and format the current stack trace
    std::string stackDump = ::vigil::StackTrace::CaptureAndFormat(1);

    // Clean the function signature for better readability
    std::string functionName = CleanFunctionSignature(loc.function_name());

    std::string normalizedFile = NormalizePathSeparators(loc.file_name());
    std::string_view fileLocation = FormatFilePath(normalizedFile, 4U);

    // Check if the logging system is initialized and safe to use
    if (::vigil::LogSystem::IsInitialized()) {
        if (message.empty()) {
            ::vigil::Log(
                ::vigil::LogLevel::Critical,
                "Assertion failed\n"
                "  Expression : {}\n"
                "  Location   : {}:{}\n"
                "  Function   : {}\n"
                "\n{}",
                exprText,
                fileLocation,
                loc.line(),
                functionName,
                stackDump);
        } else {
            ::vigil::Log(
                ::vigil::LogLevel::Critical,
                "Assertion failed\n"
                "  Expression : {}\n"
                "  Location   : {}:{}\n"
                "  Function   : {}\n"
                "  Message    : {}\n"
                "\n{}",
                exprText,
                fileLocation,
                loc.line(),
                functionName,
                message,
                stackDump);
        }
        ::vigil::LogSystem::Main().Flush();
    } else {
        // Fallback to low-level stderr if the logger isn't ready (e.g., startup/shutdown)
        std::cerr
            << "[VIGIL CRITICAL] Assertion failed\n"
            << "  Expression : " << exprText << "\n"
            << "  Location   : " << fileLocation << ":" << loc.line() << "\n"
            << "  Function   : " << functionName << "\n";

        if (!message.empty())
            std::cerr << "  Message    : " << message << "\n";

        std::cerr << "\n" << stackDump << std::endl;
    }

    // Suppress the MSVC "abort() has been called" dialog in non-debugger runs.
    // The assertion details are already logged; the dialog adds no value.
    #if defined(VIGIL_PLATFORM_WINDOWS)
        _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
    #endif

    // Break into the debugger if one is attached
    VIGIL_DEBUGBREAK_IF_ATTACHED();

    // Forcefully terminate the program to prevent execution with an invalid state
    std::abort();
}

} // namespace vigil::detail
