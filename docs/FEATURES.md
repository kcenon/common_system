# Common System - Detailed Features

**Language:** **English** | [한국어](FEATURES_KO.md)

This document provides comprehensive details about all features available in the Common System project.

---

## Table of Contents

- [Core Advantages & Benefits](#core-advantages--benefits)
- [Core Components](#core-components)
- [Integration Features](#integration-features)
- [Production Quality Features](#production-quality-features)
- [Error Handling Foundation](#error-handling-foundation)

---

## Core Advantages & Benefits

### 🚀 **Performance Excellence**

The common_system achieves zero-overhead abstractions through several key design decisions:

- **Zero-overhead abstractions**: Template-based interfaces with compile-time resolution ensure that using common_system interfaces has no runtime cost compared to hand-written code
- **Header-only design**: No library linking required means the compiler has full visibility for optimization. This enables better inlining, dead code elimination, and link-time optimizations
- **Cache-friendly patterns**: Minimal indirection and optimal memory layout reduce cache misses and improve performance
- **Compile-time optimization**: Full inlining potential for hot paths allows the compiler to optimize across abstraction boundaries

**Performance Impact:**
- Template instantiation happens at compile-time with zero runtime overhead
- No virtual function calls in performance-critical paths
- Compiler can optimize through all abstraction layers
- Link-time optimization (LTO) can eliminate all abstraction overhead

### 🛡️ **Production-Grade Reliability**

Every interface and pattern in common_system is designed with production reliability in mind:

- **Type-safe interfaces**: Strong typing prevents common runtime errors by catching mistakes at compile-time
  - Template constraints enforce correct usage
  - Compile-time checks prevent misuse
  - No unsafe casts or type erasure

- **Result<T> pattern**: Explicit error handling without exceptions
  - Forces error handling at call sites
  - No hidden control flow from exceptions
  - Clear error propagation paths
  - Zero-cost when errors don't occur

- **RAII compliance**: Resource management through standard patterns
  - All resources use smart pointers
  - Automatic cleanup on scope exit
  - Exception-safe by design
  - No manual memory management

- **Thread-safe design**: All interfaces safe for concurrent use
  - Immutable types where possible
  - Clear concurrency guarantees
  - Lock-free algorithms when appropriate
  - No data races by design

### 🔧 **Developer Productivity**

Common_system prioritizes developer experience and productivity:

- **Self-documenting interfaces**: Clear contracts with comprehensive documentation
  - Expressive type names
  - Comprehensive Doxygen comments
  - Usage examples in headers
  - Clear error messages

- **Minimal boilerplate**: Clean API design reduces code overhead
  - Fluent interfaces for common patterns
  - Type inference reduces verbosity
  - Sensible defaults
  - Helper functions for common use cases

- **Mockable abstractions**: Easy testing through interface injection
  - Pure virtual interfaces
  - Dependency injection friendly
  - Mock implementations included
  - Test helpers provided

- **IDE-friendly**: Full IntelliSense and autocomplete support
  - Complete type information
  - Template instantiation hints
  - Jump to definition works
  - Refactoring tools compatible

### 🌐 **Universal Compatibility**

Designed to work across all modern C++ environments:

- **C++17 standard**: Backward-compatible with wider compiler support
  - GCC 7+ (tested on 7, 9, 11, 13)
  - Clang 5+ (tested on 5, 10, 14, 16)
  - MSVC 2017+ (tested on 2017, 2019, 2022)
  - No compiler-specific extensions required

- **C++20 features**: Optional support for enhanced functionality when available
  - `std::source_location` for better error diagnostics
  - Concepts for clearer template constraints
  - Graceful fallback to C++17 equivalents

- **Cross-platform**: Windows, Linux, macOS without modification
  - Platform-agnostic interfaces
  - No OS-specific code in headers
  - Consistent behavior across platforms
  - CI testing on all major platforms

- **Build system agnostic**: Works with any build system
  - CMake integration provided
  - Bazel support available
  - Plain Make compatible
  - Header-only means no build required

- **Package manager ready**: Easy integration with modern C++ package managers
  - vcpkg integration available
  - Conan support included
  - FetchContent compatible
  - git submodule friendly

### 📈 **Enterprise-Ready Features**

Built for large-scale enterprise deployments:

- **Interface versioning**: Backward compatibility through careful design
  - ABI version checking at compile-time
  - Semantic versioning of interfaces
  - Deprecation warnings for old APIs
  - Migration guides provided

- **Centralized configuration**: Unified build flags across all modules
  - Feature flags for optional components
  - Consistent build configuration
  - Override points for customization
  - Configuration validation

- **Comprehensive testing**: Full test coverage with industry-standard frameworks
  - Unit tests with Google Test
  - Integration tests with real components
  - Benchmark tests for performance validation
  - Sanitizer coverage (ASan, TSan, UBSan)

- **Production examples**: Real-world usage patterns included
  - Complete integration examples
  - Best practices documentation
  - Performance optimization guides
  - Troubleshooting scenarios

---

## Core Components

### IExecutor Interface

The IExecutor interface provides a universal abstraction for task execution, enabling complete independence from any specific threading implementation.

**Key Features:**

- **Threading backend independence**: Write code once, run on any executor
- **Task-based abstraction**: Submit lambda functions, function objects, or std::function
- **Future-based results**: Type-safe async results via std::future<T>
- **Exception safety**: Exceptions propagated through futures
- **Lifetime management**: Automatic cleanup via RAII

**Interface Methods:**

```cpp
namespace kcenon::common::interfaces {
    class IExecutor {
    public:
        virtual ~IExecutor() = default;

        // Submit a task and get a future for the result
        template<typename F, typename... Args>
        auto submit(F&& func, Args&&... args)
            -> std::future<std::invoke_result_t<F, Args...>>;

        // Execute a task without returning a result
        template<typename F, typename... Args>
        void execute(F&& func, Args&&... args);

        // Get executor information
        virtual size_t thread_count() const = 0;
        virtual bool is_running() const = 0;
    };
}
```

**Usage Patterns:**

1. **Fire and forget execution**:
```cpp
executor->execute([]() {
    // Background task
    process_data();
});
```

2. **Async with result**:
```cpp
auto future = executor->submit([]() {
    return compute_value();
});
auto result = future.get();
```

3. **Chaining async operations**:
```cpp
auto future1 = executor->submit(load_data);
auto future2 = executor->submit([future1 = std::move(future1)]() mutable {
    auto data = future1.get();
    return process(data);
});
```

**Integration:**

The IExecutor interface is implemented by:
- `thread_system::thread_pool` via adapter pattern
- `network_system` for async I/O operations
- Custom implementations for specialized executors

### Result<T> Pattern

A comprehensive implementation of the Result monad pattern for type-safe error handling without exceptions.

**Design Philosophy:**

- Explicit error handling at all call sites
- No hidden control flow from exceptions
- Functional composition via monadic operations
- Zero overhead when successful
- Full type safety at compile time

**Core Operations:**

```cpp
namespace kcenon::common {
    template<typename T>
    class Result {
    public:
        // Factory methods
        static Result<T> ok(T value);
        static Result<T> error(ErrorInfo info);

        // Query state
        bool is_ok() const noexcept;
        bool is_error() const noexcept;
        explicit operator bool() const noexcept;

        // Access value (throws if error)
        T& value() &;
        const T& value() const &;
        T&& value() &&;

        // Access value or default
        T value_or(T&& default_value) const&;
        T value_or(T&& default_value) &&;

        // Access error
        const ErrorInfo& error() const;

        // Monadic operations
        template<typename F>
        auto map(F&& func) const& -> Result<std::invoke_result_t<F, const T&>>;

        template<typename F>
        auto and_then(F&& func) const& -> std::invoke_result_t<F, const T&>;

        template<typename F>
        auto or_else(F&& func) const& -> Result<T>;
    };
}
```

**Monadic Composition:**

The Result<T> pattern supports functional programming paradigms:

```cpp
// Map: Transform the success value
auto result = load_config("app.conf")
    .map([](const Config& cfg) {
        return cfg.with_defaults();
    });

// AndThen: Chain operations that return Result
auto result = load_config("app.conf")
    .and_then(validate_config)
    .and_then(apply_schema);

// OrElse: Provide fallback on error
auto result = load_config("app.conf")
    .or_else([](const ErrorInfo& err) {
        log_error(err);
        return load_default_config();
    });

// Full composition
auto result = load_config("app.conf")
    .and_then(validate_config)
    .map(apply_defaults)
    .and_then(connect_to_db)
    .or_else(use_fallback_db);
```

**Error Context:**

Rich error information includes:

```cpp
struct ErrorInfo {
    int code;                    // Error code from registry
    std::string message;         // Human-readable message
    std::string source;          // Source module/function
    std::string file;            // Source file (if available)
    int line;                    // Source line (if available)
    std::optional<std::string>
        additional_context;      // Extra context
};
```

### Event Bus Integration

When used with monitoring_system, common_system provides event bus forwarding capabilities.

**Features:**

- **Type-safe events**: Compile-time event type checking
- **Publish-subscribe pattern**: Decouple event producers from consumers
- **Multiple subscribers**: Many handlers per event type
- **Async delivery**: Non-blocking event publishing
- **Thread-safe**: Concurrent publish/subscribe operations

**Event Types:**

```cpp
namespace kcenon::common::events {
    struct module_started_event {
        std::string module_name;
        std::chrono::system_clock::time_point timestamp;
    };

    struct module_stopped_event {
        std::string module_name;
        std::chrono::system_clock::time_point timestamp;
    };

    struct error_event {
        std::string module_name;
        std::string error_message;
        int error_code;
        std::chrono::system_clock::time_point timestamp;
    };

    struct metric_event {
        std::string metric_name;
        double value;
        std::map<std::string, std::string> tags;
        std::chrono::system_clock::time_point timestamp;
    };
}
```

**Usage Examples:**

```cpp
// Get event bus instance
auto bus = common::get_event_bus();

// Publish events
bus->publish(common::events::module_started_event{
    .module_name = "my_service",
    .timestamp = std::chrono::system_clock::now()
});

// Subscribe to events
bus->subscribe<common::events::error_event>([](const auto& event) {
    std::cerr << "[" << event.module_name << "] "
              << event.error_message << " (code: "
              << event.error_code << ")\n";
});

// Subscribe with filtering
bus->subscribe<common::events::metric_event>(
    [](const auto& event) {
        return event.metric_name.starts_with("http.");
    },
    [](const auto& event) {
        record_http_metric(event);
    }
);
```

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

## Error Handling Foundation

The common_system serves as the **foundation provider** for error handling across all systems in the ecosystem.

### Centralized Error Code Registry

Complete error code registry providing system-specific ranges:

| System | Error Code Range | Purpose |
|--------|-----------------|---------|
| common_system | -1 to -99 | Foundation errors (validation, initialization) |
| thread_system | -100 to -199 | Threading errors (deadlock, pool exhaustion) |
| logger_system | -200 to -299 | Logging errors (file I/O, formatting) |
| monitoring_system | -300 to -399 | Monitoring errors (metric collection, publishing) |
| container_system | -400 to -499 | Container errors (serialization, validation) |
| database_system | -500 to -599 | Database errors (connection, query, transaction) |
| network_system | -600 to -699 | Network errors (connection, timeout, protocol) |

**Compile-Time Validation:**

Error code ranges are enforced at compile-time to prevent conflicts:

```cpp
namespace common::error_codes {
    // Compile-time range checking
    constexpr int COMMON_MIN = -1;
    constexpr int COMMON_MAX = -99;

    constexpr bool is_valid_common_code(int code) {
        return code >= COMMON_MIN && code <= COMMON_MAX;
    }

    static_assert(is_valid_common_code(NOT_FOUND));
    static_assert(is_valid_common_code(INVALID_ARGUMENT));
}
```

### Error Message Mapping

Every error code has a corresponding human-readable message:

```cpp
std::string get_error_message(int error_code) {
    switch (error_code) {
        case error_codes::NOT_FOUND:
            return "Resource not found";
        case error_codes::INVALID_ARGUMENT:
            return "Invalid argument provided";
        case error_codes::INITIALIZATION_FAILED:
            return "Initialization failed";
        // ... all error codes mapped
        default:
            return "Unknown error";
    }
}
```

### Ecosystem Adoption

All dependent systems have successfully adopted the Result<T> pattern and error code registry:

**Adoption Status:**

- ✅ thread_system: Complete Result<T> integration for all operations
- ✅ logger_system: Error handling via Result<T>, no exceptions
- ✅ monitoring_system: Result<T> for metric operations
- ✅ container_system: Serialization operations return Result<T>
- ✅ database_system: Query results and transactions use Result<T>
- ✅ network_system: Connection and I/O operations return Result<T>

**Benefits Realized:**

- Consistent error handling across all systems
- No unexpected exceptions in production
- Clear error propagation paths
- Improved error recovery and resilience
- Better error logging and diagnostics

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

**Last Updated**: 2024-11-15
**Version**: 1.0
