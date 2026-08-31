# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Conventional Commits](https://www.conventionalcommits.org/)
and this project adheres to [Semantic Versioning](https://semver.org/).

## [0.5.0] - 2026-08-31

### 🚀 Features

- Add initialization assertions and auto-create named loggers ([c809843](https://github.com/DMsuDev/Vigil/commit/c809843e81a67cf2a7aca927adcebafbe186d1a4))

  - Integrate `VIGIL_ASSERT_MSG` checks across free logging functions and
    the `VIGIL_LOG_NAMED` macro to enforce `LogSystem` initialization in Debug builds.
  - Ensure silent early return in Release builds if logging APIs are called
    prior to `LogSystem::Init()`.
  - Update `VIGIL_LOG_NAMED` to automatically create non-existent named loggers
    on demand via `LogSystem::Create()`.

- Add lifecycle hooks and per-logger level overrides ([92cfd69](https://github.com/DMsuDev/Vigil/commit/92cfd69a84b5f537e1376a7df9c13c3c3d368aa0))

  - Add `LogHooks` with event structures: `LogMessageEvent`, `LevelChangeEvent`, `FlushEvent`
  - Add `HooksRegistry` for thread-safe hook management with automatic propagation to new loggers
  - Add `SetLevel` for per-logger level overrides without affecting global state
  - Expose `SetHooks/ClearHooks` on `LogSystem`
  - Invoke callbacks outside mutex bounds to prevent re-entrant deadlocks

- Add demo for `LogSystem` hooks usage ([a471ea9](https://github.com/DMsuDev/Vigil/commit/a471ea95d2aa512d2fd98da8406fc2b88b44becf))

- Add manual scope macros and document ScopedLogger API ([d679169](https://github.com/DMsuDev/Vigil/commit/d6791694b7a81053dc28b909037de88f64ef540d))

  Expand `scoped_logger.h` with explicit level and manual block scoping macros,
  and complete the Doxygen API reference across the header.

  - Add `VIGIL_SCOPE_BEGIN` and `VIGIL_SCOPE_END` for manual block scope tracking.
  - Add explicit-level scope macros (`VIGIL_SCOPE_LOG_*`) to bypass default info level.
  - Apply `@hideinitializer` to macro definitions to suppress internal expansion clutter in Doxygen.
  - Document `stderr` fallback behavior when `LogSystem` is uninitialized or offline.

- Add `LogDir` configuration and refactor internal path handling ([10389e9](https://github.com/DMsuDev/Vigil/commit/10389e9507d2c0b8652cfb28d3cddaf7dc15691c))

  Introduce configurable log output directories across `LogSystemConfig` and `LogConfig`,
  and refactor `log_system.cpp` helper organization and path resolution.

  - Add `LogDir` field to `LogSystemConfig` and `LogConfig` for explicit directory targeting.
  - Implement `ResolveLogPath` using `std::filesystem` to handle path joining and automatic directory creation.
  - Propagate global `LogDir` inheritance to custom loggers when their local `LogDir` is empty.
  - Refactor internal path helpers and sink factory signatures to take strongly-typed `std::filesystem::path`.
  - Reorganize internal anonymous namespaces and relocate `Create()` definitions

- Add debugger detection and conditional breakpoint support for `compiler_attributes.h` ([a50007f](https://github.com/DMsuDev/Vigil/commit/a50007ff29200c900411c3a414dab708ccba8f4a))

  Add `IsDebuggerAttached()` utility to query process tracing state across
  Windows, Linux, and macOS. Introduce `VIGIL_DEBUGBREAK_IF_ATTACHED()` macro
  to conditionally trigger breakpoints only when an active debugger is present.

  Suppress MSVC CRT abort dialogs in `assert.cpp` during standard runs to prevent
  blocking UI prompts upon failure.

### 🐛 Bug Fixes

- Normalize path separators and clean macro expression formatting ([20ab227](https://github.com/DMsuDev/Vigil/commit/20ab2273cd3066cf310f202cfa1163e59a328aef))

  - Normalize file paths using '/' separators before trimming in `FormatFilePath`,
    ensuring consistent multi-platform (Windows/Linux) location formatting.
  - Move `VIGIL_STRINGIFY` execution to outer assert macros to avoid extra
    parentheses around assertion expressions.
  - Harmonize stderr fallback formatting with the primary LogSystem critical report.

### 🚜 Refactor

- Refactor logger_example into modular demos with hooks ([f2fa173](https://github.com/DMsuDev/Vigil/commit/f2fa17332c6b6503ec0b842de94a7e3ad5a7ff14))

  Refactors logger_example.cpp into modular demo functions with clear section
  banners and isolated Init/Shutdown cycles.

  - Structure code into modular `Demo_*` functions.
  - Add `Demo_LifecycleHooks` covering `SetHooks` and event handling.
  - Modernize struct initialization with C++20 designated initializers.

- Modularize ScopedLogger demonstration suite ([8c7245b](https://github.com/DMsuDev/Vigil/commit/8c7245bc3b59352e2661cd520557e75137515116))

  Restructure `scoped_example.cpp` to isolate individual `ScopedLogger` features
  into dedicated demonstration functions and improve execution flow clarity.

  - Split monolithic example into modular functions (`Demo_FunctionScope`, `Demo_ManualScope`, etc.).
  - Add explicit lifecycle control (`LogSystem::Init`/`Shutdown`) per demo block.
  - Add coverage for manual `VIGIL_SCOPE_BEGIN` / `VIGIL_SCOPE_END` nesting.

- Optimize `IsRuntimeFrame` with unordered_set and clean up `Capture function` ([c4443b1](https://github.com/DMsuDev/Vigil/commit/c4443b11b76168cb8cfeae2c855b2ef4d7782156))

- Streamline example runners and assert flag dispatching ([eecc422](https://github.com/DMsuDev/Vigil/commit/eecc422d336a9a46059879e15094789808f66419))

  - Introduce VIGIL_EXAMPLE_INIT helper macros across examples to reduce LogSystem initialization boilerplate and standardize log output directory structure.
  - Standardize function naming in assert_example to Snake_Case.
  - Refactor argument parsing in assert_example using a structured dispatch table with terminal-flag metadata.
  - Add pre-execution CLI flag validation and warnings for conflicting terminal options.
  - Clean up main entrypoint error handling across example targets.

- Move `FileOpenMode` enum to `log_system.h` and remove `file_mode.h` ([4d0c678](https://github.com/DMsuDev/Vigil/commit/4d0c67877695b5e694e225898641693aa3e17822))

- Unify stack trace backend headers and expand inline frames ([e6f648e](https://github.com/DMsuDev/Vigil/commit/e6f648e6e6f1949a64dd7f9ab1232fe30112423e))

  Consolidate the platform-specific detail headers into a single internal backend header and implement inline frame expansion support for Windows and POSIX trace backends.

  - Rename `stack_trace_win.h` and delete `stack_trace_posix.h` in favor of a unified `stack_trace_backend.h` internal header selected per-platform at build time.
  - Implement inline frame resolution on Windows via `SymAddrIncludeInlineTrace` and `SymQueryInlineTrace` (DbgHelp 6.x / Win8+).
  - Support inline frame tracking on POSIX targets within `libbacktrace` callbacks and add fallback address resolution using `backtrace_symbols()`.
  - Introduce `SimplifySymbol` in `StackTrace::Format` to clean up demangled function names (lambda operators and anonymous namespaces).
  - Update internal frame bounds, pointer parameter constness, and documentation formatting across the trace subsystem.

### 🛠️ Build System

- Add hooks_example to CMakeLists and ensure C++23 standard ([0624ff4](https://github.com/DMsuDev/Vigil/commit/0624ff488dd9a21fd3475fba3614e085f7c68784))

## [0.4.1] - 2026-08-24

### 🐛 Bug Fixes

- Resolve C++17 variadic pack and VERIFY stringify issues in Assert.h ([d5e24dc](https://github.com/DMsuDev/Vigil/commit/d5e24dcd76dffcf8781b807b319b7a27e1236643))

  In C++17, `VIGIL_ASSERT` expanded through `VIGIL_ASSERT_MSG` into
  `VIGIL_INTERNAL_ASSERT_IMPL` with an empty `__VA_ARGS__` pack, producing
  an ill-formed `fmt::format()` call in some fmt versions.

  `VIGIL_VERIFY` was chained through `VIGIL_ASSERT`, causing `VIGIL_STRINGIFY`
  to capture the asserted expression wrapped in an extra layer of parentheses,
  resulting in `(expr)` instead of `expr` in assertion log output.

  - Add `VIGIL_INTERNAL_ASSERT_IMPL_NO_MSG` for C++17 no-message paths,
    invoking `ReportAssertFailure` directly without a variadic argument.
  - Route `VIGIL_ASSERT` (C++17) through `_NO_MSG` to avoid an empty
    `fmt::format()` call entirely.
  - Redefine `VIGIL_VERIFY` independently of `VIGIL_ASSERT` to eliminate
    the double-wrapping that corrupted `VIGIL_STRINGIFY` output.
  - C++20 path is unchanged; `__VA_OPT__` handles the empty pack case
    cleanly without requiring a separate macro variant.

- Improve early return logging in DemoEarlyReturn function ([352200e](https://github.com/DMsuDev/Vigil/commit/352200e677f2dd5f15ff8d8f94c9b84d0f789529))

### 🚜 Refactor

- Remove VIGIL_API export macro from ScopedLogger ([d9a3d13](https://github.com/DMsuDev/Vigil/commit/d9a3d131e16421aedaec3417949d8274d751c5bd))

  - Prevent MSVC C4251 warning caused by exporting STL members in DLL boundary
  - ScopedLogger is fully inline/RAII and does not require binary DLL symbol export

- Remove compiler-specific diagnostic pragmas from logger_impl.h ([f4c9614](https://github.com/DMsuDev/Vigil/commit/f4c961410af324ea43127ef4b09e3b79da911ec3))

- Add portable warning control macros in `compiler_attributes.h`. ([4adb8db](https://github.com/DMsuDev/Vigil/commit/4adb8db14ae19916c2ee6563c527592b52d3408f))

  Replace raw `#pragma` blocks throughout the codebase with a unified
  warning suppression API. Migrate `log_level.h` as first consumer.

  - Add `VIGIL_INTERNAL_PRAGMA` and `VIGIL_INTERNAL_DIAG` as internal helpers.
  - Add `VIGIL_PRAGMA_PUSH_WARNING` / `VIGIL_PRAGMA_POP_WARNING` and the `VIGIL_DISABLE_WARNING_*` family
  - Add `VIGIL_PRAGMA_PUSH_WARNING_MSVC_SILENT` (MSVC-only, `warning(push,0)`) in `compiler_attributes.h`.
  - Migrate `log_level.h` from raw `#pragma` blocks to the new macros.

### 🛠️ Build System

- Fix warning policy propagation and adopt native warning-as-error ([6242fd9](https://github.com/DMsuDev/Vigil/commit/6242fd9509442c284160dfd21a27baaa20a73cdc))

  - Ensure early return when VIGIL_BUILD_WARNINGS is disabled regardless of top-level status
  - Enable -Wdocumentation diagnostic for Clang toolchains
  - Replace manual compiler flags (/WX, -Werror) with CMake 3.24+ COMPILE_WARNING_AS_ERROR property

## [0.4.0] - 2026-08-22

### 🚀 Features

- Add security policy and vulnerability reporting guidelines ([c7f5a88](https://github.com/DMsuDev/Vigil/commit/c7f5a88dae92215ed7324b61df27e106f02be86e))

- Add issue templates for bug reports and feature requests ([372f45e](https://github.com/DMsuDev/Vigil/commit/372f45e38463580f4a10e6f943d2043f92136561))

- Add `VIGIL_ASSERT_MSG` and fix C++17 variadic expansion ([115c9dc](https://github.com/DMsuDev/Vigil/commit/115c9dcfc689a628d940462e6c8cc3f9616144d0))

  - Add explicit `VIGIL_ASSERT_MSG` and `VIGIL_VERIFY_MSG` macros for formatted assertions.
  - Remove unused `VIGIL_INTERNAL_ASSERT_HAS_MSG` macro helpers.
  - Ensure C++17 macro expansion correctly passes a single formatted `std::string_view` to `ReportAssertFailure`.
  - Update `assert_example.cpp` to match new macro signatures and adjust range check types.

### 🐛 Bug Fixes

- Cast VIGIL_ACTIVE_LOG_LEVEL to uint8_t in IsLevelActive ([8663bab](https://github.com/DMsuDev/Vigil/commit/8663bab30ff5ffd89ce6afa7ee60e9816f0fb52e))

- Centralize log level gating in VIGIL_LOG_NAMED ([bffcf1b](https://github.com/DMsuDev/Vigil/commit/bffcf1b36aef7e767c4dc9423391822d1353f455))

  - Update VIGIL_LOG_NAMED to use IsLevelActive instead of raw level comparison.
  - Suppress -Wtype-limits / C4296 warnings when comparing LogLevel against compile-time gates.

### 🚜 Refactor

- Relocate symbol visibility flags and remove redundant PIC setup ([c87ccbe](https://github.com/DMsuDev/Vigil/commit/c87ccbea51c21804e0fad0bf3bc62a4492710bd2))

  - Move -fvisibility flags to src target configuration where API visibility is handled
  - Remove unnecessary global `POSITION_INDEPENDENT_CODE` target property

- Refactor macros and clean up scoped logger dependencies ([fa5b263](https://github.com/DMsuDev/Vigil/commit/fa5b26389aa22f12151e50ded9f9eff9bd69aad2))

  Relocate signature detection and concatenation macros to their dedicated detail headers,
  eliminating redundant local macro definitions and improving internal architectural layer separation.

  - Move `VIGIL_CURRENT_FUNCTION` from `platform_detection.h` to `compiler_attributes.h`.
  - Move `VIGIL_CONCAT` and `VIGIL_CONCAT_IMPL` from `scoped_logger.h` to `preprocessor_utils.h`.
  - Update `source_location.h` and `scoped_logger.h` includes to directly target the proper headers for attributes and preprocessor tools.
  - Replace local `VIGIL_FUNC_SIG` references in `scoped_logger.h` with `VIGIL_CURRENT_FUNCTION`.

### 🛠️ Build System

- Silence `libbacktrace` vendor output and log on failure ([ebc049e](https://github.com/DMsuDev/Vigil/commit/ebc049eb8724b2d1ef65735fbc9cdd310c0d76c7))

### 🔧 Maintenance

- Add FUNDING.yml to specify GitHub funding information ([630c4d3](https://github.com/DMsuDev/Vigil/commit/630c4d31644231d6249c5a99cef7a8977e42e9c2))

## [0.3.1] - 2026-08-11

### 🐛 Bug Fixes

- Properly install vendored `libbacktrace` ([a263791](https://github.com/DMsuDev/Vigil/commit/a263791f0669db432b631065b80336f16bed68e5))

  - Install libbacktrace alongside Vigil on POSIX platforms
  - Fix installed libbacktrace path in VigilConfig.cmake
  - Removed `stack_trace` headers from `VIGIL_INTERNAL_HEADERS`

- Update CMake command syntax for Windows to use PowerShell style ([ae6845c](https://github.com/DMsuDev/Vigil/commit/ae6845c09dcb1b05169c140ac6105e9d36a2ae42))

### ◀️ Reverts

- Enforce system fmt usage when installing Vigil ([c8ec327](https://github.com/DMsuDev/Vigil/commit/c8ec32740eb620be33b4817a9e7b98a635ccc84f))

- Install fmt library for Linux, macOS, and Windows in CI workflow ([678328f](https://github.com/DMsuDev/Vigil/commit/678328fce21e4a0ab36752a8aa5cacc9df4bf4ad))

## [0.3.0] - 2026-08-11

### 🚀 Features

- Add `ScopedLogger` RAII utility and scoped log macros ([5d5e0e6](https://github.com/DMsuDev/Vigil/commit/5d5e0e6e881e3d09f7bb0efd4102d7f9fbfddf02))

- Add `scoped_example` to demonstrate ScopedLogger functionality ([90c9692](https://github.com/DMsuDev/Vigil/commit/90c96923754cd63a3bd7c3072f99f27573c9f684))

- Add unit tests for `ScopedLogger` functionality ([7a24f1c](https://github.com/DMsuDev/Vigil/commit/7a24f1cf45d7917d2b9130a9ea7e187789f7b10c))

- Improve POSIX and Windows stack trace capture with libbacktrace and DbgHelp ([2c31c97](https://github.com/DMsuDev/Vigil/commit/2c31c97ed1e7df35e1e205218a8cf48316863002))

  - Added stack trace capture functionality for POSIX platforms using libbacktrace.
  - Implemented stack trace capture for Windows using DbgHelp.
  - Introduced CaptureFromAddresses method for symbolication of raw addresses.
  - Enhanced StackFrame structure to include column information and inlined frame detection.
  - Updated documentation for stack trace methods and usage examples.

### 🐛 Bug Fixes

- Fix Autotools build commands on POSIX ([2e04d8d](https://github.com/DMsuDev/Vigil/commit/2e04d8df7470ad365952cb9cfd272c0a0337ef59))

- Update file permissions for configure and install-sh scripts ([e054f2e](https://github.com/DMsuDev/Vigil/commit/e054f2e7528bef1b138b0889205beb40bf8b1449))

- Reorganize badge display for better visibility ([80d9fcd](https://github.com/DMsuDev/Vigil/commit/80d9fcdb8e330fcd615cff0dc213f76290b44f14))

### 🚜 Refactor

- Publish assets to existing GitHub releases ([016019a](https://github.com/DMsuDev/Vigil/commit/016019a8dd877c98d73eb1bf36b6d2ad090bfca0))

  - Trigger the workflow on published releases instead of tag pushes
  - Remove unnecessary fmt system dependency installation since `VIGIL_USE_SYSTEM_FMT` is disabled

- **BREAKING:** Replace `LoggerRegistry` with `LogSystem` ([7fca087](https://github.com/DMsuDev/Vigil/commit/7fca087557eee6ce6726cdda1be302f21eb9d497))

  - Reworks the logging API around `LogSystem` and updates its consumers and tests accordingly.
  - Renames the registry implementation and related test files to reflect the new architecture.

### ⚡ Performance

- Update CleanFunctionSignature calling conventions ([7502e60](https://github.com/DMsuDev/Vigil/commit/7502e604a2a74a499c63aa0532208115f07dc575))

  - Adjust calling conventions used by `CleanFunctionSignature`
  - Improve performance of function signature processing
  - Preserve existing signature-cleaning behavior
  - Enable `VIGIL_ENABLE_SCOPED_LOG` by default in the `base-dev` CMake preset

### 🛠️ Build System

- Add `libbacktrace` dependency for POSIX ([fd5a7c1](https://github.com/DMsuDev/Vigil/commit/fd5a7c1614897c3ac6ee460742fe3351b47e3c63))

### 🔧 Maintenance

- Remove OpenSSF Scorecard workflow file ([6921bbb](https://github.com/DMsuDev/Vigil/commit/6921bbb05fa39d48cca9e2972277551fb9d4ce70))

- Restrict architecture matrix to arm64 only ([ea2f28f](https://github.com/DMsuDev/Vigil/commit/ea2f28fcbaebbedc975fbe22bf0a84f3ea813b6a))

### ◀️ Reverts

- Remove unsupported installation check for vendored fmt ([5f701a4](https://github.com/DMsuDev/Vigil/commit/5f701a4526393d6f442bf426c7ed7f02dba7a1fd))

## [0.1.3] - 2026-08-07

### 🐛 Bug Fixes

- Specify shell for build and install steps in publish-release workflow ([afeffe9](https://github.com/DMsuDev/Vigil/commit/afeffe9a752cdcfef812677a87f30a2348bfdaca))

- Add missing permissions for scorecard job in OpenSSF workflow ([30651e6](https://github.com/DMsuDev/Vigil/commit/30651e6e1c50d26ebcfecea113e643ae54d500a7))

- Refine dependency management condition for system defaults ([c97e38f](https://github.com/DMsuDev/Vigil/commit/c97e38fa8f57b348cafe491f12dd1340f1d22f7e))

## [0.1.2] - 2026-08-07

### 🐛 Bug Fixes

- Add Threads package dependency and update header file inclusion ([412b2f5](https://github.com/DMsuDev/Vigil/commit/412b2f5826c7c50d5b6026d915e7c9427fa7a60c))

- Update Windows dependency installation on workflow and improve CMake configuration with dynamic checkout reference for version input ([c7e1913](https://github.com/DMsuDev/Vigil/commit/c7e19133cfb266d7425a1fd1fc36ad81808a5cb5))

### 🚜 Refactor

- Remove unused header includes ([85aa90c](https://github.com/DMsuDev/Vigil/commit/85aa90c63280745a84f9264e0e62af47caf9f901))

### 🛠️ Build System

- Add internal headers and update fmt dependency handling ([731bdfb](https://github.com/DMsuDev/Vigil/commit/731bdfb8e1cfab78d751e96ca29c5015725330aa))

- Add installation rules for spdlog target ([079900b](https://github.com/DMsuDev/Vigil/commit/079900b5a64368c4113598e20f6030657934a2b9))

- Skip bump version commits in changelog ([0a192db](https://github.com/DMsuDev/Vigil/commit/0a192db2b49455cc319bffe0c842b5a5d63bdf30))

### 🔧 Maintenance

- Remove spdlog installation from dependency setup ([9c19202](https://github.com/DMsuDev/Vigil/commit/9c1920244fc25af37a338eadb10a4866e41e0e82))

- Refactor CMake commands for improved readability and add VIGIL_INSTALL=OFF ([76687f1](https://github.com/DMsuDev/Vigil/commit/76687f115b7f4d3c48cc2ad9b301a4e662cb5029))

- Update Windows build script for fix error ([ba876d1](https://github.com/DMsuDev/Vigil/commit/ba876d1c81a2bdec6ccac8ee287ce265cfaf43b4))

- Add workflow_dispatch input for release version ([780ea29](https://github.com/DMsuDev/Vigil/commit/780ea292e615396fb3afff0eaef93ec9855894f2))

### ◀️ Reverts

- Remove spdlog dependency from VigilConfig ([52abc01](https://github.com/DMsuDev/Vigil/commit/52abc01af0b833474f80c58430e0d800a1887b8f))

## [0.1.1] - 2026-08-06

### 🐛 Bug Fixes

- Improve dependency resolution for fmt and spdlog libraries ([922106b](https://github.com/DMsuDev/Vigil/commit/922106b139379a0dc73678243967cc2a7056ded9))

- Add CMAKE_PREFIX_PATH to build configuration ([69532c3](https://github.com/DMsuDev/Vigil/commit/69532c395f1a2d9171927a65a790976d651d08d6))

### 🔧 Maintenance

- Remove scheduled trigger from OpenSSF Scorecard workflow ([cc77edc](https://github.com/DMsuDev/Vigil/commit/cc77edc719cffbeca309a82dbe2a4c7193e2f9f4))

## [0.1.0] - 2026-08-06

### 🏗️ Project Setup

- Bootstrap project structure ([ce84d3c](https://github.com/DMsuDev/Vigil/commit/ce84d3cceb881c83894e732e71c4e2188ad9c84b))

  Initialize the Vigil repository layout, CMake build system, and
  development environment.

  This commit includes:
  - CMake build system, presets, and package installation rules
  - Formatting, linting, and pre-commit tooling
  - Vendored third-party dependencies (fmt, spdlog)
  - Initial CI workflows and repository metadata

  No core library features are included in this commit.

### 🚀 Features

- Introduce compiler abstraction and core utilities ([3a77264](https://github.com/DMsuDev/Vigil/commit/3a772645e5f44a435805d1173b08bd013de6973a))

  Establish the foundational infrastructure for Vigil by introducing
  cross-platform compiler detection, attribute abstractions, and common
  core utilities.

  - detail: add platform detection, compiler feature detection, attribute
    abstractions, preprocessor helpers, and symbol visibility macros.
  - core: add smart pointer aliases, source_location wrapper, and file
    open mode definitions.

- Implement core logger system and spdlog wrapper ([2afb860](https://github.com/DMsuDev/Vigil/commit/2afb8600ed7fb0718d6f243b6894b96a3396cbde))

  - Added Logger and LoggerRegistry APIs to manage logging functionality.
  - Implement the spdlog-based logging backend
  - Introduce log levels and file modes

- Add log limiting policies to prevent log spam ([35c4769](https://github.com/DMsuDev/Vigil/commit/35c476984e25995b8d93affe9f442408904094df))

- Add UTF-8 console configuration utilities for Windows ([90e82b1](https://github.com/DMsuDev/Vigil/commit/90e82b1754abe5692b20d43b2954fdb2f28b06c6))

- Add assertion macros with logging and source-location capture ([2777a49](https://github.com/DMsuDev/Vigil/commit/2777a49881840fccb7df40b9ddf13c8bae662c38))

- Replace meta example with logging and assertion examples ([6c0d413](https://github.com/DMsuDev/Vigil/commit/6c0d4131c617f769bf4654b2a30846296fba9e3e))

- Enhance CMake presets for development and packaging configurations ([f330b33](https://github.com/DMsuDev/Vigil/commit/f330b33f573ed74e02047540214e96452ae735f7))

### 🚜 Refactor

- Add internal path formatting and signature cleaning helpers ([5064d83](https://github.com/DMsuDev/Vigil/commit/5064d839e3e3f9670742284a4cccefce8a5aeba6))

### 🛠️ Build System

- Add runtime dependency helper on Windows and update cliff config ([3f4bd95](https://github.com/DMsuDev/Vigil/commit/3f4bd954b325f22bae285a045f038bd29c02f19a))

### 🔧 Maintenance

- Add unit tests for logging functionality and test suite configuration ([a424ba3](https://github.com/DMsuDev/Vigil/commit/a424ba327dfe1a5e70fdbfea8210a640b8d9adbd))

- Add unit tests for assertion macros with logging integration ([4af832c](https://github.com/DMsuDev/Vigil/commit/4af832cf235e372fe027f85dcf8fe23a9e9d4bff))

- Add CI workflows for Linux, macOS, Windows builds and security analysis ([c4fbfb8](https://github.com/DMsuDev/Vigil/commit/c4fbfb8071718d8c36e6eadfb4c392b0ac552c37))

<!-- generated by git-cliff -->
