<div align="center">
  <picture>
    <source media="(prefers-color-scheme: dark)" srcset="docs/images/vigil_logo_light.png">
    <source media="(prefers-color-scheme: light)" srcset="docs/images/vigil_logo_dark.png">
    <img width="320" src="docs/images/vigil_logo_light.png" alt="VIGIL Logo">
  </picture>

  <p>
    <strong>Diagnostics and Logging Tools for C++</strong><br>
    Assertions • Structured Logging • Runtime Diagnostics
  </p>

  <p>
    <img src="https://img.shields.io/badge/License-MIT-green?style=flat&logo=open-source-initiative&logoColor=white" alt="License MIT">
    <img src="https://img.shields.io/badge/Platform-Linux-2d2d2d?style=flat&logo=linux&logoColor=white" alt="Platform Linux">
    <img src="https://img.shields.io/badge/Platform-Windows-0078D6?style=flat&logo=windows&logoColor=white" alt="Platform Windows">
    <img src="https://img.shields.io/badge/Platform-macOS-000000?style=flat&logo=apple&logoColor=white" alt="Platform macOS">
    <img src="https://img.shields.io/badge/Version-0.1.1-purple?style=flat" alt="Version 0.1.1">
  </p>
</div>

## Overview

**Vigil** is a C++ diagnostics library focused on structured logging and runtime error reporting.

It helps applications capture useful diagnostic information, detect failures, and simplify debugging across development and production environments.

> [!IMPORTANT]
> This project is currently in **early development** and is being uploaded to the repository in stages. The API is not yet stable and may change without notice.

## Features

- Structured logging with fmt-style formatting.
- Main logger and named logger model for subsystem-oriented diagnostics.
- Runtime log-level controls for global, console, and file sinks.
- Log spam protection with one-shot and TTL-based rate limiting.
- Assertion macros integrated with source-location capture and logging.
- Optional stack trace support (platform-dependent) for richer diagnostics.
- CMake-first integration: subproject, package install/export, and preset-friendly options.

## Install (CMake)

Vigil is designed to integrate cleanly into modern CMake workflows.

### Option 1: Add as subdirectory

If Vigil lives inside your repository (for example as a git submodule):

```cmake
cmake_minimum_required(VERSION 3.28)
project(MyApp LANGUAGES CXX)

add_subdirectory(vendor/vigil)

add_executable(my_app src/main.cpp)
target_link_libraries(my_app PRIVATE vigil::vigil)
```

### Option 2: FetchContent (recommended for external dependency)

```cmake
cmake_minimum_required(VERSION 3.28)
project(MyApp LANGUAGES CXX)

include(FetchContent)

FetchContent_Declare(
  vigil
  GIT_REPOSITORY https://github.com/DMsuDev/vigil.git
  GIT_TAG        v0.1.1
)

FetchContent_MakeAvailable(vigil)

add_executable(my_app src/main.cpp)
target_link_libraries(my_app PRIVATE vigil::vigil)
```

### Option 3: Use installed package (find_package)

After installing Vigil to your system or custom prefix:

```cmake
cmake_minimum_required(VERSION 3.28)
project(MyApp LANGUAGES CXX)

find_package(Vigil CONFIG REQUIRED)

add_executable(my_app src/main.cpp)
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

Example configure command:

```bash
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DVIGIL_BUILD_TESTS=OFF \
  -DVIGIL_BUILD_EXAMPLES=OFF \
  -DVIGIL_BUILD_SHARED=OFF
```

## Basic Usage

### 1) System logging (main logger)

```cpp
#include <vigil/vigil.h>

int main()
{
  vigil::LogSystemConfig cfg;
  cfg.Name = "MyApp";
  cfg.ConsoleLevel = vigil::LogLevel::Info;

  vigil::LoggerRegistry::Init(cfg);

  VIGIL_INFO("Application started");
  VIGIL_WARN("Running with config profile '{}'", "default");
  VIGIL_ERROR("Failed to connect to {}:{}", "127.0.0.1", 5432);

  vigil::LoggerRegistry::Shutdown();
  return 0;
}
```

### 2) System logging with log limiter

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
  vigil::LoggerRegistry::Init(cfg);

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

  vigil::LoggerRegistry::Shutdown();
  return 0;
}
```

### 3) Basic assert example

```cpp
#include <vigil/vigil.h>

int main()
{
  vigil::LogSystemConfig cfg;
  cfg.Name = "AssertDemo";
  cfg.ConsoleLevel = vigil::LogLevel::Trace;
  vigil::LoggerRegistry::Init(cfg);

  int value = 42;
  int* ptr = &value;

  VIGIL_ASSERT(value == 42, "Expected 42, got {}", value);
  VIGIL_ASSERT_NOT_NULL(ptr);
  VIGIL_ASSERT_IN_RANGE(value, 0, 100);

  vigil::LoggerRegistry::Shutdown();
  return 0;
}
```

`VIGIL_ASSERT` is active when assertions are enabled (`VIGIL_ENABLE_ASSERTS`).
`VIGIL_VERIFY` always evaluates its condition, even when asserts are disabled.

## License

This project is licensed under the **MIT License**.<br>
See the [LICENSE](LICENSE) file for more information.
