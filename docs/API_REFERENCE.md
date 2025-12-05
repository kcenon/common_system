# common_system API Reference

> **Version**: 2.1
> **Last Updated**: 2025-12-05
> **Status**: Production Ready (Tier 0)

## Table of Contents

1. [Namespace](#namespace)
2. [Result<T> Pattern (Recommended)](#resultt-pattern-recommended)
3. [Interfaces](#interfaces)
4. [Unified Logging](#unified-logging)
5. [Utilities](#utilities)

---

## Namespace

### `kcenon::common`

All public APIs of common_system are contained in this namespace

**Included Items**:
- `result<T>` - Result type
- `result_void` - Result for void
- All interfaces (`IExecutor`, `ILogger`, `IMonitor`, `IDatabase`)

---

## Result<T> Pattern (Recommended)

### `result<T>`

**Header**: `#include <kcenon/common/patterns/result.h>`

**Description**: Type-safe container for representing either a success value or an error

#### Constructors

```cpp
// Create with success value
static auto ok(T value) -> result<T>;

// Create with error
static auto err(error_info error) -> result<T>;
```

#### Core Methods

##### `is_ok()` / `is_error()`

```cpp
auto is_ok() const -> bool;
auto is_error() const -> bool;
```

**Description**: Check Result state

**Example**:
```cpp
result<int> res = result<int>::ok(42);
if (res.is_ok()) {
    // Success handling
}
```

##### `value()`

```cpp
auto value() const -> const T&;
auto value() -> T&;
```

**Description**: Access success value (throws if error)

**Exceptions**:
- `std::runtime_error`: Thrown when called on error state

**Example**:
```cpp
auto res = result<int>::ok(42);
int val = res.value();  // 42
```

##### `error()`

```cpp
auto error() const -> const error_info&;
```

**Description**: Access error information

**Example**:
```cpp
auto res = result<int>::err(error_info{1, "Failed"});
auto err = res.error();
std::cout << err.message << std::endl;
```

---

### `result_void`

**Header**: `#include <kcenon/common/patterns/result.h>`

**Description**: Result for void return (represents success/failure only)

#### Usage Example

```cpp
auto process() -> result_void {
    if (/* success condition */) {
        return result_void::ok();
    }
    return result_void::err(error_info{1, "Process failed"});
}

auto res = process();
if (res.is_ok()) {
    // Success handling
}
```

---

## Interfaces

### `IExecutor`

**Header**: `#include <kcenon/common/interfaces/i_executor.h>`

**Description**: Task executor interface

#### Pure Virtual Functions

```cpp
virtual auto execute(std::function<void()> task) -> result_void = 0;
virtual auto shutdown() -> result_void = 0;
```

**Implementation Example**:
```cpp
class MyExecutor : public kcenon::common::IExecutor {
public:
    auto execute(std::function<void()> task) -> result_void override {
        // Execute task implementation
        task();
        return result_void::ok();
    }

    auto shutdown() -> result_void override {
        // Shutdown handling
        return result_void::ok();
    }
};
```

---

### `ILogger`

**Header**: `#include <kcenon/common/interfaces/logger_interface.h>`

**Description**: Logger interface

#### Pure Virtual Functions

```cpp
virtual auto log(log_level level, const std::string& message) -> VoidResult = 0;
virtual auto log(log_level level, const std::string& message,
                 const std::string& file, int line, const std::string& function) -> VoidResult = 0;
virtual auto log(const log_entry& entry) -> VoidResult = 0;
virtual auto is_enabled(log_level level) const -> bool = 0;
virtual auto set_level(log_level level) -> VoidResult = 0;
virtual auto get_level() const -> log_level = 0;
virtual auto flush() -> VoidResult = 0;
```

**Log Levels**:
```cpp
enum class log_level {
    trace = 0,
    debug = 1,
    info = 2,
    warning = 3,
    error = 4,
    critical = 5,
    off = 6
};
```

---

### `GlobalLoggerRegistry`

**Header**: `#include <kcenon/common/interfaces/global_logger_registry.h>`

**Description**: Thread-safe singleton registry for managing logger instances across all subsystems.

This class resolves circular dependencies between subsystems by providing a centralized, decoupled logging registry that can be bound at runtime.

#### Singleton Access

```cpp
static GlobalLoggerRegistry& instance();
```

#### Default Logger Management

```cpp
// Set the default logger
VoidResult set_default_logger(std::shared_ptr<ILogger> logger);

// Get the default logger (returns NullLogger if not set)
std::shared_ptr<ILogger> get_default_logger();

// Check if default logger is available
bool has_default_logger() const;
```

#### Named Logger Management

```cpp
// Register a logger with a name
VoidResult register_logger(const std::string& name, std::shared_ptr<ILogger> logger);

// Get a logger by name (returns NullLogger if not found)
std::shared_ptr<ILogger> get_logger(const std::string& name);

// Remove a logger by name
VoidResult unregister_logger(const std::string& name);

// Check if a logger is registered
bool has_logger(const std::string& name) const;
```

#### Factory Support (Lazy Initialization)

```cpp
// Register a factory for lazy logger creation
VoidResult register_factory(const std::string& name, LoggerFactory factory);

// Set a factory for the default logger
VoidResult set_default_factory(LoggerFactory factory);
```

#### Utility Methods

```cpp
// Clear all registered loggers and factories
void clear();

// Get the number of registered loggers
size_t size() const;

// Get the shared NullLogger instance
static std::shared_ptr<ILogger> null_logger();
```

#### Convenience Functions

```cpp
// Get the global registry
GlobalLoggerRegistry& get_registry();

// Get the default logger
std::shared_ptr<ILogger> get_logger();

// Get a named logger
std::shared_ptr<ILogger> get_logger(const std::string& name);
```

**Usage Example**:
```cpp
#include <kcenon/common/interfaces/global_logger_registry.h>

using namespace kcenon::common::interfaces;

// Register a default logger
auto console_logger = std::make_shared<ConsoleLogger>();
get_registry().set_default_logger(console_logger);

// Register named loggers
get_registry().register_logger("network", std::make_shared<NetworkLogger>());
get_registry().register_logger("database", std::make_shared<DatabaseLogger>());

// Use loggers
get_logger()->log(log_level::info, "Application started");
get_logger("network")->log(log_level::debug, "Connection established");
get_logger("database")->log(log_level::warning, "Slow query detected");

// Lazy initialization with factory
get_registry().register_factory("metrics", []() {
    return std::make_shared<MetricsLogger>();
});
```

---

### `NullLogger`

**Header**: `#include <kcenon/common/interfaces/global_logger_registry.h>`

**Description**: A no-op logger implementation for fallback scenarios.

All logging operations are no-ops that return success. This ensures code functions silently when logging is not configured.

```cpp
class NullLogger : public ILogger {
    // All operations return VoidResult::ok({})
    // is_enabled() always returns false
    // get_level() always returns log_level::off
};
```

---

### `IMonitor`

**Header**: `#include <kcenon/common/interfaces/i_monitor.h>`

**Description**: Monitoring interface

#### Pure Virtual Functions

```cpp
virtual auto record_metric(const std::string& name, double value) -> result_void = 0;
virtual auto start_timer(const std::string& name) -> result_void = 0;
virtual auto stop_timer(const std::string& name) -> result_void = 0;
```

---

### `IDatabase`

**Header**: `#include <kcenon/common/interfaces/i_database.h>`

**Description**: Database interface

#### Pure Virtual Functions

```cpp
virtual auto connect(const connection_info& info) -> result_void = 0;
virtual auto execute(const std::string& query) -> result<query_result> = 0;
virtual auto disconnect() -> result_void = 0;
```

---

## Unified Logging

### Logging Functions

**Header**: `#include <kcenon/common/logging/log_functions.h>`

**Description**: Inline logging functions with automatic source_location capture.

#### Core Functions

```cpp
namespace kcenon::common::logging {
    // Log with specified level to default logger
    VoidResult log(log_level level, std::string_view message,
                   const source_location& loc = source_location::current());

    // Log to a specific logger instance
    VoidResult log(log_level level, std::string_view message,
                   const std::shared_ptr<ILogger>& logger,
                   const source_location& loc = source_location::current());

    // Log to a named logger
    VoidResult log(log_level level, std::string_view message,
                   const std::string& logger_name,
                   const source_location& loc = source_location::current());
}
```

#### Level-Specific Functions

```cpp
// Log to default logger
VoidResult log_trace(std::string_view message, ...);
VoidResult log_debug(std::string_view message, ...);
VoidResult log_info(std::string_view message, ...);
VoidResult log_warning(std::string_view message, ...);
VoidResult log_error(std::string_view message, ...);
VoidResult log_critical(std::string_view message, ...);

// Log to named logger (using "_to" suffix to avoid overload ambiguity)
VoidResult log_trace_to(const std::string& logger_name, std::string_view message, ...);
VoidResult log_debug_to(const std::string& logger_name, std::string_view message, ...);
VoidResult log_info_to(const std::string& logger_name, std::string_view message, ...);
VoidResult log_warning_to(const std::string& logger_name, std::string_view message, ...);
VoidResult log_error_to(const std::string& logger_name, std::string_view message, ...);
VoidResult log_critical_to(const std::string& logger_name, std::string_view message, ...);
```

#### Utility Functions

```cpp
// Check if a log level is enabled
bool is_enabled(log_level level);
bool is_enabled(log_level level, const std::string& logger_name);

// Flush buffered log messages
VoidResult flush();
VoidResult flush(const std::string& logger_name);
```

**Usage Example**:
```cpp
#include <kcenon/common/logging/log_functions.h>

using namespace kcenon::common::logging;
using namespace kcenon::common::interfaces;

// Basic logging (uses default logger)
log_info("Application started");
log_warning("Low memory condition");
log_error("Connection failed");

// Using named loggers (note: logger_name comes first)
log_info_to("network", "Request received");
log_debug_to("database", "Query executed");

// Check before expensive message construction
if (is_enabled(log_level::debug)) {
    log_debug("Detailed state: " + expensive_to_string(state));
}
```

---

### Logging Macros

**Header**: `#include <kcenon/common/logging/log_macros.h>`

**Description**: Convenient preprocessor macros for logging.

#### Standard Macros

```cpp
LOG_TRACE(msg)     // Log trace message
LOG_DEBUG(msg)     // Log debug message
LOG_INFO(msg)      // Log info message
LOG_WARNING(msg)   // Log warning message
LOG_ERROR(msg)     // Log error message
LOG_CRITICAL(msg)  // Log critical message
```

#### Named Logger Macros

```cpp
LOG_TRACE_TO(logger_name, msg)
LOG_DEBUG_TO(logger_name, msg)
LOG_INFO_TO(logger_name, msg)
LOG_WARNING_TO(logger_name, msg)
LOG_ERROR_TO(logger_name, msg)
LOG_CRITICAL_TO(logger_name, msg)
```

#### Conditional Logging

```cpp
// Log only if level is enabled (avoids message construction when disabled)
LOG_IF(level, msg)
LOG_IF_TO(level, logger_name, msg)
```

#### Utility Macros

```cpp
LOG_FLUSH()                          // Flush default logger
LOG_FLUSH_TO(logger_name)            // Flush named logger
LOG_IS_ENABLED(level)                // Check if level is enabled
LOG_IS_ENABLED_FOR(level, logger_name)
```

#### Legacy Compatibility

```cpp
// These macros redirect to LOG_* equivalents (deprecated)
THREAD_LOG_TRACE(msg)
THREAD_LOG_DEBUG(msg)
THREAD_LOG_INFO(msg)
THREAD_LOG_WARNING(msg)
THREAD_LOG_ERROR(msg)
THREAD_LOG_CRITICAL(msg)
```

#### Compile-Time Level Filtering

Define `KCENON_MIN_LOG_LEVEL` before including the header to disable lower log levels at compile time:

```cpp
// Disable trace and debug in release builds
#define KCENON_MIN_LOG_LEVEL 2  // 0=trace, 1=debug, 2=info, etc.
#include <kcenon/common/logging/log_macros.h>

LOG_TRACE("This becomes a no-op");  // Compiled out
LOG_DEBUG("This too");               // Compiled out
LOG_INFO("This is logged");          // Active
```

**Usage Example**:
```cpp
#include <kcenon/common/logging/log_macros.h>

// Simple logging
LOG_INFO("Server starting on port 8080");
LOG_DEBUG("Configuration loaded");

// Named logger
LOG_ERROR_TO("database", "Connection timeout");

// Conditional (efficient when debug is disabled)
LOG_IF(log_level::debug, expensive_debug_string());

// Flush before shutdown
LOG_FLUSH();
```

---

## Utilities

### error_info

**Structure**:
```cpp
struct error_info {
    int code;
    std::string message;
    std::string details;
};
```

**Usage Example**:
```cpp
return result<int>::err(error_info{
    .code = 404,
    .message = "Not found",
    .details = "Resource does not exist"
});
```

---

## Usage Examples

### Basic Usage

```cpp
#include <kcenon/common/patterns/result.h>

using namespace kcenon::common;

auto divide(int a, int b) -> result<int> {
    if (b == 0) {
        return result<int>::err(error_info{1, "Division by zero"});
    }
    return result<int>::ok(a / b);
}

int main() {
    auto res = divide(10, 2);

    if (res.is_ok()) {
        std::cout << "Result: " << res.value() << std::endl;  // 5
    } else {
        std::cout << "Error: " << res.error().message << std::endl;
    }

    return 0;
}
```

### Interface Usage

```cpp
#include <kcenon/common/interfaces/i_logger.h>

class ConsoleLogger : public kcenon::common::ILogger {
public:
    auto log(log_level level, const std::string& message) -> result_void override {
        std::cout << "[" << to_string(level) << "] " << message << std::endl;
        return result_void::ok();
    }

    auto flush() -> result_void override {
        std::cout.flush();
        return result_void::ok();
    }
};

int main() {
    ConsoleLogger logger;
    logger.log(log_level::info, "Application started");

    return 0;
}
```

---

## Migration Guide

### From v1.x to v2.0

**Changes**:
- `Result<T>` → `result<T>` (lowercase)
- `result::success()` → `result<T>::ok()`
- `result::failure()` → `result<T>::err()`
- `get_value()` → `value()`
- `get_error()` → `error()`

**Migration Example**:
```cpp
// v1.x
auto res = Result<int>::success(42);
int val = res.get_value();

// v2.0
auto res = result<int>::ok(42);
int val = res.value();
```

---

**Created**: 2025-11-21
**Version**: 2.0
**Author**: kcenon@naver.com
