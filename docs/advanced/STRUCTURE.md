---
doc_id: "COM-ARCH-003"
doc_title: "Common System - Project Structure"
doc_version: "1.0.0"
doc_date: "2026-04-04"
doc_status: "Released"
project: "common_system"
category: "ARCH"
---

# Common System - Project Structure

> **SSOT**: This document is the single source of truth for **Common System - Project Structure**.

<!-- TODO: target file does not exist -->
**English | [한국어](STRUCTURE.kr.md)**

---

## Table of Contents

- [Overview](#overview)
- [Directory Layout](#directory-layout)
- [Namespace Organization](#namespace-organization)
- [Header-Only Structure](#header-only-structure)
- [Component Layout](#component-layout)
- [Design Philosophy](#design-philosophy)

## Overview

The Common System is a **header-only foundational library** providing essential interfaces and design patterns for the entire ecosystem. As a pure interface library with no implementation dependencies, it enables clean separation of concerns and zero-overhead abstractions.

**Key Characteristics**:
- **Header-Only**: No compilation required, zero runtime overhead
- **Zero Dependencies**: Pure C++20 with standard library only
- **Interface-First**: Contracts without implementation
- **Template-Based**: Compile-time optimization and type safety
- **Version**: 0.1.0.0

## Directory Layout

```
common_system/
├── 📁 include/kcenon/common/          # Public header-only interfaces
│   ├── 📁 interfaces/                 # Abstract interface definitions
│   │   ├── executor_interface.h       # IExecutor - Task execution abstraction
│   │   ├── logger_interface.h         # ILogger - Logging abstraction
│   │   ├── database_interface.h       # IDatabase - Database abstraction
│   │   └── monitoring_interface.h     # IMonitor - Monitoring abstraction
│   ├── 📁 patterns/                   # Design pattern implementations
│   │   ├── result.h                   # Result<T> error handling pattern
│   │   ├── result_helpers.h           # Helper functions for Result<T>
│   │   └── event_bus.h                # Event bus pattern (header-only)
│   ├── 📁 adapters/                   # Adapter pattern utilities
│   │   ├── adapters.h                 # Common adapter utilities
│   │   ├── typed_adapter.h            # Type-safe adapter helpers
│   │   └── smart_adapter.h            # Smart pointer adapters
│   ├── 📁 error/                      # Error handling infrastructure
│   │   └── error_codes.h              # Centralized error code registry
│   ├── 📁 config/                     # Build configuration
│   │   └── features.cmake             # Feature detection and flags
│   └── common.h                       # Convenience header (includes all)
├── 📁 tests/                          # Unit tests
│   ├── 📁 unit/                       # Component unit tests
│   │   ├── result_tests.cpp           # Result<T> pattern tests
│   │   ├── executor_tests.cpp         # IExecutor interface tests
│   │   ├── error_code_tests.cpp       # Error code registry tests
│   │   └── adapter_tests.cpp          # Adapter pattern tests
│   └── CMakeLists.txt                 # Test configuration
├── 📁 integration_tests/              # Integration tests
│   ├── 📁 framework/                  # Test framework
│   │   ├── test_helpers.h             # Test utilities
│   │   └── system_fixture.h           # System-wide fixtures
│   └── 📁 scenarios/                  # Integration scenarios
│       ├── result_integration_test.cpp
│       ├── adapter_integration_test.cpp
│       └── error_handling_test.cpp
├── 📁 examples/                       # Usage examples
│   ├── 📁 basic/                      # Basic usage
│   │   ├── result_example.cpp         # Result<T> usage
│   │   ├── executor_example.cpp       # IExecutor usage
│   │   └── adapter_example.cpp        # Adapter pattern
│   └── 📁 advanced/                   # Advanced patterns
│       ├── monadic_result.cpp         # Monadic operations
│       ├── custom_executor.cpp        # Custom executor implementation
│       └── event_bus_example.cpp      # Event bus usage
├── 📁 docs/                           # Comprehensive documentation
│   ├── ARCHITECTURE.md                # Architecture overview
│   ├── INTEGRATION.md                 # Integration guide
│   ├── MIGRATION.md                   # Migration guide
│   ├── ERROR_HANDLING.md              # Error handling guide
│   ├── SMART_POINTER_GUIDELINES.md    # Smart pointer best practices
│   ├── RAII_GUIDELINES.md             # RAII best practices
│   └── API.md                         # API reference
├── 📁 cmake/                          # CMake modules
│   ├── common_systemConfig.cmake.in   # Package config template
│   └── features.cmake                 # Feature configuration
├── 📄 CMakeLists.txt                  # Build configuration (header-only)
├── 📄 README.md                       # Project overview
├── 📄 STRUCTURE.md                    # Structure documentation (this file)
├── 📄 INTEGRATION.md                  # Integration guide
├── 📄 CHANGELOG.md                    # Version history
├── 📄 LICENSE                         # BSD 3-Clause License
└── 📄 .clang-format                   # Code formatting rules
```

## Namespace Organization

### Primary Namespace Hierarchy

```cpp
kcenon::common                         // Root namespace
├── interfaces                         // Abstract interface definitions
│   ├── IExecutor                      // Task execution abstraction
│   ├── ILogger                        // Logging abstraction
│   ├── IDatabase                      // Database abstraction
│   └── IMonitor                       // Monitoring abstraction
├── patterns                           // Design patterns
│   ├── Result<T>                      // Error handling pattern
│   ├── result_helpers                 // Helper functions
│   └── EventBus                       // Event bus pattern
├── adapters                           // Adapter patterns
│   ├── typed_adapter                  // Type-safe adapters
│   └── smart_adapter                  // Smart pointer adapters
└── error                              // Error handling
    ├── error_code                     // Error code enum
    └── error_registry                 // Error code registry
```

### Namespace Usage Examples

```cpp
// Interfaces
#include <kcenon/common/interfaces/executor_interface.h>
using kcenon::common::interfaces::IExecutor;

std::shared_ptr<IExecutor> executor = create_executor();
executor->submit([]() { /* task */ });

// Patterns
#include <kcenon/common/patterns/result.h>
using kcenon::common::Result;

Result<Config> load_config(const std::string& path) {
    // Implementation with error handling
}

// Error codes
#include <kcenon/common/error/error_codes.h>
using kcenon::common::error_code;

if (result.has_error()) {
    error_code code = result.error().code();
}

// Adapters
#include <kcenon/common/adapters/smart_adapter.h>
using kcenon::common::adapters::make_smart_adapter;

auto adapted = make_smart_adapter(original_ptr);
```

## Header-Only Structure

### Compilation Model

Common system is **entirely header-only**, meaning:
- **No `.cpp` files**: All code is in headers
- **No library linking**: Include headers directly
- **Template-based**: Heavy use of templates and inline functions
- **Zero runtime overhead**: Everything resolved at compile-time

### Include Patterns

#### Option 1: Include All (Convenience)
```cpp
#include <kcenon/common/common.h>  // Includes everything

using namespace kcenon::common;
// All interfaces, patterns, and utilities available
```

#### Option 2: Selective Includes (Recommended)
```cpp
// Include only what you need
#include <kcenon/common/patterns/result.h>
#include <kcenon/common/interfaces/executor_interface.h>

using kcenon::common::Result;
using kcenon::common::interfaces::IExecutor;
```

#### Option 3: Component-Specific
```cpp
// For error handling only
#include <kcenon/common/error/error_codes.h>

// For adapters only
#include <kcenon/common/adapters/smart_adapter.h>

// For event bus only
#include <kcenon/common/patterns/event_bus.h>
```

### Build Integration

#### CMake (FetchContent)
```cmake
include(FetchContent)
FetchContent_Declare(
    common_system
    GIT_REPOSITORY https://github.com/kcenon/common_system.git
    GIT_TAG v0.2.0  # Pin to a specific release tag; do NOT use main
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(common_system)

target_link_libraries(your_target PRIVATE kcenon::common)
```

#### CMake (find_package)
```cmake
find_package(common_system CONFIG REQUIRED)
target_link_libraries(your_target PRIVATE kcenon::common_system)
```

#### Direct Include
```cmake
# Add include directory directly
target_include_directories(your_target PRIVATE
    /path/to/common_system/include
)
```

## Component Layout

### Interfaces (`interfaces/`)

Pure abstract interfaces for cross-system contracts.

#### IExecutor Interface
- **File**: `interfaces/executor_interface.h`
- **Purpose**: Task execution abstraction
- **Key Methods**:
  - `submit(task)`: Submit task for execution
  - `submit(task, args...)`: Submit task with arguments
  - Returns `std::future<T>` for result retrieval

**Example**:
```cpp
class IExecutor {
public:
    virtual ~IExecutor() = default;

    template<typename F, typename... Args>
    auto submit(F&& f, Args&&... args)
        -> std::future<std::invoke_result_t<F, Args...>> = 0;
};
```

#### ILogger Interface
- **File**: `interfaces/logger_interface.h`
- **Purpose**: Logging abstraction
- **Key Methods**:
  - `log(level, message)`: Log message at level
  - `set_level(level)`: Set minimum log level
  - `is_enabled(level)`: Check if level enabled

**Example**:
```cpp
class ILogger {
public:
    virtual ~ILogger() = default;

    virtual auto log(log_level level, const std::string& message) -> void = 0;
    virtual auto set_level(log_level min_level) -> void = 0;
    virtual auto is_enabled(log_level level) const -> bool = 0;
};
```

#### IDatabase Interface
- **File**: `interfaces/database_interface.h`
- **Purpose**: Database abstraction
- **Key Methods**:
  - `execute(query)`: Execute SQL query
  - `begin_transaction()`: Start transaction
  - `commit()`: Commit transaction

#### IMonitor Interface
- **File**: `interfaces/monitoring_interface.h`
- **Purpose**: Monitoring and metrics abstraction
- **Key Methods**:
  - `record_metric(name, value)`: Record metric
  - `collect_metrics()`: Retrieve all metrics
  - `is_healthy()`: Health check

### Patterns (`patterns/`)

Reusable design patterns for common problems.

#### Result&lt;T&gt; Pattern
- **File**: `patterns/result.h`
- **Purpose**: Type-safe error handling without exceptions
- **Design**: Similar to Rust's `Result<T,E>` and C++23's `std::expected`

**Core Structure**:
```cpp
template<typename T>
class Result {
public:
    // Construction
    static Result ok(T value);
    static Result error(error_info err);

    // Status checks
    bool has_value() const;
    bool has_error() const;
    explicit operator bool() const;

    // Value access
    T& value();
    const T& value() const;
    error_info& error();
    const error_info& error() const;

    // Monadic operations
    template<typename F>
    auto map(F&& f) -> Result</* return type */>;

    template<typename F>
    auto and_then(F&& f) -> Result</* return type */>;

    template<typename F>
    auto or_else(F&& f) -> Result<T>;
};
```

**Usage Example**:
```cpp
Result<Config> load_config(const std::string& path) {
    if (!exists(path)) {
        return Result<Config>::error(
            error_code::NOT_FOUND,
            "Configuration file not found"
        );
    }

    try {
        auto config = parse_file(path);
        return Result<Config>::ok(config);
    } catch (const std::exception& e) {
        return Result<Config>::error(
            error_code::PARSE_ERROR,
            e.what()
        );
    }
}

// Monadic composition
auto result = load_config("app.conf")
    .and_then(validate_config)
    .map(apply_defaults)
    .or_else(load_fallback_config);
```

#### Result Helpers
- **File**: `patterns/result_helpers.h`
- **Purpose**: Convenience functions for Result<T>

**Helper Functions**:
```cpp
// Convenience constructors
template<typename T>
Result<T> ok(T value);

template<typename T>
Result<T> error(error_code code, const std::string& message);

// Status checks
template<typename T>
bool is_ok(const Result<T>& result);

template<typename T>
bool is_error(const Result<T>& result);

// Value extraction
template<typename T>
T get_value(Result<T>& result);

template<typename T>
error_info get_error(Result<T>& result);
```

#### Event Bus
- **File**: `patterns/event_bus.h`
- **Purpose**: Publish-subscribe event pattern
- **Usage**: Cross-system event communication

**Example**:
```cpp
auto bus = common::get_event_bus();

// Subscribe
bus->subscribe<ErrorEvent>([](const ErrorEvent& event) {
    std::cerr << "Error: " << event.message << std::endl;
});

// Publish
bus->publish(ErrorEvent{"module_name", "Error message"});
```

### Adapters (`adapters/`)

Adapter patterns for bridging different interfaces.

#### Typed Adapter
- **File**: `adapters/typed_adapter.h`
- **Purpose**: Type-safe adapter conversions

#### Smart Adapter
- **File**: `adapters/smart_adapter.h`
- **Purpose**: Smart pointer lifecycle management

**Example**:
```cpp
#include <kcenon/common/adapters/smart_adapter.h>

auto raw_ptr = create_resource();
auto smart_ptr = make_smart_adapter(raw_ptr);  // Wraps in unique_ptr
```

### Error Handling (`error/`)

Centralized error code registry and management.

#### Error Code Registry
- **File**: `error/error_codes.h`
- **Purpose**: System-wide error code allocation and management

**Error Code Ranges**:
```cpp
namespace kcenon::common {

enum class error_code : int {
    // Common system: -1 to -99
    success = 0,
    unknown_error = -1,
    invalid_argument = -2,
    not_found = -3,
    already_exists = -4,
    timeout = -5,
    cancelled = -6,
    permission_denied = -7,
    resource_exhausted = -8,

    // Thread system: -100 to -199
    // Logger system: -200 to -299
    // Monitoring system: -300 to -399
    // Container system: -400 to -499
    // Database system: -500 to -599
    // Network system: -600 to -699
};

// Error info structure
struct error_info {
    error_code code;
    std::string message;
    std::string source;  // Component that generated error

    error_info(error_code c, std::string msg, std::string src = "")
        : code(c), message(std::move(msg)), source(std::move(src)) {}
};

} // namespace kcenon::common
```

**Compile-Time Validation**:
```cpp
// Ensure no error code conflicts across systems
static_assert(static_cast<int>(error_code::thread_system_max) > -100);
static_assert(static_cast<int>(error_code::logger_system_max) > -200);
// ... more assertions for each system
```

## Design Philosophy

### Principles

#### 1. Zero Dependencies
- **No external libraries**: Only C++20 standard library
- **Self-contained**: All functionality in headers
- **Portable**: Works on any C++20 compiler

#### 2. Zero Overhead
- **Template-based**: All abstractions resolved at compile-time
- **Inline functions**: Everything can be inlined
- **No virtual calls**: Where possible, use CRTP or concepts

#### 3. Interface-First Design
- **Pure interfaces**: Define contracts, not implementations
- **Dependency inversion**: Depend on abstractions, not concretions
- **Testability**: Easy to mock and test

#### 4. Type Safety
- **Strong typing**: No implicit conversions
- **Compile-time checks**: Catch errors early
- **Concepts**: Use C++20 concepts for constraints (where available)

#### 5. Exception-Free Core
- **Result<T>**: Explicit error handling
- **No throws**: Core interfaces don't throw
- **Optional exceptions**: Higher-level code can throw if needed

### Usage Patterns

#### Pattern 1: Interface Implementation
```cpp
// Define interface in common_system
class ILogger {
    virtual void log(log_level level, const std::string& msg) = 0;
};

// Implement in logger_system
class logger : public ILogger {
    void log(log_level level, const std::string& msg) override {
        // Implementation
    }
};
```

#### Pattern 2: Error Handling
```cpp
// Return Result<T> from operations
Result<Data> load_data() {
    if (error_condition) {
        return error<Data>(error_code::NOT_FOUND, "Data not found");
    }
    return ok(data);
}

// Use monadic operations
auto result = load_data()
    .and_then(validate_data)
    .map(transform_data)
    .or_else(use_default);
```

#### Pattern 3: Adapter Bridge
```cpp
// Bridge two different interfaces
class MyExecutorAdapter : public IExecutor {
    std::shared_ptr<ThirdPartyExecutor> executor_;
public:
    auto submit(task) -> std::future<...> override {
        return executor_->async_execute(task);  // Adapt interface
    }
};
```

## Performance Characteristics

### Compile-Time Impact
- **Header-only**: Longer initial compilation
- **Template instantiation**: Multiple instantiations possible
- **Mitigation**: Use extern templates, forward declarations

### Runtime Performance
- **Zero overhead**: All abstractions optimized away
- **No virtual calls**: Where CRTP or concepts used
- **Inline expansion**: Full inlining potential

### Memory Footprint
- **No data**: Pure interfaces have no data members
- **Result<T>**: Size of T + error_info (optimizable with std::expected in C++23)
- **Event bus**: Memory proportional to subscribers

## Best Practices

### 1. Selective Includes
Include only what you need to reduce compilation time:
```cpp
// Good: Minimal includes
#include <kcenon/common/patterns/result.h>

// Avoid: Including everything
#include <kcenon/common/common.h>  // Only if you need everything
```

### 2. Forward Declarations
Use forward declarations where possible:
```cpp
// In header
namespace kcenon::common {
    template<typename T> class Result;
}

// In implementation
#include <kcenon/common/patterns/result.h>
```

### 3. Consistent Error Handling
Always use Result<T> for operations that can fail:
```cpp
// Good: Explicit error handling
Result<Data> load() { /* ... */ }

// Avoid: Exceptions for control flow
Data load() { throw std::runtime_error("..."); }
```

### 4. Interface Segregation
Keep interfaces small and focused:
```cpp
// Good: Single responsibility
class ILogger { /* logging only */ };
class IMonitor { /* monitoring only */ };

// Avoid: Kitchen sink interface
class ILoggerMonitor { /* too many responsibilities */ };
```

## Migration Guide

### From Legacy Code

#### Step 1: Add common_system
```cmake
find_package(common_system CONFIG REQUIRED)
target_link_libraries(your_target PRIVATE kcenon::common_system)
```

#### Step 2: Replace Exception-Based Error Handling
```cpp
// Before
Data load() {
    if (error) throw std::runtime_error("Error");
    return data;
}

// After
Result<Data> load() {
    if (error) return error<Data>(error_code::LOAD_FAILED, "Error");
    return ok(data);
}
```

#### Step 3: Use Interfaces
```cpp
// Before: Concrete dependency
class MyService {
    std::shared_ptr<ConcreteLogger> logger_;
};

// After: Interface dependency
class MyService {
    std::shared_ptr<ILogger> logger_;
};
```

## References

- [README.md](../../README.md) - Project overview
- [ARCHITECTURE.md](../ARCHITECTURE.md) - Architecture documentation
- [INTEGRATION.md](../guides/INTEGRATION.md) - Integration guide
- [ERROR_HANDLING.md](../guides/ERROR_HANDLING.md) - Error handling guide
- [SMART_POINTER_GUIDELINES.md](../guides/SMART_POINTER_GUIDELINES.md) - Smart pointer best practices
- [RAII_GUIDELINES.md](../guides/RAII_GUIDELINES.md) - RAII best practices
