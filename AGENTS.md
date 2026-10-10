# AGENTS.md

Notes for AI agents (and humans) changing **Vigil**, a cross-platform C++17 library for logging, assertions, stack traces and crash handling.

The README explains how to *use* Vigil. This file explains how to *change* it without breaking it.

## At a glance

|             |                                                                              |
| :---------- | :--------------------------------------------------------------------------- |
| Language    | C++17 for the library, C++20 for the examples (designated initializers)      |
| Build       | CMake 3.28 or newer, with Ninja or Visual Studio                             |
| Platforms   | Windows (MSVC, clang-cl), Linux (GCC, Clang), macOS (Clang)                  |
| Third party | `spdlog`, `fmt` and `libbacktrace` live in `vendor/`. Don't edit them        |
| `fmt`       | Can come from the system (`VIGIL_USE_SYSTEM_FMT`); install builds require it |
| Ground rule | No new dependencies without asking first                                     |

## Where things live

| Path                    | What's there                                                                      |
| :---------------------- | :-------------------------------------------------------------------------------- |
| `include/vigil/`        | Public headers. `vigil.h` is the umbrella                                         |
| `include/vigil/detail/` | Compiler and platform helpers. Installed only because public headers include them |
| `src/`                  | Implementation and private headers, never installed                               |
| `src/trace/`            | Stack trace capture, with one backend per platform (POSIX and Windows)            |
| `cmake/`                | Build helpers, version loader, vendor modules, package templates                  |
| `tests/unit/`           | GoogleTest suites                                                                 |
| `tests/crash/`          | Crash harness: one trigger per fatal failure                                      |
| `tests/cmake_*/`        | Consumer tests (`add_subdirectory`, `FetchContent`, `find_package`)               |
| `examples/`             | Sample programs, one target each                                                  |
| `VERSION`               | The only place the version is edited. `version.h` is generated from it            |

If you move or add files, update this table in the same change.

## Build and test

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DVIGIL_BUILD_TESTS=ON -DVIGIL_BUILD_EXAMPLES=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Every `VIGIL_*` option is declared in the root `CMakeLists.txt`. Debug and RelWithDebInfo turn asserts and scoped logging on unless you set them yourself.

Presets live in `CMakePresets.json` (`cmake --list-presets` shows them all, including the extra Windows variants):

| Preset family                          | Use it for                                                     |
| :------------------------------------- | :------------------------------------------------------------- |
| `*-dev`                                | Day-to-day work: unit tests, examples, asserts, scoped logging |
| `*-ci`                                 | Same as dev, with warnings as errors                           |
| `*-package-static`, `*-package-shared` | Install target and system `fmt`, no tests                      |
| `*-consumer-full`                      | Consumer and crash tests against an installed package          |
| `*-consumer-source`                    | Same, without the `find_package` test                          |

Run a subset by label: `ctest -L unit`, `ctest -L crash`, `ctest -L install`.

> [!WARNING]
> **Windows gotcha.** The crash and consumer tests (`VIGIL_CRASH_TESTS`, `VIGIL_CONSUMER_TESTS`) pass `CMAKE_BUILD_TYPE` to `ctest --build-and-test`, and the Visual Studio generator leaves it empty. On Windows, configure with Ninja and an explicit build type, for example `-G Ninja -DCMAKE_BUILD_TYPE=Release`.

A change is done when it builds without warnings, `ctest` passes, and any behavior change comes with a test. Say which platforms you couldn't try.

## Architecture rules

- **spdlog stays hidden.** It never appears in public headers. `fmt` is the one exception, because format strings are part of the public API.
- **Public API** lives in `vigil` and is marked `VIGIL_API`. Internals go in `vigil::detail` or an anonymous namespace.
- **Platform and compiler checks** go through `VIGIL_PLATFORM_*`, `VIGIL_COMPILER_*` and the wrappers in `compiler_attributes.h`. Don't use raw `_WIN32`, `__linux__` or compiler attributes.
- **Ownership:** `Scope<T>` by default, `Shared<T>` only when ownership is really shared.
- **Log macros** (`VIGIL_LOG_TRACE`, `_DEBUG`, `_INFO`, `_WARN`, `_ERROR`, `_CRITICAL`) drop disabled levels at compile time without evaluating their arguments. Keep it that way.
- **New public header:** add it to the umbrella if it belongs there, and make sure it gets installed.
- **CMake:** prefer per-target `PRIVATE` settings over global flags. `vigil_version.cmake` has to load before `project()`. A new public dependency also needs an entry in `VigilConfig.cmake.in` and `Vigil.pc.in`.

## Things that look wrong but aren't

Each of these has a reason. Please read it before "fixing" anything.

### Logging

- `g_Mutex` is recursive because the crash handler can re-enter logging on the same thread. A plain mutex would deadlock.
- `OnLevelChange`, `OnFlush` and `OnShutdown` run **after** the lock is released: copy the callback while locked, unlock, then call it. `OnMessage` is different. It runs on the logging thread inside the sink's lock, so keep it fast and never call back into `LogSystem` from it.
- Before `Init()`, the free functions and macros assert in debug builds and print one warning to `stderr` in release. They never throw. `LogSystem::Create`, `Main` and `Get` do throw `std::logic_error`.
- Removing or replacing a logger also has to drop its name in spdlog, or that name can never be reused.
- File sinks are lazy and thread-safe (`_mt`). If the file can't be opened, console logging keeps working.

### Crash handling (`crash_guard.cpp`)

- Everything reachable from `SignalCrash` and the signal handlers must be async-signal-safe: no allocation, locks, logging or `std::string`. The heavy work happens on the watchdog thread.
- `StackTrace::Capture` and `Format` are **not** async-signal-safe. That's why the handler stores raw addresses and the watchdog symbolizes them later.
- `g_State` is leaked on purpose, so a crash during static destruction still finds it alive.
- Every new step in `Report()` has to be bounded and exception-safe. The order is deliberate: log, then report file, then user callback (the least trusted part). `ArmDeadline` kills the process if any of it hangs.
- Threads created by the application call `CrashHandler::InstallThreadAltStack()`. Do the same in examples and tests that spawn threads.

### Assertions

- `VIGIL_ASSERT*` doesn't evaluate its condition when asserts are off. Anything with side effects belongs in `VIGIL_VERIFY*`.
- `assert.h` has a C++17 path and a C++20 path (C++17 avoids empty `fmt::format()` calls, C++20 uses `__VA_OPT__`). Change both or neither, and build under both standards.

### Stack traces

- With `VIGIL_ENABLE_STACK_TRACE` off, the API still compiles and returns empty results.
- Resolution is serialized with a mutex, because neither libbacktrace (as configured) nor DbgHelp is thread-safe.

## Writing tests

- Put new unit tests in `tests/unit/test_*.cpp`. They're discovered automatically, so there's no CMake to edit.
- `LogSystem` is a process-wide singleton. Wrap each test in `vigil::test::ScopedRegistry`: it resets the system, silences the console and sends log files to `test_logs/`.
- Read output with `AttachTestSink`, and assert on message content rather than on spdlog's output pattern.
- `LogOncePolicy` and `LogTTLPolicy` keep global state with no reset, so give every test its own key.
- Death tests call `LogSystem::Shutdown()` first, because spdlog's background threads don't survive the fork. They only run when asserts are enabled.
- New crash trigger: add a header under `tests/crash/project/triggers/`, a branch in `main.cpp`, and an entry in `VIGIL_CRASH_TRIGGERS`. These tests are expected to die (`WILL_FAIL`).
- New example: register it in `examples/CMakeLists.txt`.

## Style

| Element                     | Convention                    | Example                                       |
| :-------------------------- | :---------------------------- | :-------------------------------------------- |
| Types, functions, methods   | `PascalCase`                  | `LogSystem::Init`                             |
| Members                     | `m_` + `PascalCase`           | `m_Impl`                                      |
| Constants                   | `k` + `PascalCase`            | `kMaxLines`                                   |
| File-level globals, statics | `g_` / `s_` prefix            | `g_Mutex`                                     |
| Locals and parameters       | `camelCase`                   | `framesToSkip`                                |
| Macros                      | `VIGIL_UPPER_SNAKE`           | `VIGIL_ASSERT_MSG`                            |
| Struct fields               | Follow the struct's own style | `LogConfig::LogDir`, `StackFrame::symbolName` |

- Everything is in English: code, comments, commits and docs.
- Headers start with `#pragma once`. Project includes use quotes and the full `vigil/...` path.
- `[[nodiscard]]` on getters and factories, `noexcept` where it fits.
- Warnings are strict (`-Wconversion`, `-Wold-style-cast`, `-Wshadow`, `/W4`). Use `static_cast`, and suppress only locally with the `VIGIL_PRAGMA_*` macros.
- Doxygen only for public API that isn't obvious, short and in third person. In `.cpp` files, explain *why*, not *what*.
- Copy the license header from a neighboring file.

## Git and releases

- **Conventional Commits** with a scope: `feat(logging): ...`, `fix(crash): ...`. The types are `feat`, `fix`, `refactor`, `perf`, `build`, `ci`, `test`, `chore`, `docs`, `style` and `revert`.
- `CHANGELOG.md` is generated by git-cliff, so don't edit it. Heads up: `docs` and `style` commits don't reach it (except `docs(api)`), so pick the type with that in mind.
- One logical change per commit, and don't mix refactors with behavior changes.
- Follow `.github/PULL_REQUEST_TEMPLATE.md`, and flag public API changes as possible breaking changes.
- If you change a public signature, update the README and `examples/` too.
- Run `pre-commit run --all-files` before proposing changes.
- Workflows: pin actions by full commit SHA with a version comment, and keep `permissions` minimal.
- Versions: pre-releases are `X.Y.Z-WIP`. CMake also accepts `-dev`, but `pre-release.yml` doesn't. Release tags are `vX.Y.Z`.
