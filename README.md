# Common System

**Status: active** | **Latest published release:** [v0.2.0](https://github.com/kcenon/common_system/releases/tag/v0.2.0)

Version `1.0.0` is staged in [VERSION](VERSION), but has not been tagged or published.
See the [release history](CHANGELOG.md) and [versioning policy](VERSIONING.md).

**Language:** **English** | [한국어](README.kr.md)

[![CI](https://github.com/kcenon/common_system/actions/workflows/ci.yml/badge.svg)](https://github.com/kcenon/common_system/actions/workflows/ci.yml)
[![Coverage](https://github.com/kcenon/common_system/actions/workflows/coverage.yml/badge.svg)](https://github.com/kcenon/common_system/actions/workflows/coverage.yml)
[![Static Analysis](https://github.com/kcenon/common_system/actions/workflows/static-analysis.yml/badge.svg)](https://github.com/kcenon/common_system/actions/workflows/static-analysis.yml)
[![Documentation](https://github.com/kcenon/common_system/actions/workflows/build-Doxygen.yaml/badge.svg)](https://github.com/kcenon/common_system/actions/workflows/build-Doxygen.yaml)

## Table of Contents

- [Overview](#overview)
- [Architecture](#architecture)
- [Getting Started](#getting-started)
- [API Reference](#api-reference)
- [Features](#features)
- [Configuration](#configuration)
- [Integration](#integration)
- [Performance](#performance)
- [Testing](#testing)
- [Troubleshooting](#troubleshooting)
- [Contributing](#contributing)
- [Changelog](#changelog)
- [License](#license)

## Overview

Common System provides C++20 headers for Result-based error handling, task and
logging interfaces, dependency injection, event dispatch, and resilience patterns.
It supplies shared interfaces used by the kcenon system libraries.

Header mode is the default. Optional named modules are experimental and require
a compiler with module dependency scanning support.

The [documentation index](docs/README.md) links to guides and design documents.
The [generated API documentation](https://kcenon.github.io/common_system/) describes
the public headers.

## Architecture

Applications provide implementations of interfaces such as `IExecutor` and
`ILogger`. `Result<T>` carries a success value or error information between them.
The service container registers and resolves application dependencies.

`simple_event_bus` snapshots handlers under a mutex, then invokes them
synchronously after releasing it. Handler work runs on the publishing thread.
See the [event-bus implementation](include/kcenon/common/patterns/event_bus.h).

The [architecture guide](docs/ARCHITECTURE.md) contains layer and dependency diagrams.
The [layout standard](docs/kcenon-system-layout.md) describes the directory,
build, and test conventions shared by the ecosystem.

## Getting Started

### Requirements

- C++20: GCC 11+, Clang 14+, MSVC 2022 (19.30+), or Apple Clang 14+ for headers.
- CMake 3.20+ for current source header builds; CMake 3.28+ for v0.2.0 or modules.
- Named modules need a supported compiler and scanner; Apple Clang is unsupported.

See the [compiler and dependency matrices](docs/guides/QUICK_START.md#requirements)
before combining systems with different compiler requirements.

### Install the Published Release

Create `CMakeLists.txt` and `main.cpp` in an application directory.
This FetchContent example pins the published release and uses its `kcenon::common`
target. It disables the dependency's tests, examples, and benchmarks.

```cmake
cmake_minimum_required(VERSION 3.28)
project(common_example LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
set(COMMON_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(COMMON_BUILD_INTEGRATION_TESTS OFF CACHE BOOL "" FORCE)
set(COMMON_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(COMMON_BUILD_BENCHMARKS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    common_system
    GIT_REPOSITORY https://github.com/kcenon/common_system.git
    GIT_TAG v0.2.0
)
FetchContent_MakeAvailable(common_system)

add_executable(common_example main.cpp)
target_link_libraries(common_example PRIVATE kcenon::common)
```

### First Program

Save this as `main.cpp`. Check the result before reading a success value:

```cpp
#include <kcenon/common/patterns/result.h>

using namespace kcenon::common;

Result<int> validate_port(int port) {
    if (port < 1 || port > 65535) {
        return make_error<int>(error_codes::INVALID_ARGUMENT,
                               "Port must be between 1 and 65535");
    }
    return ok(port);
}

int main() {
    auto result = validate_port(8080);
    return result.is_ok() && result.value() == 8080 ? 0 : 1;
}
```

Configure, build, and run the application:

```bash
cmake -S . -B build
cmake --build build --config Release
./build/common_example
```

With a multi-configuration generator, use `build/Release/common_example.exe` on
Windows or `build/Release/common_example` on Unix.

For the [vcpkg overlay](docs/guides/QUICK_START.md#vcpkg-overlay),
[direct headers](docs/guides/QUICK_START.md#direct-headers-and-source-targets),
and [named modules](docs/guides/CXX20_MODULES.md), follow the respective setup guide.

## API Reference

Header paths below are relative to `kcenon/common/`.

| Component | Purpose | Header |
| --- | --- | --- |
| `Result<T>` / `VoidResult` | Success or error values | `patterns/result.h` |
| `IExecutor` / `IJob` | Job submission and completion | `interfaces/executor_interface.h` |
| `ILogger` | Logging interface | `interfaces/logger_interface.h` |
| `service_container` | Dependency registration and resolution | `di/service_container.h` |
| `simple_event_bus` | Synchronous event dispatch | `patterns/event_bus.h` |
| `circuit_breaker` | Failure tracking and recovery | `resilience/circuit_breaker.h` |
| `config_loader` | Configuration loading | `config/config_loader.h` |

See the [API reference](docs/API_REFERENCE.md) and the
[setup and API examples](docs/guides/QUICK_START.md#api-examples) for job submission,
health checks, circuit breakers, and error handling.

## Features

- Compose `Result<T>` operations with `and_then`, `map`, and `or_else`.
- Register dependencies with singleton, transient, or scoped lifetimes.
- Subscribe handlers to typed events and publish synchronously.
- Track circuit-breaker state across requests with the config and guard API.
- Use configuration loaders, watchers, and command-line parsing.
- Reuse circular buffers, object pools, and C++20 concepts.

Examples include [Result](examples/result_example.cpp),
[executors](examples/executor_example.cpp), and
[multiple systems](examples/multi_system_app/).
The [example build instructions](docs/guides/QUICK_START.md#running-repository-examples)
explain how to run them from a source checkout.

## Configuration

The [CMake options](CMakeLists.txt) control header mode, examples, integration tests,
and optional YAML support. The [module guide](docs/guides/CXX20_MODULES.md) describes
the separate module target and toolchain checks.

Application configuration is covered by the [configuration guide](docs/CONFIG_GUIDE.md),
[loader](docs/CONFIG_LOADER.md), [watcher](docs/CONFIG_WATCHER.md), and
[CLI parser](docs/CONFIG_CLI_PARSER.md) documents.

## Integration

The [ecosystem overview](docs/ECOSYSTEM_OVERVIEW.md) maps downstream consumers.
Use its [version baseline](docs/ECOSYSTEM_OVERVIEW.md#versions) when choosing
compatible port versions and the highest compiler requirement in your dependency chain.

The [ecosystem vcpkg workflow](https://github.com/kcenon/common_system/actions/workflows/ecosystem-vcpkg-integration.yml)
builds consumer ports on Ubuntu and macOS. Its
[definition](.github/workflows/ecosystem-vcpkg-integration.yml) records PR path filters,
the Wednesday 03:43 UTC schedule, and manual dispatch support.

## Performance

See [benchmark evidence and methodology](docs/BENCHMARKS.md) for recorded workloads,
the measurement environment, commands, raw results, and interpretation limits.

## Testing

[CI](https://github.com/kcenon/common_system/actions/workflows/ci.yml) builds on Linux,
macOS, and Windows. The Ubuntu sanitizer matrix runs tests with address, thread,
and undefined sanitizers; see the [workflow definition](.github/workflows/ci.yml).

Coverage reports come from the [coverage workflow](https://github.com/kcenon/common_system/actions/workflows/coverage.yml).
Configured coverage policy lives in [codecov.yml](codecov.yml).
Workflow pages provide run-specific results.

Documentation changes run the [README linter](scripts/readme_lint.py) and
[documentation audit](.github/workflows/doc-audit.yml).
See the [README policy](docs/contributing/README_POLICY.md) for claim and badge rules.

## Troubleshooting

Start with [setup checks](docs/guides/QUICK_START.md#troubleshooting), the
[troubleshooting guide](docs/guides/TROUBLESHOOTING.md), or [FAQ](docs/guides/FAQ.md).
Report reproducible problems through [GitHub Issues](https://github.com/kcenon/common_system/issues).

## Contributing

Read the [contribution guide](docs/contributing/CONTRIBUTING.md) for development setup,
code style, and the pull request process.
Use the [error-code guidelines](docs/guides/ERROR_CODE_GUIDELINES.md) when adding errors.

## Changelog

[CHANGELOG.md](CHANGELOG.md) records repository changes; published versions appear
in [GitHub Releases](https://github.com/kcenon/common_system/releases).
The API policy for staged v1.0.0 takes effect when that release is tagged;
see [VERSIONING.md](VERSIONING.md).

## License

Common System uses the [BSD 3-Clause License](LICENSE).
