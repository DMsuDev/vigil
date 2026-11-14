# User Guide

This guide covers every public feature of Vigil. Each section is self-contained, so you can jump straight to the one you need.

- [User Guide](#user-guide)
  - [Conventions](#conventions)
  - [Logging](#logging)
    - [Initialize and shut down](#initialize-and-shut-down)
    - [Writing messages](#writing-messages)
    - [Log levels](#log-levels)
    - [Controlling levels at runtime](#controlling-levels-at-runtime)
    - [Sinks, files, and flushing](#sinks-files-and-flushing)
    - [Named loggers](#named-loggers)
    - [Asynchronous logging](#asynchronous-logging)
    - [Before Init and after Shutdown](#before-init-and-after-shutdown)
  - [Rate-limited logging](#rate-limited-logging)
  - [Lifecycle hooks](#lifecycle-hooks)
  - [Assertions](#assertions)
    - [Macro reference](#macro-reference)
  - [Scoped logging](#scoped-logging)
    - [Macro reference](#macro-reference-1)
  - [Stack traces](#stack-traces)
  - [Crash handling](#crash-handling)
    - [API](#api)
    - [What is intercepted](#what-is-intercepted)
    - [What happens when the process crashes](#what-happens-when-the-process-crashes)
    - [The crash report file](#the-crash-report-file)
  - [Utilities](#utilities)
    - [UTF-8 console](#utf-8-console)
    - [Version information](#version-information)
    - [Smart pointer aliases](#smart-pointer-aliases)
    - [Source locations](#source-locations)

## Conventions

- Include `<vigil/vigil.h>`. Everything lives in the `vigil` namespace.
- Headers under `vigil/detail/` are installed only because public headers include them. Treat them as internal: they carry no stability guarantee.
- Several examples use designated initializers (`{ .Name = "App" }`), which need C++20. In C++17 create the config struct and assign its fields.
- Formatting uses [`fmt`](https://fmt.dev) syntax: `"{}"`, `"{:08d}"`, `"{1} {0}"`.
- Logging calls are safe to make from multiple threads. Registry operations (`Create`, `Remove`, `SetMain`, level changes, hooks) are serialized internally.

## Logging

### Initialize and shut down

Initialize once at startup and shut down before exit:

```cpp
#include <vigil/vigil.h>

int main()
{
    vigil::LogSystemConfig config;
    config.Name         = "MyApp";
    config.LogDir       = "logs";
    config.ConsoleLevel = vigil::LogLevel::Info;
    vigil::LogSystem::Init(config);

    vigil::Info("Application started");

    vigil::LogSystem::Shutdown();
}
```

`LogSystemConfig` fields:

| Field            | Default                                   | Meaning                                                                                       |
| :--------------- | :---------------------------------------- | :-------------------------------------------------------------------------------------------- |
| `Name`           | `"Main"`                                  | Name of the main logger. It appears in every message.                                         |
| `LogDir`         | empty (current directory)                 | Directory for log files. Created automatically.                                               |
| `LogFile`        | empty (`<Name>.log`)                      | File name of the main log file.                                                               |
| `ConsoleLevel`   | `Trace` in Debug builds, `Info` otherwise | Minimum severity printed to the console.                                                      |
| `Async`          | `false`                                   | Deliver messages from a background thread. See [Asynchronous logging](#asynchronous-logging). |
| `AsyncQueueSize` | `8192`                                    | Capacity of the asynchronous queue.                                                           |

Behavior to know:

- A second `Init()` call while the system is initialized is ignored. The first configuration wins.
- If initialization fails (for example the thread pool cannot start), Vigil prints a message to `stderr` and stays uninitialized. It does not throw.
- `Shutdown()` is idempotent. After it returns you may call `Init()` again with a new configuration.
- `Shutdown()` invalidates every `Logger&` you obtained earlier. Do not keep them across a shutdown.

### Writing messages

There are three equivalent ways to log through the main logger:

```cpp
// 1. Free functions (spdlog-style).
vigil::Info("Connected to {}:{}", "127.0.0.1", 5432);
vigil::Warn("Config file not found, using defaults.");

// 2. Macros: can be removed at compile time (see "Log levels").
VIGIL_LOG_INFO("Build number: {:06}", 1337);
VIGIL_LOG_DEBUG("x = {}", 42);

// 3. The Logger object.
vigil::LogSystem::Main().Error("Failed to open {}", "settings.toml");
```

Each severity has a function and a macro:

| Level      | Function               | Macro                     | Typical use               |
| :--------- | :--------------------- | :------------------------ | :------------------------ |
| `Trace`    | `vigil::Trace(...)`    | `VIGIL_LOG_TRACE(...)`    | Verbose internal flow     |
| `Debug`    | `vigil::Debug(...)`    | `VIGIL_LOG_DEBUG(...)`    | Developer diagnostics     |
| `Info`     | `vigil::Info(...)`     | `VIGIL_LOG_INFO(...)`     | Normal operational events |
| `Warn`     | `vigil::Warn(...)`     | `VIGIL_LOG_WARN(...)`     | Recoverable anomalies     |
| `Error`    | `vigil::Error(...)`    | `VIGIL_LOG_ERROR(...)`    | Operation failures        |
| `Critical` | `vigil::Critical(...)` | `VIGIL_LOG_CRITICAL(...)` | System-level failures     |

`vigil::Log(level, ...)` and `Logger::Log(level, ...)` take the level as a runtime value.

Every function has two forms. With only a message it logs the text verbatim (no formatting, braces are kept). With extra arguments it formats the message with `fmt`. Formatting is skipped entirely when the message would be filtered out by the logger's level.

With C++20 and compiler support for `consteval`, format strings are checked at compile time. As a result, `vigil::Info("{} {}", 1)` fails to compile due to the argument mismatch.

In C++17, format strings are checked at runtime instead, so invalid format strings may throw `fmt::format_error` when formatting is performed.

### Log levels

Levels are ordered from most to least verbose: `Trace`, `Debug`, `Info`, `Warn`, `Error`, `Critical`, and `Off`. `vigil::ToString(level)` returns the display name.

There are two independent filters:

1. **Compile time.** `VIGIL_LOG_*` macros below `VIGIL_ACTIVE_LOG_LEVEL` expand to nothing: the call is removed and its arguments are never evaluated. The default keeps every level. Set it for the whole target, for example `VIGIL_ACTIVE_LOG_LEVEL=2` keeps `Info` and above. The numeric values are `0` Trace through `6` Off (the `VIGIL_LEVEL_*` constants).
2. **Runtime.** Each logger has a level (initially `Trace`), and each sink has its own level. A message must pass the logger level first, then the level of every sink it reaches.

`vigil::IsLevelActive(level)` reports whether a level survives the compile-time filter, which is useful before building an expensive message by hand:

```cpp
if (vigil::IsLevelActive(vigil::LogLevel::Debug))
    vigil::Debug("state dump: {}", ExpensiveDump());
```

Only the macros are stripped at compile time. Free functions and `Logger` methods are always compiled in and filtered at runtime.

### Controlling levels at runtime

| Call                                   | Effect                                                                   |
| :------------------------------------- | :----------------------------------------------------------------------- |
| `logger.SetLevel(level)`               | Sets one logger's level and fires `OnLevelChange`.                       |
| `LogSystem::SetLevel(name, level)`     | Sets one **named** logger's level and fires `OnLevelChange`.             |
| `LogSystem::SetGlobalLevel(level)`     | Sets all logger levels and fires `OnLevelChange` once.                   |
| `LogSystem::SetConsoleLevel(level)`    | Sets the level of the shared console sink.                               |
| `LogSystem::SetGlobalFileLevel(level)` | Sets the level of the shared file sink and of every dedicated file sink. |

`OnLevelChange` fires on every call, even if the level does not change. When a call is made from inside an `OnMessage` or `OnLevelChange` callback, the level is still applied but the hook is not fired.

```cpp
auto& logger = vigil::LogSystem::Main();
logger.SetLevel(vigil::LogLevel::Warn);
VIGIL_LOG_INFO("dropped");
logger.SetLevel(vigil::LogLevel::Trace);

// Different thresholds per sink: console shows Info+, file records Warn+.
vigil::LogSystem::SetConsoleLevel(vigil::LogLevel::Info);
vigil::LogSystem::SetGlobalFileLevel(vigil::LogLevel::Warn);
```

### Sinks, files, and flushing

Every logger writes to two sinks by default:

| Sink    | Level                | Output format                      | Notes                                                             |
| :------ | :------------------- | :--------------------------------- | :---------------------------------------------------------------- |
| Console | `ConsoleLevel`       | `[14:03:22] [MyApp] message`       | Colored output on stdout.                                         |
| File    | `Trace` (everything) | `[14:03:22] [info] MyApp: message` | Created lazily, thread-safe, rotating (10 MiB per file, 5 files). |

The file path is `LogDir/LogFile`. If `LogFile` is empty the name is derived from the logger name (`Name.log`; a name that already ends in `.log` is not doubled).

**Lazy creation.** The log file does not exist until a message actually reaches the file sink. Messages filtered out by level never create it, so an application that logs nothing leaves no empty `.log` files. If the file cannot be opened, Vigil prints one error to `stderr` and console logging keeps working.

**Flushing.** A logger flushes automatically after any message at `Warn` or above. Otherwise flush explicitly:

```cpp
vigil::LogSystem::Main().Flush();      // main logger
vigil::LogSystem::Flush("Network");    // one named logger
vigil::LogSystem::FlushAll();          // main + every named logger
```

`LogSystem::Flush(name)` only looks at named loggers. To flush the main logger use `Main().Flush()` or `FlushAll()`.

### Named loggers

Create per-subsystem loggers that share the main sinks, or give them a dedicated file:

```cpp
vigil::LogSystem::Init({ .Name = "Engine", .LogDir = "logs" });

// Simple named logger: shares the main file sink.
auto& net = vigil::LogSystem::Create("Network");
net.Info("Connected to server.");
net.Warn("Packet loss detected: {}%", 12);

// Named logger with its own file and file level.
vigil::LogSystem::Create({
    .Name      = "Physics",
    .LogFile   = "physics.log",
    .FileMode  = vigil::FileOpenMode::Truncate,
    .FileLevel = vigil::LogLevel::Debug,
});
vigil::LogSystem::Get("Physics").Debug("Simulation step complete.");

// Log through a named logger with a macro. The logger is created on demand.
VIGIL_LOG_NAMED("Network", vigil::LogLevel::Error, "Disconnected from server.");

// Find() returns nullptr instead of throwing.
if (auto* audio = vigil::LogSystem::Find("Audio"))
    audio->Info("Audio subsystem ready.");
```

| Function                       | Description                                                               |
| :----------------------------- | :------------------------------------------------------------------------ |
| `LogSystem::Create(name)`      | Creates or returns a named logger that shares the main file sink.         |
| `LogSystem::Create(LogConfig)` | Creates or returns a named logger with its own configuration.             |
| `LogSystem::Get(name)`         | Returns a named logger. Throws `std::runtime_error` if it does not exist. |
| `LogSystem::Find(name)`        | Returns a pointer, or `nullptr` if it does not exist. Never throws.       |
| `LogSystem::Remove(name)`      | Removes a named logger and releases its sinks. No-op if unknown.          |
| `LogSystem::SetMain(name)`     | Promotes a named logger to be the main logger. No-op if unknown.          |

`LogConfig` fields: `Name` (default `"New"`), `LogDir`, `LogFile`, `FileMode` (`Append` or `Truncate`), `FileLevel`, `Async`, `AsyncQueueSize`.

Rules worth remembering:

- **Existing names win.** If a logger with that name already exists, `Create` returns it and ignores the new configuration.
- **When a dedicated file is created.** `Create(LogConfig)` opens its own file whenever a file name, a directory, or a file level is in effect. The directory counts even when it is inherited from `LogSystemConfig::LogDir`. If none is set, the logger shares the main file sink. `Create(name)` always shares it.
- **File mode.** `Append` uses a rotating file; `Truncate` clears the file on open and does not rotate.
- **Lifetime.** References returned by `Create`/`Get` stay valid until the logger is removed or the system shuts down. After `SetMain`, references to the old main logger are no longer valid.
- **Name reuse.** Removing a logger frees its name, so you can create another with the same name.

### Asynchronous logging

Set `Async = true` to hand messages to a background thread, which keeps the call site fast:

```cpp
vigil::LogSystem::Init({ .Name = "App", .Async = true, .AsyncQueueSize = 16384 });
```

- All asynchronous loggers share a single background thread pool, created the first time one is needed.
- A named logger can override the global setting through `LogConfig::Async`. A synchronous main logger can coexist with asynchronous named loggers, and vice versa.
- `AsyncQueueSize` only matters when the pool is created. Once it exists, later values are ignored.
- **If the queue fills up, the oldest queued messages are overwritten.** Size the queue for your peak burst.
- With an asynchronous main logger, Vigil flushes every second. `Shutdown()` drains the queue, so no queued message is lost on a clean exit.
- `OnMessage` hooks run on the background thread for asynchronous loggers.

### Before Init and after Shutdown

| Call                                                      | Before `Init()` / after `Shutdown()`                                                                                                  |
| :-------------------------------------------------------- | :------------------------------------------------------------------------------------------------------------------------------------ |
| Free functions, enabled `VIGIL_LOG_*` macros              | Arguments are evaluated, then an assertion fails if enabled; otherwise one warning is printed on `stderr` and the message is dropped. |
| `VIGIL_LOG_NAMED`                                         | Assertion failure if enabled; otherwise one warning on `stderr`. The logger name, level, and message arguments are not evaluated.     |
| `LogSystem::Main`, `Create`, `Get`                        | Throw `std::logic_error`.                                                                                                             |
| `LogSystem::Find`                                         | Returns `nullptr`.                                                                                                                    |
| `Remove`, `SetMain`, level setters, `Flush`, hook setters | Silent no-op.                                                                                                                         |

Compile-time-disabled `VIGIL_LOG_*` macros do not evaluate their arguments, regardless of initialization. Because hook setters are no-ops before `Init()`, register hooks after initializing. Hooks are cleared by `Shutdown()`.

## Rate-limited logging

Prevent log spam in hot code paths without keeping your own state:

```cpp
for (int i = 0; i < 1000; ++i)
{
    // Emitted exactly once for this key for the lifetime of the process.
    vigil::LogOncePolicy::LogOnce(
        "startup-notice", vigil::LogLevel::Warn, "Running without a config file.");

    // Emitted at most once every 0.5 seconds.
    vigil::LogTTLPolicy::LogTTL(
        "heartbeat", 0.5, vigil::LogLevel::Info, "Service heartbeat OK.");
}
```

| Policy                   | Parameters                          | Behavior                                         |
| :----------------------- | :---------------------------------- | :----------------------------------------------- |
| `LogOncePolicy::LogOnce` | key, level, message                 | Logs the first time the key is seen, never again |
| `LogTTLPolicy::LogTTL`   | key, TTL in seconds, level, message | Logs at most once per TTL window per key         |

Both also expose `ShouldLog(...)` if you want the decision without the logging (`LogOncePolicy::ShouldLog(key)`, `LogTTLPolicy::ShouldLog(key, ttlSeconds)`).

Things to know:

- Keys are global to the process and thread-safe. Use a distinct key per call site (a function name works well). There is no reset.
- Messages are written to the **main** logger and passed through verbatim (braces are not interpreted).

## Lifecycle hooks

Hooks let you observe logging events without changing the pipeline. They are useful for in-app consoles, telemetry, and test assertions.

```cpp
vigil::LogSystem::Init({ .Name = "App" });

vigil::LogSystem::SetHooks({
    .OnMessage = [](const vigil::LogMessageEvent& e) {
        // Mirror every message to an in-app console or telemetry sink.
    },
    .OnLevelChange = [](const vigil::LevelChangeEvent& e) {
        // A logger's level changed.
    },
    .OnFlush = [](const vigil::FlushEvent& e) {
        // A logger was flushed.
    },
    .OnShutdown = [] {
        // Shutdown is starting.
    },
});

vigil::Info("This message is observed.");
vigil::LogSystem::ClearHooks();
vigil::Info("This one is not.");

vigil::LogSystem::Shutdown();
```

| Hook            | Fires when                                                     | Event data                                               |
| :-------------- | :------------------------------------------------------------- | :------------------------------------------------------- |
| `OnMessage`     | A message passes a logger's level filter.                      | `Level`, `Message`, `LoggerName`                         |
| `OnLevelChange` | A logger level changes outside `OnMessage` or `OnLevelChange`. | `LoggerName` (empty when global), `OldLevel`, `NewLevel` |
| `OnFlush`       | `LogSystem::Flush(name)` or `FlushAll()` flushes a logger.     | `LoggerName` (one event per flushed logger)              |
| `OnShutdown`    | At the start of `LogSystem::Shutdown()`.                       | none                                                     |

Registration functions:

| Function                          | Description                                                 |
| :-------------------------------- | :---------------------------------------------------------- |
| `LogSystem::SetHooks(LogHooks)`   | Registers all hooks at once, replacing every previous hook. |
| `LogSystem::SetOnMessage(cb)`     | Registers or replaces the message callback.                 |
| `LogSystem::SetOnLevelChange(cb)` | Registers or replaces the level-change callback.            |
| `LogSystem::SetOnFlush(cb)`       | Registers or replaces the flush callback.                   |
| `LogSystem::SetOnShutdown(cb)`    | Registers or replaces the shutdown callback.                |
| `LogSystem::ClearHooks()`         | Removes every hook.                                         |

Pass `nullptr` to a setter to clear that single hook.

Rules for callbacks:

- **`OnMessage` runs on the logging thread, inside the sink's lock.** Keep it fast and non-blocking, and never call `LogSystem` functions from it. `Logger::SetLevel()` is allowed but applies silently and does not fire `OnLevelChange`. For asynchronous loggers it runs on the background thread.
- **The other hooks run after Vigil's internal lock is released**, so they may call back into `LogSystem`. Nested level changes do not recursively invoke `OnLevelChange`. Avoid recursively triggering other hooks: for example, `Shutdown()` from `OnShutdown` causes unbounded recursion.
- `OnLevelChange` callbacks may execute concurrently and are not guaranteed to run in change order. Protect any shared state they access. `OldLevel` represents the level immediately before the corresponding update, even when concurrent silent updates occur inside `OnMessage`. For global level changes, `OldLevel` represents the main logger's previous level.
- Exceptions thrown by `OnLevelChange`, `OnFlush`, or `OnShutdown` propagate to the caller. The new level is already applied when `OnLevelChange` throws. An exception from `OnShutdown` aborts `Shutdown()` before teardown begins.
- `LogMessageEvent::Message` is the raw message text (the log pattern is not applied). Its strings are owned copies and safe to keep.
- `LevelChangeEvent::LoggerName` and `FlushEvent::LoggerName` are `std::string_view`s that are only valid during the callback. Copy them if you need them later.
- `OnMessage` receives messages from the main logger and from named loggers, including ones created before or after the hook was registered.

## Assertions

Assertion macros check a condition and terminate the process on failure.

```cpp
int  value = 42;
int* ptr   = &value;

VIGIL_ASSERT(value > 0);
VIGIL_ASSERT_MSG(value == 42, "Expected 42, got {}", value);
VIGIL_ASSERT_NOT_NULL(ptr);
VIGIL_ASSERT_IN_RANGE(value, 0, 100);
```

A failing assertion produces output like this:

```text
[App] Assertion failed
  Expression : value == 0
  Location   : src/main.cpp:12
  Function   : main()
  Message    : Expected 0, got 42.

Stack trace (3 frames):

#0 0x00005807BEF5FF26 in main() at src/main.cpp:12
#1 0x000076962942A601 in __libc_start_call_main() at libc_start_call_main.h:58
#2 0x000076962942A718 in __libc_start_main_impl() at libc-start.c:347
```

### Macro reference

| Macro                                  | Custom message | When assertions are disabled                   | Use case                                |
| :------------------------------------- | :------------: | :--------------------------------------------- | :-------------------------------------- |
| `VIGIL_ASSERT(check)`                  |       no       | Removed, `check` not evaluated                 | Preconditions without side effects      |
| `VIGIL_ASSERT_MSG(check, fmt, ...)`    |      yes       | Removed, `check` not evaluated                 | Preconditions that need runtime context |
| `VIGIL_ASSERT_NOT_NULL(ptr)`           |       no       | Removed                                        | Null pointer guards                     |
| `VIGIL_ASSERT_IN_RANGE(val, min, max)` |       no       | Removed                                        | Inclusive bounds validation             |
| `VIGIL_VERIFY(check)`                  |       no       | `check` still evaluated, result ignored        | Expressions with side effects           |
| `VIGIL_VERIFY_MSG(check, fmt, ...)`    |      yes       | `check` still evaluated, result ignored        | Side effects that need failure context  |
| `VIGIL_UNREACHABLE_ASSERT()`           |       no       | Optimizer hint (undefined behavior if reached) | Unreachable `switch` defaults           |

`VIGIL_ASSERT*` macros are active only when `VIGIL_ENABLE_ASSERTS` is defined (on by default in Debug builds). Put anything with side effects inside `VIGIL_VERIFY*`, never inside `VIGIL_ASSERT*`:

```cpp
VIGIL_VERIFY(file.close());   // closes the file in every build; reports failure when assertions are on
VIGIL_ASSERT(file.close());   // WRONG: the file is never closed when assertions are off
```

Details:

- `VIGIL_ASSERT_IN_RANGE` evaluates each argument more than once. Pass plain variables or constants.
- Messages use `fmt` syntax. In C++17 `VIGIL_ASSERT` has no message; use `VIGIL_ASSERT_MSG` when you need one.
- On failure Vigil logs at `Critical` through the main logger and flushes. If logging is not initialized, the same report goes to `stderr`.
- If a debugger is attached, execution breaks into it. Then the process calls `std::abort()`. With the [crash handler](#crash-handling) installed, the resulting `SIGABRT` also produces a crash report.
- `VIGIL_UNREACHABLE_ASSERT()` reports and aborts when assertions are enabled. When disabled it becomes `VIGIL_UNREACHABLE()`, so reaching it is undefined behavior.

## Scoped logging

Scoped logging adds automatic entry and exit messages, with elapsed time, around functions and blocks. Enable it with `-DVIGIL_ENABLE_SCOPED_LOG=ON` (on by default in Debug builds). When disabled, every macro compiles to nothing.

```cpp
static void LoadAssets()
{
    VIGIL_SCOPED_LOG_FUNCTION();

    {
        VIGIL_SCOPED_LOG("Parsing manifest");
        // ...
    }

    {
        VIGIL_SCOPED_LOG("Uploading textures");
        // ...
    }
}
```

Output (at `Trace` level):

```text
[trace] >> void LoadAssets()
[trace] >> Parsing manifest
[trace] << Parsing manifest (15.12 ms)
[trace] >> Uploading textures
[trace] << Uploading textures (20.31 ms)
[trace] << void LoadAssets() (37.48 ms)
```

Durations below one millisecond are shown in microseconds, for example `640 µs`.

### Macro reference

| Macro                                    | Description                                              |
| :--------------------------------------- | :------------------------------------------------------- |
| `VIGIL_SCOPED_LOG(name)`                 | RAII scope at `Trace` level.                             |
| `VIGIL_SCOPED_LOG_LEVEL(name, level)`    | RAII scope at an explicit level.                         |
| `VIGIL_SCOPED_LOG_FUNCTION()`            | RAII scope named after the current function, at `Trace`. |
| `VIGIL_SCOPED_LOG_FUNCTION_LEVEL(level)` | Same, at an explicit level.                              |
| `VIGIL_SCOPE_BEGIN(name)`                | Opens a manual block scope at `Trace` level.             |
| `VIGIL_SCOPE_BEGIN_LEVEL(name, level)`   | Opens a manual block scope at an explicit level.         |
| `VIGIL_SCOPE_END()`                      | Closes a manual block scope.                             |

Manual scopes open a real C++ block with `{` and close it with `}`, so they must be paired in the same block and can be nested:

```cpp
VIGIL_SCOPE_BEGIN("Pipeline");
    VIGIL_SCOPE_BEGIN("Stage 1");
        // ...
    VIGIL_SCOPE_END();
VIGIL_SCOPE_END();
```

Details:

- Scopes log through the main logger, so `LogSystem` must be initialized when the scope is **entered**. Creating a scope earlier throws `std::logic_error`.
- If `LogSystem` has been shut down by the time the scope exits (for example on an early return), the exit message is written to `stderr` so the timing is never lost.
- Pick a higher level (`Warn`, `Error`) for scopes that must remain visible when `Trace` is filtered out.
- You can use `vigil::ScopedLogger` directly. It accepts a string literal, a `std::string_view`, or an owned `std::string`.

## Stack traces

Vigil captures stack traces for [assertion failures](#assertions) and [crash reports](#crash-handling). A trace looks like this:

```text
Stack trace (3 frames):

#0 0x00005807BEF5FF26 in main() at src/main.cpp:12
#1 0x000076962942A601 in __libc_start_call_main() at libc_start_call_main.h:58
#2 0x000076962942A718 in __libc_start_main_impl() at libc-start.c:347
```

Each line is `#index address in symbol() at file:line`. Source paths are shortened to the last two components, inlined call sites are marked `[inlined]`, and runtime start-up frames (`mainCRTStartup`, `_start`, `__libc_start_main`, ...) are trimmed from the bottom.

What you need to get useful traces:

| Platform     | Backend                                     | Requirement                                                                               |
| :----------- | :------------------------------------------ | :---------------------------------------------------------------------------------------- |
| Windows      | `CaptureStackBackTrace` and DbgHelp         | A PDB next to your binary. Inline frames need Windows 8 or newer.                         |
| Linux, macOS | `backtrace()` and the vendored libbacktrace | Debug info (`-g`) in the executable. Without it you only get addresses or function names. |

Controlling the feature:

- `VIGIL_ENABLE_STACK_TRACE=OFF` removes the backends. Assertions and crash reports still work, but print `Stack trace: <disabled by compile options>`.
- Captures hold up to 64 frames by default.
- Symbol resolution is serialized with an internal mutex and allocates memory, so it is never done from a signal handler. The crash handler captures raw addresses first and resolves them later on a separate thread.

> [!NOTE]
> The `StackTrace` class lives in `src/trace/` and is not part of the installed public API. Stack traces are exposed through assertion output and crash reports.

## Crash handling

`CrashHandler` intercepts fatal signals and unhandled exceptions.

```cpp
#include <vigil/vigil.h>

#include <cstdio>

int main()
{
    // Optional: crashes are also written to the log when LogSystem is initialized.
    vigil::LogSystem::Init({ .Name = "App", .LogDir = "logs" });

    vigil::CrashHandler::Install(
        [](const vigil::CrashInfo& info)
        {
            std::fprintf(stderr, "Crash: %s\n", info.reason);
            if (info.reportPath)
                std::fprintf(stderr, "Report: %s\n", info.reportPath);
        },
        "logs/crashes");

    // ... run the application ...
}
```

`CrashHandler` can be installed independently of the logging system. Without `LogSystem` the report goes to `stderr`.

### API

| Function                                           | Description                                                                                     |
| :------------------------------------------------- | :---------------------------------------------------------------------------------------------- |
| `CrashHandler::Install(callback, reportDirectory)` | Arms every crash hook. Both arguments are optional. Repeated calls are ignored; the first wins. |
| `CrashHandler::InstallThreadAltStack()`            | Prepares the calling thread to survive a stack overflow. Call it on every thread you create.    |
| `CrashHandler::IsInstalled()`                      | True once `Install()` has been called.                                                          |

Call `Install()` from the main thread before creating other threads. The main thread is covered automatically. Call `InstallThreadAltStack()` at the start of every secondary thread; it does nothing until `Install()` has run.

`CrashInfo` fields (strings are valid only during the callback):

| Field        | Meaning                                                                 |
| :----------- | :---------------------------------------------------------------------- |
| `reason`     | Short description of the failure.                                       |
| `stackTrace` | Formatted stack trace, or `nullptr`.                                    |
| `reportPath` | Path of the crash report file, or `nullptr` if it could not be written. |

### What is intercepted

| Platform     | Intercepted failures                                                                                                                                                                                                                      |
| :----------- | :---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Linux, macOS | `SIGSEGV`, `SIGABRT`, `SIGFPE`, `SIGILL`, `SIGBUS`, and everything that reaches `std::terminate` (uncaught exceptions, `noexcept` violations, pure virtual calls, uncaught `std::bad_alloc`).                                             |
| Windows      | Unhandled structured exceptions (access violation, illegal instruction, divide by zero, overflow, stack overflow, and others), `std::terminate`, pure virtual calls, `operator new` failure, `abort()`, and CRT invalid-parameter errors. |

On Windows, a stack overflow is reported without a stack trace because there is no room to walk the stack, and it is not yet covered by the crash test suite.

### What happens when the process crashes

1. The faulting thread does only async-signal-safe work: it records the reason and raw return addresses, wakes a watchdog thread, and parks itself.
2. The watchdog starts a 3-second deadline timer. If anything below hangs (a corrupted heap, a held lock), the process is killed when it expires.
3. The watchdog symbolizes the trace, logs it at `Critical` and flushes the logs (or writes to `stderr`).
4. It writes the crash report file.
5. It calls your callback. Exceptions thrown by the callback are swallowed.
6. It flushes, breaks into an attached debugger, and terminates the process with exit code `1`.

Consequences:

- Only the **first** crash is reported. Other threads that crash meanwhile are parked.
- The process exits through `_exit` (or `TerminateProcess`), so **no core dump** is produced.

### The crash report file

The report is named `crash_<YYYYMMDD>_<HHMMSS>_<pid>.txt` (UTC time) and is written to the first writable directory among:

1. the `reportDirectory` you passed to `Install()` (resolved to an absolute path at install time, so a later `chdir` does not move it), then
2. the system temporary directory.

```text
Vigil crash report
Time:    2026-11-13 10:15:30 UTC
Process: 12345

Reason:
Segmentation fault (SIGSEGV)

Stack trace:
Stack trace (3 frames):
...
```

See [`examples/diagnostics/crash_guard_example.cpp`](https://github.com/DMsuDev/vigil/blob/main/examples/diagnostics/crash_guard_example.cpp) for a runnable setup, and [Architecture](architecture.md#crash-pipeline) for the design.

## Utilities

### UTF-8 console

```cpp
if (!vigil::EnableUTF8Console())
    std::fprintf(stderr, "Unicode output may not display correctly.\n");
```

On Windows this switches the console input and output code pages to UTF-8 so Unicode text renders correctly. Call it once at startup, before any console I/O. On other platforms it does nothing and returns `true`.

### Version information

`<vigil/version.h>` (included by `vigil.h`) exposes the version at compile time:

| Item                                      | Example value / use                              |
| :---------------------------------------- | :----------------------------------------------- |
| `VIGIL_VERSION_MAJOR/MINOR/PATCH`         | `0`, `8`, `2`                                    |
| `VIGIL_VERSION_STRING`                    | `"0.8.2"`                                        |
| `VIGIL_VERSION_FULL`                      | `"0.8.2"`, or `"0.9.0-WIP"` for pre-releases     |
| `VIGIL_VERSION`                           | Packed integer `major*10000 + minor*100 + patch` |
| `VIGIL_VERSION_AT_LEAST(maj, min, pat)`   | Preprocessor gate, usable in `#if`               |
| `vigil::version::major/minor/patch`       | `constexpr` integers                             |
| `vigil::version::string / full_string`    | `constexpr std::string_view`                     |
| `vigil::version::is_prerelease`           | `true` for `-WIP` builds                         |
| `vigil::version::at_least(maj, min, pat)` | `constexpr` runtime/`if constexpr` check         |

```cpp
#if VIGIL_VERSION_AT_LEAST(0, 8, 0)
    // use an API introduced in 0.8.0
#endif
```

### Smart pointer aliases

Vigil standardizes on two ownership aliases from `vigil/core/smart_pointers.h`:

- `vigil::Scope<T>` is `std::unique_ptr<T>`, created with `vigil::CreateScope<T>(...)`.
- `vigil::Shared<T>` is `std::shared_ptr<T>`, created with `vigil::CreateShared<T>(...)`.

Prefer `Scope` unless ownership is genuinely shared.

### Source locations

`vigil::SourceLocation` carries a file, line, and function name. `VIGIL_CURRENT_LOC()` captures the current one using `std::source_location` in C++20 and compiler macros in C++17 (where the column is always `0`).
