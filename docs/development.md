# Development

Everything you need to work on Vigil day to day: building, testing, static analysis, code style, commits, continuous integration, and releases. For the internal design read [Architecture](architecture.md); for the contribution process read [CONTRIBUTING.md](https://github.com/DMsuDev/vigil/blob/main/CONTRIBUTING.md).

- [Development](#development)
  - [Prerequisites](#prerequisites)
  - [Building](#building)
  - [Testing](#testing)
  - [Static analysis and warnings](#static-analysis-and-warnings)
  - [Code style](#code-style)
  - [Pre-commit hooks](#pre-commit-hooks)
  - [Commits and changelog](#commits-and-changelog)
  - [Continuous integration](#continuous-integration)
  - [Versioning and releases](#versioning-and-releases)
  - [Writing documentation](#writing-documentation)
  - [Pull request checklist](#pull-request-checklist)

## Prerequisites

- CMake 3.28 or newer and a C++17 compiler (GCC 13+, Clang 18+, MSVC, or Apple Clang).
- Ninja is recommended: it is required for `compile_commands.json` and clang-tidy.
- On Linux and macOS: a POSIX shell, `make`, and a C compiler (the vendored `libbacktrace` is built with `configure` and `make`).
- On Windows: Visual Studio with the C++ workload. Use a Developer shell for the `windows-ninja-*` presets.
- Optional: [pre-commit](https://pre-commit.com) and [git-cliff](https://git-cliff.org) (changelog).
- Network access for the first configure: GoogleTest is fetched with `FetchContent`.

## Building

The quickest route is a preset (see [Getting started](getting-started.md#presets)):

```bash
cmake --preset linux-dev
cmake --build --preset linux-dev-debug
ctest --preset linux-dev-debug
```

Without presets:

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Notes:

- Debug and RelWithDebInfo builds turn assertions and scoped logging on by default.
- The same sources must build as a **static** and a **shared** library. When you touch exports, check both (`-DBUILD_SHARED_LIBS=ON`).
- Build under C++17 **and** C++20 (`-DCMAKE_CXX_STANDARD=17`) when you touch macros or public headers: `assert.h` has a separate implementation for each.
- Library code must not depend on C++20 features. Only examples and tests may use them.

## Testing

| Suite    | Enabled by                                 | Location        | What it does                                                                                                           |
| :------- | :----------------------------------------- | :-------------- | :--------------------------------------------------------------------------------------------------------------------- |
| Unit     | `VIGIL_BUILD_TESTS=ON` (default top-level) | `tests/unit/`   | GoogleTest suite compiled into one executable. Sources are discovered automatically.                                   |
| Crash    | `VIGIL_CRASH_TESTS=ON`                     | `tests/crash/`  | Builds a program that intentionally crashes in many ways and expects the handler to report each one.                   |
| Consumer | `VIGIL_CONSUMER_TESTS=ON`                  | `tests/cmake_*` | Configures and builds tiny projects that consume Vigil through `FetchContent`, `add_subdirectory`, and `find_package`. |

Test labels let you run a subset:

```bash
ctest --test-dir build -L unit
ctest --test-dir build -L crash
ctest --test-dir build -L install
```

Guidelines:

- Unit tests link against the library and may include private headers from `src/`.
- Unit tests that touch the global `LogSystem` must reset it (`Shutdown()`) so tests stay independent.
- GoogleTest is fetched at configure time (v1.17.0), so the unit suite needs network access the first time.
- Crash tests are expected to die; they are registered with `WILL_FAIL`. Add new triggers as described in [Architecture](architecture.md#add-a-crash-trigger).
- Crash and consumer suites pass `CMAKE_BUILD_TYPE` to `ctest --build-and-test`. Multi-config generators leave it empty, so configure these suites with a single-config generator and an explicit build type, as CI does.
- Coverage: configure with `-DVIGIL_ENABLE_COVERAGE=ON` (GCC, Clang, or Apple Clang, with tests enabled).

## Static analysis and warnings

- `VIGIL_BUILD_WARNINGS` (default `ON` at top level) applies a strict warning set to Vigil's own targets. CI also sets `VIGIL_WARNINGS_AS_ERRORS=ON`.
- `VIGIL_ENABLE_CLANG_TIDY=ON` runs clang-tidy during compilation using the repository's `.clang-tidy`. It needs the Ninja or Makefile generators. The `*-ci` Ninja presets enable it.
- Fix the underlying problem instead of suppressing a warning. If a suppression is unavoidable, scope it as narrowly as possible and explain why in a comment.

## Code style

| Item                    | Convention                                                                  |
| :---------------------- | :-------------------------------------------------------------------------- |
| Types and functions     | `PascalCase`                                                                |
| Local variables         | `camelCase`                                                                 |
| Private members         | `m_` prefix (`m_Name`)                                                      |
| Static and global state | `s_` and `g_` prefixes (`g_Mutex`)                                          |
| Constants               | Follow the surrounding code; `constexpr` over macros                        |
| Macros                  | `VIGIL_UPPER_SNAKE_CASE`, prefixed, with parentheses around macro arguments |
| Namespaces              | Public API in `vigil`; internals in `vigil::detail` or anonymous namespaces |
| Ownership               | `Scope<T>` by default, `Shared<T>` only for shared ownership                |
| Platform checks         | `VIGIL_PLATFORM_*`, `VIGIL_COMPILER_*`, wrappers in `compiler_attributes.h` |
| Language                | Comments, docs, identifiers, and commit messages in English                 |

Other rules:

- Mark exported public symbols `VIGIL_API`.
- Do not add third-party types to public headers (the existing `fmt` usage is the exception).
- Comment _why_, not _what_. Public declarations get Doxygen-style comments.
- Keep CMake changes minimal and consistent with the existing helper functions.

## Pre-commit hooks

The repository ships a `.pre-commit-config.yaml` with whitespace and line-ending checks, YAML formatting, `markdownlint`, `codespell`, `actionlint`, a commit message check, and a manual `gitleaks` scan.

```bash
pip install pre-commit
pre-commit install --install-hooks
pre-commit install --hook-type commit-msg
pre-commit run --all-files
```

Markdown changes under `docs/` are linted, so run the hooks before opening a pull request.

## Commits and changelog

Vigil follows [Conventional Commits](https://www.conventionalcommits.org). The changelog is generated from them by git-cliff (see `cliff.toml`) and is never edited by hand.

```text
<type>(<optional scope>): <short imperative summary>

<optional body: explain why, not what>

<optional footer: BREAKING CHANGE: ..., Closes #123>
```

| Type       | Use for                               | In the changelog    |
| :--------- | :------------------------------------ | :------------------ |
| `feat`     | New functionality                     | Yes                 |
| `fix`      | Bug fixes                             | Yes                 |
| `perf`     | Performance improvements              | Yes                 |
| `refactor` | Restructuring without behavior change | Yes                 |
| `docs`     | Documentation                         | Only as `docs(api)` |
| `test`     | Tests                                 | Maintenance         |
| `ci`       | CI and workflow changes               | Maintenance         |
| `chore`    | Tooling and housekeeping              | Maintenance         |

Mark breaking changes with `!` after the type or scope (`feat(api)!: ...`) or a `BREAKING CHANGE:` footer. Use the `docs(api)` scope for documentation changes users should see in release notes (API behavior, new options); plain `docs:` commits stay out of the changelog.

## Continuous integration

| Workflow          | Trigger               | What it does                                                                                        |
| :---------------- | :-------------------- | :-------------------------------------------------------------------------------------------------- |
| `linux-build`     | Push and pull request | GCC and Clang, C++20 and C++23, static and shared, Debug and Release; runs the unit tests.          |
| `windows-build`   | Push and pull request | MSVC, Win32 and x64, C++20 and C++23, static and shared, Debug and Release.                         |
| `macos-build`     | Push and pull request | macOS arm64 build and unit tests.                                                                   |
| `integration`     | Push and pull request | Crash tests and consumer tests on the three platforms.                                              |
| `security-codeql` | See the workflow file | CodeQL analysis of the C++ sources.                                                                 |
| `pre-release`     | Manual                | Builds a pre-release (`-WIP` version) and publishes it as a GitHub pre-release.                     |
| `publish-release` | Release published     | Builds static packages for Linux x64, Windows x64, and macOS arm64 and uploads them with checksums. |

Dependabot keeps GitHub Actions up to date.

## Versioning and releases

Vigil uses [Semantic Versioning](https://semver.org). `VERSION` is the single source of truth:

- Stable releases are `X.Y.Z`.
- Development builds on `main` are `X.Y.Z-WIP`. CMake accepts only these two forms.
- `version.h` is generated from `VERSION` at configure time. Never edit it.

To cut a release:

1. Make sure `main` is green in CI.
2. Remove `-WIP` from `VERSION`, regenerate `CHANGELOG.md` with git-cliff, and merge.
3. Create a GitHub release with the tag `vX.Y.Z`. The `publish-release` workflow validates the tag format, builds the packages, and attaches them with a `SHA256SUMS.txt` file.
4. Bump `VERSION` to the next `-WIP` version.

Pre-releases are published manually from the `pre-release` workflow and tagged from the `-WIP` version in `VERSION`.

## Writing documentation

- Everything is in English. Keep the README short and move detail into `docs/`.
- One topic, one place: link instead of copying. Feature behavior belongs in the [User guide](user-guide.md), build and options in [Getting started](getting-started.md), internals in [Architecture](architecture.md).
- Code samples must compile. Prefer C++17-compatible samples and state when C++20 is required.
- Keep the CMake options table in Getting started and the `Debug`-dependent defaults in sync with the root `CMakeLists.txt`.
- When you add or move files, update the repository map in [AGENTS.md](https://github.com/DMsuDev/vigil/blob/main/AGENTS.md).
- Run `pre-commit run --all-files` to catch markdown and spelling problems.

## Pull request checklist

- [ ] The change builds on your platform as a static **and** shared library.
- [ ] Unit tests added or updated, and `ctest` passes.
- [ ] Builds without warnings under the strict warning set.
- [ ] Public headers: exports marked `VIGIL_API`, new headers listed in `VIGIL_HEADERS`.
- [ ] Behavior changes are reflected in the [User guide](user-guide.md) and, if user-visible, in `examples/`.
- [ ] Commit messages follow Conventional Commits.
- [ ] `pre-commit run --all-files` is clean.
