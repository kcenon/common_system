# Common System - Project Structure

**Language:** **English** | [한국어](PROJECT_STRUCTURE.kr.md)

This document provides a comprehensive overview of the common_system project structure, explaining the purpose of each directory and key files.

---

## Table of Contents

- [Repository Overview](#repository-overview)
- [Directory Structure](#directory-structure)
- [Header Organization](#header-organization)
- [Build System](#build-system)
- [Testing Infrastructure](#testing-infrastructure)
- [Documentation](#documentation)
- [Integration](#integration)

---

## Repository Overview

The common_system is organized as a **header-only library** with supporting build infrastructure, tests, examples, and comprehensive documentation.

### Key Characteristics

- **Header-only**: All core functionality in `include/` directory
- **Zero dependencies**: No external libraries required for core functionality
- **CMake-based**: Modern CMake 3.16+ build system
- **Comprehensive testing**: Unit tests, integration tests, and benchmarks
- **Well-documented**: Doxygen API docs and user guides

---

## Directory Structure

```
common_system/
├── .github/                    # GitHub Actions CI/CD configuration
│   └── workflows/
│       ├── ci.yml             # Main CI pipeline (build, test, sanitizers)
│       ├── coverage.yml        # Code coverage reporting
│       ├── static-analysis.yml # clang-tidy, cppcheck
│       └── build-Doxygen.yaml  # Documentation generation
│
├── cmake/                      # CMake modules and configuration
│   ├── Modules/               # Custom CMake find modules
│   ├── Config.cmake.in        # Package configuration template
│   ├── Dependencies.cmake      # External dependency management
│   └── CompilerFlags.cmake     # Compiler-specific flags
│
├── docs/                       # Documentation (Markdown and Doxygen)
│   ├── guides/                # User guides and tutorials
│   │   ├── BEST_PRACTICES.md
│   │   ├── ERROR_HANDLING.md
│   │   ├── INTEGRATION.md
│   │   ├── QUICK_START.md
│   │   ├── FAQ.md
│   │   ├── TROUBLESHOOTING.md
│   │   ├── RAII_GUIDELINES.md
│   │   ├── SMART_POINTER_GUIDELINES.md
│   │   └── ERROR_CODE_GUIDELINES.md
│   │
│   ├── advanced/              # Advanced topics
│   │   ├── DEPENDENCY_MATRIX.md
│   │   ├── MIGRATION.md
│   │   ├── IEXECUTOR_MIGRATION_GUIDE.md
│   │   ├── NAMESPACE_MIGRATION.md
│   │   └── STRUCTURE.md
│   │
│   ├── contributing/          # Contribution guidelines
│   │   ├── CODE_STYLE.md
│   │   └── PR_TEMPLATE.md
│   │
│   ├── 01-ARCHITECTURE.md     # System architecture overview
│   ├── CHANGELOG.md           # Version history and changes
│   ├── FEATURES.md            # Detailed feature documentation
│   ├── BENCHMARKS.md          # Performance benchmarks
│   ├── PROJECT_STRUCTURE.md   # This file
│   ├── README.md              # Documentation index
│   └── mainpage.dox           # Doxygen main page
│
├── include/                    # Public header files (header-only library)
│   └── kcenon/
│       └── common/
│           ├── common.h        # Master include file
│           │
│           ├── interfaces/     # Abstract interfaces
│           │   ├── executor_interface.h      # IExecutor task execution
│           │   └── ...
│           │
│           ├── patterns/       # Design patterns
│           │   ├── result.h                  # Result<T,E> monad
│           │   ├── event_bus.h               # Event pub/sub
│           │   └── ...
│           │
│           ├── error/          # Error handling
│           │   ├── error_codes.h             # Centralized error codes
│           │   ├── error_info.h              # Error context
│           │   └── exception_mapper.h        # Exception to Result mapping
│           │
│           ├── config/         # Configuration and feature flags
│           │   ├── build_config.h            # Build-time configuration
│           │   ├── feature_flags.h           # Optional features
│           │   └── abi_version.h             # ABI compatibility
│           │
│           ├── utils/          # Utility functions
│           │   ├── source_location.h         # C++20 source_location support
│           │   ├── type_traits.h             # Type utilities
│           │   └── ...
│           │
│           └── adapters/       # Adapter patterns
│               └── ...
│
├── src/                        # Source files (minimal - header-only)
│   ├── config/                # Configuration implementation
│   │   └── build_config.cpp   # Build configuration (if needed)
│   │
│   └── modules/               # C++20 Module files
│       ├── common.cppm        # Primary module interface
│       ├── utils.cppm         # Utility partition
│       ├── error.cppm         # Error codes partition
│       ├── result.cppm        # Result<T> aggregator
│       │   └── result/
│       │       ├── core.cppm
│       │       └── utilities.cppm
│       ├── concepts.cppm      # C++20 concepts partition
│       ├── interfaces.cppm    # Interfaces aggregator
│       │   └── interfaces/
│       │       └── core.cppm
│       ├── config.cppm        # Configuration partition
│       ├── di.cppm            # Dependency injection partition
│       ├── patterns.cppm      # Design patterns partition
│       └── logging.cppm       # Logging utilities partition
│
├── tests/                      # Test suite
│   ├── unit/                  # Unit tests
│   │   ├── exception_mapper_test.cpp
│   │   ├── result_test.cpp
│   │   ├── executor_test.cpp
│   │   └── CMakeLists.txt
│   │
│   └── CMakeLists.txt         # Test build configuration
│
├── integration_tests/          # Integration and ecosystem tests
│   ├── scenarios/             # Integration test scenarios
│   │   ├── executor_integration_test.cpp
│   │   ├── event_bus_integration_test.cpp
│   │   └── cross_module_test.cpp
│   │
│   ├── performance/           # Performance/benchmark tests
│   │   ├── result_benchmarks.cpp
│   │   ├── executor_benchmarks.cpp
│   │   └── event_bus_benchmarks.cpp
│   │
│   ├── failures/              # Compile-fail tests
│   │   └── ...
│   │
│   ├── framework/             # Test framework utilities
│   │   └── test_helpers.h
│   │
│   └── CMakeLists.txt         # Integration test configuration
│
├── examples/                   # Example code and tutorials
│   ├── result_example.cpp     # Result<T> pattern examples
│   ├── executor_example.cpp   # IExecutor usage examples
│   ├── abi_version_example.cpp # ABI version checking example
│   ├── unwrap_demo.cpp        # Error handling patterns
│   └── CMakeLists.txt         # Examples build configuration
│
├── scripts/                    # Build and automation scripts
│   ├── build.sh               # Unix build script
│   ├── build.bat              # Windows build script
│   ├── test.sh                # Unix test runner
│   ├── test.bat               # Windows test runner
│   ├── clean.sh               # Clean build artifacts
│   └── clean.bat              # Windows clean script
│
├── CMakeLists.txt              # Root CMake configuration
├── LICENSE                     # BSD 3-Clause License
├── README.md                   # Main project README
├── README.kr.md                # Korean README
├── CONTRIBUTING.md             # Contribution guidelines
├── .gitignore                  # Git ignore rules
├── .clang-format              # Code formatting rules
├── .clang-tidy                # Static analysis configuration
└── Doxyfile                   # Doxygen configuration
```

---

## Header Organization

### Master Include

**`include/kcenon/common/common.h`**

The master header file that includes all public interfaces. Users can include this single header to access all common_system functionality:

```cpp
#include <kcenon/common/common.h>

// Now you have access to:
// - Result<T> pattern
// - IExecutor interface
// - Event bus
// - Error codes
// - All utilities
```

### Interface Headers

**`include/kcenon/common/interfaces/`**

Abstract interface definitions for ecosystem integration:

| Header | Purpose | Key Types |
|--------|---------|-----------|
| `executor_interface.h` | Task execution abstraction | `IExecutor` |
| `logger_interface.h` | Logging abstraction | `ILogger` (if present) |
| `monitor_interface.h` | Monitoring abstraction | `IMonitor` (if present) |

**Key Design Principles:**
- Pure virtual interfaces (abstract base classes)
- No implementation dependencies
- ABI-stable design
- Documented contracts

### Pattern Headers

**`include/kcenon/common/patterns/`**

Reusable design pattern implementations:

| Header | Purpose | Key Types |
|--------|---------|-----------|
| `result.h` | Result monad for error handling | `Result<T>`, `Ok<T>`, `Error` |
| `event_bus.h` | Event publish/subscribe | `EventBus`, `Event` types |
| `observer.h` | Observer pattern | `Observable<T>`, `Observer<T>` |
| `command.h` | Command pattern | `Command`, `CommandQueue` |

**Usage:**
```cpp
#include <kcenon/common/patterns/result.h>

using namespace kcenon::common;

Result<int> parse_number(const std::string& str) {
    try {
        int value = std::stoi(str);
        return ok(value);
    } catch (const std::exception& e) {
        return make_error<int>(
            error_codes::INVALID_ARGUMENT,
            e.what(),
            "parse_number"
        );
    }
}
```

### Error Handling Headers

**`include/kcenon/common/error/`**

Comprehensive error handling infrastructure:

| Header | Purpose | Contents |
|--------|---------|----------|
| `error_codes.h` | Centralized error code registry | All error codes across ecosystem |
| `error_info.h` | Error context and metadata | `ErrorInfo` struct |
| `exception_mapper.h` | Exception to Result conversion | `try_catch()`, `map_exception()` |

**Error Code Registry:**

The error code registry provides compile-time validated error code ranges:

```cpp
namespace kcenon::common::error_codes {
    // Common system errors (-1 to -99)
    constexpr int SUCCESS = 0;
    constexpr int NOT_FOUND = -1;
    constexpr int INVALID_ARGUMENT = -2;
    constexpr int INITIALIZATION_FAILED = -3;
    // ... more error codes

    // Error code ranges for other systems
    namespace ranges {
        constexpr int COMMON_MIN = -1;
        constexpr int COMMON_MAX = -99;
        constexpr int THREAD_MIN = -100;
        constexpr int THREAD_MAX = -199;
        // ... more ranges
    }
}
```

### Configuration Headers

**`include/kcenon/common/config/`**

Build-time configuration and feature flags:

| Header | Purpose | Key Definitions |
|--------|---------|-----------------|
| `build_config.h` | Build configuration | Feature flags, platform detection |
| `feature_flags.h` | Optional features | `COMMON_ENABLE_COROUTINES`, etc. |
| `abi_version.h` | ABI compatibility | Version constants, compatibility checks |

**Feature Detection:**

```cpp
#include <kcenon/common/config/build_config.h>

#if COMMON_HAS_CPP20
    // Use C++20 features
    #include <source_location>
#else
    // Fallback to C++17
    #include <kcenon/common/utils/source_location.h>
#endif
```

### Utility Headers

**`include/kcenon/common/utils/`**

Utility functions and helper classes:

| Header | Purpose | Contents |
|--------|---------|----------|
| `source_location.h` | Source location support | C++17/C++20 compatibility |
| `type_traits.h` | Type utilities | Template metaprogramming helpers |
| `string_utils.h` | String utilities | String manipulation helpers |
| `scope_guard.h` | RAII scope guard | Automatic cleanup utilities |

---

## Build System

### Root CMakeLists.txt

**Key Configuration Options:**

```cmake
# Build options
option(BUILD_TESTING "Build tests" ON)
option(BUILD_EXAMPLES "Build examples" ON)
option(BUILD_BENCHMARKS "Build benchmarks" OFF)
option(BUILD_DOCS "Build documentation" OFF)

# Integration options
option(BUILD_WITH_THREAD_SYSTEM "Enable thread_system integration" OFF)
option(BUILD_WITH_LOGGER_SYSTEM "Enable logger_system integration" OFF)
option(BUILD_WITH_MONITORING_SYSTEM "Enable monitoring_system integration" OFF)

# Feature flags
option(COMMON_ENABLE_CPP20 "Enable C++20 features" OFF)
option(COMMON_ENABLE_COROUTINES "Enable coroutine support" OFF)

# C++20 Module support
option(COMMON_BUILD_MODULES "Build C++20 module version" OFF)
```

### C++20 Module Support

C++20 modules provide an alternative to header-only usage with potentially faster compilation.

**Requirements:**
- CMake 3.28+
- Ninja generator
- Clang 16+, GCC 14+, or MSVC 2022 17.4+

**Module Structure:**

```
kcenon.common                    # Primary module
├── :utils                       # Tier 1: CircularBuffer, ObjectPool
├── :error                       # Tier 1: Error codes
├── :result                      # Tier 2: Result<T>, error_info
│   ├── :result.core
│   └── :result.utilities
├── :concepts                    # Tier 2: C++20 concepts
├── :interfaces                  # Tier 3: IExecutor, ILogger, etc.
│   └── :interfaces.core
├── :config                      # Tier 3: Configuration
├── :di                          # Tier 3: Dependency injection
├── :patterns                    # Tier 4: EventBus
└── :logging                     # Tier 4: Logging utilities
```

**Building with Modules:**

```bash
cmake -G Ninja -B build -DCOMMON_BUILD_MODULES=ON
cmake --build build
```

### CMake Modules

**`cmake/` Directory:**

| File | Purpose |
|------|---------|
| `Config.cmake.in` | Package config template for `find_package()` |
| `Dependencies.cmake` | External dependency management |
| `CompilerFlags.cmake` | Compiler-specific flags and optimizations |
| `Testing.cmake` | Test configuration and discovery |
| `Sanitizers.cmake` | Address/Thread/UB sanitizer configuration |

### Build Targets

**Available targets:**

```bash
# Main library target (interface library)
kcenon::common

# C++20 Module target (when COMMON_BUILD_MODULES=ON)
kcenon::common_modules

# Test targets
common_tests           # Unit tests
common_integration     # Integration tests
common_benchmarks      # Performance benchmarks

# Example targets
result_example
executor_example
abi_version_example

# Documentation targets
doxygen               # Generate API documentation
```

---

## Testing Infrastructure

### Unit Tests

**Location:** `tests/unit/`

Unit tests use Google Test framework and cover:
- Result<T> pattern operations (success, error, monadic composition)
- Error code registry validation
- Exception mapper functionality
- Type trait utilities

**Running unit tests:**

```bash
./scripts/test.sh
# or
ctest --test-dir build
```

### Integration Tests

**Location:** `integration_tests/`

Integration tests verify:
- Cross-module integration patterns
- IExecutor implementations from thread_system
- Event bus integration with monitoring_system
- Full ecosystem compatibility

**Organization:**

```
integration_tests/
├── scenarios/        # Real-world integration scenarios
├── performance/      # Performance and benchmark tests
├── failures/         # Compile-fail tests (negative tests)
└── framework/        # Test utilities and helpers
```

### Sanitizer Testing

**Automated sanitizer builds in CI:**

- **AddressSanitizer (ASan)**: Detects memory errors
- **ThreadSanitizer (TSan)**: Detects data races
- **UndefinedBehaviorSanitizer (UBSan)**: Catches undefined behavior

**Run locally:**

```bash
# Build with sanitizers
./scripts/build.sh --sanitize

# Run tests with sanitizers
./scripts/test.sh --sanitize
```

### Coverage Reporting

**Code coverage tracking:**

```bash
# Build with coverage
./scripts/build.sh --coverage

# Run tests and generate report
./scripts/test.sh --coverage

# View HTML report
open build/coverage/index.html
```

---

## Documentation

### User Documentation

**Location:** `docs/`

Documentation is organized by audience and topic:

**Getting Started:**
- `docs/guides/QUICK_START.md` - Quick start guide
- `docs/guides/INTEGRATION.md` - Integration guide
- `docs/01-ARCHITECTURE.md` - Architecture overview

**User Guides:**
- `docs/guides/ERROR_HANDLING.md` - Error handling patterns
- `docs/guides/BEST_PRACTICES.md` - Best practices
- `docs/guides/TROUBLESHOOTING.md` - Common issues and solutions
- `docs/guides/FAQ.md` - Frequently asked questions

**Advanced Topics:**
- `docs/advanced/MIGRATION.md` - Migration from older versions
- `docs/advanced/DEPENDENCY_MATRIX.md` - Dependency analysis
- `docs/advanced/STRUCTURE.md` - Detailed structure documentation

**Reference:**
- `docs/FEATURES.md` - Complete feature documentation
- `docs/BENCHMARKS.md` - Performance benchmarks
- `docs/PROJECT_STRUCTURE.md` - This document

### API Documentation

**Doxygen-generated API docs:**

All public headers include comprehensive Doxygen comments:

```cpp
/**
 * @brief Submit a task for asynchronous execution
 *
 * @tparam F Callable type (lambda, function object, function pointer)
 * @tparam Args Argument types for the callable
 * @param func The callable to execute
 * @param args Arguments to pass to the callable
 * @return std::future with the result of the callable
 *
 * @note This method is thread-safe and can be called concurrently
 * @throws std::bad_alloc if task queue allocation fails
 *
 * @example
 * @code
 * auto future = executor->submit([]() { return 42; });
 * int result = future.get();
 * @endcode
 */
template<typename F, typename... Args>
auto submit(F&& func, Args&&... args) -> std::future</*...*/>;
```

**Generate API docs:**

```bash
./scripts/build.sh --docs
# or
doxygen Doxyfile
```

---

## Integration

### Ecosystem Integration

The common_system integrates with all other systems in the KCENON ecosystem:

**Integration Pattern:**

1. **Define interfaces** in common_system (e.g., `IExecutor`)
2. **Implement interfaces** in specialized systems (e.g., `thread_system::thread_pool`)
3. **Adapt implementations** to common interfaces via adapter pattern
4. **Use common interfaces** in client code for loose coupling

**Example Integration:**

```cpp
// 1. Common system defines interface
namespace kcenon::common::interfaces {
    class IExecutor { /* ... */ };
}

// 2. Thread system implements executor
namespace kcenon::thread {
    class thread_pool { /* ... */ };
}

// 3. Adapter converts thread_pool to IExecutor
namespace kcenon::thread::adapters {
    std::shared_ptr<common::interfaces::IExecutor>
    make_common_executor(std::shared_ptr<thread_pool> pool);
}

// 4. Client uses common interface
void client_code(std::shared_ptr<common::interfaces::IExecutor> executor) {
    executor->submit([]() { /* work */ });
}
```

### CMake Integration

**As a subdirectory:**

```cmake
add_subdirectory(common_system)
target_link_libraries(my_app PRIVATE kcenon::common)
```

**Using FetchContent:**

```cmake
include(FetchContent)
FetchContent_Declare(
    common_system
    GIT_REPOSITORY https://github.com/kcenon/common_system.git
    GIT_TAG main
)
FetchContent_MakeAvailable(common_system)
target_link_libraries(my_app PRIVATE kcenon::common)
```

**As installed package:**

```cmake
find_package(common_system REQUIRED)
target_link_libraries(my_app PRIVATE kcenon::common)
```

---

## File Naming Conventions

### Headers

- **Interface headers**: `*_interface.h` (e.g., `executor_interface.h`)
- **Pattern headers**: `*.h` (e.g., `result.h`, `event_bus.h`)
- **Utility headers**: `*_utils.h` (e.g., `string_utils.h`)
- **Config headers**: `*_config.h` (e.g., `build_config.h`)

### Tests

- **Unit tests**: `*_test.cpp` (e.g., `result_test.cpp`)
- **Integration tests**: `*_integration_test.cpp`
- **Benchmark tests**: `*_benchmarks.cpp`

### Examples

- **Example files**: `*_example.cpp` (e.g., `executor_example.cpp`)
- **Demo files**: `*_demo.cpp` (e.g., `unwrap_demo.cpp`)

---

## Build Artifacts

### Build Directory Structure

After building, the `build/` directory contains:

```
build/
├── include/                  # Generated/configured headers
├── lib/                      # Libraries (if any)
├── bin/                      # Executables (tests, examples)
│   ├── common_tests
│   ├── result_example
│   └── executor_example
├── tests/                    # Test results
│   └── test-results.xml
├── coverage/                 # Coverage reports
│   └── index.html
├── doxygen/                  # API documentation
│   └── html/
│       └── index.html
└── CMakeFiles/              # CMake internal files
```

### Installed Files

After `cmake --install`, the following files are installed:

```
<install-prefix>/
├── include/
│   └── kcenon/
│       └── common/
│           └── [all headers]
├── lib/
│   └── cmake/
│       └── common_system/
│           ├── common_systemConfig.cmake
│           └── common_systemTargets.cmake
└── share/
    └── doc/
        └── common_system/
            └── [documentation]
```

---

## Development Workflow

### Adding a New Interface

1. Create header in `include/kcenon/common/interfaces/`
2. Add Doxygen documentation
3. Update `include/kcenon/common/common.h` to include new header
4. Add unit tests in `tests/unit/`
5. Add integration tests in `integration_tests/scenarios/`
6. Add example in `examples/`
7. Document in `docs/guides/`

### Adding a New Pattern

1. Create header in `include/kcenon/common/patterns/`
2. Implement template methods inline (header-only)
3. Add comprehensive Doxygen comments
4. Create unit tests
5. Add usage examples
6. Update documentation

---

**Last Updated**: 2025-01-03
**Version**: 0.1.1.0
