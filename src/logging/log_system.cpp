// -----------------------------------------------------------------------------
//  Copyright (c) 2026 @DMsuDev. Licensed under the MIT License.
//  See LICENSE file in the project root for full license text.
// -----------------------------------------------------------------------------

#include "vigil/logging/log_system.h"

#include "logging/detail/logger_impl.h"
#include "logging/detail/spd_convert.h"
#include "logging/detail/hooks_registry.h"

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/async.h>

#include <filesystem>
#include <unordered_map>
#include <mutex>
#include <stdexcept>
#include <assert.h>

namespace vigil {

//==============================================================================
// Registry state
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

detail::HooksRegistry g_Hooks;

Shared<Logger> g_MainLogger;
std::unordered_map<std::string, Shared<Logger>> g_NamedLoggers;

// The file sink created during Init() is kept alive here so that named loggers
// can share it (i.e. all output goes to the same log file by default).
spdlog::sink_ptr g_SharedFileSink;
spdlog::sink_ptr g_ConsoleSink;

// Directory configured at Init() time. Named loggers inherit this when their
// own LogDir is empty.
std::string g_GlobalLogDir;

static bool IsInitializedUnsafe() noexcept { return g_Initialized; }

void EnsureInitialized(const char* function)
{
    if (!g_Initialized)
        throw std::logic_error(
            std::string("[Vigil] LogSystem::") + function + "() called before Init().");
}

} // namespace

//==============================================================================
// Path helpers
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

// Resolves the final log file path from a directory, an explicit file name,
// and a fallback logger name.
//
// If logFile is empty, the file name is derived as MakeLogFileName(name).
// If logDir is non-empty, it is prepended and created if it does not exist.
//
//   ("",     ""          ) -> "Name.log"
//   ("logs", ""          ) -> "logs/Name.log"
//   ("",     "custom.log") -> "custom.log"
//   ("logs", "custom.log") -> "logs/custom.log"
std::filesystem::path ResolveLogPath(
    const std::string& logDir,
    const std::string& logFile,
    const std::string& name)
{
    const std::filesystem::path file =
        logFile.empty() ? MakeLogFileName(name) : logFile;

    if (logDir.empty())
        return file;

    const std::filesystem::path dir(logDir);
    std::filesystem::create_directories(dir); // no-op if already exists
    return dir / file;
}

} // namespace

//==============================================================================
// Sink factories
//==============================================================================

namespace {

/// @brief Creates a console sink filtering at @p level.
std::shared_ptr<spdlog::sinks::stdout_color_sink_mt> MakeConsoleSink(LogLevel level)
{
    auto sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    sink->set_level(detail::ToSpdLevel(level));
    sink->set_pattern("%^[%T] [%n] %v%$");
    return sink;
}

/// @brief Creates a file sink (rotating or truncating) for @p filePath, filtering at @p level.
spdlog::sink_ptr MakeFileSink(
    const std::filesystem::path& filePath,
    FileOpenMode                 fileMode,
    LogLevel                     level)
{
    spdlog::sink_ptr sink;

    if (fileMode == FileOpenMode::Truncate)
    {
        sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
            filePath.string(), /*truncate=*/true);
    }
    else
    {
        // 10 MiB per file, keep up to 5 rotating files.
        sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            filePath.string(), 1024 * 1024 * 10, 5);
    }

    sink->set_level(detail::ToSpdLevel(level));
    sink->set_pattern("[%T] [%l] %n: %v");
    return sink;
}

/// @brief Builds an spdlog logger (sync or async) from the provided sinks.
std::shared_ptr<spdlog::logger> MakeSpdLogger(
    std::string_view              name,
    std::vector<spdlog::sink_ptr> sinks)
{
    std::shared_ptr<spdlog::logger> logger;

    if (g_Async)
    {
        logger = std::make_shared<spdlog::async_logger>(
            name.data(),
            sinks.begin(), sinks.end(),
            spdlog::thread_pool(),
            spdlog::async_overflow_policy::overrun_oldest);
    }
    else
    {
        logger = std::make_shared<spdlog::logger>(
            name.data(),
            sinks.begin(), sinks.end());
    }

    logger->set_level(spdlog::level::trace);
    logger->flush_on(spdlog::level::warn);
    spdlog::register_logger(logger);

    return logger;
}

/// @brief Creates a LoggerImpl backed by @p consoleSink and optionally @p fileSink.
Shared<detail::LoggerImpl> CreateLoggerImpl(
    std::string_view name,
    spdlog::sink_ptr consoleSink,
    spdlog::sink_ptr fileSink)
{
    assert(consoleSink && "CreateLoggerImpl: console sink must not be null");

    std::vector<spdlog::sink_ptr> sinks;
    sinks.reserve(fileSink ? 2 : 1);
    sinks.push_back(consoleSink);
    if (fileSink)
        sinks.push_back(fileSink);

    auto spdLogger = MakeSpdLogger(name, std::move(sinks));

    return std::make_shared<detail::LoggerImpl>(
        std::move(spdLogger),
        std::move(consoleSink),
        std::move(fileSink));
}

} // namespace

//==============================================================================
// Miscellaneous helpers
//==============================================================================

namespace {

/// @brief Retrieves all spdlog logger handles across main and named loggers.
auto AllSpdLoggers()
{
    std::vector<std::shared_ptr<spdlog::logger>> loggers;
    loggers.reserve(1 + g_NamedLoggers.size());

    if (g_MainLogger)
        loggers.push_back(g_MainLogger->Impl().m_Logger);

    for (auto& [_, handle] : g_NamedLoggers)
        if (handle)
            loggers.push_back(handle->Impl().m_Logger);

    return loggers;
}

} // namespace

//==============================================================================
// Init / Shutdown
//==============================================================================

void LogSystem::Init(const LogSystemConfig& config)
{
    std::scoped_lock lock(g_Mutex);

    if (g_Initialized)
        return;

    try
    {
        g_Async = config.Async;
        if (g_Async)
            spdlog::init_thread_pool(config.AsyncQueueSize, 1);

        const LogLevel consoleLevel = config.ConsoleLevel.value_or(
        #if defined(VIGIL_BUILD_DEBUG)
            LogLevel::Trace
        #else
            LogLevel::Info
        #endif
        );

        g_ConsoleSink = MakeConsoleSink(consoleLevel);

        // Store the global log directory so named loggers can inherit it.
        g_GlobalLogDir = config.LogDir;

        const auto logPath = ResolveLogPath(config.LogDir, config.LogFile, config.Name);
        g_SharedFileSink   = MakeFileSink(logPath, FileOpenMode::Append, LogLevel::Trace);

        auto impl    = CreateLoggerImpl(config.Name, g_ConsoleSink, g_SharedFileSink);
        g_MainLogger = std::make_shared<Logger>(impl);
        spdlog::set_default_logger(impl->m_Logger);

        if (g_Async)
            spdlog::flush_every(std::chrono::seconds(1));

        g_Initialized = true;
    }
    catch (const spdlog::spdlog_ex& ex)
    {
        std::fprintf(stderr, "[Vigil] Log init failed: %s\n", ex.what());
    }
}

void LogSystem::Shutdown()
{
    LifecycleCallback shutdownCb;
    {
        std::scoped_lock lock(g_Mutex);
        if (!g_Initialized) return;
        shutdownCb = g_Hooks.Hooks.OnShutdown;
    }

    if (shutdownCb) shutdownCb();

    {
        std::scoped_lock lock(g_Mutex);
        if (!g_Initialized) return; // guard against concurrent Shutdown()

        spdlog::shutdown();
        g_MainLogger.reset();
        g_NamedLoggers.clear();
        g_ConsoleSink.reset();
        g_SharedFileSink.reset();
        g_GlobalLogDir.clear();
        g_Hooks       = {};
        g_Initialized = false;
    }
}

//==============================================================================
// Hook registration
//==============================================================================

void LogSystem::SetHooks(LogHooks hooks)
{
    std::scoped_lock lock(g_Mutex);
    if (!IsInitializedUnsafe()) return;

    g_Hooks.Hooks = std::move(hooks);
    g_Hooks.UpdateMessageSink(AllSpdLoggers());
}

void LogSystem::ClearHooks()
{
    std::scoped_lock lock(g_Mutex);
    if (!IsInitializedUnsafe()) return;

    g_Hooks.Clear(AllSpdLoggers());
}

void LogSystem::SetOnMessage(LogMessageCallback callback)
{
    std::scoped_lock lock(g_Mutex);
    if (!IsInitializedUnsafe()) return;

    g_Hooks.Hooks.OnMessage = std::move(callback);
    g_Hooks.UpdateMessageSink(AllSpdLoggers());
}

void LogSystem::SetOnLevelChange(LevelChangeCallback callback)
{
    std::scoped_lock lock(g_Mutex);
    if (!IsInitializedUnsafe()) return;

    g_Hooks.Hooks.OnLevelChange = std::move(callback);
}

void LogSystem::SetOnFlush(FlushCallback callback)
{
    std::scoped_lock lock(g_Mutex);
    if (!IsInitializedUnsafe()) return;

    g_Hooks.Hooks.OnFlush = std::move(callback);
}

void LogSystem::SetOnShutdown(LifecycleCallback callback)
{
    std::scoped_lock lock(g_Mutex);
    if (!IsInitializedUnsafe()) return;

    g_Hooks.Hooks.OnShutdown = std::move(callback);
}

//==============================================================================
// Logger lifecycle management
//==============================================================================

Logger& LogSystem::Create(std::string_view name)
{
    std::scoped_lock lock(g_Mutex);
    EnsureInitialized("Create");

    std::string key{name};
    if (auto it = g_NamedLoggers.find(key); it != g_NamedLoggers.end())
        return *it->second;

    auto impl = CreateLoggerImpl(key, g_ConsoleSink, g_SharedFileSink);
    g_Hooks.AttachSinkTo(impl->m_Logger);

    auto handle      = std::make_shared<Logger>(impl);
    auto [it, _]     = g_NamedLoggers.emplace(std::move(key), std::move(handle));
    return *it->second;
}

Logger& LogSystem::Create(const LogConfig& config)
{
    std::scoped_lock lock(g_Mutex);
    EnsureInitialized("Create");

    std::string key{config.Name};
    if (auto it = g_NamedLoggers.find(key); it != g_NamedLoggers.end())
        return *it->second;

    // Named loggers inherit the global log directory when their own is empty.
    const std::string& effectiveDir =
        config.LogDir.empty() ? g_GlobalLogDir : config.LogDir;

    spdlog::sink_ptr fileSink = g_SharedFileSink;
    if (!config.LogFile.empty() || !effectiveDir.empty())
    {
        const auto logPath = ResolveLogPath(effectiveDir, config.LogFile, config.Name);
        const LogLevel fileLevel = config.FileLevel.value_or(LogLevel::Trace);
        fileSink = MakeFileSink(logPath, config.FileMode, fileLevel);
    }

    auto impl    = CreateLoggerImpl(key, g_ConsoleSink, fileSink);
    g_Hooks.AttachSinkTo(impl->m_Logger);

    auto handle  = std::make_shared<Logger>(impl);
    auto [it, _] = g_NamedLoggers.emplace(std::move(key), std::move(handle));
    return *it->second;
}

void LogSystem::Remove(std::string_view name)
{
    std::scoped_lock lock(g_Mutex);
    if (!IsInitializedUnsafe()) return;

    auto it = g_NamedLoggers.find(std::string{name});
    if (it == g_NamedLoggers.end()) return;

    spdlog::drop(std::string{name});
    g_NamedLoggers.erase(it);
}

void LogSystem::SetMain(std::string_view name)
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

bool LogSystem::IsInitialized() noexcept
{
    std::scoped_lock lock(g_Mutex);
    return g_Initialized;
}

Logger& LogSystem::Main()
{
    std::scoped_lock lock(g_Mutex);
    EnsureInitialized("Main");
    return *g_MainLogger;
}

Logger& LogSystem::Get(std::string_view name)
{
    std::scoped_lock lock(g_Mutex);
    EnsureInitialized("Get");

    auto it = g_NamedLoggers.find(std::string{name});
    if (it != g_NamedLoggers.end())
        return *it->second;

    throw std::runtime_error("[Vigil] Logger not found: " + std::string{name});
}

Logger* LogSystem::Find(std::string_view name)
{
    std::scoped_lock lock(g_Mutex);
    if (!IsInitializedUnsafe()) return nullptr;

    auto it = g_NamedLoggers.find(std::string{name});
    return it != g_NamedLoggers.end() ? it->second.get() : nullptr;
}

//==============================================================================
// Global log level control
//==============================================================================

void LogSystem::SetGlobalLevel(LogLevel level)
{
    LevelChangeCallback cb;
    LogLevel old;
    {
        std::scoped_lock lock(g_Mutex);
        if (!IsInitializedUnsafe()) return;

        old = g_MainLogger->GetLevel();
        g_MainLogger->SetLevel(level);
        for (auto& [_, handle] : g_NamedLoggers)
            handle->SetLevel(level);

        cb = g_Hooks.Hooks.OnLevelChange;
    }

    if (cb) cb(LevelChangeEvent{ {}, old, level });
}

void LogSystem::SetGlobalFileLevel(LogLevel level)
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

void LogSystem::SetConsoleLevel(LogLevel level)
{
    std::scoped_lock lock(g_Mutex);
    if (!IsInitializedUnsafe()) return;

    g_ConsoleSink->set_level(detail::ToSpdLevel(level));
}

void LogSystem::SetLevel(std::string_view name, LogLevel level)
{
    LevelChangeCallback cb;
    LogLevel old;
    {
        std::scoped_lock lock(g_Mutex);
        if (!IsInitializedUnsafe()) return;

        auto it = g_NamedLoggers.find(std::string{name});
        if (it == g_NamedLoggers.end()) return;

        old = it->second->GetLevel();
        it->second->SetLevel(level);
        cb = g_Hooks.Hooks.OnLevelChange;
    }

    if (cb) cb(LevelChangeEvent{ name, old, level });
}

//==============================================================================
// Flush control
//==============================================================================

void LogSystem::FlushAll()
{
    std::vector<std::string> flushedNames;
    FlushCallback cb;
    {
        std::scoped_lock lock(g_Mutex);
        if (!IsInitializedUnsafe()) return;

        g_MainLogger->Flush();
        flushedNames.emplace_back(g_MainLogger->GetName());

        for (auto& [name, logger] : g_NamedLoggers)
        {
            logger->Flush();
            flushedNames.emplace_back(name);
        }

        cb = g_Hooks.Hooks.OnFlush;
    }

    if (cb)
        for (const auto& name : flushedNames)
            cb(FlushEvent{ name });
}

void LogSystem::Flush(std::string_view name)
{
    FlushCallback cb;
    std::string loggerName;
    {
        std::scoped_lock lock(g_Mutex);
        if (!IsInitializedUnsafe()) return;

        auto it = g_NamedLoggers.find(std::string{name});
        if (it == g_NamedLoggers.end()) return;

        it->second->Flush();
        loggerName = it->first; // copy before lock release: string_view may dangle
        cb = g_Hooks.Hooks.OnFlush;
    }

    if (cb) cb(FlushEvent{ loggerName });
}

} // namespace vigil
