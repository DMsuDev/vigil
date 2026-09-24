// -----------------------------------------------------------------------------
//  Copyright (c) 2026 @DMsuDev. Licensed under the MIT License.
//  See LICENSE file in the project root for full license text.
// -----------------------------------------------------------------------------

#include "vigil/crash_guard.h"

#include "trace/stack_trace.h"

#include "vigil/detail/compiler_attributes.h"
#include "vigil/detail/platform_detection.h"
#include "vigil/logging/log_system.h"

#include <atomic>
#include <cerrno>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <exception>
#include <fstream>
#include <new>
#include <string>
#include <system_error>
#include <thread>
#include <vector>

#if defined(VIGIL_PLATFORM_WINDOWS)
    #define WIN32_LEAN_AND_MEAN
    #include <Windows.h>
    #include <crtdbg.h>
#else
    #include <execinfo.h>
    #include <semaphore.h>
    #include <signal.h>
    #include <unistd.h>
#endif

namespace vigil {

namespace {

namespace fs = std::filesystem;

// =============================================================================
// Constants
// =============================================================================

namespace Constants {
    constexpr unsigned MaxFrames       = 64;
    constexpr unsigned DeadlineSeconds = 3;

#if defined(VIGIL_PLATFORM_WINDOWS)
    constexpr DWORD ThreadNameException = 0x406D1388;
    constexpr ULONG StackGuaranteeBytes = 64 * 1024;
#else
    constexpr std::size_t AltStackSize  = 64 * 1024;
#endif
}

// =============================================================================
// Global state
// =============================================================================

struct CrashState
{
    std::atomic<bool> installed{false};
    std::atomic<bool> claimed{false};   // Set by the first thread that reports a crash.

    CrashCallback          callback;
    std::vector<fs::path>  reportDirs;

    // Written by the crashing thread, read by the watchdog after it is woken.
    std::atomic<const char*> reason{"Unknown fatal error"};
    std::atomic<unsigned>    frameCount{0};
    void*                    frames[Constants::MaxFrames]{};

#if defined(VIGIL_PLATFORM_WINDOWS)
    HANDLE wake{nullptr};
#else
    sem_t  wake;
#endif
};

// Intentionally leaked: a crash during static destruction must still find it alive.
CrashState& g_State = *new CrashState;

#if !defined(VIGIL_PLATFORM_WINDOWS)
thread_local char g_AltStackBuffer[Constants::AltStackSize];
#endif

// =============================================================================
// Platform primitives
// =============================================================================

[[noreturn]] void HardExit() noexcept
{
#if defined(VIGIL_PLATFORM_WINDOWS)
    TerminateProcess(GetCurrentProcess(), EXIT_FAILURE);
    std::abort();
#else
    _exit(EXIT_FAILURE);
#endif
}

// Blocks the calling thread forever; the watchdog ends the process.
[[noreturn]] void Park() noexcept
{
    for (;;)
    {
#if defined(VIGIL_PLATFORM_WINDOWS)
        Sleep(INFINITE);
#else
        pause();
#endif
    }
}

unsigned CaptureFrames() noexcept
{
#if defined(VIGIL_PLATFORM_WINDOWS)
    return CaptureStackBackTrace(0, Constants::MaxFrames, g_State.frames, nullptr);
#else
    return static_cast<unsigned>(backtrace(g_State.frames, static_cast<int>(Constants::MaxFrames)));
#endif
}

unsigned long ProcessId() noexcept
{
#if defined(VIGIL_PLATFORM_WINDOWS)
    return GetCurrentProcessId();
#else
    return static_cast<unsigned long>(getpid());
#endif
}

void WakeWatchdog() noexcept
{
#if defined(VIGIL_PLATFORM_WINDOWS)
    if (!SetEvent(g_State.wake)) HardExit();
#else
    if (sem_post(&g_State.wake) != 0) HardExit();
#endif
}

void WaitForCrash() noexcept
{
#if defined(VIGIL_PLATFORM_WINDOWS)
    if (WaitForSingleObject(g_State.wake, INFINITE) != WAIT_OBJECT_0) HardExit();
#else
    int result;
    do { result = sem_wait(&g_State.wake); }
    while (result == -1 && errno == EINTR);

    if (result == -1) HardExit();
#endif
}

// Kills the process if the report pipeline hangs (corrupted heap, held locks, ...).
#if defined(VIGIL_PLATFORM_WINDOWS)

DWORD WINAPI DeadlineProc(LPVOID)
{
    Sleep(Constants::DeadlineSeconds * 1000);
    HardExit();
}

void ArmDeadline() noexcept
{
    HANDLE h = CreateThread(nullptr, 0, DeadlineProc, nullptr, 0, nullptr);
    if (!h) HardExit();

    SetThreadPriority(h, THREAD_PRIORITY_TIME_CRITICAL);
    CloseHandle(h);
}

#else

void ArmDeadline() noexcept
{
    struct sigaction action{};
    action.sa_handler = [](int) { HardExit(); };
    sigemptyset(&action.sa_mask);

    if (sigaction(SIGALRM, &action, nullptr) != 0) HardExit();
    alarm(Constants::DeadlineSeconds);
}

#endif

// =============================================================================
// Crash report  (watchdog thread only)
// =============================================================================

// Logs and flushes; falls back to stderr when the logger is not available.
void Emit(const std::string& message)
{
    if (LogSystem::IsInitialized())
    {
        VIGIL_LOG_CRITICAL("{}", message);
        LogSystem::FlushAll();
    }
    else
    {
        std::fprintf(stderr, "%s\n", message.c_str());
    }
}

std::string CaptureTrace()
{
    // This acquire load pairs with the release store in SignalCrash(), so the
    // frames and the reason are fully visible after it.
    const unsigned count = g_State.frameCount.load(std::memory_order_acquire);
    if (count == 0)
        return {};

    try {
        return StackTrace::Format(StackTrace::CaptureFromAddresses(g_State.frames, count));
    } catch (...) {
        return "[vigil] Stack trace symbolication failed.";
    }
}

std::string FormatUtc(const char* format)
{
    const std::time_t now = std::time(nullptr);
    std::tm           utc{};

#if defined(VIGIL_PLATFORM_WINDOWS)
    gmtime_s(&utc, &now);
#else
    gmtime_r(&now, &utc);
#endif

    char buffer[64];
    return std::strftime(buffer, sizeof(buffer), format, &utc) ? buffer : "unknown";
}

// Writes the report to the first candidate directory that accepts it.
// Returns the file path, or an empty string if every candidate failed.
std::string WriteReport(const char* reason, const std::string& trace)
{
    try
    {
        const std::string name = "crash_" + FormatUtc("%Y%m%d_%H%M%S")
                               + "_" + std::to_string(ProcessId()) + ".txt";

        for (const fs::path& dir : g_State.reportDirs)
        {
            std::error_code ec;
            fs::create_directories(dir, ec);

            const fs::path path = dir / name;
            std::ofstream  file(path);
            if (!file)
                continue;

            file << "Vigil crash report\n"
                 << "Time:    " << FormatUtc("%Y-%m-%d %H:%M:%S") << " UTC\n"
                 << "Process: " << ProcessId() << "\n\n"
                 << "Reason:\n" << reason << "\n\n"
                 << "Stack trace:\n" << (trace.empty() ? "Not available." : trace) << '\n';
            file.close();

            if (!file.fail())
                return path.string();
        }
    }
    catch (...) {}

    return {};
}

// Log first (most reliable path), then the report file, then user code (least trusted).
[[noreturn]] VIGIL_NOINLINE void Report()
{
    const std::string trace  = CaptureTrace();
    const char*       reason = g_State.reason.load(std::memory_order_relaxed);

    std::string message = std::string("Fatal error:\n") + reason;
    if (!trace.empty())
        message += "\n\n" + trace + '\n';
    Emit(message);

    const std::string path = WriteReport(reason, trace);
    Emit(path.empty() ? std::string("Crash report could not be written.")
                      : "Crash report written to " + path);

    if (g_State.callback)
    {
        const CrashInfo info{
            reason,
            trace.empty() ? nullptr : trace.c_str(),
            path.empty()  ? nullptr : path.c_str()
        };

        try { g_State.callback(info); } catch (...) {}
    }

    std::fflush(nullptr);
    VIGIL_DEBUGBREAK_IF_ATTACHED();

    HardExit();
}

// =============================================================================
// Crash signaling and watchdog thread
// =============================================================================

// Entry point for every fatal path. Async-signal-safe: no allocation, no locks.
// Hands the crash to the watchdog thread and parks the faulting thread.
[[noreturn]] void SignalCrash(const char* reason, bool captureFrames = true) noexcept
{
    // Only the first crash is reported; the deadline timer covers a crash
    // inside the watchdog itself.
    if (g_State.claimed.exchange(true, std::memory_order_acq_rel))
        Park();

    g_State.reason.store(reason, std::memory_order_relaxed);

    // Skipped on stack overflow, where there is no room to walk the stack.
    const unsigned count = captureFrames ? CaptureFrames() : 0;
    g_State.frameCount.store(count, std::memory_order_release);

    WakeWatchdog();
    Park();
}

[[noreturn]] void WatchdogMain() noexcept
{
    WaitForCrash();
    ArmDeadline();
    Report();
}

void StartWatchdog()
{
#if defined(VIGIL_PLATFORM_WINDOWS)
    g_State.wake = CreateEventA(nullptr, /*bManualReset=*/TRUE, /*bInitialState=*/FALSE, nullptr);
    if (!g_State.wake) HardExit();
#else
    if (sem_init(&g_State.wake, /*pshared=*/0, /*value=*/0) != 0) HardExit();
#endif

    std::thread(WatchdogMain).detach();
}

// =============================================================================
// std::terminate handler  (all platforms)
// =============================================================================

[[noreturn]] void OnTerminate()
{
    std::string reason = "std::terminate() called";

    if (std::exception_ptr ep = std::current_exception())
    {
        try { std::rethrow_exception(ep); }
        catch (const std::exception& e) {
            reason += "\nUnhandled exception: ";
            reason += e.what();
        }
        catch (...) {
            reason += "\nUnhandled non-standard exception.";
        }
    }
    else
    {
        reason += " (no active exception — possible noexcept violation, "
                  "pure virtual call, or explicit std::terminate()).";
    }

    SignalCrash(reason.c_str());
}

// =============================================================================
// Platform handlers
// =============================================================================

#if defined(VIGIL_PLATFORM_WINDOWS)

[[noreturn]] void OnPureCall()
{
    SignalCrash("Pure virtual function call");
}

[[noreturn]] void OnNewFailure()
{
    SignalCrash("std::bad_alloc / operator new failure");
}

[[noreturn]] void OnAbort(int)
{
    SignalCrash("SIGABRT (abort)");
}

[[noreturn]] void OnInvalidParameter(
    const wchar_t* expression,
    const wchar_t* function,
    const wchar_t* file,
    unsigned int   line,
    uintptr_t      /*reserved*/)
{
    auto narrow = [](const wchar_t* w) -> std::string {
        if (!w) return {};
        int n = WideCharToMultiByte(CP_UTF8, 0, w, -1, nullptr, 0, nullptr, nullptr);
        if (n <= 1) return {};
        std::string s(static_cast<std::size_t>(n - 1), '\0');
        WideCharToMultiByte(CP_UTF8, 0, w, -1, s.data(), n, nullptr, nullptr);
        return s;
    };

    std::string reason = "CRT invalid parameter";
    if (function)   { reason += "\nFunction: ";   reason += narrow(function);   }
    if (expression) { reason += "\nExpression: "; reason += narrow(expression); }
    if (file)       { reason += "\nFile: ";       reason += narrow(file) + ':' + std::to_string(line); }

    SignalCrash(reason.c_str());
}

const char* DescribeException(DWORD code) noexcept
{
    switch (code) {
        case EXCEPTION_ACCESS_VIOLATION:      return "Access violation";
        case EXCEPTION_ILLEGAL_INSTRUCTION:   return "Illegal instruction";
        case EXCEPTION_INT_DIVIDE_BY_ZERO:    return "Integer divide by zero";
        case EXCEPTION_INT_OVERFLOW:          return "Integer overflow";
        case EXCEPTION_FLT_DIVIDE_BY_ZERO:    return "Float divide by zero";
        case EXCEPTION_FLT_OVERFLOW:          return "Float overflow";
        case EXCEPTION_FLT_UNDERFLOW:         return "Float underflow";
        case EXCEPTION_STACK_OVERFLOW:        return "Stack overflow";
        case EXCEPTION_ARRAY_BOUNDS_EXCEEDED: return "Array bounds exceeded";
        case EXCEPTION_DATATYPE_MISALIGNMENT: return "Datatype misalignment";
        case EXCEPTION_BREAKPOINT:            return "Breakpoint";
        default:                              return "Unknown structured exception (SEH)";
    }
}

LONG WINAPI OnUnhandledException(EXCEPTION_POINTERS* info)
{
    if (!info || !info->ExceptionRecord)
        return EXCEPTION_CONTINUE_SEARCH;

    const DWORD code = info->ExceptionRecord->ExceptionCode;

    if (code == Constants::ThreadNameException)
        return EXCEPTION_CONTINUE_SEARCH;

    // Returning EXCEPTION_EXECUTE_HANDLER here would let the OS kill the process
    // before the watchdog finishes, so every path parks the thread instead.
    if (code == EXCEPTION_STACK_OVERFLOW)
        SignalCrash("Stack overflow (EXCEPTION_STACK_OVERFLOW)", /*captureFrames=*/false);

    SignalCrash(DescribeException(code));
}

void InstallPlatformHandlers()
{
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
    _CrtSetReportMode(_CRT_ASSERT, 0);
    _CrtSetReportMode(_CRT_ERROR,  0);

    SetUnhandledExceptionFilter(OnUnhandledException);
    _set_purecall_handler(&OnPureCall);
    _set_invalid_parameter_handler(&OnInvalidParameter);
    std::set_new_handler(&OnNewFailure);

    // SEH already covers access violations, FPU faults and illegal instructions.
    signal(SIGABRT, &OnAbort);
}

#else

const char* DescribeSignal(int signum) noexcept
{
    switch (signum) {
        case SIGSEGV: return "Segmentation fault (SIGSEGV)";
        case SIGABRT: return "Abort signal (SIGABRT)";
        case SIGFPE:  return "Floating-point exception (SIGFPE)";
        case SIGILL:  return "Illegal instruction (SIGILL)";
        case SIGBUS:  return "Bus error (SIGBUS)";
        default:      return "Unknown fatal signal";
    }
}

// Warning: Only async-signal-safe calls permitted here.
[[noreturn]] void OnFatalSignal(int signum)
{
    SignalCrash(DescribeSignal(signum));
}

void InstallPlatformHandlers()
{
    // The first backtrace() call may allocate (it loads libgcc); prime it here
    // so it never happens inside a signal handler.
    void* probe = nullptr;
    backtrace(&probe, 1);

    struct sigaction action{};
    action.sa_handler = OnFatalSignal;
    action.sa_flags   = SA_ONSTACK | SA_RESTART;
    sigemptyset(&action.sa_mask);

    for (int sig : { SIGSEGV, SIGABRT, SIGFPE, SIGILL, SIGBUS })
    {
        if (sigaction(sig, &action, nullptr) != 0)
            HardExit();
    }
}

#endif

std::vector<fs::path> ResolveReportDirs(const fs::path& preferred)
{
    std::vector<fs::path> dirs;
    std::error_code       ec;

    // Resolved now, not at crash time, so a later chdir() cannot move the reports.
    if (!preferred.empty())
    {
        fs::path absolute = fs::absolute(preferred, ec);
        if (!absolute.empty()) dirs.push_back(std::move(absolute));
    }

    fs::path temp = fs::temp_directory_path(ec);
    if (!temp.empty()) dirs.push_back(std::move(temp));

    return dirs;
}

} // namespace

// =============================================================================
// CrashHandler public API
// =============================================================================

void CrashHandler::Install(CrashCallback callback, const fs::path& reportDirectory)
{
    if (g_State.installed.exchange(true, std::memory_order_acq_rel))
        return;

    g_State.callback   = std::move(callback);
    g_State.reportDirs = ResolveReportDirs(reportDirectory);

    // The watchdog and the alt stack must exist before any handler is armed.
    StartWatchdog();
    InstallThreadAltStack();

    std::set_terminate(&OnTerminate);
    InstallPlatformHandlers();
}

void CrashHandler::InstallThreadAltStack() noexcept
{
    if (!g_State.installed.load(std::memory_order_acquire))
        return;

#if defined(VIGIL_PLATFORM_WINDOWS)
    ULONG bytes = Constants::StackGuaranteeBytes;
    SetThreadStackGuarantee(&bytes);
#else
    stack_t stack{};
    stack.ss_sp   = g_AltStackBuffer;
    stack.ss_size = sizeof(g_AltStackBuffer);

    if (sigaltstack(&stack, nullptr) != 0)
        HardExit();
#endif
}

bool CrashHandler::IsInstalled() noexcept
{
    return g_State.installed.load(std::memory_order_acquire);
}

} // namespace vigil
