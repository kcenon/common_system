# common_system API Reference

> **Version**: 2.0
> **Last Updated**: 2025-11-21
> **Status**: Production Ready (Tier 0)

## Table of Contents

1. [Namespace](#namespace)
2. [Result<T> Pattern (Recommended)](#resultt-pattern-recommended)
3. [Interfaces](#interfaces)
4. [Utilities](#utilities)

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

**Header**: `#include <kcenon/common/interfaces/i_logger.h>`

**Description**: Logger interface

#### Pure Virtual Functions

```cpp
virtual auto log(log_level level, const std::string& message) -> result_void = 0;
virtual auto flush() -> result_void = 0;
```

**Log Levels**:
```cpp
enum class log_level {
    trace,
    debug,
    info,
    warning,
    error,
    fatal
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
**Author**: common_system team
