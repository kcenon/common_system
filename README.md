[![CI](https://github.com/kcenon/common_system/actions/workflows/ci.yml/badge.svg)](https://github.com/kcenon/common_system/actions/workflows/ci.yml)
[![Code Coverage](https://github.com/kcenon/common_system/actions/workflows/coverage.yml/badge.svg)](https://github.com/kcenon/common_system/actions/workflows/coverage.yml)
[![Static Analysis](https://github.com/kcenon/common_system/actions/workflows/static-analysis.yml/badge.svg)](https://github.com/kcenon/common_system/actions/workflows/static-analysis.yml)
[![Ecosystem vcpkg Integration](https://github.com/kcenon/common_system/actions/workflows/ecosystem-vcpkg-integration.yml/badge.svg)](https://github.com/kcenon/common_system/actions/workflows/ecosystem-vcpkg-integration.yml)
[![codecov](https://codecov.io/gh/kcenon/common_system/branch/main/graph/badge.svg)](https://codecov.io/gh/kcenon/common_system)
[![Documentation](https://github.com/kcenon/common_system/actions/workflows/build-Doxygen.yaml/badge.svg)](https://github.com/kcenon/common_system/actions/workflows/build-Doxygen.yaml)
[![License](https://img.shields.io/github/license/kcenon/common_system)](https://github.com/kcenon/common_system/blob/main/LICENSE)

# Common System

> **Language:** **English** | [한국어](README.kr.md)

## Table of Contents

- [Overview](#overview)
- [Key Features](#key-features)
- [Quick Start](#quick-start)
- [Requirements](#requirements)
- [Installation](#installation)
- [Architecture](#architecture)
- [Core Concepts](#core-concepts)
- [API Overview](#api-overview)
- [Examples](#examples)
- [Performance](#performance)
- [Ecosystem Integration](#ecosystem-integration)
- [Compliance](#compliance)
- [Contributing](#contributing)
- [License](#license)

---

## Overview

A foundational C++20 header-only library providing essential interfaces and design patterns for building modular, loosely-coupled system architectures. Designed as the cornerstone of the ecosystem, it enables seamless integration between system modules while maintaining zero runtime overhead through template-based abstractions and interface-driven design.

**Key Value Propositions**:
- **Zero-overhead abstractions**: Template-based interfaces with compile-time resolution
- **Well-tested**: coverage enforced by a 40% project / 60% patch Codecov gate (80% is the long-term target; see `codecov.yml`), zero sanitizer warnings, full CI/CD
- **Header-only design**: No library linking, no dependencies, instant integration
- **C++20 Module support**: Optional module-based build for faster compilation
- **Ecosystem foundation**: Powers thread_system, network_system, database_system, and more

**Release status:** The latest published release is [v0.2.0](https://github.com/kcenon/common_system/releases/tag/v0.2.0). Version `1.0.0` is recorded in [VERSION](VERSION), [CHANGELOG.md](CHANGELOG.md), and [vcpkg.json](vcpkg.json), but has not yet been tagged or published.

### API Stability

The following API stability policy applies once v1.0.0 is tagged:

- **No breaking changes** to public headers within the same major version
- **No removal** of public functions, classes, or type aliases without a major version bump
- **Stable CMake targets**: `common_system::common_system`, `kcenon::common_system`, `kcenon::common`
- **Stable `#include` paths**: All headers under `kcenon/common/` are part of the public API
- **Result\<T\> as primary error handling**: Public APIs return `Result<T>` instead of throwing exceptions. The `unwrap()` method intentionally throws when called on an error result (Rust-style panic semantics)

See [VERSIONING.md](VERSIONING.md) for the full versioning policy and release process.

---

## Key Features

| Category | Feature | Description | Status |
|----------|---------|-------------|--------|
| **Patterns** | Result<T> | Rust-inspired monadic error handling (and_then, map, or_else) | Stable |
| **Patterns** | Circuit Breaker | Resilience pattern with CLOSED/OPEN/HALF_OPEN states | Stable |
| **Patterns** | Event Bus | Thread-safe synchronous pub/sub | Stable |
| **Interfaces** | IExecutor / IJob | Universal task execution abstraction | Stable |
| **Interfaces** | ILogger / IMetricCollector | Monitoring and logging interfaces | Stable |
| **DI** | Service Container | Thread-safe DI with singleton/transient/scoped lifetimes | Stable |
| **Config** | Config Loader / Watcher | Configuration management with file watching | Stable |
| **Config** | CLI Parser | Command-line argument parsing | Stable |
| **Utils** | Circular Buffer / Object Pool | High-performance utility data structures | Stable |
| **Concepts** | C++20 Concepts | Resultable, Unwrappable, callable, container, etc. | Stable |

---

## Quick Start

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

[Full Getting Started Guide](docs/guides/QUICK_START.md)

---

## Requirements

| Dependency | Version | Required | Description |
|------------|---------|----------|-------------|
| C++20 Compiler | GCC 11+ / Clang 14+ / MSVC 2022+ / Apple Clang 14+ | Yes | C++20 features (concepts) |
| CMake | 3.20+ for current `main` header builds; 3.28+ for v0.2.0 and C++20 modules | For CMake builds | Direct header inclusion does not require CMake |

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

### Dependency Flow

```
common_system (Foundation Layer - No Dependencies)
       |
       | provides interfaces to
       |
       +-- thread_system (implements IExecutor)
       +-- logger_system (uses Result<T>)
       +-- container_system (uses Result<T>)
       +-- monitoring_system (event bus)
       +-- network_system (uses IExecutor)
       +-- database_system (uses Result<T> and IExecutor)
```

---

## Installation

### Installation via vcpkg

From a checkout of this repository, use the bundled [overlay port](vcpkg-ports/kcenon-common-system/):

```bash
vcpkg install kcenon-common-system --overlay-ports=./vcpkg-ports --classic
```

Configure your application with `-DCMAKE_TOOLCHAIN_FILE=/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake`, replacing `/path/to/vcpkg` with your vcpkg checkout. In your `CMakeLists.txt`, after defining `your_target`:
```cmake
find_package(common_system CONFIG REQUIRED)
target_link_libraries(your_target PRIVATE kcenon::common_system)
```

### CMake FetchContent (Recommended)

```cmake
include(FetchContent)
FetchContent_Declare(
    common_system
    GIT_REPOSITORY https://github.com/kcenon/common_system.git
    GIT_TAG v0.2.0
)
FetchContent_MakeAvailable(common_system)

target_link_libraries(your_target PRIVATE kcenon::common)
```

### Header-Only Usage (Simplest)

```bash
git clone https://github.com/kcenon/common_system.git
# Include headers directly - no build required!
```

```cpp
#include <kcenon/common/interfaces/executor_interface.h>
#include <kcenon/common/patterns/result.h>
```

### C++20 Modules

This experimental build requires a supported compiler and its module dependency scanner; Apple Clang is unsupported. Link importing consumers to `kcenon::common_modules` from a source build with modules enabled.

```bash
# Build with C++20 module support (requires CMake 3.28+, Ninja, Clang 16+/GCC 14+)
cmake -G Ninja -B build -DCOMMON_BUILD_MODULES=ON
cmake --build build
```

In a consuming CMake project, after adding common_system from source with modules enabled and defining `your_target`, match the module's C++20 compilation mode:

```cmake
set_target_properties(your_target PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS OFF
)
target_link_libraries(your_target PRIVATE kcenon::common_modules)
```

```cpp
import kcenon.common;

int main() {
    auto result = kcenon::common::ok(42);
    return result.is_ok() && result.value() == 42 ? 0 : 1;
}
```

> For the full dual-build strategy, compiler/CMake matrix, and fallback behavior,
> see the [C++20 Modules Guide](docs/guides/CXX20_MODULES.md).

---

## Architecture

### Module Structure

```
include/kcenon/common/
  adapters/       - Adapter pattern (adapter.h, smart_adapter.h)
  bootstrap/      - System bootstrapper
  concepts/       - C++20 concepts (Resultable, Unwrappable, callable, container, etc.)
  config/         - Feature flags, ABI version, config loader/watcher, CLI parser
  di/             - Dependency injection (service_container, unified_bootstrapper)
  error/          - Error codes and error category system
  interfaces/     - Core abstractions (IExecutor, IJob, ILogger, IDatabase, IThreadPool, etc.)
  logging/        - Log functions and macros
  patterns/       - Result<T>, event_bus
  resilience/     - Circuit breaker (CLOSED/OPEN/HALF_OPEN state machine)
  utils/          - Circular buffer, object pool, enum serialization
```

### Ecosystem Position

```
                    +------------------+
                    |  common_system   | <-- Foundation Layer
                    |  (interfaces)    |
                    +--------+---------+
                             | provides interfaces
       +---------------------+---------------------+
       |                     |                     |
+------v-------+    +--------v--------+   +-------v--------+
|thread_system |    |network_system   |   |monitoring_sys. |
|(implements   |    |(uses IExecutor) |   |(event bus)     |
| IExecutor)   |    +-----------------+   +----------------+
+--------------+             |                     |
       |                     |                     |
       +---------------------+---------------------+
                             | all use
                    +--------v---------+
                    | Result<T> pattern|
                    | Error handling   |
                    +------------------+
```

[Complete Architecture Guide](docs/ARCHITECTURE.md)

### Layout Standard

The kcenon ecosystem follows a canonical directory, build-system, and test-infrastructure layout defined in [kcenon-system-layout.md](docs/kcenon-system-layout.md). This standard governs all eight ecosystem systems and is owned by `common_system` as the foundation tier.

---

## Core Concepts

### Result<T> Pattern

Type-safe error handling without exceptions, inspired by Rust:

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

---

## API Overview

| Component | Purpose | Header |
|-----------|---------|--------|
| `Result<T>` / `VoidResult` | Monadic error handling | `patterns/result.h` |
| `IExecutor` / `IJob` | Task execution interface | `interfaces/executor_interface.h` |
| `ILogger` | Logging abstraction | `interfaces/logger_interface.h` |
| `service_container` | Dependency injection | `di/service_container.h` |
| `simple_event_bus` | Synchronous pub/sub | `patterns/event_bus.h` |
| `circuit_breaker` | Resilience pattern | `resilience/circuit_breaker.h` |
| `config_loader` | Configuration management | `config/config_loader.h` |
| `circular_buffer` | Fixed-size ring buffer | `utils/circular_buffer.h` |
| `object_pool` | Object pooling | `utils/object_pool.h` |

[Complete API Reference](docs/API_REFERENCE.md)

---

## Examples

| Example | Description | Difficulty |
|---------|-------------|------------|
| [result_example](examples/result_example.cpp) | Result<T> error handling patterns | Beginner |
| [executor_example](examples/executor_example.cpp) | Executor interface and thread management | Beginner |
| [abi_version_example](examples/abi_version_example.cpp) | ABI version checking and compatibility | Intermediate |
| [unwrap_demo](examples/unwrap_demo.cpp) | Result unwrapping and chaining | Intermediate |
| [multi_system_app](examples/multi_system_app/) | Multi-system integration example | Advanced |

### Running Examples

```bash
cmake -B build -DCOMMON_BUILD_EXAMPLES=ON
cmake --build build
./build/examples/result_example
```

---

## Performance

| Operation | Time (ns) | Allocations | Notes |
|-----------|-----------|-------------|-------|
| Result<T> creation | 2.3 | 0 | Stack-only operation |
| Result<T> error check | 0.8 | 0 | Single bool check |
| IExecutor submit | 45.2 | 1 | Task queue insertion |
| Event publish | 12.4 | 0 | Mutex-protected synchronous dispatch |

**Key Performance Characteristics:**
- Result<T> is 400x faster than exceptions in error paths
- IExecutor is 53x faster than std::async for high-frequency tasks
- Zero-overhead abstractions - compiler optimizes away all abstraction layers

**Quality Metrics**:
- **Test coverage**: enforced Codecov gate floor of 40% project / 60% patch; 80% is the long-term target per the gradual-improvement plan in `codecov.yml`. The live measured value is shown by the Codecov badge at the top of this file.
- **Sanitizer tests**: 18/18 passing with zero warnings
- **Cross-platform**: Ubuntu, macOS, Windows
- **Zero memory leaks**: AddressSanitizer verified
- **Zero data races**: ThreadSanitizer verified
- **RAII Grade: A** - All resources managed through smart pointers

[Full Benchmarks](docs/BENCHMARKS.md)

---

## Ecosystem Integration

### Ecosystem Dependency Map

```mermaid
graph TD
    A[common_system] --> B[thread_system]
    A --> C[container_system]
    B --> D[logger_system]
    B --> E[monitoring_system]
    D --> F[database_system]
    E --> F
    F --> G[network_system]
    G --> H[pacs_system]

    style A fill:#f9f,stroke:#333,stroke-width:3px
```

> **Ecosystem reference**:
> [thread_system](https://github.com/kcenon/thread_system) — Tier 1: Implements IExecutor interface
> [container_system](https://github.com/kcenon/container_system) — Tier 1: Uses Result&lt;T&gt; for error handling
> [logger_system](https://github.com/kcenon/logger_system) — Tier 2: Uses ILogger, Result&lt;T&gt;
> [monitoring_system](https://github.com/kcenon/monitoring_system) — Tier 3: Uses event bus, IMonitor
> [database_system](https://github.com/kcenon/database_system) — Tier 3: Uses Result&lt;T&gt;, IExecutor
> [network_system](https://github.com/kcenon/network_system) — Tier 4: Uses IExecutor, Result&lt;T&gt;
> [pacs_system](https://github.com/kcenon/pacs_system) — Tier 5: Full ecosystem consumer

### Ecosystem Version Baseline

Downstream consumers should pin against a known-good set of port versions. The current baseline is published in [`docs/ECOSYSTEM_OVERVIEW.md#versions`](docs/ECOSYSTEM_OVERVIEW.md#versions) with a reproducible `vcpkg-configuration.json` snippet.

### Ecosystem CI Verification

The [Ecosystem vcpkg Integration](https://github.com/kcenon/common_system/actions/workflows/ecosystem-vcpkg-integration.yml) workflow validates that all 8 ecosystem ports install and build correctly as a consumer would experience them. It tests each port in bottom-up dependency order (Layer 0 through Layer 7) on Ubuntu and macOS. It runs on pull requests targeting `main` that change `vcpkg-ports/**`, `vcpkg.json`, `vcpkg-configuration.json`, `tests/ecosystem-consumer/**`, or the workflow itself, and is scheduled weekly on Wednesday at 03:43 UTC. It also supports manual dispatch.

This common system serves as the foundational layer (Tier 0) that all other system modules build upon:

```
common_system (Tier 0 - Foundation)
       |
       +-- thread_system     (Tier 1) - Implements IExecutor
       +-- container_system  (Tier 1) - Uses Result<T>
       +-- logger_system     (Tier 2) - Uses ILogger, Result<T>
       +-- monitoring_system (Tier 3) - Uses Event Bus
       +-- database_system   (Tier 3) - Uses Result<T>, IExecutor
       +-- network_system    (Tier 4) - Uses IExecutor
       +-- pacs_system       (Tier 5) - Full ecosystem consumer
```

### Integration Example

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

### Documentation

| Category | Document | Description |
|----------|----------|-------------|
| **Guides** | [Quick Start](docs/guides/QUICK_START.md) | Get up and running in minutes |
| | [Best Practices](docs/guides/BEST_PRACTICES.md) | Recommended usage patterns |
| | [FAQ](docs/guides/FAQ.md) | Frequently asked questions |
| | [Troubleshooting](docs/guides/TROUBLESHOOTING.md) | Common issues and solutions |
| **Advanced** | [C++20 Modules Guide](docs/guides/CXX20_MODULES.md) | Dual-build strategy and toolchain matrix |
| | [Architecture](docs/ARCHITECTURE.md) | System design and principles |
| | [Migration](docs/advanced/MIGRATION.md) | Version upgrade guide |
| | [IExecutor Migration](docs/advanced/IEXECUTOR_MIGRATION_GUIDE.md) | Executor API migration |
| | [Runtime Binding](docs/architecture/RUNTIME_BINDING.md) | Core design pattern |
| **Contributing** | [Contributing](docs/contributing/CONTRIBUTING.md) | How to contribute |
| | [Error Code Guidelines](docs/guides/ERROR_CODE_GUIDELINES.md) | Error code management |

---

## Compliance

`common_system` and its ecosystem siblings provide technical primitives that organizations may use as part of their compliance programs. The libraries are not themselves certified; adopters integrate them and supply the organizational controls.

- [ISO Standards Overview](docs/compliance/ISO_OVERVIEW.md) — ecosystem-level index of every ISO standard the kcenon systems touch, with links to per-system mapping documents.

Per-system compliance docs are published under each repository's `docs/compliance/` directory (e.g., [logger_system ISO/IEC 27001 mapping](https://github.com/kcenon/logger_system/blob/develop/docs/compliance/iso-27001.md)). See the overview for the full list.

---

## Contributing

We welcome contributions! Please see [CONTRIBUTING.md](docs/contributing/CONTRIBUTING.md) for guidelines.

### Quick Links

- [Development Setup](docs/contributing/CONTRIBUTING.md#development-setup)
- [Code Style](docs/contributing/CONTRIBUTING.md#code-style-guidelines)
- [Pull Request Process](docs/contributing/CONTRIBUTING.md#submitting-changes)

### Support

- **Issues**: [GitHub Issues](https://github.com/kcenon/common_system/issues)
- **Email**: kcenon@naver.com

---

## License

This project is licensed under the BSD 3-Clause License - see the [LICENSE](LICENSE) file for details.

---

<p align="center">
  Made with care by the kcenon team
</p>
