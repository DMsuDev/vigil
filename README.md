<div align="center">
  <picture>
    <source media="(prefers-color-scheme: dark)" srcset="docs/images/vigil_logo_light.png">
    <source media="(prefers-color-scheme: light)" srcset="docs/images/vigil_logo_dark.png">
    <img width="320" src="docs/images/vigil_logo_light.png" alt="VIGIL Logo">
  </picture>

  <p>
    <strong>Logging and Diagnostics for C++</strong><br>
    Structured Logging • Assertions • Runtime Instrumentation
  </p>

  [![License MIT](https://img.shields.io/github/license/DMsuDev/vigil?style=flat&logo=open-source-initiative&logoColor=white)](https://github.com/DMsuDev/vigil/blob/main/LICENSE)
  ![Version](https://img.shields.io/github/v/release/DMsuDev/vigil?style=flat&label=Version&color=purple)
  [![Linux Build](https://github.com/DMsuDev/vigil/actions/workflows/linux-build.yml/badge.svg)](https://github.com/DMsuDev/vigil/actions/workflows/linux-build.yml)
  [![Windows Build](https://github.com/DMsuDev/vigil/actions/workflows/windows-build.yml/badge.svg)](https://github.com/DMsuDev/vigil/actions/workflows/windows-build.yml)
  [![macOS Build](https://github.com/DMsuDev/vigil/actions/workflows/macos-build.yml/badge.svg)](https://github.com/DMsuDev/vigil/actions/workflows/macos-build.yml)
  [![Integration Build](https://github.com/DMsuDev/vigil/actions/workflows/install-tests.yml/badge.svg)](https://github.com/DMsuDev/vigil/actions/workflows/install-tests.yml)

</div>

## Overview

**Vigil** is a C++ logging and diagnostics library built around structured logging, assertions, stack traces, and runtime instrumentation. It provides a consistent interface for recording application events, detecting failures early, and capturing rich context when things go wrong.

Supports **C++17 and later** on **Linux**, **Windows**, and **macOS**.

## Features

- **Structured Logging:** `fmt`-style messages, compile-time filtering, main and named loggers, console/rotating file sinks, and thread-safe output.
- **Lifecycle Hooks:** Event callbacks for mirroring messages to in-app consoles, telemetry, or test assertions.
- **Rate Limiting:** One-shot (`LogOncePolicy::LogOnce`) and TTL-based (`LogTTLPolicy::LogTTL`) policies for high-frequency log paths.
- **Diagnostics & Assertions:** Rich assertions with source locations, cross-platform stack traces, and thread-safe failure reporting.
- **Scoped Instrumentation:** Optional RAII entry/exit logging with elapsed-time measurement and adaptive microsecond/millisecond formatting.

## Stability notice

> [!IMPORTANT]
> Always pin to a **release tag** when integrating Vigil. The `main` branch is the active development branch and may contain breaking changes, incomplete features, or unstable APIs at any point.
>
> ```cmake
> # Correct -- pin to a release tag
> FetchContent_Declare(vigil GIT_TAG v0.6.1 ...)
>
> # Avoid -- main is not guaranteed to be stable
> FetchContent_Declare(vigil GIT_TAG main ...)
> ```

## CMake integration

### Option 1: FetchContent (recommended)

```cmake
include(FetchContent)

FetchContent_Declare(
  vigil
  GIT_REPOSITORY https://github.com/DMsuDev/vigil.git
  GIT_TAG        v0.6.1   # tag or hash: pin to a release tag
  GIT_SHALLOW    TRUE
)

FetchContent_MakeAvailable(vigil)
target_link_libraries(my_app PRIVATE vigil::vigil)
```

### Option 2: Subdirectory

```cmake
add_subdirectory(vendor/vigil)
target_link_libraries(my_app PRIVATE vigil::vigil)
```

### Option 3: Installed package

```cmake
find_package(Vigil CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE vigil::vigil)
```

If CMake cannot locate `VigilConfig.cmake`, pass the install prefix explicitly:

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH="/path/to/vigil/install"
```

### CMake options

| Option                        |   Default    | Description                                                                               |
| :---------------------------- | :----------: | :---------------------------------------------------------------------------------------- |
| `VIGIL_BUILD_EXAMPLES`        |    `OFF`     | Build example targets.                                                                    |
| `VIGIL_BUILD_TESTS`           |    `OFF`     | Build the test suite.                                                                     |
| `VIGIL_CONSUMER_TESTS`        |    `OFF`     | Build slow CMake consumer integration tests (find_package/FetchContent/add_subdirectory). |
| `VIGIL_INSTALL_TESTS_CLEANUP` |     `ON`     | Delete generated consumer project dirs after `VIGIL_CONSUMER_TESTS` run.                  |
| `VIGIL_INSTALL`               |    `OFF`     | Generate install targets and package metadata.                                            |
| `VIGIL_USE_SYSTEM_FMT`        |    `OFF`     | Prefer a system-installed `fmt` when available.                                           |
| `VIGIL_BUILD_SHARED`          |    `OFF`     | Build Vigil as a shared library.                                                          |
| `VIGIL_BUILD_WARNINGS`        |     `ON`     | Enable strict compiler warning suites.                                                    |
| `VIGIL_WARNINGS_AS_ERRORS`    |    `OFF`     | Treat compiler warnings as hard errors.                                                   |
| `VIGIL_ENABLE_STACK_TRACE`    |     `ON`     | Enable stack trace capture and symbolication.                                             |
| `VIGIL_ENABLE_ASSERTS`        | `ON` (Debug) | Enable assertion macros with logging and stack traces.                                    |
| `VIGIL_ENABLE_SCOPED_LOG`     |    `OFF`     | Enable RAII scope instrumentation macros.                                                 |

Example configure command:

```bash
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DVIGIL_BUILD_TESTS=OFF \
  -DVIGIL_BUILD_EXAMPLES=OFF
```

## Usage

<details>
<summary>Logging</summary>

<br>

Initialize the logging system once at startup and emit messages using the API or convenience macros with `fmt`-style formatting.

```cpp
#include <vigil/vigil.h>

int main()
{
    vigil::LogSystem::Init({
        .Name         = "MyApp",
        .LogDir       = "logs",
        .ConsoleLevel = vigil::LogLevel::Info,
    });

    vigil::Info("Application started: version {}.{}", 1, 0);
    vigil::Warn("Config file not found, using defaults.");
    vigil::Error("Failed to connect to {}:{}", "127.0.0.1", 5432);

    VIGIL_LOG_DEBUG("Debug detail: x = {}", 42);
    VIGIL_LOG_INFO("Build number: {:06}", 1337);

    vigil::LogSystem::Shutdown();
}
```

> [!IMPORTANT]
> Calling any logging function before `LogSystem::Init()` triggers a `VIGIL_ASSERT` in debug builds and is a silent no-op in release builds, arguments are never evaluated.

### Log levels

| Level      | Macro                     | Typical use               |
| :--------- | :------------------------ | :------------------------ |
| `Trace`    | `VIGIL_LOG_TRACE(...)`    | Verbose internal flow     |
| `Debug`    | `VIGIL_LOG_DEBUG(...)`    | Developer diagnostics     |
| `Info`     | `VIGIL_LOG_INFO(...)`     | Normal operational events |
| `Warn`     | `VIGIL_LOG_WARN(...)`     | Recoverable anomalies     |
| `Error`    | `VIGIL_LOG_ERROR(...)`    | Operation failures        |
| `Critical` | `VIGIL_LOG_CRITICAL(...)` | System-level failures     |

</details>

<details>
<summary>Named loggers</summary>

<br>

Create per-subsystem loggers that share the main file sink or write to a dedicated file.

```cpp
#include <vigil/vigil.h>

int main()
{
    vigil::LogSystem::Init({
        .Name   = "Engine",
        .LogDir = "logs",
    });

    // Simple named logger — shares the main file sink.
    auto& net = vigil::LogSystem::Create("Network");
    net.Info("Connected to server.");
    net.Warn("Packet loss detected: {}%", 12);

    // Named logger with a dedicated file and custom level.
    vigil::LogSystem::Create({
        .Name      = "Physics",
        .LogDir    = "logs/physics",
        .LogFile   = "physics.log",
        .FileMode  = vigil::FileOpenMode::Truncate,
        .FileLevel = vigil::LogLevel::Debug,
    });

    vigil::LogSystem::Get("Physics").Debug("Simulation step complete.");

    // Log through a named logger via macro.
    VIGIL_LOG_NAMED("Network", vigil::LogLevel::Error, "Disconnected from server.");

    // Find() returns nullptr instead of throwing when a logger does not exist.
    if (auto* audio = vigil::LogSystem::Find("Audio"))
        audio->Info("Audio subsystem ready.");

    vigil::LogSystem::Shutdown();
}
```

### Logger API

| Function                       | Description                                                         |
| :----------------------------- | :------------------------------------------------------------------ |
| `LogSystem::Create(name)`      | Creates or retrieves a named logger sharing the main file sink.     |
| `LogSystem::Create(LogConfig)` | Creates or retrieves a named logger with a dedicated configuration. |
| `LogSystem::Get(name)`         | Returns a named logger; throws if not found.                        |
| `LogSystem::Find(name)`        | Returns a pointer to a named logger, or `nullptr` if not found.     |
| `LogSystem::Remove(name)`      | Removes a named logger and releases its sinks.                      |
| `LogSystem::SetMain(name)`     | Promotes a named logger to replace the main logger.                 |

</details>

<details>
<summary>Rate-limited logging</summary>

<br>

Prevent log spam in high-frequency code paths without manual state management.

```cpp
#include <vigil/vigil.h>
#include <chrono>
#include <thread>

int main()
{
    vigil::LogSystem::Init({ .Name = "App" });

    for (int i = 0; i < 20; ++i)
    {
        // Emitted exactly once for this key, for the lifetime of the process.
        vigil::LogOncePolicy::LogOnce(
            "startup-notice",
            vigil::LogLevel::Warn,
            "Running without a config file.");

        // Emitted at most once every 500 ms.
        vigil::LogTTLPolicy::LogTTL(
            "heartbeat",
            0.5,
            vigil::LogLevel::Info,
            "Service heartbeat OK.");

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    vigil::LogSystem::Shutdown();
}
```

| Policy                   | Parameters                 | Behavior                                        |
| :----------------------- | :------------------------- | :---------------------------------------------- |
| `LogOncePolicy::LogOnce` | string key                 | Logs exactly once per key per process lifetime. |
| `LogTTLPolicy::LogTTL`   | string key + TTL (seconds) | Logs at most once per TTL window.               |

</details>

<details>
<summary>Assertions</summary>

<br>

Assertion macros evaluate conditions and, on failure, log the expression, source location, formatted message, and full stack trace before terminating the process.

```cpp
#include <vigil/vigil.h>

int main()
{
    vigil::LogSystem::Init({ .Name = "App" });

    int  value = 42;
    int* ptr   = &value;

    VIGIL_ASSERT(value > 0);
    VIGIL_ASSERT_MSG(value == 42, "Expected 42, got {}", value);
    VIGIL_ASSERT_NOT_NULL(ptr);
    VIGIL_ASSERT_IN_RANGE(value, 0, 100);

    vigil::LogSystem::Shutdown();
}
```

A failing assertion produces output like:

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

### Assertion macro reference

| Macro                                  | Custom message |  Release behavior   | Use case                                        |
| :------------------------------------- | :------------: | :-----------------: | :---------------------------------------------- |
| `VIGIL_ASSERT(check)`                  |       —        |    Stripped out     | Preconditions with no side-effects.             |
| `VIGIL_ASSERT_MSG(check, ...)`         |       ✓        |    Stripped out     | Preconditions requiring runtime context.        |
| `VIGIL_ASSERT_NOT_NULL(ptr)`           |       —        |    Stripped out     | Null pointer guards.                            |
| `VIGIL_ASSERT_IN_RANGE(val, min, max)` |       —        |    Stripped out     | Bounds validation.                              |
| `VIGIL_VERIFY(check)`                  |       —        | Condition evaluated | Side-effect expressions (e.g. `file.close()`).  |
| `VIGIL_VERIFY_MSG(check, ...)`         |       ✓        | Condition evaluated | Side-effect checks needing failure context.     |
| `VIGIL_UNREACHABLE_ASSERT()`           |       —        |       UB hint       | Unreachable switch defaults or dead code paths. |

> [!NOTE]
> `VIGIL_ASSERT*` macros are active only when `VIGIL_ENABLE_ASSERTS` is defined and are completely stripped in release builds. `VIGIL_VERIFY*` macros **always evaluate their condition** regardless of build type.

</details>

<details>
<summary>Scoped logging</summary>

<br>

Enable with `-DVIGIL_ENABLE_SCOPED_LOG=ON`. Instruments functions and code blocks with automatic entry/exit messages and elapsed time at zero overhead when disabled.

```cpp
#include <vigil/vigil.h>

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

Output:

```text
[trace] >> void LoadAssets()
[trace] >> Parsing manifest
[trace] << Parsing manifest (15 ms)
[trace] >> Uploading textures
[trace] << Uploading textures (20 ms)
[trace] << void LoadAssets() (37 ms)
```

### Macro reference

| Macro                                    | Description                                                            |
| :--------------------------------------- | :--------------------------------------------------------------------- |
| `VIGIL_SCOPED_LOG(name)`                 | RAII scope at `Trace` level.                                           |
| `VIGIL_SCOPED_LOG_LEVEL(name, level)`    | RAII scope at an explicit level.                                       |
| `VIGIL_SCOPED_LOG_FUNCTION()`            | RAII scope using the compiler function signature at `Trace`.           |
| `VIGIL_SCOPED_LOG_FUNCTION_LEVEL(level)` | RAII scope using the compiler function signature at an explicit level. |
| `VIGIL_SCOPE_BEGIN(name)`                | Opens a manual block scope at `Trace` level.                           |
| `VIGIL_SCOPE_BEGIN_LEVEL(name, level)`   | Opens a manual block scope at an explicit level.                       |
| `VIGIL_SCOPE_END()`                      | Closes a manual block scope.                                           |

> [!NOTE]
> When `VIGIL_ENABLE_SCOPED_LOG` is not defined, all macros expand to `((void)0)`. `VIGIL_SCOPE_BEGIN` and `VIGIL_SCOPE_END` expand to bare `{` and `}` to preserve block structure.

</details>

<details>
<summary>Lifecycle hooks</summary>

<br>

Observe logging events without modifying the pipeline — useful for in-app consoles, telemetry, or test assertions.

```cpp
#include <vigil/vigil.h>

int main()
{
    vigil::LogSystem::Init({ .Name = "App" });

    vigil::LogSystem::SetHooks({
        .OnMessage = [](const vigil::LogMessageEvent& e) {
            // Mirror every message to an in-app console, telemetry sink, etc.
        },
        .OnLevelChange = [](const vigil::LevelChangeEvent& e) {
            // React when any logger's severity level changes.
        },
        .OnFlush = [](const vigil::FlushEvent& e) {
            // Notified after any flush operation.
        },
        .OnShutdown = [] {
            // Called at the start of LogSystem::Shutdown().
        },
    });

    vigil::Info("This message is observed by the hook.");
    vigil::LogSystem::ClearHooks();
    vigil::Info("This message is not.");

    vigil::LogSystem::Shutdown();
}
```

### Hook API

| Function                          | Description                                                |
| :-------------------------------- | :--------------------------------------------------------- |
| `LogSystem::SetHooks(LogHooks)`   | Registers all hooks at once, replacing any previously set. |
| `LogSystem::SetOnMessage(cb)`     | Registers a callback for every emitted message.            |
| `LogSystem::SetOnLevelChange(cb)` | Registers a callback for logger level changes.             |
| `LogSystem::SetOnFlush(cb)`       | Registers a callback after any flush.                      |
| `LogSystem::SetOnShutdown(cb)`    | Registers a callback at the start of shutdown.             |
| `LogSystem::ClearHooks()`         | Removes all registered hooks.                              |

</details>

## Examples

Fully worked examples covering every feature are available under [`examples/`](examples/):

| Example        | Source                                                                               | Covers                                                                    |
| :------------- | :----------------------------------------------------------------------------------- | :------------------------------------------------------------------------ |
| Logging        | [`examples/logging/logger_example.cpp`](examples/logging/logger_example.cpp)         | Basic logging, named loggers, rate limiting, level control, hooks, flush. |
| Assertions     | [`examples/diagnostics/assert_example.cpp`](examples/diagnostics/assert_example.cpp) | All assertion macros, safe and intentional-failure cases, CLI dispatch.   |
| Scoped logging | [`examples/logging/scoped_example.cpp`](examples/logging/scoped_example.cpp)         | Function scope, nested scopes, explicit levels, manual BEGIN/END.         |
| Hooks          | [`examples/logging/hooks_example.cpp`](examples/logging/hooks_example.cpp)           | SetHooks, individual setters, ClearHooks, named logger events.            |

> [!NOTE]
> Examples are built with `VIGIL_BUILD_EXAMPLES=ON` and require **C++20** (for designated initializers), even though Vigil itself only requires C++17. They are compiled against whichever Vigil version/commit is checked out, so pin to a release tag to match the API shown above.

## License

Vigil is licensed under the **MIT License**.<br>
See [LICENSE](LICENSE) for more information.
