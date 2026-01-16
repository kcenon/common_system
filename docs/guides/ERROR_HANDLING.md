# Error Handling Guidelines

> **Language:** **English** | [한국어](ERROR_HANDLING_KO.md)


## Table of Contents

- [Overview](#overview)
- [Principles](#principles)
  - [1. Exception-Free Error Handling](#1-exception-free-error-handling)
  - [2. Centralized Error Codes](#2-centralized-error-codes)
  - [3. Type-Safe Error Propagation](#3-type-safe-error-propagation)
- [Using Result<T>](#using-resultt)
  - [Basic Usage](#basic-usage)
  - [Factory Functions](#factory-functions)
  - [Checking Results](#checking-results)
  - [Unwrapping Results](#unwrapping-results)
- [Monadic Operations](#monadic-operations)
  - [map - Transform Success Value](#map-transform-success-value)
  - [and_then - Chain Operations](#and_then-chain-operations)
  - [or_else - Handle Errors](#or_else-handle-errors)
- [Error Propagation Patterns](#error-propagation-patterns)
  - [Pattern 1: Early Return](#pattern-1-early-return)
  - [Pattern 2: RETURN_IF_ERROR Macro](#pattern-2-return_if_error-macro)
  - [Pattern 3: ASSIGN_OR_RETURN Macro](#pattern-3-assign_or_return-macro)
  - [Pattern 4: Monadic Chaining](#pattern-4-monadic-chaining)
- [Error Code Usage](#error-code-usage)
  - [Defining Error Codes](#defining-error-codes)
  - [Using Error Codes](#using-error-codes)
  - [Getting Error Messages](#getting-error-messages)
- [Best Practices](#best-practices)
  - [1. Use Result<T> at Module Boundaries](#1-use-resultt-at-module-boundaries)
  - [2. Provide Context in Errors](#2-provide-context-in-errors)
  - [3. Use Appropriate Error Codes](#3-use-appropriate-error-codes)
  - [4. Document Error Conditions](#4-document-error-conditions)
  - [5. Handle Errors Appropriately](#5-handle-errors-appropriately)
- [Exception Conversion](#exception-conversion)
- [Migration Guide](#migration-guide)
  - [Step 1: Add Error Codes](#step-1-add-error-codes)
  - [Step 2: Update Function Signatures](#step-2-update-function-signatures)
  - [Step 3: Replace Exceptions](#step-3-replace-exceptions)
  - [Step 4: Update Callers](#step-4-update-callers)
- [Testing](#testing)
  - [Testing Success Cases](#testing-success-cases)
  - [Testing Error Cases](#testing-error-cases)
  - [Testing Error Propagation](#testing-error-propagation)
- [Performance Considerations](#performance-considerations)
  - [Result<T> Performance](#resultt-performance)
  - [Optimization Tips](#optimization-tips)
- [Common Patterns](#common-patterns)
  - [Pattern: Resource Acquisition](#pattern-resource-acquisition)
  - [Pattern: Validation Chain](#pattern-validation-chain)
  - [Pattern: Error Recovery](#pattern-error-recovery)
- [Result vs Event Bus: When to Use Each](#result-vs-event-bus-when-to-use-each)
  - [Quick Decision Guide](#quick-decision-guide)
  - [Result<T>: Synchronous Error Handling](#resultt-synchronous-error-handling)
  - [Event Bus: Asynchronous Error Notification](#event-bus-asynchronous-error-notification)
  - [Combined Pattern: Result + Event](#combined-pattern-result--event)
  - [Anti-Patterns to Avoid](#anti-patterns-to-avoid)
- [References](#references)

**Version**: 1.0
**Last Updated**: 2025-10-09

---

## Overview

This document provides comprehensive guidelines for error handling across all systems using the `Result<T>` pattern. The goal is to provide exception-free, type-safe error handling with clear error propagation.

---

## Principles

### 1. Exception-Free Error Handling

Use `Result<T>` for all error-prone operations at module boundaries.

**Why?**
- Explicit error handling in function signatures
- Compiler-enforced error checking
- Better performance (no exception unwinding)
- Clear error propagation paths

### 2. Centralized Error Codes

All error codes are defined in `error/error_codes.h` with designated ranges per system.

**Error Code Ranges**:
```cpp
0:            Success
-1 to -99:    Common errors
-100 to -199: thread_system
-200 to -299: logger_system
-300 to -399: monitoring_system
-400 to -499: container_system
-500 to -599: database_system
-600 to -699: network_system
```

### 3. Type-Safe Error Propagation

Use monadic operations (`map`, `and_then`, `or_else`) for composing error-handling logic.

---

## Using Result<T>

### Basic Usage

```cpp
#include <kcenon/common/patterns/result.h>
#include <kcenon/common/error/error_codes.h>

using namespace common;

// Function that may fail
Result<int> divide(int a, int b) {
    if (b == 0) {
        return error<int>(
            error::codes::common::invalid_argument,
            "Division by zero",
            "math"
        );
    }
    return ok(a / b);
}

// Using the result
void example() {
    auto result = divide(10, 2);

    if (result.is_ok()) {
        std::cout << "Result: " << result.value() << std::endl;
    } else {
        std::cout << "Error: " << result.error().message << std::endl;
    }
}
```

### Factory Functions

```cpp
// Success
Result<int> success = ok(42);
Result<void> void_success = ok();  // For void operations

// Error
Result<int> failure = error<int>(
    error::codes::common::not_found,
    "Resource not found",
    "resource_manager"
);

// With details
Result<int> detailed_error = error<int>(
    error::codes::common::io_error,
    "File operation failed",
    "file_system",
    "Permission denied for /etc/config"
);
```

### Checking Results

```cpp
Result<int> result = some_operation();

// Method 1: is_ok / is_err
if (result.is_ok()) {
    // Safe to access value
    int value = result.value();
}

// Method 2: Helper functions
if (is_ok(result)) {
    int value = get_value(result);
}

if (is_error(result)) {
    const auto& err = get_error(result);
    log_error(err.message);
}
```

### Unwrapping Results

```cpp
// unwrap() - throws if error (use sparingly)
try {
    int value = result.unwrap();
} catch (const std::runtime_error& e) {
    // Handle exception
}

// unwrap_or() - provides default value
int value = result.unwrap_or(0);

// get_if_ok() - returns pointer or nullptr
if (const int* value = get_if_ok(result)) {
    std::cout << *value << std::endl;
}
```

---

## Monadic Operations

### map - Transform Success Value

Transform the value inside a successful Result.

```cpp
Result<int> calculate() {
    return ok(10);
}

// Transform int to string
Result<std::string> result = calculate()
    .map([](int x) { return std::to_string(x * 2); });

// result contains "20"
```

### and_then - Chain Operations

Chain operations that return Results (flatMap/bind).

```cpp
Result<int> parse_int(const std::string& str) {
    try {
        return ok(std::stoi(str));
    } catch (...) {
        return error<int>(
            error::codes::common::invalid_argument,
            "Invalid integer format",
            "parser"
        );
    }
}

Result<int> validate_positive(int value) {
    if (value > 0) {
        return ok(value);
    }
    return error<int>(
        error::codes::common::invalid_argument,
        "Value must be positive",
        "validator"
    );
}

// Chain operations
Result<std::string> input = ok(std::string("42"));
Result<int> result = input
    .and_then(parse_int)
    .and_then(validate_positive);
```

### or_else - Handle Errors

Provide alternative behavior on error.

```cpp
Result<int> result = risky_operation()
    .or_else([](const error_info& err) -> Result<int> {
        log_error(err.message);
        return ok(0);  // Default value
    });
```

---

## Error Propagation Patterns

### Pattern 1: Early Return

```cpp
Result<int> complex_operation() {
    auto step1 = first_step();
    if (step1.is_err()) {
        return error<int>(step1.error());
    }

    auto step2 = second_step(step1.value());
    if (step2.is_err()) {
        return error<int>(step2.error());
    }

    return ok(step2.value() * 2);
}
```

### Pattern 2: RETURN_IF_ERROR Macro

```cpp
Result<int> complex_operation() {
    RETURN_IF_ERROR(first_step());
    RETURN_IF_ERROR(second_step());

    return ok(42);
}
```

### Pattern 3: ASSIGN_OR_RETURN Macro

```cpp
Result<int> complex_operation() {
    ASSIGN_OR_RETURN(auto value1, first_step());
    ASSIGN_OR_RETURN(auto value2, second_step(value1));

    return ok(value1 + value2);
}
```

### Pattern 4: Monadic Chaining

```cpp
Result<int> complex_operation() {
    return first_step()
        .and_then(second_step)
        .and_then(third_step)
        .map([](auto x) { return x * 2; });
}
```

---

## Error Code Usage

### Defining Error Codes

Error codes are centrally defined in `error/error_codes.h`:

```cpp
namespace common {
namespace error {
namespace codes {

namespace my_system {
    constexpr int base = -700;  // New system range

    constexpr int operation_failed = base + 0;
    constexpr int invalid_state = base + 1;
    // ...
}

} // namespace codes
} // namespace error
} // namespace common
```

### Using Error Codes

```cpp
#include <kcenon/common/error/error_codes.h>

using namespace common::error;

Result<void> start_server(int port) {
    if (port < 1024) {
        return error<std::monostate>(
            codes::network_system::bind_failed,
            "Cannot bind to privileged port",
            "server"
        );
    }

    // Start server...
    return ok();
}
```

### Getting Error Messages

```cpp
int code = error::codes::logger_system::file_open_failed;

// Get message
std::string_view msg = error::get_error_message(code);
// "Failed to open log file"

// Get category
std::string_view category = error::get_category_name(code);
// "LoggerSystem"
```

---

## Best Practices

### 1. Use Result<T> at Module Boundaries

```cpp
// Good: Public API returns Result
class DatabaseManager {
public:
    Result<Connection> connect(const std::string& url);
    Result<QueryResult> execute(const std::string& query);
};

// Internal functions can use exceptions
private:
    void internal_helper() {
        // May throw - caught and converted at boundary
    }
```

### 2. Provide Context in Errors

```cpp
// Bad: Generic error
return error<int>(
    error::codes::common::io_error,
    "Error",
    "system"
);

// Good: Specific error with context
return error<int>(
    error::codes::database_system::connection_failed,
    "Failed to connect to PostgreSQL",
    "connection_pool",
    "Host: localhost:5432, Database: myapp"
);
```

### 3. Use Appropriate Error Codes

```cpp
// Bad: Using wrong category
return error<int>(
    error::codes::logger_system::file_open_failed,  // Wrong!
    "Database connection failed",
    "database"
);

// Good: Using correct category
return error<int>(
    error::codes::database_system::connection_failed,
    "Database connection failed",
    "database"
);
```

### 4. Document Error Conditions

```cpp
/**
 * @brief Open database connection
 * @param url Connection URL
 * @return Result<Connection> on success
 *
 * Error conditions:
 * - database_system::connection_failed: Cannot reach database server
 * - database_system::connection_timeout: Connection attempt timed out
 * - common::invalid_argument: Invalid connection URL format
 */
Result<Connection> connect(const std::string& url);
```

### 5. Handle Errors Appropriately

```cpp
auto result = operation();

result
    .map([](auto value) {
        // Handle success
        process(value);
        return value;
    })
    .or_else([](const error_info& err) {
        // Log error
        logger->error("Operation failed: {}", err.message);

        // Notify monitoring
        if (err.code == error::codes::common::out_of_memory) {
            monitor->alert("Critical: Out of memory");
        }

        return ok(default_value());
    });
```

---

## Exception Conversion

For interfacing with exception-based code:

```cpp
#include <kcenon/common/patterns/result.h>

// Convert exceptions to Result
Result<int> safe_operation() {
    return try_catch<int>([&]() {
        return risky_function_that_throws();
    }, "operation_name");
}

// For void operations
VoidResult safe_void_operation() {
    return try_catch_void([&]() {
        risky_void_function();
    }, "operation_name");
}
```

---

## Migration Guide

### Step 1: Add Error Codes

Add system-specific error codes to `error/error_codes.h`:

```cpp
namespace my_system {
    constexpr int base = -800;
    constexpr int specific_error = base + 0;
}
```

### Step 2: Update Function Signatures

```cpp
// Before
void process_data(const Data& data);

// After
Result<void> process_data(const Data& data);
```

### Step 3: Replace Exceptions

```cpp
// Before
void connect(const std::string& url) {
    if (url.empty()) {
        throw std::invalid_argument("Empty URL");
    }
}

// After
Result<void> connect(const std::string& url) {
    if (url.empty()) {
        return error<std::monostate>(
            error::codes::common::invalid_argument,
            "Empty URL",
            "connect"
        );
    }
    return ok();
}
```

### Step 4: Update Callers

```cpp
// Before
try {
    connect(url);
} catch (const std::exception& e) {
    handle_error(e.what());
}

// After
auto result = connect(url);
if (result.is_err()) {
    handle_error(result.error().message);
}
```

---

## Testing

### Testing Success Cases

```cpp
TEST(MyTest, SuccessfulOperation) {
    auto result = my_function(valid_input);

    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(42, result.value());
}
```

### Testing Error Cases

```cpp
TEST(MyTest, ErrorHandling) {
    auto result = my_function(invalid_input);

    ASSERT_TRUE(result.is_err());
    EXPECT_EQ(
        error::codes::common::invalid_argument,
        result.error().code
    );
    EXPECT_EQ("Invalid input", result.error().message);
}
```

### Testing Error Propagation

```cpp
TEST(MyTest, ErrorPropagation) {
    // Test that errors propagate correctly
    auto result = complex_operation();

    ASSERT_TRUE(result.is_err());
    EXPECT_EQ("Step 1 failed", result.error().message);
    EXPECT_EQ("step1", result.error().module);
}
```

---

## Performance Considerations

### Result<T> Performance

- **Size**: `sizeof(Result<T>) ≈ sizeof(T) + sizeof(error_info) + alignment`
- **Copy cost**: Same as copying T or error_info
- **Move cost**: Same as moving T or error_info (typically cheap)
- **Inlining**: Most operations inline well with optimization enabled

### Optimization Tips

1. **Use move semantics**:
   ```cpp
   return ok(std::move(large_object));
   ```

2. **Avoid unnecessary copies**:
   ```cpp
   // Good: Pass by const reference
   Result<void> process(const Result<Data>& input);

   // Good: Move when consuming
   Result<void> process(Result<Data>&& input);
   ```

3. **Use references for large values**:
   ```cpp
   if (result.is_ok()) {
       const auto& value = result.value();  // No copy
   }
   ```

---

## Common Patterns

### Pattern: Resource Acquisition

```cpp
class FileHandle {
public:
    static Result<FileHandle> open(const std::string& path) {
        FILE* file = fopen(path.c_str(), "r");
        if (!file) {
            return error<FileHandle>(
                error::codes::logger_system::file_open_failed,
                "Cannot open file",
                "FileHandle",
                path
            );
        }
        return ok(FileHandle(file));
    }

private:
    explicit FileHandle(FILE* f) : file_(f) {}
    std::unique_ptr<FILE, decltype(&fclose)> file_;
};
```

### Pattern: Validation Chain

```cpp
Result<User> create_user(const std::string& name, int age) {
    return validate_name(name)
        .and_then([age](auto) { return validate_age(age); })
        .and_then([name, age](auto) {
            return User::create(name, age);
        });
}
```

### Pattern: Error Recovery

```cpp
Result<Config> load_config() {
    return load_from_file("config.json")
        .or_else([](const error_info&) {
            // Try fallback
            return load_from_file("config.default.json");
        })
        .or_else([](const error_info&) {
            // Use hardcoded defaults
            return ok(Config::default_config());
        });
}
```

---

## Result vs Event Bus: When to Use Each

The Common System provides two primary mechanisms for error propagation and inter-module communication. Understanding when to use each is critical for consistent, maintainable code.

### Quick Decision Guide

| Scenario | Use | Reason |
|----------|-----|--------|
| Function returning success/failure | `Result<T>` | Synchronous, type-safe return value |
| Operation that may fail | `Result<T>` | Explicit error handling required |
| Broadcasting state changes | `event_bus` | Multiple listeners may need notification |
| Logging errors for monitoring | `event_bus` | Decoupled from call stack |
| Cross-module error notification | Both | `Result<T>` for caller, event for observers |

### Result<T>: Synchronous Error Handling

**Use `Result<T>` when:**

1. **The caller must handle the error** - The function returns a value that may be an error
2. **Error handling is part of control flow** - The caller needs to decide what to do next
3. **Type safety is important** - Compile-time guarantees about error handling
4. **Performance is critical** - No virtual dispatch or memory allocation

```cpp
// Good: Caller explicitly handles the error
Result<Connection> connect(const std::string& url) {
    if (url.empty()) {
        return make_error<Connection>(
            error_codes::INVALID_ARGUMENT,
            "URL cannot be empty",
            "connection_pool"
        );
    }
    // ... connection logic
    return ok(connection);
}

void process() {
    auto result = connect(url);
    if (result.is_err()) {
        // Caller decides: retry, fallback, or propagate
        handle_connection_error(result.error());
        return;
    }
    use_connection(result.value());
}
```

### Event Bus: Asynchronous Error Notification

**Use `event_bus` when:**

1. **Multiple components need to know** - Broadcasting to unknown number of listeners
2. **Decoupled notification** - Sender doesn't need to know who receives
3. **Audit/monitoring purposes** - Logging, metrics, alerting
4. **Cross-module communication** - Loose coupling between systems

```cpp
// Good: Notify monitoring systems about the error
void handle_critical_error(const error_info& err) {
    // Publish error event for monitoring, alerting, metrics
    get_event_bus().publish(events::error_event{
        "database_system",
        err.message,
        err.code
    });
}
```

### Combined Pattern: Result + Event

For critical operations, combine both patterns:

```cpp
Result<void> critical_operation() {
    auto result = perform_operation();

    if (result.is_err()) {
        // 1. Publish event for monitoring/logging (fire-and-forget)
        get_event_bus().publish(events::error_event{
            "critical_system",
            result.error().message,
            result.error().code
        });

        // 2. Return error for caller to handle (synchronous)
        return result;
    }

    return ok();
}
```

### Anti-Patterns to Avoid

**Don't use `event_bus` for:**
- Returning errors to callers (use `Result<T>`)
- Control flow decisions (use `Result<T>`)
- Required error handling (use `Result<T>`)

**Don't use `Result<T>` for:**
- Fire-and-forget notifications (use `event_bus`)
- Broadcasting to multiple unknown receivers (use `event_bus`)
- Metrics and telemetry (use `event_bus`)

```cpp
// BAD: Using event_bus for error that caller must handle
void bad_connect(const std::string& url) {
    if (url.empty()) {
        get_event_bus().publish(events::error_event{"conn", "empty url", -1});
        return;  // Caller has no way to know about the error!
    }
}

// BAD: Using Result for fire-and-forget notification
Result<void> bad_log_metric(const std::string& name, double value) {
    // Caller doesn't need to handle metric logging errors
    return ok();  // Should use event_bus instead
}
```

### Summary

| Pattern | Error Handling | Communication | Coupling |
|---------|---------------|---------------|----------|
| `Result<T>` | Synchronous, required | 1:1 (caller) | Tight (function signature) |
| `event_bus` | Async, optional | 1:N (broadcast) | Loose (decoupled) |

---

## References

- [Result<T> Implementation](../include/kcenon/common/patterns/result.h)
- [Event Bus Implementation](../include/kcenon/common/patterns/event_bus.h)
- [Error Codes Registry](../include/kcenon/common/error/error_codes.h)
- [RAII Guidelines](RAII_GUIDELINES.md)
- [Smart Pointer Guidelines](SMART_POINTER_GUIDELINES.md)

---

*Last Updated: 2026-01-16*
