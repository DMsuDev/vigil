# Getting Started

This page takes you from zero to a working Vigil build: requirements, the three ways to integrate it, every CMake option, the available presets, and fixes for common problems.

- [Requirements](#requirements)
- [Adding Vigil to your project](#adding-vigil-to-your-project)
- [Your first program](#your-first-program)
- [CMake options](#cmake-options)
- [Presets](#presets)
- [Troubleshooting](#troubleshooting)

## Requirements

| Item          | Requirement                                                                                                                                           |
| :------------ | :---------------------------------------------------------------------------------------------------------------------------------------------------- |
| Language      | C++17 or newer for the library. The bundled examples use C++20 (designated initializers).                                                             |
| Build system  | CMake 3.28 or newer.                                                                                                                                  |
| Platforms     | Windows, Linux, macOS.                                                                                                                                |
| Compilers     | MSVC, clang-cl, Clang, GCC. CI covers GCC 13 and Clang 18 on Linux, MSVC on Windows, and the default Xcode toolchain on macOS (arm64).                |
| Architectures | x86_64 and ARM64 are validated. x86 (32-bit) is experimental.                                                                                         |
| Dependencies  | `fmt` (part of Vigil's public API), `spdlog` (private, vendored), `libbacktrace` (vendored, POSIX only), a threads library, and `dbghelp` on Windows. |

> [!NOTE]
> On Linux and macOS, `libbacktrace` is configured and built with `configure` and `make` during your build. You need a POSIX shell, `make`, and a C compiler. This only applies when `VIGIL_ENABLE_STACK_TRACE` is `ON` (the default).

## Adding Vigil to your project

Always pin to a **release tag** (or a commit hash). The `main` branch can change at any time.

### Option 1: FetchContent (recommended)

```cmake
include(FetchContent)

FetchContent_Declare(
  vigil
  GIT_REPOSITORY https://github.com/DMsuDev/vigil.git
  GIT_TAG        v0.8.2  # pin to a release tag (use the latest tag available)
  GIT_SHALLOW    TRUE
)

FetchContent_MakeAvailable(vigil)
target_link_libraries(my_app PRIVATE vigil::vigil)
```

For a commit hash instead of a tag, remove `GIT_SHALLOW`: CMake only supports
branch and tag names with shallow Git clones.

### Option 2: Subdirectory

Copy or vendor Vigil into your tree (a Git submodule works well) and add it:

```cmake
add_subdirectory(vendor/vigil)
target_link_libraries(my_app PRIVATE vigil::vigil)
```

When Vigil is consumed through Option 1 or 2 it builds as a subproject: its tests, examples, and strict warnings are `OFF` by default, so your build is not affected. The vendored `fmt` and `spdlog` are used.

### Option 3: Installed package

Build and install Vigil once, then find it from any project:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DVIGIL_INSTALL=ON
cmake --build build --parallel --config Release
cmake --install build --config Release --prefix ./path/to/install
```

```cmake
find_package(Vigil CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE vigil::vigil)
```

If CMake cannot locate `VigilConfig.cmake`, pass the install prefix:

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH="/path/to/install"
```

> [!IMPORTANT]
> `VIGIL_INSTALL=ON` forces `VIGIL_USE_SYSTEM_FMT=ON`, because `fmt` appears in Vigil's public headers. Install `fmt` first (`libfmt-dev` on Debian/Ubuntu, `brew install fmt` on macOS, `vcpkg install fmt` on Windows). Consumers of the installed package need `fmt` to be discoverable too, since `find_package(Vigil)` calls `find_dependency(fmt CONFIG)`.

An install contains:

- the public headers, including the generated `vigil/version.h`
- the `vigil` library and the vendored libraries it links against (`libbacktrace.a` on POSIX)
- the CMake package files under `lib/cmake/Vigil`
- a pkg-config file (`Vigil.pc`) under `lib/pkgconfig`
- the README, LICENSE, and complete `docs/` directory under `share/doc/Vigil`

Pre-built static archives for Linux x64, Windows x64, and macOS arm64 are attached to each [GitHub release](https://github.com/DMsuDev/vigil/releases), together with a `SHA256SUMS.txt` file.

> [!NOTE]
> CMake is the best-supported way to consume Vigil. The `Vigil.pc` file only describes Vigil itself: you must also link `fmt` and the vendored libraries, add `/Zc:preprocessor` on MSVC, and remember that Debug builds produce `vigild` (debug postfix `d`).

## Your first program

```cpp
#include <vigil/vigil.h>

int main()
{
    vigil::LogSystemConfig config;
    config.Name         = "MyApp";
    config.LogDir       = "logs";
    config.ConsoleLevel = vigil::LogLevel::Info;
    vigil::LogSystem::Init(config);

    vigil::Info("Hello from Vigil {}", VIGIL_VERSION_FULL);

    vigil::LogSystem::Shutdown();
}
```

The examples in this documentation sometimes use designated initializers, for example `vigil::LogSystem::Init({ .Name = "MyApp" })`. They require C++20. In C++17 assign the fields of the config struct as shown above.

Running the program prints the message to the console and creates `logs/MyApp.log`. The file is only created once the first message reaches it. Continue with the [User guide](user-guide.md).

## CMake options

Every option is declared in the root `CMakeLists.txt` and is used as `-DOPTION=VALUE`.

### Library

| Option                     | Default                      | Description                                                                          |
| :------------------------- | :--------------------------- | :----------------------------------------------------------------------------------- |
| `VIGIL_BUILD_SHARED`       | value of `BUILD_SHARED_LIBS` | Build Vigil as a shared library instead of a static one.                             |
| `VIGIL_USE_SYSTEM_FMT`     | value of `VIGIL_INSTALL`     | Use a system-installed `fmt` instead of the vendored copy.                           |
| `VIGIL_ENABLE_STACK_TRACE` | `ON`                         | Enable stack trace capture and symbolication (used by assertions and crash reports). |
| `VIGIL_ENABLE_ASSERTS`     | `OFF`, `ON` in Debug builds  | Enable the `VIGIL_ASSERT*` macros. When off they compile to nothing.                 |
| `VIGIL_ENABLE_SCOPED_LOG`  | `OFF`, `ON` in Debug builds  | Enable the RAII scope instrumentation macros.                                        |
| `VIGIL_INSTALL`            | `OFF`                        | Generate the install target and package metadata.                                    |

### Build quality

| Option                     | Default             | Description                                                                             |
| :------------------------- | :------------------ | :-------------------------------------------------------------------------------------- |
| `VIGIL_BUILD_WARNINGS`     | `ON` when top-level | Enable the strict compiler warning suite for Vigil's own targets.                       |
| `VIGIL_WARNINGS_AS_ERRORS` | `OFF`               | Treat warnings as errors. Requires `VIGIL_BUILD_WARNINGS=ON`.                           |
| `VIGIL_ENABLE_CLANG_TIDY`  | `OFF`               | Run clang-tidy during compilation. Needs a Ninja or Makefile generator.                 |
| `VIGIL_ENABLE_COVERAGE`    | `OFF`               | Instrument for code coverage (GCC, Clang, AppleClang). Requires `VIGIL_BUILD_TESTS=ON`. |

### Tests and examples

| Option                        | Default             | Description                                                                         |
| :---------------------------- | :------------------ | :---------------------------------------------------------------------------------- |
| `VIGIL_BUILD_TESTS`           | `ON` when top-level | Build the GoogleTest unit suite.                                                    |
| `VIGIL_BUILD_EXAMPLES`        | `ON` when top-level | Build the example programs.                                                         |
| `VIGIL_CRASH_TESTS`           | `OFF`               | Build the crash integration harness (processes that intentionally die).             |
| `VIGIL_CONSUMER_TESTS`        | `OFF`               | Build the slow consumer tests (`FetchContent`, `add_subdirectory`, `find_package`). |
| `VIGIL_INSTALL_TESTS_CLEANUP` | `ON` when top-level | Delete generated consumer project directories after the consumer tests run.         |

"When top-level" means Vigil is the root project. When Vigil is consumed as a subproject these options default to `OFF`.

### Things worth knowing about these options

- **Debug-dependent defaults.** `VIGIL_ENABLE_ASSERTS` and `VIGIL_ENABLE_SCOPED_LOG` default to `ON` for `Debug` and `RelWithDebInfo`, but only if you did not set them yourself. With a multi-config generator (Visual Studio, Ninja Multi-Config) the default is decided once at configure time and applies to every configuration. Set the option explicitly if you need different behavior per configuration.
- **Settings propagate to consumers.** `VIGIL_ENABLE_STACK_TRACE`, `VIGIL_ENABLE_ASSERTS`, and `VIGIL_ENABLE_SCOPED_LOG` are public compile definitions, so code that links `vigil::vigil` sees the same configuration as the library. Likewise `VIGIL_SHARED_LIBRARY` is defined for consumers of a shared build, and `/Zc:preprocessor` is added for MSVC consumers.
- **Shared libraries.** `BUILD_SHARED_LIBS=ON` is enough to get a shared Vigil, because `VIGIL_BUILD_SHARED` follows it. Public symbols are marked `VIGIL_API`; on GCC and Clang everything else is hidden. On Windows the DLL must sit next to your executable (Vigil's own examples and tests copy it automatically).
- **Compile-time log level.** Define `VIGIL_ACTIVE_LOG_LEVEL` to strip `VIGIL_LOG_*` calls below a severity at compile time: `0` Trace, `1` Debug, `2` Info, `3` Warn, `4` Error, `5` Critical, `6` Off. Example: `target_compile_definitions(my_app PRIVATE VIGIL_ACTIVE_LOG_LEVEL=2)`. The default keeps everything.
- **C++ standard.** Vigil requires C++17 and sets that as the minimum for its own target. Your code picks its own standard, for example `-DCMAKE_CXX_STANDARD=20`.

Example configure command:

```bash
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DVIGIL_BUILD_TESTS=OFF \
  -DVIGIL_BUILD_EXAMPLES=OFF
```

## Presets

`CMakePresets.json` provides ready-made configurations. List them with `cmake --list-presets`.

Preset names follow `<platform>-<family>`, where the platform is `windows-vs`, `windows-ninja`, `linux`, or `macos`.

| Family            | Use it for                                                               |
| :---------------- | :----------------------------------------------------------------------- |
| `dev`             | Day-to-day work: unit tests, examples, assertions, scoped logging.       |
| `ci`              | Same as `dev` with warnings as errors (and clang-tidy on Ninja presets). |
| `package-static`  | Install target with system `fmt`, static library, no tests.              |
| `package-shared`  | Install target with system `fmt`, shared library, no tests.              |
| `consumer-full`   | Consumer and crash tests against an installed package.                   |
| `consumer-source` | Consumer and crash tests without the `find_package` test.                |

`dev`, `ci`, and the `consumer-*` families also have `-debug` and `-release` build and test presets. For example:

```bash
cmake --preset linux-dev
cmake --build --preset linux-dev-debug
ctest --preset linux-dev-debug
```

Notes:

- `windows-vs-*` presets use the Visual Studio generator. `windows-ninja-*` presets use Ninja Multi-Config with `cl`, so run them from a Visual Studio Developer shell.
- Build output goes to `build/<preset>` and installs to `install/<preset>`.
- `base` and the other `cfg-*`, `link-*`, and `plat-*` presets are hidden building blocks.

## Troubleshooting

### CMake cannot find `VigilConfig.cmake`

Pass the install prefix: `-DCMAKE_PREFIX_PATH=/path/to/install`. On Windows with vcpkg also pass the vcpkg toolchain file so `fmt` is found.

### `find_package(Vigil)` fails on `fmt`

The installed package depends on `fmt` and looks for it with `find_dependency(fmt CONFIG)`. Install `fmt` and make sure its prefix is part of `CMAKE_PREFIX_PATH` (or use the vcpkg toolchain file).

### Warning: `VIGIL_USE_SYSTEM_FMT=OFF is incompatible with VIGIL_INSTALL=ON`

Expected. Install builds require a system `fmt`, and Vigil forces the option on. See [Option 3](#option-3-installed-package).

### Macro errors on MSVC (`__VA_OPT__`, variadic macros)

Vigil's macros need the conformant preprocessor. Linking `vigil::vigil` adds `/Zc:preprocessor` automatically. If you do not use the CMake target (for example with pkg-config), add `/Zc:preprocessor` yourself.

### Stack traces show addresses but no file or line

Symbol information is missing from your binary. Compile with debug info (`-g` on GCC and Clang, a PDB on MSVC). On POSIX, `libbacktrace` reads DWARF from the executable itself. Stripped release builds only produce raw addresses or function names.

### Assertions or scoped logging do nothing in my Release build

They are only enabled when `VIGIL_ENABLE_ASSERTS` and `VIGIL_ENABLE_SCOPED_LOG` are on. Enable them explicitly if you want them in Release.

### Nothing is logged, and I see a warning on stderr

Logging was used before `LogSystem::Init()` or after `Shutdown()`. In builds with assertions enabled this is an assertion failure instead. See the [User guide](user-guide.md#before-init-and-after-shutdown).

### Unicode characters look broken in the Windows console

Call `vigil::EnableUTF8Console()` once at startup, before any console I/O.

### `libbacktrace` fails to build on Linux or macOS

The vendored copy is built with `/bin/sh configure` and `make`. Make sure a shell, `make`, and a C compiler are available. The build log is printed when it fails. As a last resort, configure with `-DVIGIL_ENABLE_STACK_TRACE=OFF`.

### Crash or consumer tests fail with an empty build configuration

These tests pass `CMAKE_BUILD_TYPE` to `ctest --build-and-test`, and multi-config generators leave it empty. For these suites configure a single-config generator with an explicit build type, as CI does:

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DVIGIL_CRASH_TESTS=ON
```

### Examples do not compile

The examples use designated initializers and need C++20. They are built that way automatically when `VIGIL_BUILD_EXAMPLES=ON`.

### Still stuck?

Search the [open and closed issues](https://github.com/DMsuDev/vigil/issues), then open a [bug report](https://github.com/DMsuDev/vigil/issues/new?template=bug_report.yml) with your Vigil version, compiler, and CMake version. For open-ended questions use [Discussions](https://github.com/DMsuDev/vigil/discussions).
