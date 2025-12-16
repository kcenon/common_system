[![CI](https://github.com/kcenon/common_system/actions/workflows/ci.yml/badge.svg)](https://github.com/kcenon/common_system/actions/workflows/ci.yml)
[![Code Coverage](https://github.com/kcenon/common_system/actions/workflows/coverage.yml/badge.svg)](https://github.com/kcenon/common_system/actions/workflows/coverage.yml)
[![Static Analysis](https://github.com/kcenon/common_system/actions/workflows/static-analysis.yml/badge.svg)](https://github.com/kcenon/common_system/actions/workflows/static-analysis.yml)
[![Doxygen](https://github.com/kcenon/common_system/actions/workflows/build-Doxygen.yaml/badge.svg)](https://github.com/kcenon/common_system/actions/workflows/build-Doxygen.yaml)
[![License](https://img.shields.io/github/license/kcenon/common_system)](https://github.com/kcenon/common_system/blob/main/LICENSE)

# Common System Project

> **Language:** **English** | [한국어](README_KO.md)

## Overview

The Common System Project is a foundational C++17 header-only library providing essential interfaces and design patterns for building modular, loosely-coupled system architectures. Designed as the cornerstone of the ecosystem, it enables seamless integration between system modules while maintaining zero runtime overhead through template-based abstractions and interface-driven design.

### Key Highlights

- **Zero-overhead abstractions**: Template-based interfaces with compile-time resolution
- **Header-only design**: No library linking, no dependencies, instant integration
- **Well-tested**: 80%+ test coverage, zero sanitizer warnings, full CI/CD
- **Universal compatibility**: C++20 standard with modern language features
- **Ecosystem foundation**: Powers thread_system, network_system, database_system, and more

> **Latest Updates**: Complete separation from individual modules, comprehensive Result<T> pattern implementation, IExecutor interface standardization with ABI version checking, unified `kcenon::common` namespace, event bus integration tests, and enhanced documentation structure.

## Core Features

- **IExecutor Interface**: Universal task execution abstraction for any threading backend
- **Result<T> Pattern**: Type-safe error handling without exceptions, inspired by Rust
- **Event Bus**: Publish-subscribe pattern for decoupled event-driven architecture
- **Error Code Registry**: Centralized error code system across all ecosystem modules
- **Smart Interfaces**: Mockable abstractions for easy testing and dependency injection
- **C++20 Concepts**: Compile-time type validation with clear error messages

[📚 Detailed Features Documentation →](docs/FEATURES.md)

## Project Ecosystem

This common system serves as the foundational layer that all other system modules build upon:

### Dependency Architecture

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

### Dependent Projects

- **[thread_system](https://github.com/kcenon/thread_system)**: Core threading framework implementing IExecutor
- **[network_system](https://github.com/kcenon/network_system)**: Asynchronous network library using IExecutor
- **[logger_system](https://github.com/kcenon/logger_system)**: High-performance logging with Result<T>
- **[monitoring_system](https://github.com/kcenon/monitoring_system)**: Metrics and event bus implementation
- **[container_system](https://github.com/kcenon/container_system)**: Data serialization with Result<T>
- **[database_system](https://github.com/kcenon/database_system)**: Database abstraction with Result<T> and IExecutor

[🏗️ Complete Architecture Guide →](docs/01-ARCHITECTURE.md)

## Quick Start

### System Requirements

- **Compiler**: C++20 compatible (GCC 11+, Clang 14+, MSVC 2022+, Apple Clang 14+)
- **Build System**: CMake 3.20 or higher
- **Platform**: Windows, Linux, macOS (x86_64, ARM64)

### Installation

#### Option 1: Header-Only Usage (Simplest)

```bash
git clone https://github.com/kcenon/common_system.git
# Include headers directly - no build required!
```

```cpp
#include <kcenon/common/interfaces/executor_interface.h>
#include <kcenon/common/patterns/result.h>
```

#### Option 2: CMake Integration

```cmake
# Using FetchContent (recommended)
include(FetchContent)
FetchContent_Declare(
    common_system
    GIT_REPOSITORY https://github.com/kcenon/common_system.git
    GIT_TAG main
)
FetchContent_MakeAvailable(common_system)

target_link_libraries(your_target PRIVATE kcenon::common)
```

#### Option 3: Conan Package Manager

```bash
# Add the package from source
conan create . --build=missing

# Or add to your conanfile.txt
[requires]
common_system/1.0.0

# Then install
conan install . --build=missing
```

#### Option 4: System-wide Installation

```bash
git clone https://github.com/kcenon/common_system.git
cd common_system
./scripts/build.sh --release --install-prefix=/usr/local
sudo cmake --build build --target install
```

### Building from Source

```bash
# Clone repository
git clone https://github.com/kcenon/common_system.git
cd common_system

# Build with tests and examples
./scripts/build.sh --release --tests --examples

# Run tests
./scripts/test.sh

# Clean build artifacts
./scripts/clean.sh
```

### Windows Build

```batch
REM Using Visual Studio 2022
scripts\build.bat --vs2022 --release

REM Run tests
scripts\test.bat --release

REM Clean artifacts
scripts\clean.bat
```

[📖 Full Getting Started Guide →](docs/guides/QUICK_START.md)

## Usage Examples

### IExecutor Interface

Universal task execution abstraction for any threading backend:

```cpp
#include <kcenon/common/interfaces/executor_interface.h>

class MyService {
    std::shared_ptr<common::interfaces::IExecutor> executor_;

public:
    void process_async(const Data& data) {
        auto future = executor_->submit([data]() {
            // Process data asynchronously
            return process(data);
        });

        // Continue with other work...
    }
};
```

### Result<T> Pattern

Type-safe error handling without exceptions:

```cpp
#include <kcenon/common/patterns/result.h>

common::Result<Config> load_config(const std::string& path) {
    if (!std::filesystem::exists(path)) {
        return common::make_error<Config>(
            common::error_codes::NOT_FOUND,
            "Configuration file not found",
            "config_loader"
        );
    }

    try {
        auto config = parse_json_file(path);
        return common::ok(config);
    } catch (const std::exception& e) {
        return common::make_error<Config>(
            common::error_codes::INVALID_ARGUMENT,
            e.what(),
            "config_loader"
        );
    }
}

// Usage with monadic operations
auto result = load_config("app.conf")
    .and_then(validate_config)
    .map(apply_defaults)
    .or_else([](const auto& error) {
        log_error(error);
        return load_fallback_config();
    });
```

### Event Bus Integration

When used with monitoring_system:

```cpp
#include <kcenon/common/patterns/event_bus.h>

// Publish events
auto bus = common::get_event_bus();
bus->publish(common::events::module_started_event("my_service"));

// Subscribe to events
bus->subscribe<common::events::error_event>([](const auto& event) {
    std::cerr << "Error in " << event.module_name
              << ": " << event.error_message << std::endl;
});
```

[📘 More Examples →](examples/)

## Integration Examples

### With thread_system

```cpp
#include <kcenon/thread/core/thread_pool.h>
#include <kcenon/thread/adapters/common_executor_adapter.h>

// Create thread pool
auto thread_pool = std::make_shared<kcenon::thread::thread_pool>(4);

// Adapt to common interface
auto executor = kcenon::thread::adapters::make_common_executor(thread_pool);

// Use with any IExecutor-based API
process_with_executor(executor);
```

### With network_system

```cpp
#include <network_system/integration/executor_adapter.h>

// Use common executor with network system
void setup_network(std::shared_ptr<common::interfaces::IExecutor> executor) {
    auto adapted_pool = kcenon::network::integration::make_thread_pool_adapter(executor);

    network_system::server server(adapted_pool);
    // Network operations now use the common executor
}
```

[🔗 Integration Guide →](docs/guides/INTEGRATION.md)

## Performance Highlights

| Operation | Time (ns) | Allocations | Notes |
|-----------|-----------|-------------|-------|
| Result<T> creation | 2.3 | 0 | Stack-only operation |
| Result<T> error check | 0.8 | 0 | Single bool check |
| IExecutor submit | 45.2 | 1 | Task queue insertion |
| Event publish | 12.4 | 0 | Lock-free operation |

*Platform: Intel i7-9700K @ 3.6GHz, GCC 11.2 -O3*

**Key Performance Characteristics:**
- Zero-overhead abstractions - compiler optimizes away all abstraction layers
- Result<T> is 400x faster than exceptions in error paths
- IExecutor is 53x faster than std::async for high-frequency tasks
- Event bus scales linearly with subscriber count
- GlobalLoggerRegistry is thread-safe with O(1) logger access

[⚡ Full Benchmarks →](docs/BENCHMARKS.md)

## Documentation

### Getting Started
- [Quick Start Guide](docs/guides/QUICK_START.md) - Get up and running in minutes
- [Architecture Overview](docs/01-ARCHITECTURE.md) - System design and principles
- [Integration Guide](docs/guides/INTEGRATION.md) - Integrate with your project

### Core Documentation
- [Features](docs/FEATURES.md) - Detailed feature descriptions
- [Error Handling Guide](docs/guides/ERROR_HANDLING.md) - Result<T> pattern and best practices
- [Best Practices](docs/guides/BEST_PRACTICES.md) - Recommended usage patterns
- [FAQ](docs/guides/FAQ.md) - Frequently asked questions

### Runtime Binding (v2.0)
- [Runtime Binding Architecture](docs/architecture/RUNTIME_BINDING.md) - Core design pattern for decoupled systems
- [Migration Guide](docs/guides/MIGRATION_RUNTIME_BINDING.md) - Migrate to runtime binding pattern
- [Logging Best Practices](docs/guides/LOGGING_BEST_PRACTICES.md) - Effective logging patterns
- [Logging Troubleshooting](docs/guides/TROUBLESHOOTING_LOGGING.md) - Diagnose logging issues

### Advanced Topics
- [Migration Guide](docs/advanced/MIGRATION.md) - Migrating to common_system
- [IExecutor Migration](docs/advanced/IEXECUTOR_MIGRATION_GUIDE.md) - Executor API migration
- [RAII Guidelines](docs/guides/RAII_GUIDELINES.md) - Resource management patterns
- [Smart Pointer Guidelines](docs/guides/SMART_POINTER_GUIDELINES.md) - Smart pointer usage

### C++20 Features
- [Concepts Guide](docs/guides/CONCEPTS_GUIDE.md) - C++20 Concepts for compile-time type validation

### Reference
- [Error Code Guidelines](docs/guides/ERROR_CODE_GUIDELINES.md) - Error code management
- [Project Structure](docs/PROJECT_STRUCTURE.md) - Repository organization
- [Dependency Matrix](docs/advanced/DEPENDENCY_MATRIX.md) - Ecosystem dependencies
- [Compatibility Matrix](docs/COMPATIBILITY.md) - Version compatibility across systems
- [Troubleshooting](docs/guides/TROUBLESHOOTING.md) - Common issues and solutions

[📖 Complete Documentation Index →](docs/)

## Testing

The project includes comprehensive testing:

```bash
# Run all tests
./scripts/test.sh

# Run with coverage
./scripts/test.sh --coverage

# Run specific tests
./scripts/test.sh --filter "Result*"

# Benchmark tests
./scripts/test.sh --benchmark
```

**Quality Metrics:**
- Test coverage: 80%+ (target: 85%)
- Sanitizer tests: 18/18 passing with zero warnings
- Cross-platform: Ubuntu, macOS, Windows
- Zero memory leaks (AddressSanitizer verified)
- Zero data races (ThreadSanitizer verified)

## Production Quality

### Multi-Platform CI/CD
- Automated sanitizer builds (ThreadSanitizer, AddressSanitizer, UBSanitizer)
- Cross-platform testing: Ubuntu (GCC/Clang), macOS (Apple Clang), Windows (MSVC)
- Code coverage tracking with codecov integration
- Static analysis with clang-tidy and cppcheck

### Thread Safety
- All interfaces designed for safe concurrent access
- Result<T> is immutable and thread-safe after construction
- IExecutor contract specifies concurrent call guarantees
- Event bus operations use lock-free design where possible

### Resource Management (RAII - Grade A)
- All resources managed through smart pointers
- No manual memory management in any interface
- AddressSanitizer validation: 18/18 tests pass with zero memory leaks
- Exception-safe design validated

### Error Handling Foundation

Centralized error code registry providing system-specific ranges:

- common_system: -1 to -99
- thread_system: -100 to -199
- logger_system: -200 to -299
- monitoring_system: -300 to -399
- container_system: -400 to -499
- database_system: -500 to -599
- network_system: -600 to -699

Compile-time validation prevents code conflicts across all systems. All dependent systems successfully adopted the Result<T> pattern and error code registry.

## Contributing

We welcome contributions! Please see [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

### Development Workflow

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'feat: add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

### Code Style

- Follow the existing code style (clang-format configuration provided)
- Write comprehensive unit tests for new features
- Update documentation as needed
- Ensure all tests pass before submitting PR

## Roadmap

**Completed:**
- [x] IExecutor interface standardization
- [x] Result<T> pattern implementation
- [x] Event bus forwarding
- [x] Centralized build configuration
- [x] ABI version checking for compatibility validation
- [x] Unified `kcenon::common` namespace
- [x] Task-based IExecutor interface
- [x] Comprehensive documentation reorganization
- [x] C++20 standard with modern language features
- [x] Runtime binding architecture (GlobalLoggerRegistry, SystemBootstrapper)
- [x] Unified logging macros (LOG_*)
- [x] C++20 source_location integration
- [x] C++20 Concepts for type validation
- [x] Package manager support (Conan)

**Planned:**
- [ ] Coroutine support for async patterns
- [ ] std::expected migration (C++23)
- [ ] Additional design patterns (Observer, Command)
- [ ] Package manager official registry (vcpkg, Conan Center)

## Support

- **Issues**: [GitHub Issues](https://github.com/kcenon/common_system/issues)
- **Discussions**: [GitHub Discussions](https://github.com/kcenon/common_system/discussions)
- **Email**: kcenon@naver.com

## License

This project is licensed under the BSD 3-Clause License - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

- Inspired by Rust's Result<T,E> type and error handling
- Interface design influenced by Java's ExecutorService
- Event bus pattern from reactive programming frameworks
- Build system patterns from modern C++ best practices

## Citation

If you use this project in your research or commercial projects, please cite:

```bibtex
@software{common_system,
  author = {Dongcheol Shin},
  title = {Common System: Foundational Interfaces for Modular C++ Architecture},
  year = {2024},
  url = {https://github.com/kcenon/common_system}
}
```

---

<p align="center">
  Made with ❤️ by 🍀☀🌕🌥 🌊
</p>
