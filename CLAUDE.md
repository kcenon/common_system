# common_system

## Overview

Foundational C++20 header-only library providing essential interfaces, patterns, and
design abstractions for building modular, loosely-coupled system architectures.
Tier 0 cornerstone of the kcenon ecosystem with zero external runtime dependencies.

## Architecture

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

Key abstractions:
- `Result<T>` / `VoidResult` — Rust-inspired monadic error handling (`and_then`, `map`, `or_else`)
- `IExecutor` / `IJob` — Task execution interface
- `ILogger` / `IMetricCollector` — Monitoring and logging interfaces
- `service_container` — Thread-safe DI with singleton/transient/scoped lifetimes
- `simple_event_bus` — Thread-safe synchronous pub/sub
- `circuit_breaker` — Resilience pattern with RAII guard

## Build & Test

```bash
# Default (header-only, no tests)
cmake --preset default && cmake --build build

# With tests
cmake --preset debug && cmake --build build-debug
cd build-debug && ctest --output-on-failure
```

Key CMake options:
- `COMMON_HEADER_ONLY` (ON) — Header-only mode
- `COMMON_BUILD_TESTS` — Unit tests (Google Test 1.17.0)
- `COMMON_BUILD_BENCHMARKS` — Performance benchmarks (Google Benchmark 1.9.5)
- `COMMON_BUILD_MODULES` (OFF) — C++20 modules (requires CMake 3.28+)
- `BUILD_WITH_YAML_CPP` — Optional YAML config support

Presets: `default`, `debug`, `release`, `asan`, `tsan`, `ubsan`, `ci`, `vcpkg`

CI: Multi-platform (Ubuntu GCC/Clang, macOS, Windows MSVC), coverage, static analysis,
sanitizers, benchmarks, CVE scan, SBOM, Doxygen docs.

## Key Patterns

- **Header-only by default** — `INTERFACE` CMake target, zero build cost for consumers
- **Result\<T\>** — `std::optional<T>` value + `std::optional<error_info>` error; default ctor deleted
- **C++20 concepts** — `Resultable`, `Unwrappable`, domain-specific concepts throughout
- **Interface-driven design** — Pure virtual `IExecutor`, `IJob`, `ILogger`, `IDatabase`
- **Error code ranges** — Centralized registry: common (-1 to -99), thread (-100 to -199), etc.
- **snake_case naming** — Enforced by clang-tidy; private members use `_` suffix

## Ecosystem Position

**Tier 0** — Foundation layer. All ecosystem projects depend on this.

| Downstream | Uses |
|------------|------|
| thread_system | `IExecutor` |
| container_system | `Result<T>` |
| logger_system | `Result<T>`, `ILogger` |
| monitoring_system | Event bus |
| database_system | `Result<T>`, `IExecutor` |
| network_system | `IExecutor` |

## Dependencies

**Runtime**: None (header-only)
**Optional**: yaml-cpp (config support)
**Dev/test**: Google Test 1.17.0, Google Benchmark 1.9.5

## Known Constraints

- C++20 required (GCC 11+, Clang 14+, MSVC 2022+, Apple Clang 14+)
- C++20 modules experimental (CMake 3.28+, Clang 16+/GCC 14+; Apple Clang unsupported)
- `Result<T>` is NOT thread-safe for concurrent modification (concurrent reads safe)
- Pre-1.0 (v0.2.0): API may change between minor versions
- Platform: Linux, macOS, Windows; UWP/Xbox excluded
