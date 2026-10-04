---
doc_id: "COM-GUID-021"
doc_title: "Quick Start Guide - common_system"
doc_version: "1.1.0"
doc_date: "2026-09-12"
doc_status: "Released"
project: "common_system"
category: "GUID"
---

# Setup and API Examples

This guide holds the setup details and examples linked from the
[English](../../README.md) and [Korean](../../README.kr.md) READMEs.
The examples target current source headers unless a release is explicitly pinned.

## Requirements

Use a C++20 compiler. Current source header builds require CMake 3.20+;
the published v0.2.0 release and named modules require CMake 3.28+.
Direct inclusion needs an include path and C++20 mode, but no CMake invocation.

### Compiler Requirements

common_system enforces minimum compiler versions at CMake configure time via
`KcenonCompilerRequirements.cmake`. Downstream systems can include this module
for consistent enforcement.

| Build Mode | GCC | Clang | MSVC | Apple Clang |
|------------|-----|-------|------|-------------|
| **Header-only** (default) | 11+ | 14+ | 2022 (19.30+) | 14+ |
| **C++20 Modules** (optional) | 14+ | 16+ | 2022 17.4 (19.34+) | Not supported |

### Ecosystem-Wide Compiler Requirements

When using multiple systems together, use the **highest** requirement from your dependency chain:

| Usage Scenario | GCC | Clang | MSVC | Apple Clang | Notes |
|----------------|-----|-------|------|-------------|-------|
| common_system only | 11+ | 14+ | 2022+ | 14+ | Baseline |
| + thread_system | **13+** | **17+** | 2022+ | 14+ | Higher requirements |
| + logger_system | 11+ | 14+ | 2022+ | 14+ | Optional thread_system |
| + container_system | 11+ | 14+ | 2022+ | 14+ | Uses common_system |
| + monitoring_system | **13+** | **17+** | 2022+ | 14+ | Requires thread_system |
| + database_system | **13+** | **17+** | 2022+ | 14+ | Full ecosystem |
| + network_system | **13+** | **17+** | 2022+ | 14+ | Requires thread_system |

> **Note**: If using any system that depends on thread_system, you need GCC 13+ or Clang 17+.
> All systems can include `KcenonCompilerRequirements.cmake` from common_system for
> automated version enforcement at configure time.

## Installation

### Published Release with FetchContent

Use the complete `CMakeLists.txt` and `main.cpp` in the
[README](../../README.md#getting-started). It pins v0.2.0 and links
`kcenon::common`, the target available from that release.
The `1.0.0` in current repository metadata is staged; see
[VERSION](../../VERSION) and [versioning](../../VERSIONING.md).

### vcpkg Overlay

From a checkout of this repository, use the bundled
[overlay port](../../vcpkg-ports/kcenon-common-system/):

```bash
vcpkg install kcenon-common-system --overlay-ports=./vcpkg-ports --classic
```

Configure your application with
`-DCMAKE_TOOLCHAIN_FILE=/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake`,
replacing `/path/to/vcpkg` with your vcpkg checkout. In your `CMakeLists.txt`,
after defining `your_target`:

```cmake
find_package(common_system CONFIG REQUIRED)
target_link_libraries(your_target PRIVATE kcenon::common_system)
```

### Direct Headers and Source Targets

```bash
git clone https://github.com/kcenon/common_system.git
c++ -std=c++20 -I./common_system/include main.cpp -o example
./example
```

Use the README's `main.cpp` for this command. A current source build also exposes
`common_system::common_system`, `kcenon::common_system`, and `kcenon::common`.
Do not assume these aliases all exist in older releases.

### Experimental Named Modules

Use the [C++20 Modules Guide](CXX20_MODULES.md) for explicit enablement,
compiler/scanner requirements, target selection, and C++20 mode matching.
Apple Clang is unsupported. A successful header fallback does not validate an import.

## API Examples

### Result<T> Pattern

Compose success values or propagate errors with `Result<T>`. Check the result before
accessing its value; `unwrap()` throws when used on an error.

```cpp
#include <kcenon/common/patterns/result.h>

using namespace kcenon::common;

int main() {
    auto result = ok(21)
        .and_then([](int value) { return ok(value * 2); })
        .map([](int value) { return value + 1; })
        .or_else([](const error_info&) { return ok(0); });
    return result.is_ok() && result.value() == 43 ? 0 : 1;
}
```

### IExecutor Interface

Submit a job through an executor supplied by your application. The returned `Result` reports submission errors; its future represents job completion.

```cpp
#include <kcenon/common/interfaces/executor_interface.h>
#include <future>
#include <memory>

namespace common = kcenon::common;

class example_job final : public common::interfaces::IJob {
public:
    common::VoidResult execute() override { return common::ok(); }
};

common::Result<std::future<void>> schedule(common::interfaces::IExecutor& executor) {
    return executor.execute(std::make_unique<example_job>());
}
```

### Health Monitoring

Create, register, and run a sample health check. Replace the callback with your application's check logic.

```cpp
#include <kcenon/common/interfaces/monitoring.h>
#include <chrono>

using namespace kcenon::common::interfaces;

int main() {
    health_monitor monitor;
    auto check = health_check_builder()
        .name("sample")
        .type(health_check_type::dependency)
        .timeout(std::chrono::seconds{5})
        .with_check([]() {
            health_check_result result;
            result.status = health_status::healthy;
            return result;
        }).build();
    if (check.is_err()) return 1;
    auto registered = monitor.register_check("sample", check.value());
    if (registered.is_err() || !registered.value()) return 1;
    auto result = monitor.check("sample");
    return result.is_ok() && result.value().is_healthy() ? 0 : 1;
}
```

### Error Code Registry

Centralized error code registry providing system-specific ranges:

| System | Range | Purpose |
|--------|-------|---------|
| common_system | -1 to -99 | Core errors |
| thread_system | -100 to -199 | Threading errors |
| logger_system | -200 to -299 | Logging errors |
| monitoring_system | -300 to -399 | Monitoring errors |
| container_system | -400 to -499 | Container errors |
| database_system | -500 to -599 | Database errors |
| network_system | -600 to -699 | Network errors |

### Circuit Breaker

Protect an operation with a circuit breaker. Keep the breaker alive across requests so it can track failures.

```cpp
#include <kcenon/common/patterns/result.h>
#include <kcenon/common/resilience/circuit_breaker.h>
#include <chrono>

using namespace kcenon::common;
using namespace kcenon::common::resilience;

Result<int> perform_operation() { return ok(42); } // Sample application operation.

int main() {
    circuit_breaker breaker(circuit_breaker_config{
        .failure_threshold = 5,
        .timeout = std::chrono::seconds{30}
    });
    if (!breaker.allow_request()) return 1;
    auto guard = breaker.make_guard();
    auto result = perform_operation();
    if (result.is_err()) return 1; // The guard records failure on destruction.
    guard.record_success();
    return 0;
}
```

### Handling an Error

```cpp
#include <kcenon/common/patterns/result.h>
#include <iostream>

// Sample application operation returning an error.
kcenon::common::Result<int> do_something() {
    return kcenon::common::make_error<int>(
        kcenon::common::error_codes::NOT_FOUND, "Resource not found");
}

int main() {
    auto result = do_something();
    if (result.is_err()) {
        const auto& error = result.error();
        std::cerr << error.message << " (code: " << error.code << ")\n";
    }
    return 0;
}
```

## Event Dispatch

`simple_event_bus` snapshots handlers while holding a mutex, then calls the
handlers synchronously after releasing it. Handler work therefore runs on the
publishing thread. See the [implementation](../../include/kcenon/common/patterns/event_bus.h)
and [recorded measurements](../BENCHMARKS.md); allocation behavior is not established
by the retained timing output.

## Running Repository Examples

From the repository root:

```bash
cmake -B build -DCOMMON_BUILD_EXAMPLES=ON
cmake --build build
./build/examples/result_example
```

The [executor example](../../examples/executor_example.cpp) supplies an executor;
the submission function above intentionally accepts one supplied by the application.
Other examples cover [ABI versions](../../examples/abi_version_example.cpp),
[unwrapping](../../examples/unwrap_demo.cpp), and
[multiple systems](../../examples/multi_system_app/).

## Architecture and Ecosystem

The [architecture guide](../ARCHITECTURE.md) contains the layer and dependency
diagrams. The [ecosystem overview](../ECOSYSTEM_OVERVIEW.md) maps consumers and
provides a [version baseline](../ECOSYSTEM_OVERVIEW.md#versions).
Use the highest compiler requirement in your dependency chain.

The [ecosystem vcpkg workflow](../../.github/workflows/ecosystem-vcpkg-integration.yml)
builds consumer ports in dependency order on Ubuntu and macOS. It runs on pull
requests to `main` touching the overlay ports, vcpkg manifests/configuration,
ecosystem consumer tests, or that workflow. It also runs on Wednesday at 03:43 UTC
and supports manual dispatch.

## Troubleshooting

- Missing headers: check the include path and use C++20 mode.
- Result errors: construct failures with `make_error<T>()`, inspect `is_err()`,
  and read `error()` before trying to access a success value.
- Executor integration: pass an `IJob` to `execute()` and check the returned
  `Result<std::future<void>>` for submission errors.
- Module configuration: check that `kcenon::common_modules` was actually created
  and that your compiler's dependency scanner is installed.

See the [troubleshooting guide](TROUBLESHOOTING.md),
[error-code guidelines](ERROR_CODE_GUIDELINES.md), and
[documentation index](../README.md) for further topics.
