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
[![Integration Build](https://github.com/DMsuDev/vigil/actions/workflows/integration.yml/badge.svg)](https://github.com/DMsuDev/vigil/actions/workflows/integration.yml)

</div>

## Overview

**Vigil** is a C++ logging and diagnostics library built around structured logging, assertions, stack traces, and runtime instrumentation. It provides a consistent interface for recording application events, detecting failures early, and capturing rich context when things go wrong.

Supports **C++17 and later** on **Linux**, **Windows**, and **macOS**.

## Features

- **Structured logging:** `fmt`-style messages, compile-time filtering, a main logger plus named loggers, console and rotating file sinks, optional asynchronous output. [Learn more](docs/user-guide.md#logging)
- **Lifecycle hooks:** event callbacks for mirroring messages to in-app consoles, telemetry, or test assertions. [Learn more](docs/user-guide.md#lifecycle-hooks)
- **Rate limiting:** one-shot (`LogOncePolicy`) and TTL-based (`LogTTLPolicy`) policies for high-frequency code paths. [Learn more](docs/user-guide.md#rate-limited-logging)
- **Assertions:** rich assertions with source location, formatted messages, and a full stack trace on failure. [Learn more](docs/user-guide.md#assertions)
- **Scoped instrumentation:** optional RAII entry/exit logging with elapsed-time measurement. [Learn more](docs/user-guide.md#scoped-logging)
- **Crash handling:** intercepts fatal signals and unhandled exceptions, then logs a stack trace and writes a crash report before the process exits. [Learn more](docs/user-guide.md#crash-handling)

## Stability notice

> [!IMPORTANT]
> Always pin to a **release tag** when integrating Vigil. The `main` branch is the active development branch and may contain breaking changes, incomplete features, or unstable APIs at any point.
>
> ```cmake
> # Correct: pin to a release tag
> FetchContent_Declare(vigil GIT_REPOSITORY https://github.com/DMsuDev/vigil.git GIT_TAG v0.8.2)
>
> # Avoid: main is not guaranteed to be stable
> FetchContent_Declare(vigil GIT_REPOSITORY https://github.com/DMsuDev/vigil.git GIT_TAG main)
> ```

## Quick start

### 1. Add Vigil to your project

```cmake
include(FetchContent)

FetchContent_Declare(
  vigil
  GIT_REPOSITORY https://github.com/DMsuDev/vigil.git
  GIT_TAG        v0.8.2  # pin to a release tag
  GIT_SHALLOW    TRUE
)

FetchContent_MakeAvailable(vigil)
target_link_libraries(my_app PRIVATE vigil::vigil)
```

`add_subdirectory()` and installed-package (`find_package`) setups are covered in [Getting started](docs/getting-started.md#adding-vigil-to-your-project).

### 2. Log, assert, shut down

```cpp
#include <vigil/vigil.h>

int main()
{
    vigil::LogSystemConfig config;
    config.Name   = "MyApp";
    config.LogDir = "logs";
    vigil::LogSystem::Init(config);

    vigil::Info("Application started: version {}.{}", 1, 0);
    vigil::Warn("Config file not found, using defaults.");

    int value = 42;
    VIGIL_ASSERT_MSG(value > 0, "Expected a positive value, got {}", value);

    vigil::LogSystem::Shutdown();
}
```

> [!TIP]
> Call `LogSystem::Init()` once at startup and `LogSystem::Shutdown()` before exit. Logging before `Init()` is dropped (and asserts in builds with assertions enabled). See [Before Init and after Shutdown](docs/user-guide.md#before-init-and-after-shutdown).

## Documentation

| Document                                   | What you will find                                                                          |
| :----------------------------------------- | :------------------------------------------------------------------------------------------ |
| [Getting started](docs/getting-started.md) | Requirements, installation, CMake options and presets, troubleshooting                      |
| [User guide](docs/user-guide.md)           | Logging, hooks, rate limiting, assertions, scoped logging, stack traces, crash handling     |
| [Architecture](docs/architecture.md)       | How Vigil is built: layers, threading model, crash pipeline, build system, design decisions |
| [Development](docs/development.md)         | Building, testing, CI, code style, commits, and releases                                    |
| [Changelog](CHANGELOG.md)                  | Release history                                                                             |

## Examples

Fully worked examples covering every feature are available under [`examples/`](https://github.com/DMsuDev/vigil/tree/main/examples):

| Example        | Source                                                                    | Covers                                      |
| :------------- | :------------------------------------------------------------------------ | :------------------------------------------ |
| Logging        | [`logger_example.cpp`](examples/logging/logger_example.cpp)               | Basic configuration, rate limits and levels |
| Assertions     | [`assert_example.cpp`](examples/diagnostics/assert_example.cpp)           | Assertion macros, failures and CLI dispatch |
| Scoped logging | [`scoped_example.cpp`](examples/logging/scoped_example.cpp)               | Function scopes, nested and manual markers  |
| Hooks          | [`hooks_example.cpp`](examples/logging/hooks_example.cpp)                 | Hook configuration and cleanup, events      |
| Crash handling | [`crash_guard_example.cpp`](examples/diagnostics/crash_guard_example.cpp) | Exception capture, OS signals setup         |

> [!NOTE]
> Examples are built with `VIGIL_BUILD_EXAMPLES=ON` and require **C++20** (for designated initializers), even though Vigil itself only requires C++17. They are compiled against whichever Vigil version or commit is checked out, so pin to a release tag to match the API shown in the documentation.

## Contributing

Contributions are welcome! Please read [CONTRIBUTING.md](https://github.com/DMsuDev/vigil/blob/main/CONTRIBUTING.md) before submitting changes.

For AI-assisted contributions, see [AGENTS.md](https://github.com/DMsuDev/vigil/blob/main/AGENTS.md).

## License

Vigil is licensed under the **MIT License**.<br>
See [LICENSE](LICENSE) for more information.
