[![CI](https://github.com/kcenon/common_system/actions/workflows/ci.yml/badge.svg)](https://github.com/kcenon/common_system/actions/workflows/ci.yml)
[![Code Coverage](https://github.com/kcenon/common_system/actions/workflows/coverage.yml/badge.svg)](https://github.com/kcenon/common_system/actions/workflows/coverage.yml)
[![Static Analysis](https://github.com/kcenon/common_system/actions/workflows/static-analysis.yml/badge.svg)](https://github.com/kcenon/common_system/actions/workflows/static-analysis.yml)
[![Doxygen](https://github.com/kcenon/common_system/actions/workflows/build-Doxygen.yaml/badge.svg)](https://github.com/kcenon/common_system/actions/workflows/build-Doxygen.yaml)
[![License](https://img.shields.io/github/license/kcenon/common_system)](https://github.com/kcenon/common_system/blob/main/LICENSE)

# Common System

> **Language:** **English** | [한국어](README.kr.md)

## Overview

A foundational C++20 header-only library providing essential interfaces and design patterns for building modular, loosely-coupled system architectures. Designed as the cornerstone of the ecosystem, it enables seamless integration between system modules while maintaining zero runtime overhead through template-based abstractions and interface-driven design.

**Key Value Propositions**:
- 🚀 **Zero-overhead abstractions**: Template-based interfaces with compile-time resolution
- 🔒 **Well-tested**: 80%+ test coverage, zero sanitizer warnings, full CI/CD
- 🏗️ **Header-only design**: No library linking, no dependencies, instant integration
- 🛡️ **C++20 Module support**: Optional module-based build for faster compilation
- 🌐 **Ecosystem foundation**: Powers thread_system, network_system, database_system, and more

**Latest Updates** (2026-01):
- ✅ Complete separation from individual modules
- ✅ Comprehensive Result<T> pattern implementation
- ✅ IExecutor interface standardization with ABI version checking
- ✅ Health monitoring system with dependency graph and recovery handlers
- ✅ Circuit breaker pattern for fault tolerance and resilience
- ✅ IStats interface for unified statistics collection and monitoring

---

## Quick Start

### Basic Example

```cpp
#include <kcenon/common/patterns/result.h>

using namespace kcenon::common;

Result<Config> load_config(const std::string& path) {
    if (!std::filesystem::exists(path)) {
        return make_error<Config>(
            error_codes::NOT_FOUND,
            "Configuration file not found",
            "config_loader"
        );
    }
    auto config = parse_json_file(path);
    return ok(config);
}

// Usage with monadic operations
auto result = load_config("app.conf")
    .and_then(validate_config)
    .map(apply_defaults);
```

📖 **[Full Getting Started Guide →](docs/guides/QUICK_START.md)**

---

## Requirements

| Dependency | Version | Required | Description |
|------------|---------|----------|-------------|
| C++20 Compiler | GCC 11+ / Clang 14+ / MSVC 2022+ / Apple Clang 14+ | Yes | C++20 features required |
| CMake | 3.20+ | Yes | Build system |

### Dependency Flow

```
common_system (Foundation Layer - No Dependencies)
       │
       │ provides interfaces to
       │
       ├── thread_system (implements IExecutor)
       ├── logger_system (uses Result<T>)
       ├── container_system (uses Result<T>)
       ├── monitoring_system (event bus)
       ├── network_system (uses IExecutor)
       └── database_system (uses Result<T> and IExecutor)
```

### Ecosystem-Wide Compiler Requirements

When using multiple systems together, use the **highest** requirement from your dependency chain:

| Usage Scenario | GCC | Clang | MSVC | Notes |
|----------------|-----|-------|------|-------|
| common_system only | 11+ | 14+ | 2022+ | Baseline |
| + thread_system | **13+** | **17+** | 2022+ | Higher requirements |
| + logger_system | 11+ | 14+ | 2022+ | Optional thread_system |
| + container_system | 11+ | 14+ | 2022+ | Uses common_system |
| + monitoring_system | **13+** | **17+** | 2022+ | Requires thread_system |
| + database_system | **13+** | **17+** | 2022+ | Full ecosystem |
| + network_system | **13+** | **17+** | 2022+ | Requires thread_system |

> **Note**: If using any system that depends on thread_system, you need GCC 13+ or Clang 17+.

---

## Installation

### Option 1: Header-Only Usage (Simplest)

```bash
git clone https://github.com/kcenon/common_system.git
# Include headers directly - no build required!
```

```cpp
#include <kcenon/common/interfaces/executor_interface.h>
#include <kcenon/common/patterns/result.h>
```

### Option 2: CMake Integration (Recommended)

```cmake
include(FetchContent)
FetchContent_Declare(
    common_system
    GIT_REPOSITORY https://github.com/kcenon/common_system.git
    GIT_TAG main
)
FetchContent_MakeAvailable(common_system)

target_link_libraries(your_target PRIVATE kcenon::common)
```

### Option 3: C++20 Modules

```bash
# Build with C++20 module support (requires CMake 3.28+, Ninja, Clang 16+/GCC 14+)
cmake -G Ninja -B build -DCOMMON_BUILD_MODULES=ON
cmake --build build
```

```cpp
import kcenon.common;

int main() {
    auto result = kcenon::common::ok(42);
    if (result.is_ok()) {
        std::cout << result.value() << std::endl;
    }
    return 0;
}
```

### Option 4: Conan Package Manager

```bash
conan create . --build=missing
```

---

## Architecture

### Ecosystem Integration

This common system serves as the foundational layer that all other system modules build upon:

```
                    ┌──────────────────┐
                    │  common_system   │ ◄── Foundation Layer
                    │  (interfaces)    │
                    └────────┬─────────┘
                             │ provides interfaces
       ┌─────────────────────┼─────────────────────┐
       │                     │                     │
┌──────▼───────┐    ┌────────▼────────┐   ┌───────▼────────┐
│thread_system │    │network_system   │   │monitoring_sys. │
│(implements   │    │(uses IExecutor) │   │(event bus)     │
│ IExecutor)   │    └─────────────────┘   └────────────────┘
└──────────────┘             │                     │
       │                     │                     │
       └─────────────────────┼─────────────────────┘
                             │ all use
                    ┌────────▼─────────┐
                    │ Result<T> pattern│
                    │ Error handling   │
                    └──────────────────┘
```

📖 **[Complete Architecture Guide →](docs/01-ARCHITECTURE.md)**

---

## Core Features

### IExecutor Interface

Universal task execution abstraction for any threading backend:

```cpp
#include <kcenon/common/interfaces/executor_interface.h>

class MyService {
    std::shared_ptr<common::interfaces::IExecutor> executor_;

public:
    void process_async(const Data& data) {
        auto future = executor_->submit([data]() {
            return process(data);
        });
    }
};
```

### Result<T> Pattern

Type-safe error handling without exceptions, inspired by Rust:

```cpp
#include <kcenon/common/patterns/result.h>

auto result = load_config("app.conf")
    .and_then(validate_config)
    .map(apply_defaults)
    .or_else([](const auto& error) {
        log_error(error);
        return load_fallback_config();
    });
```

### Health Monitoring

Comprehensive health check system with dependency graph:

```cpp
#include <kcenon/common/interfaces/monitoring.h>

auto& monitor = global_health_monitor();

auto db_check = health_check_builder()
    .name("database")
    .type(health_check_type::dependency)
    .timeout(std::chrono::seconds{5})
    .with_check([]() { /* check logic */ })
    .build();

monitor.register_check("database", db_check.value());
monitor.add_dependency("api", "database");
```

📖 **[Detailed Features Documentation →](docs/FEATURES.md)**

---

## Documentation

| Category | Document | Description |
|----------|----------|-------------|
| **Guides** | [Quick Start](docs/guides/QUICK_START.md) | Get up and running in minutes |
| | [Best Practices](docs/guides/BEST_PRACTICES.md) | Recommended usage patterns |
| | [FAQ](docs/guides/FAQ.md) | Frequently asked questions |
| | [Troubleshooting](docs/guides/TROUBLESHOOTING.md) | Common issues and solutions |
| **Advanced** | [Architecture](docs/01-ARCHITECTURE.md) | System design and principles |
| | [Migration](docs/advanced/MIGRATION.md) | Version upgrade guide |
| | [IExecutor Migration](docs/advanced/IEXECUTOR_MIGRATION_GUIDE.md) | Executor API migration |
| | [Runtime Binding](docs/architecture/RUNTIME_BINDING.md) | Core design pattern |
| **Contributing** | [Contributing](CONTRIBUTING.md) | How to contribute |
| | [Error Code Guidelines](docs/guides/ERROR_CODE_GUIDELINES.md) | Error code management |

---

## Performance

| Operation | Time (ns) | Allocations | Notes |
|-----------|-----------|-------------|-------|
| Result<T> creation | 2.3 | 0 | Stack-only operation |
| Result<T> error check | 0.8 | 0 | Single bool check |
| IExecutor submit | 45.2 | 1 | Task queue insertion |
| Event publish | 12.4 | 0 | Lock-free operation |

**Key Performance Characteristics:**
- Result<T> is 400x faster than exceptions in error paths
- IExecutor is 53x faster than std::async for high-frequency tasks
- Zero-overhead abstractions - compiler optimizes away all abstraction layers

📖 **[Full Benchmarks →](docs/BENCHMARKS.md)**

---

## Error Handling Foundation

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

---

## Production Quality

### Quality Metrics
- **Test coverage**: 80%+ (target: 85%)
- **Sanitizer tests**: 18/18 passing with zero warnings
- **Cross-platform**: Ubuntu, macOS, Windows
- **Zero memory leaks**: AddressSanitizer verified
- **Zero data races**: ThreadSanitizer verified

### RAII Grade: A
- All resources managed through smart pointers
- No manual memory management in any interface
- Exception-safe design validated

---

## Contributing

We welcome contributions! Please see [CONTRIBUTING.md](docs/contributing/CONTRIBUTING.md) for guidelines.

### Quick Links

- [Development Setup](docs/contributing/CONTRIBUTING.md#development-workflow)
- [Code Style](docs/contributing/CONTRIBUTING.md#code-style)
- [Pull Request Process](docs/contributing/CONTRIBUTING.md#development-workflow)

---

## Support

- **Issues**: [GitHub Issues](https://github.com/kcenon/common_system/issues)
- **Discussions**: [GitHub Discussions](https://github.com/kcenon/common_system/discussions)
- **Email**: kcenon@naver.com

---

## License

This project is licensed under the BSD 3-Clause License - see the [LICENSE](LICENSE) file for details.

---

## Acknowledgments

- Inspired by Rust's Result<T,E> type and error handling
- Interface design influenced by Java's ExecutorService
- Event bus pattern from reactive programming frameworks
