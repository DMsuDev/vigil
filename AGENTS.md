# Vigil Agent Guide

Short, practical rules for AI agents and new contributors changing **Vigil**, a cross-platform C++17 library for logging, assertions, stack traces, and crash handling.

> [!NOTE]
> This file is the quick version. The README and [User guide](docs/user-guide.md) explain how to _use_ Vigil; [Architecture](docs/architecture.md) and [Development](docs/development.md) explain how it works and how to work on it. When in doubt, read those before changing behavior.

## At a glance

| Item           | Value                                                                                 |
| :------------- | :------------------------------------------------------------------------------------ |
| Language       | C++17 for the library. Examples and tests may use C++20.                              |
| Build          | CMake 3.28+, presets in `CMakePresets.json`                                           |
| Platforms      | Windows, Linux, macOS                                                                 |
| Dependencies   | `fmt` (public), `spdlog` (private), `libbacktrace` (POSIX), all vendored in `vendor/` |
| Public API     | `include/vigil/` (umbrella header: `vigil.h`)                                         |
| Version source | `VERSION` (never edit `version.h`)                                                    |

## Repository map

| Path                | What lives there                                                           |
| :------------------ | :------------------------------------------------------------------------- |
| `include/vigil/`    | Public headers. `detail/` is installed but internal.                       |
| `src/`              | Implementation and private headers. The only place that includes `spdlog`. |
| `cmake/`            | Build helpers, vendor modules (`libbacktrace`), package templates.         |
| `vendor/`           | Third-party sources. Do not edit.                                          |
| `tests/unit/`       | GoogleTest unit suite.                                                     |
| `tests/crash/`      | Crash integration harness (processes that die on purpose).                 |
| `tests/cmake_*`     | Consumer tests (`FetchContent`, `add_subdirectory`, `find_package`).       |
| `examples/`         | One program per feature. Require C++20.                                    |
| `docs/`             | Getting started, user guide, architecture, development.                    |
| `CMakePresets.json` | Dev, CI, package, and consumer presets.                                    |

When you add, move, or delete files, update this table.

## Build, test, finish

```bash
cmake --preset linux-dev            # or windows-vs-dev, windows-ninja-dev, macos-dev
cmake --build --preset linux-dev-debug
ctest --preset linux-dev-debug
```

Before you call a change done:

- [ ] It builds as a static **and** a shared library.
- [ ] It builds under C++17 **and** C++20 if you touched macros or public headers.
- [ ] Unit tests pass and new behavior has a test.
- [ ] No new warnings (the strict warning set is on by default).
- [ ] Public headers are listed in `VIGIL_HEADERS` and exported with `VIGIL_API`.
- [ ] Docs and `examples/` match the new behavior.
- [ ] Commit messages follow Conventional Commits.

## Rules

### Architecture

- Never expose `spdlog` in a public header. `fmt` is the only third-party type allowed there.
- Public API goes in namespace `vigil`; internals in `vigil::detail` or an anonymous namespace.
- Use `VIGIL_PLATFORM_*`, `VIGIL_COMPILER_*`, and `compiler_attributes.h`. Never raw `_WIN32`, `__linux__`, or compiler attributes.
- Default ownership is `Scope<T>`. Use `Shared<T>` only when ownership really is shared.
- Everything reachable from a crash or signal handler must be async-signal-safe: no allocation, no locks, no logging, no `std::string`.
- Callbacks (`OnLevelChange`, `OnFlush`, `OnShutdown`) must be invoked outside Vigil's internal lock.
- Reentrant callbacks must not trigger the same hook recursively.
- Put anything with side effects in `VIGIL_VERIFY*`, never in `VIGIL_ASSERT*`.

### Build system

- `VERSION` is the single source of truth. `vigil_load_version()` (in `cmake/helpers/build_helpers.cmake`) must run before `project()`. Accepted formats are `X.Y.Z` and `X.Y.Z-WIP`.
- New `.cpp` files must be added to `VIGIL_SOURCES` in `src/CMakeLists.txt`.
- A new public dependency also needs an entry in `VigilConfig.cmake.in` and `Vigil.pc.in`. Ask before adding any dependency.
- Keep CMake changes minimal and consistent with the existing helpers.

### Style

- `PascalCase` types and functions, `camelCase` locals, `m_` private members, `g_` and `s_` for globals and statics.
- Macros are `VIGIL_UPPER_SNAKE_CASE` and parenthesize their arguments.
- English only: code, comments, docs, and commit messages. Comment _why_, not _what_.
- Fix warnings instead of suppressing them.

### Git

- Conventional Commits: `type(scope): summary`. Types: `feat`, `fix`, `perf`, `refactor`, `docs`, `test`, `ci`, `chore`.
- Use `docs(api)` only for documentation that belongs in release notes. Mark breaking changes with `!` or a `BREAKING CHANGE:` footer.
- Never edit `CHANGELOG.md` by hand; it is generated by git-cliff.

## Looks wrong, is not

Do not "fix" these without reading [Deliberate design decisions](docs/architecture.md#deliberate-design-decisions).

| What you will see                                       | Why it is correct                                                                   |
| :------------------------------------------------------ | :---------------------------------------------------------------------------------- |
| `g_Mutex` is a recursive mutex                          | The crash handler can re-enter logging on the same thread.                          |
| Callbacks copied under the lock, called after it        | Prevents deadlocks when callbacks call back into `LogSystem`.                       |
| `OnMessage` runs _inside_ the sink lock                 | It is the one exception, so it must never call `LogSystem`.                         |
| Logging before `Init()` asserts or warns, never throws  | Free functions and macros must be safe anywhere. `Create`, `Main`, `Get` do throw.  |
| Removing a logger also calls `spdlog::drop(name)`       | Otherwise the name can never be reused.                                             |
| Log files are created lazily                            | No empty `.log` files and no abort when a file cannot be opened.                    |
| `g_State` in the crash handler is leaked                | A crash during static destruction must still find it alive.                         |
| Report order is log, file, then user callback           | Most reliable first, least trusted last; a deadline kills hangs.                    |
| Two `VIGIL_ASSERT` implementations (C++17 and C++20)    | Avoids calling `fmt::format()` with an empty argument pack on C++17.                |
| `VIGIL_ASSERT` does not evaluate its condition when off | That is the contract. Side effects belong in `VIGIL_VERIFY`.                        |
| Stack trace API compiles with traces disabled           | Call sites need no `#if`; the result is just empty.                                 |
| Crash and consumer tests need an explicit build type    | The harness forwards `CMAKE_BUILD_TYPE`, which multi-config generators leave empty. |

## Where to read more

- [Getting started](docs/getting-started.md): requirements, options, presets, troubleshooting.
- [User guide](docs/user-guide.md): every public feature.
- [Architecture](docs/architecture.md): layers, threading, crash pipeline, build system.
- [Development](docs/development.md): tests, CI, style, commits, releases.
- [CONTRIBUTING.md](CONTRIBUTING.md): contribution process.
