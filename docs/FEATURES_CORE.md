---
doc_id: "COM-FEAT-002a"
doc_title: "Common System - Core Features"
doc_version: "1.0.0"
doc_date: "2026-04-04"
doc_status: "Released"
project: "common_system"
category: "FEAT"
---

# Common System - Core Features

> This document covers the core advantages, components, resilience patterns, and error handling foundation of the Common System.
> Split from [FEATURES.md](FEATURES.md) for readability.

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

## Resilience Patterns

The `kcenon::common::resilience` namespace provides production-grade fault tolerance primitives for distributed systems. Include the umbrella header `<kcenon/common/resilience/resilience.h>` to access all resilience components.

### Circuit Breaker

The circuit breaker pattern prevents cascading failures by temporarily blocking requests to a failing service, allowing it time to recover.

**State Machine:**

```
CLOSED ──(failure threshold exceeded)──► OPEN
  ▲                                        │
  │                                        │ (timeout expires)
  │                                        ▼
  └──(success threshold met)────── HALF_OPEN
                                      │
                                      └──(any failure)──► OPEN
```

| State | Behavior |
|-------|----------|
| `CLOSED` | Normal operation. Requests pass through, failures are tracked within a sliding time window. |
| `OPEN` | Failure threshold exceeded. All requests are immediately rejected. Transitions to `HALF_OPEN` after timeout. |
| `HALF_OPEN` | Recovery testing. A limited number of probe requests are allowed. Success closes the circuit; any failure reopens it. |

**Configuration (`circuit_breaker_config`):**

```cpp
namespace kcenon::common::resilience {

struct circuit_breaker_config {
    // Number of failures to trip the circuit (CLOSED -> OPEN)
    std::size_t failure_threshold = 5;

    // Successes required to close the circuit (HALF_OPEN -> CLOSED)
    std::size_t success_threshold = 2;

    // Sliding time window for failure tracking (failures outside expire)
    std::chrono::milliseconds failure_window = std::chrono::seconds(60);

    // Cooldown before transitioning from OPEN to HALF_OPEN
    std::chrono::milliseconds timeout = std::chrono::seconds(30);

    // Maximum probe requests allowed in HALF_OPEN state
    std::size_t half_open_max_requests = 3;
};

}
```

**Core API (`circuit_breaker`):**

```cpp
namespace kcenon::common::resilience {

class circuit_breaker : public interfaces::IStats {
public:
    explicit circuit_breaker(circuit_breaker_config config = {});

    // Check if a request should be allowed through
    [[nodiscard]] auto allow_request() -> bool;

    // Record operation outcomes
    auto record_success() -> void;
    auto record_failure(const std::exception* e = nullptr) -> void;

    // Query current state
    [[nodiscard]] auto get_state() const -> circuit_state;

    // RAII guard for automatic success/failure recording
    [[nodiscard]] auto make_guard() -> guard;

    // IStats interface - observability
    [[nodiscard]] auto get_stats() const
        -> std::unordered_map<std::string, interfaces::stats_value> override;
    [[nodiscard]] auto to_json() const -> std::string override;
    [[nodiscard]] auto name() const -> std::string_view override;
};

}
```

**RAII Guard:**

The `circuit_breaker::guard` class automatically records a failure when destroyed unless `record_success()` is explicitly called. This ensures operations that throw exceptions are correctly tracked.

```cpp
class circuit_breaker::guard {
public:
    explicit guard(circuit_breaker& breaker);
    ~guard();  // Records failure if record_success() was not called

    auto record_success() -> void;

    // Non-copyable, non-movable
    guard(const guard&) = delete;
    guard& operator=(const guard&) = delete;
};
```

**Usage Example:**

```cpp
#include <kcenon/common/resilience/resilience.h>

using namespace kcenon::common::resilience;

// Configure the breaker
circuit_breaker_config config{
    .failure_threshold = 5,
    .success_threshold = 2,
    .failure_window = std::chrono::seconds(60),
    .timeout = std::chrono::seconds(30),
    .half_open_max_requests = 3
};
circuit_breaker breaker(config);

// Pattern 1: Manual check and record
if (!breaker.allow_request()) {
    return make_error("Service unavailable - circuit is open");
}
try {
    auto result = call_remote_service();
    breaker.record_success();
    return result;
} catch (const std::exception& e) {
    breaker.record_failure(&e);
    throw;
}

// Pattern 2: RAII guard (recommended)
if (breaker.allow_request()) {
    auto guard = breaker.make_guard();
    auto result = call_remote_service();
    guard.record_success();  // Prevents automatic failure recording
    return result;
}
// If call_remote_service() throws, ~guard() records the failure automatically
```

**Observability:**

The circuit breaker implements `IStats`, providing real-time metrics:

```cpp
auto stats = breaker.get_stats();
// Returns: current_state, failure_count, consecutive_successes,
//          half_open_requests, failure_threshold, is_open

auto json = breaker.to_json();
// Returns JSON representation of all statistics
```

**Failure Window:**

The `failure_window` class provides a sliding time window for failure tracking. Failures older than the configured `failure_window` duration are automatically expired and not counted toward the threshold.

**Thread Safety:**
- All public methods on `circuit_breaker` and `failure_window` are thread-safe.
- State transitions are protected by internal synchronization.
- Safe for concurrent access from multiple threads.

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

**Last Updated**: 2026-02-08
**Version**: 0.2.0
