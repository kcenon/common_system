---
doc_id: "COM-FEAT-002c"
doc_title: "Common System - Integration & Advanced Features"
doc_version: "1.0.0"
doc_date: "2026-04-04"
doc_status: "Released"
project: "common_system"
category: "FEAT"
---

# Common System - Integration & Advanced Features

> This document covers ecosystem integration, production quality features, and advanced capabilities of the Common System.
> Split from [FEATURES.md](FEATURES.md) for readability.

---

## Integration Features

### With thread_system

Complete integration examples for using thread_system with common_system interfaces:

```cpp
#include <kcenon/thread/core/thread_pool.h>
#include <kcenon/thread/adapters/common_executor_adapter.h>

// Create thread pool
auto thread_pool = std::make_shared<kcenon::thread::thread_pool>(
    4  // worker threads
);

// Adapt to common IExecutor interface
auto executor = kcenon::thread::adapters::make_common_executor(thread_pool);

// Now use with any IExecutor-based API
void process_with_executor(std::shared_ptr<common::interfaces::IExecutor> exec) {
    auto future = exec->submit([]() {
        return compute_expensive_operation();
    });

    // Do other work while computation runs...

    auto result = future.get();
}

process_with_executor(executor);
```

### With network_system

Network operations using common executor abstraction:

```cpp
#include <network_system/integration/executor_adapter.h>
#include <network_system/server.h>

void setup_network(std::shared_ptr<common::interfaces::IExecutor> executor) {
    // Adapt common executor to network system's thread pool interface
    auto network_pool = kcenon::network::integration::make_thread_pool_adapter(executor);

    // Create server with adapted executor
    network_system::server server(network_pool);

    // All network operations now use the common executor
    server.listen(8080);
}
```

### With logger_system

Error handling integration:

```cpp
#include <kcenon/logger/logger.h>
#include <kcenon/common/patterns/result.h>

common::Result<void> initialize_logging(const std::string& log_path) {
    try {
        auto logger = kcenon::logger::create_logger(log_path);

        if (!logger) {
            return common::make_error<void>(
                common::error_codes::INITIALIZATION_FAILED,
                "Failed to create logger",
                "initialize_logging"
            );
        }

        return common::ok();

    } catch (const std::exception& e) {
        return common::make_error<void>(
            common::error_codes::EXCEPTION,
            e.what(),
            "initialize_logging"
        );
    }
}

// Usage
auto result = initialize_logging("/var/log/app.log");
if (!result) {
    std::cerr << "Logging initialization failed: "
              << result.error().message << "\n";
    return 1;
}
```

### Unified Feature Flags

The common_system provides a unified feature flag system for detecting C++ features and controlling system integration across all ecosystem modules.

**Header Organization:**

```cpp
#include <kcenon/common/config/feature_flags.h>  // Main entry point

// Individual headers for specific needs:
// feature_flags_core.h    - Preprocessor helpers, compiler/platform detection
// feature_detection.h     - C++ standard library feature detection
// feature_system_deps.h   - System module integration flags
```

**C++ Feature Detection Macros:**

| Macro | Description | Detected Via |
|-------|-------------|--------------|
| `KCENON_HAS_CPP17` | C++17 support | `__cplusplus >= 201703L` |
| `KCENON_HAS_CPP20` | C++20 support | `__cplusplus >= 202002L` |
| `KCENON_HAS_CPP23` | C++23 support | `__cplusplus >= 202302L` |
| `KCENON_HAS_SOURCE_LOCATION` | `std::source_location` | `__cpp_lib_source_location` |
| `KCENON_HAS_JTHREAD` | `std::jthread` | `__cpp_lib_jthread` |
| `KCENON_HAS_FORMAT` | `std::format` | `__cpp_lib_format` |
| `KCENON_HAS_CONCEPTS` | C++20 concepts | `__cpp_concepts` |
| `KCENON_HAS_RANGES` | C++20 ranges | `__cpp_lib_ranges` |
| `KCENON_HAS_COROUTINES` | C++20 coroutines | `__cpp_impl_coroutine` |
| `KCENON_HAS_EXPECTED` | `std::expected` (C++23) | `__cpp_lib_expected` |

**System Integration Macros:**

| Macro | Description |
|-------|-------------|
| `KCENON_WITH_COMMON_SYSTEM` | common_system types available (auto-defined when header is included) |
| `KCENON_WITH_THREAD_SYSTEM` | thread_system integration enabled |
| `KCENON_WITH_LOGGER_SYSTEM` | logger_system integration enabled |
| `KCENON_WITH_MONITORING_SYSTEM` | monitoring_system integration enabled |
| `KCENON_WITH_CONTAINER_SYSTEM` | container_system integration enabled |
| `KCENON_WITH_NETWORK_SYSTEM` | network_system integration enabled |
| `KCENON_WITH_DATABASE_SYSTEM` | database_system integration enabled |
| `KCENON_WITH_MESSAGING_SYSTEM` | messaging_system integration enabled |

**Usage Example:**

```cpp
#include <kcenon/common/config/feature_flags.h>

#if KCENON_HAS_SOURCE_LOCATION
    #include <source_location>
    using location_type = std::source_location;
#else
    using location_type = kcenon::common::source_location;
#endif

#if KCENON_HAS_JTHREAD
    std::jthread worker([](std::stop_token st) { /* ... */ });
#else
    std::thread worker([]{ /* ... */ });
#endif
```

**CMake Integration:**

The `features.cmake` module provides functions for configuring feature flags:

```cmake
include(cmake/features.cmake)

# Configure feature flags for a target
kcenon_configure_features(my_target
    THREAD_SYSTEM ON
    LOGGER_SYSTEM ON
    LEGACY_ALIASES ON
)

# Detect features at configure time
kcenon_detect_features()
message(STATUS "jthread available: ${KCENON_DETECTED_JTHREAD}")
```

**Legacy Alias Support:**

For backward compatibility, legacy macro names are available when `KCENON_ENABLE_LEGACY_ALIASES=1` (default):

| Legacy Macro | New Macro |
|--------------|-----------|
| `COMMON_HAS_SOURCE_LOCATION` | `KCENON_HAS_SOURCE_LOCATION` |
| `USE_THREAD_SYSTEM` | `KCENON_WITH_THREAD_SYSTEM` |
| `BUILD_WITH_THREAD_SYSTEM` | `KCENON_WITH_THREAD_SYSTEM` |
| `BUILD_WITH_LOGGER` | `KCENON_WITH_LOGGER_SYSTEM` |
| `BUILD_WITH_MONITORING` | `KCENON_WITH_MONITORING_SYSTEM` |

> **Note:** Legacy aliases are deprecated and will be removed in v1.0.0. Migrate to KCENON_* macros.

### Ecosystem Integration Flags

For flexible integration with ecosystem modules:

**Available Flags:**

- `KCENON_WITH_COMMON_SYSTEM`: Indicates common_system types are available (auto-set)
- `KCENON_WITH_THREAD_SYSTEM`: Enable thread_system integration
- `KCENON_WITH_CONTAINER_SYSTEM`: Enable container_system integration
- `KCENON_WITH_LOGGER_SYSTEM`: Enable logger_system integration
- `KCENON_WITH_MONITORING_SYSTEM`: Enable monitoring_system integration
- `KCENON_WITH_NETWORK_SYSTEM`: Enable network_system integration
- `KCENON_WITH_DATABASE_SYSTEM`: Enable database_system integration

**CMake Usage:**

```cmake
include(cmake/features.cmake)

# Configure feature flags
kcenon_configure_features(my_target
    THREAD_SYSTEM ON
    LOGGER_SYSTEM ON
    DATABASE_SYSTEM ON
)

# Or use traditional compile definitions
target_compile_definitions(my_target PUBLIC
    KCENON_WITH_THREAD_SYSTEM=1
    KCENON_WITH_LOGGER_SYSTEM=1
)

# Link to your target
target_link_libraries(my_app
    PRIVATE
        kcenon::common
        kcenon::thread
        kcenon::logger
        kcenon::database
)
```

---

## Production Quality Features

### Build & Testing Infrastructure

**Multi-Platform Continuous Integration:**

The common_system is continuously tested across multiple platforms and compilers:

- **Ubuntu Linux**
  - GCC 7, 9, 11, 13
  - Clang 5, 10, 14, 16
  - Full sanitizer coverage

- **macOS**
  - Apple Clang (Xcode 12, 13, 14, 15)
  - arm64 and x86_64 architectures
  - Native M1/M2 testing

- **Windows**
  - MSVC 2017, 2019, 2022
  - Both x86 and x64 builds
  - Debug and Release configurations

**Automated Sanitizer Builds:**

Every commit is tested with:
- **ThreadSanitizer (TSan)**: Detects data races and threading issues
- **AddressSanitizer (ASan)**: Detects memory errors and leaks
- **UndefinedBehaviorSanitizer (UBSan)**: Catches undefined behavior

**Quality Metrics:**

Current production quality metrics:
- Test coverage: 80%+ (target: 85%)
- Sanitizer tests: 18/18 passing with zero warnings
- Static analysis: Baseline established, zero new warnings
- Documentation coverage: 100% of public APIs

### Thread Safety & Concurrency

**Thread-Safe by Design:**

All common_system interfaces are designed for safe concurrent access:

- **Result<T>**: Immutable after construction, safe to share across threads
- **IExecutor**: Thread-safe submit() and execute() operations
- **Event bus**: Lock-free publish/subscribe operations
- **Error registry**: Compile-time initialization, runtime read-only

**Concurrency Guarantees:**

```cpp
// Safe concurrent Result<T> usage
void worker_thread(std::shared_ptr<Result<Data>> result) {
    // Multiple threads can safely read the same Result
    if (result->is_ok()) {
        process(result->value());
    }
}

// Safe concurrent IExecutor usage
void process_batch(std::shared_ptr<IExecutor> executor,
                  const std::vector<Task>& tasks) {
    // Multiple threads can submit to the same executor
    for (const auto& task : tasks) {
        executor->submit([task]() {
            task.execute();
        });
    }
}
```

**Validation:**

- ThreadSanitizer compliance verified across all ecosystem components
- Zero data race warnings in production use
- Comprehensive concurrency contract documentation
- Proper synchronization for all shared state

### Resource Management (RAII - Grade A)

**Perfect RAII Compliance:**

Every resource in common_system follows RAII principles:

- All resources managed through smart pointers (`std::shared_ptr`, `std::unique_ptr`)
- No manual memory management anywhere in the codebase
- Automatic cleanup on scope exit
- Exception-safe by design

**Validation Results:**

- AddressSanitizer: 18/18 tests pass with zero memory leaks
- Resource cleanup verified in all error paths
- No resource leaks detected in production use
- Exception safety validated across all operations

**RAII Patterns:**

```cpp
// Executor lifetime management
{
    auto executor = create_executor();
    executor->submit(task);
    // Executor automatically cleans up when scope exits
}

// Result<T> resource management
{
    auto result = load_resource();
    if (result.is_ok()) {
        use_resource(result.value());
    }
    // Resource automatically released when result goes out of scope
}

// Event subscription management
{
    auto subscription = event_bus->subscribe<Event>(handler);
    // Subscription automatically unsubscribes on destruction
}
```

---

## Advanced Features

### Source Location Support (C++20)

When compiled with C++20, common_system provides enhanced error diagnostics:

```cpp
#if __cplusplus >= 202002L
    #include <source_location>

    template<typename T>
    Result<T> make_error(
        int code,
        std::string message,
        std::source_location loc = std::source_location::current()
    ) {
        ErrorInfo info{
            .code = code,
            .message = std::move(message),
            .file = loc.file_name(),
            .line = loc.line(),
            .function = loc.function_name()
        };
        return Result<T>::error(std::move(info));
    }
#endif
```

### ABI Version Checking

Compile-time ABI compatibility verification:

```cpp
namespace common::abi {
    constexpr int MAJOR = 1;
    constexpr int MINOR = 0;
    constexpr int PATCH = 0;

    constexpr int VERSION = (MAJOR << 16) | (MINOR << 8) | PATCH;
}

// Client code can verify ABI compatibility
static_assert(common::abi::MAJOR == 1, "Incompatible ABI version");
```

### Custom Error Types

Extend the error system with custom error types:

```cpp
namespace my_system::errors {
    constexpr int MY_CUSTOM_ERROR = -1001;  // Outside common range

    inline std::string get_error_message(int code) {
        if (code == MY_CUSTOM_ERROR) {
            return "My custom error occurred";
        }
        return common::get_error_message(code);
    }
}
```

---

**Last Updated**: 2026-02-08
**Version**: 0.2.0
