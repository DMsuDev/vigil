// -----------------------------------------------------------------------------
//  Copyright (c) 2026 @DMsuDev. Licensed under the MIT License.
//  See LICENSE file in the project root for full license text.
// -----------------------------------------------------------------------------

#include "vigil/logging/logger_registry.h"

#include "logging/detail/logger_impl.h"
#include "logging/detail/spd_convert.h"

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/async.h>

#include <unordered_map>
#include <mutex>
#include <stdexcept>
#include <assert.h>

namespace vigil {

//==============================================================================
// Helpers
//==============================================================================

namespace {

/// @brief Appends ".log" to @p stem unless it already ends with that extension.
std::string MakeLogFileName(std::string_view stem)
{
    constexpr std::string_view kExt = ".log";
    if (stem.size() >= kExt.size() &&
        stem.substr(stem.size() - kExt.size()) == kExt)
        return std::string{stem};
    return std::string{stem} + std::string{kExt};
}

} // namespace

//==============================================================================
// Logger registry state
//==============================================================================

namespace {

// Guards every access to the logger registry (main logger, named loggers and
// initialization state).
//
// Recursive mutex is intentionally used because the engine crash handler may
// re-enter the logging subsystem on the same thread while a previous logging
// operation is still holding this lock. Using std::mutex would deadlock in that
// scenario.
std::recursive_mutex g_Mutex;
bool g_Initialized = false;
bool g_Async       = false;

Shared<Logger> g_MainLogger;
std::unordered_map<std::string, Shared<Logger>> g_NamedLoggers;

// The file sink created during Init() is kept alive here so that named loggers
// can share it (i.e. all output goes to the same log file by default).
spdlog::sink_ptr g_SharedFileSink;

spdlog::sink_ptr g_ConsoleSink;

static bool IsInitializedUnsafe() noexcept { return g_Initialized; }

// Guards against use-after-forgetting-to-call-Init(): asserts alone are
// compiled out under NDEBUG and would otherwise let callers dereference a
// null main logger or a null shared sink further down the call chain.
void EnsureInitialized(const char* function)
{
    if (!g_Initialized)
        throw std::logic_error(std::string("[Vigil] LoggerRegistry::") + function + "() called before Init().");
}

/// @brief Creates a console sink filtering at @p level.
std::shared_ptr<spdlog::sinks::stdout_color_sink_mt> MakeConsoleSink(LogLevel level)
{
    auto sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    sink->set_level(detail::ToSpdLevel(level));
    sink->set_pattern("%^[%T] [%n] %v%$");
    return sink;
}

/// @brief Creates a file sink (rotating or truncating) for @p filePath, filtering at @p level.
spdlog::sink_ptr MakeFileSink(const std::string& filePath, FileOpenMode fileMode, LogLevel level)
{
    spdlog::sink_ptr sink;
    if (fileMode == FileOpenMode::Truncate) {
        sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(filePath, /*truncate=*/true);
    } else {
        // 10 MiB per file, keep up to 5 rotating files.
        sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            filePath, 1024 * 1024 * 10, 5);
    }
    sink->set_level(detail::ToSpdLevel(level));
    sink->set_pattern("[%T] [%l] %n: %v");
    return sink;
}

/// @brief Builds an spdlog logger (sync or async) from the provided sinks.
std::shared_ptr<spdlog::logger> MakeSpdLogger(
    const std::string_view name,
    std::vector<spdlog::sink_ptr> sinks)
{
    std::shared_ptr<spdlog::logger> logger;

    if (g_Async) {
        logger = std::make_shared<spdlog::async_logger>(
            name.data(),
            sinks.begin(), sinks.end(),
            spdlog::thread_pool(),
            spdlog::async_overflow_policy::overrun_oldest);
    } else {
        logger = std::make_shared<spdlog::logger>(
            name.data(),
            sinks.begin(), sinks.end());
    }

    logger->set_level(spdlog::level::trace);
    logger->flush_on(spdlog::level::warn);
    spdlog::register_logger(logger);

    return logger;
}

/// @brief Creates a LoggerImpl backed by a fresh console sink and @p fileSink.
///        If @p fileSink is null a console-only logger is created.
Shared<detail::LoggerImpl> CreateLoggerImpl(
    const std::string_view name,
    spdlog::sink_ptr       consoleSink,
    spdlog::sink_ptr       fileSink)
{
    assert(consoleSink && "CreateLoggerImpl: Console sink must not be null");

    std::vector<spdlog::sink_ptr> sinks;
    sinks.reserve(fileSink ? 2 : 1);
    sinks.push_back(consoleSink);
    if (fileSink)
        sinks.push_back(fileSink);

    auto spdLogger = MakeSpdLogger(name, std::move(sinks));

    return std::make_shared<detail::LoggerImpl>(
        std::move(spdLogger),
        std::move(consoleSink),
        std::move(fileSink));   // nullptr if this logger shares the main file sink
}

} // namespace

//==============================================================================
// Logger facade
//==============================================================================

void LoggerRegistry::Init(const LogSystemConfig& config)
{
    std::scoped_lock lock(g_Mutex);

    if (g_Initialized)
        return;

    try {
        g_Async = config.Async;
        if (g_Async)
            spdlog::init_thread_pool(config.AsyncQueueSize, 1);

        LogLevel consoleLevel;

        if (config.ConsoleLevel.has_value()) {
            consoleLevel = config.ConsoleLevel.value();
        }
        else {
        #if defined(VIGIL_BUILD_DEBUG)
            consoleLevel = LogLevel::Trace;
        #else
            consoleLevel = LogLevel::Info;
        #endif
        }

        g_ConsoleSink = MakeConsoleSink(consoleLevel);

        g_SharedFileSink = MakeFileSink(
            MakeLogFileName(config.LogFile.empty() ? config.Name : config.LogFile),
            FileOpenMode::Append,
            LogLevel::Trace);

        auto impl = CreateLoggerImpl(config.Name, g_ConsoleSink, g_SharedFileSink);
        g_MainLogger = std::make_shared<Logger>(impl);
        spdlog::set_default_logger(impl->m_Logger);

        if (g_Async)
            spdlog::flush_every(std::chrono::seconds(1));

        g_Initialized = true;

    } catch (const spdlog::spdlog_ex& ex) {
        std::fprintf(stderr, "[Vigil] Log init failed: %s\n", ex.what());
    }
}

Logger& LoggerRegistry::Create(std::string_view name)
{
    std::scoped_lock lock(g_Mutex);
    EnsureInitialized("Create");

    std::string key{name};
    if (auto it = g_NamedLoggers.find(key); it != g_NamedLoggers.end())
        return *it->second;

    auto impl   = CreateLoggerImpl(key, g_ConsoleSink, g_SharedFileSink);
    auto handle = std::make_shared<Logger>(impl);

    auto [it, _] = g_NamedLoggers.emplace(std::move(key), std::move(handle));
    return *it->second;
}

Logger& LoggerRegistry::Create(const LogConfig& config)
{
    std::scoped_lock lock(g_Mutex);
    EnsureInitialized("Create");

    std::string key{config.Name};
    if (auto it = g_NamedLoggers.find(key); it != g_NamedLoggers.end())
        return *it->second;

    spdlog::sink_ptr fileSink = g_SharedFileSink;
    if (!config.LogFile.empty()) {
        LogLevel fileLevel = config.FileLevel.value_or(LogLevel::Trace);
        fileSink = MakeFileSink(MakeLogFileName(config.LogFile), config.FileMode, fileLevel);
    }

    auto impl     = CreateLoggerImpl(key, g_ConsoleSink, fileSink);
    auto handle   = std::make_shared<Logger>(impl);

    auto [it, _] = g_NamedLoggers.emplace(std::move(key), std::move(handle));
    return *it->second;
}

void LoggerRegistry::Shutdown()
{
    std::scoped_lock lock(g_Mutex);
    if (!g_Initialized) return;

    spdlog::shutdown();
    g_MainLogger.reset();
    g_NamedLoggers.clear();
    g_ConsoleSink.reset();
    g_SharedFileSink.reset();
    g_Initialized = false;
}

//==============================================================================
// Logger lifecycle management
//==============================================================================

void LoggerRegistry::Remove(std::string_view name)
{
    std::scoped_lock lock(g_Mutex);
    if (!IsInitializedUnsafe()) return;

    auto it = g_NamedLoggers.find(std::string{name});
    if (it == g_NamedLoggers.end()) return;

    spdlog::drop(std::string{name});
    g_NamedLoggers.erase(it);
}

void LoggerRegistry::SetMain(std::string_view name)
{
    std::scoped_lock lock(g_Mutex);
    if (!IsInitializedUnsafe()) return;

    auto it = g_NamedLoggers.find(std::string{name});
    if (it == g_NamedLoggers.end()) return;

    g_MainLogger = std::move(it->second);
    g_NamedLoggers.erase(it);
    spdlog::set_default_logger(g_MainLogger->Impl().m_Logger);
}

//==============================================================================
// Global logger accessors
//==============================================================================

bool LoggerRegistry::IsInitialized() noexcept
{
    std::scoped_lock lock(g_Mutex);
    return g_Initialized;
}

Logger& LoggerRegistry::Main()
{
    std::scoped_lock lock(g_Mutex);
    EnsureInitialized("Main");

    return *g_MainLogger;
}

Logger& LoggerRegistry::Get(std::string_view name)
{
    std::scoped_lock lock(g_Mutex);
    EnsureInitialized("Get");

    auto it = g_NamedLoggers.find(std::string{name});
    if (it != g_NamedLoggers.end())
        return *it->second;
    throw std::runtime_error("Logger not found: " + std::string{name});
}

Logger* LoggerRegistry::Find(std::string_view name)
{
    std::scoped_lock lock(g_Mutex);
    if (!IsInitializedUnsafe()) return nullptr;

    auto it = g_NamedLoggers.find(std::string{name});
    return it != g_NamedLoggers.end() ? it->second.get() : nullptr;
}

//==============================================================================
// Global log level control
//==============================================================================

void LoggerRegistry::SetGlobalLevel(LogLevel level)
{
    std::scoped_lock lock(g_Mutex);
    if (!IsInitializedUnsafe()) return;

    g_MainLogger->SetLevel(level);
    for (auto& [name, handle] : g_NamedLoggers)
        handle->SetLevel(level);
}

void LoggerRegistry::SetGlobalFileLevel(LogLevel level)
{
    std::scoped_lock lock(g_Mutex);
    if (!IsInitializedUnsafe()) return;

    const auto spdLevel = detail::ToSpdLevel(level);

    if (g_SharedFileSink)
        g_SharedFileSink->set_level(spdLevel);

    for (auto& [_, handle] : g_NamedLoggers)
    {
        auto sink = handle->Impl().m_FileSink;

        if (sink && sink != g_SharedFileSink)
            sink->set_level(spdLevel);
    }
}

void LoggerRegistry::SetConsoleLevel(LogLevel level)
{
    std::scoped_lock lock(g_Mutex);
    if (!IsInitializedUnsafe()) return;

    g_ConsoleSink->set_level(detail::ToSpdLevel(level));
}

//==============================================================================
// Flush control
//==============================================================================

void LoggerRegistry::FlushAll()
{
    std::scoped_lock lock(g_Mutex);
    if (!IsInitializedUnsafe()) return;

    g_MainLogger->Flush();
    for (auto& [_, logger] : g_NamedLoggers)
        logger->Flush();
}

void LoggerRegistry::Flush(std::string_view name)
{
    std::scoped_lock lock(g_Mutex);
    if (auto it = g_NamedLoggers.find(std::string{name}); it != g_NamedLoggers.end())
        it->second->Flush();
}

} // namespace vigil
