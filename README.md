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

</div>

<div align="center">

[![License MIT](https://img.shields.io/github/license/DMsuDev/vigil?style=flat&logo=open-source-initiative&logoColor=white)](https://github.com/DMsuDev/vigil/blob/main/LICENSE)
![Version](https://img.shields.io/github/v/release/DMsuDev/vigil?style=flat&label=Version&color=purple)
[![Linux Build](https://github.com/DMsuDev/vigil/actions/workflows/linux-build.yml/badge.svg)](https://github.com/DMsuDev/vigil/actions/workflows/linux-build.yml)
[![Windows Build](https://github.com/DMsuDev/vigil/actions/workflows/windows-build.yml/badge.svg)](https://github.com/DMsuDev/vigil/actions/workflows/windows-build.yml)
[![macOS Build](https://github.com/DMsuDev/vigil/actions/workflows/macos-build.yml/badge.svg)](https://github.com/DMsuDev/vigil/actions/workflows/macos-build.yml)

</div>

## Overview

**Vigil** is a C++ logging and diagnostics library built around structured logging, assertions, and runtime instrumentation.

It is designed to provide a consistent interface for recording application events, detecting failures, and capturing useful context when debugging. Vigil supports **C++17 and later** on **Linux, Windows, and macOS**.

## Features

- Structured logging with fmt-style formatting.
- Main and named loggers for subsystem-oriented diagnostics.
- Configurable log levels and console/file sinks.
- One-shot and TTL-based rate limiting to prevent log spam.
- Assertion macros with source-location capture and logging.
- Optional stack trace support for richer failure diagnostics.
- RAII scope instrumentation with automatic entry/exit logging and elapsed time.
- CMake integration with subproject, `FetchContent`, and package installation support.

## CMake Integration

Choose the option that best fits your project:

### Option 1: Add as subdirectory

If Vigil lives inside your repository (for example as a git submodule):

```cmake
add_subdirectory(vendor/vigil)
target_link_libraries(my_app PRIVATE vigil::vigil)
```

### Option 2: [`FetchContent`](https://cmake.org/cmake/help/latest/module/FetchContent.html) (recommended for external dependencies)

```cmake
include(FetchContent)

FetchContent_Declare(
  vigil
  GIT_REPOSITORY https://github.com/DMsuDev/vigil.git
  GIT_TAG        # HASH or Tag
)

FetchContent_MakeAvailable(vigil)
target_link_libraries(my_app PRIVATE vigil::vigil)
```

### Option 3: Use installed package (find_package)

After installing Vigil to your system or custom prefix:

```cmake
find_package(Vigil CONFIG REQUIRED)

target_link_libraries(my_app PRIVATE vigil::vigil)
```

If CMake cannot locate `VigilConfig.cmake`, set a prefix path when configuring:

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH="/path/to/vigil/install/prefix"
```

### Useful Vigil CMake options

- `VIGIL_BUILD_EXAMPLES`: Build example targets.
- `VIGIL_BUILD_TESTS`: Build test suite.
- `VIGIL_INSTALL`: Generate install targets and package metadata.
- `VIGIL_USE_SYSTEM_FMT`: Prefer system-installed `fmt` when available.
- `VIGIL_BUILD_SHARED`: Build Vigil as a shared library.
- `VIGIL_ENABLE_STACK_TRACE`: Enable stack trace and symbolication support.
- `VIGIL_ENABLE_SCOPED_LOG`: Enable `VIGIL_SCOPED_LOG` and `VIGIL_SCOPED_LOG_FUNCTION` macros.

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

Initialize the logger and write messages at different log levels.

```cpp
#include <vigil/vigil.h>

int main()
{
  vigil::LogSystemConfig cfg;
  cfg.Name = "MyApp";
  cfg.ConsoleLevel = vigil::LogLevel::Info;

  vigil::LogSystem::Init(cfg);

  VIGIL_INFO("Application started");
  VIGIL_WARN("Running with config profile '{}'", "default");
  VIGIL_ERROR("Failed to connect to {}:{}", "127.0.0.1", 5432);

  vigil::LogSystem::Shutdown();
  return 0;
}
```

</details>

<details>
<summary>Rate-limited logging</summary>

Use this pattern in hot loops or noisy code paths to prevent log flooding.

```cpp
#include <vigil/vigil.h>

#include <chrono>
#include <thread>

int main()
{
  vigil::LogSystemConfig cfg;
  cfg.Name = "LimiterDemo";
  cfg.ConsoleLevel = vigil::LogLevel::Trace;
  vigil::LogSystem::Init(cfg);

  for (int i = 0; i < 10; ++i)
  {
    vigil::LogOncePolicy::LogOnce(
      "startup-warning",
      vigil::LogLevel::Warn,
      "This warning is logged only once."
    );

    vigil::LogTTLPolicy::LogTTL(
      "health-ping",
      1.0,
      vigil::LogLevel::Info,
      "Service heartbeat OK"
    );

    std::this_thread::sleep_for(std::chrono::milliseconds(250));
  }

  vigil::LogSystem::Shutdown();
  return 0;
}
```

</details>

<details>
<summary>Assertions</summary>

```cpp
#include <vigil/vigil.h>

int main()
{
  vigil::LogSystemConfig cfg;
  cfg.Name = "AssertDemo";
  cfg.ConsoleLevel = vigil::LogLevel::Trace;
  vigil::LogSystem::Init(cfg);

  int value = 42;
  int* ptr = &value;

  VIGIL_ASSERT(value == 42, "Expected 42, got {}", value);
  VIGIL_ASSERT_NOT_NULL(ptr);
  VIGIL_ASSERT_IN_RANGE(value, 0, 100);

  vigil::LogSystem::Shutdown();
  return 0;
}
```

`VIGIL_ASSERT` is enabled in `Debug` and `RelWithDebInfo` builds and disabled in `Release` builds.
`VIGIL_VERIFY` always evaluates its condition, regardless of the build configuration.

</details>

<details>
<summary>Scoped logging</summary>

Instrument functions and code blocks with automatic entry/exit trace logging and elapsed time. Enable at configure time with `-DVIGIL_ENABLE_SCOPED_LOG=ON`.

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

int main()
{
    vigil::LogSystemConfig cfg;
    cfg.Name         = "MyApp";
    cfg.ConsoleLevel = vigil::LogLevel::Trace;
    vigil::LogSystem::Init(cfg);

    LoadAssets();

    vigil::LogSystem::Shutdown();
    return 0;
}
```

Output:

```text
[TRACE] >> void LoadAssets()
[TRACE] >> Parsing manifest
[TRACE] << Parsing manifest (15 ms)
[TRACE] >> Uploading textures
[TRACE] << Uploading textures (20 ms)
[TRACE] << void LoadAssets() (37 ms)
```

When `VIGIL_ENABLE_SCOPED_LOG` is disabled, both macros expand to `((void)0)` and incur no runtime overhead.

</details>

## Contributing

Contributions are always welcome! ❤️ Whether you are reporting bugs, fixing issues, adding new examples, or improving the documentation, your help is appreciated.

Before opening a pull request:

- Keep pull requests focused: prefer small, atomic PRs that address a single feature or fix.
- Write clear commit messages using [Conventional Commits](https://www.conventionalcommits.org/).
- Ensure the project builds cleanly without introducing new compiler warnings.

For major changes or new features, please open an issue first to discuss what you would like to change.

## License

Vigil is licensed under the **MIT License**.
See the [LICENSE](LICENSE) file for more information.
