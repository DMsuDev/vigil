# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Conventional Commits](https://www.conventionalcommits.org/)
and this project adheres to [Semantic Versioning](https://semver.org/).

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
