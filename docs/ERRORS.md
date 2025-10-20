# Error Handling Guidelines

> **Language:** **English** | [한국어](ERRORS_KO.md)


## Table of Contents

- [Overview](#overview)
- [Recommended Pattern: Result<T>](#recommended-pattern-resultt)
  - [Basic Result Type Definition](#basic-result-type-definition)
- [Usage Examples](#usage-examples)
  - [Function Returning Result](#function-returning-result)
  - [Handling Results](#handling-results)
- [Migration Strategy](#migration-strategy)
  - [Phase 1: Guidelines (Current)](#phase-1-guidelines-current)
  - [Phase 2: Gradual Adoption](#phase-2-gradual-adoption)
  - [Phase 3: Standardization](#phase-3-standardization)
- [Module-Specific Adaptations](#module-specific-adaptations)
  - [thread_system](#thread_system)
  - [logger_system](#logger_system)
  - [network_system](#network_system)
  - [database_system](#database_system)
- [Error Code Ranges](#error-code-ranges)
- [Common Error Codes](#common-error-codes)
- [Best Practices](#best-practices)
- [Performance Considerations](#performance-considerations)
- [Future Enhancements](#future-enhancements)

## Overview

This document defines standardized error handling patterns for all system modules to ensure consistency and maintainability.

## Recommended Pattern: Result<T>

We recommend using a Result type for error handling at module boundaries (APIs), while allowing exceptions internally for implementation details.

### Basic Result Type Definition

```cpp
#include <variant>
#include <optional>
#include <string>

namespace common {

/**
 * @struct error_info
 * @brief Standard error information
 */
struct error_info {
    int code;
    std::string message;
    std::string module;
    std::optional<std::string> details;

    error_info(int c, const std::string& msg, const std::string& mod = "")
        : code(c), message(msg), module(mod) {}
};

/**
 * @brief Result type for error handling
 *
 * Similar to std::expected (C++23) or Rust's Result<T,E>
 */
template<typename T>
using Result = std::variant<T, error_info>;

/**
 * @brief Check if result contains a value
 */
template<typename T>
bool is_ok(const Result<T>& result) {
    return std::holds_alternative<T>(result);
}

/**
 * @brief Check if result contains an error
 */
template<typename T>
bool is_error(const Result<T>& result) {
    return std::holds_alternative<error_info>(result);
}

/**
 * @brief Get value from result (throws if error)
 */
template<typename T>
const T& get_value(const Result<T>& result) {
    return std::get<T>(result);
}

/**
 * @brief Get error from result (throws if value)
 */
template<typename T>
const error_info& get_error(const Result<T>& result) {
    return std::get<error_info>(result);
}

/**
 * @brief Get value or default
 */
template<typename T>
T value_or(const Result<T>& result, T default_value) {
    if (is_ok(result)) {
        return get_value(result);
    }
    return default_value;
}

} // namespace common
```

## Usage Examples

### Function Returning Result

```cpp
#include <common/result.h>

common::Result<std::string> read_config(const std::string& path) {
    if (path.empty()) {
        return common::error_info{
            -1, "Path cannot be empty", "config_reader"
        };
    }

    try {
        // Internal implementation can use exceptions
        auto content = internal_read_file(path);
        return content;  // Success case
    } catch (const std::exception& e) {
        // Convert exception to Result at boundary
        return common::error_info{
            -2, e.what(), "config_reader"
        };
    }
}
```

### Handling Results

```cpp
// Method 1: Using helper functions
auto result = read_config("app.conf");
if (common::is_ok(result)) {
    const auto& config = common::get_value(result);
    process_config(config);
} else {
    const auto& error = common::get_error(result);
    log_error(error.code, error.message);
}

// Method 2: Using std::visit
std::visit([](auto&& arg) {
    using T = std::decay_t<decltype(arg)>;
    if constexpr (std::is_same_v<T, std::string>) {
        // Handle success
        process_config(arg);
    } else if constexpr (std::is_same_v<T, common::error_info>) {
        // Handle error
        log_error(arg.code, arg.message);
    }
}, result);

// Method 3: Using value_or
auto config = common::value_or(result, std::string("default.conf"));
```

## Migration Strategy

### Phase 1: Guidelines (Current)
- Document the Result pattern
- Provide helper functions and examples
- No breaking changes required

### Phase 2: Gradual Adoption
- New APIs should use Result<T> for public interfaces
- Internal implementation can continue using exceptions
- Add adapters at module boundaries

### Phase 3: Standardization
- Migrate existing APIs gradually with deprecation notices
- Maintain backward compatibility through overloads
- Document migration in release notes

## Module-Specific Adaptations

### thread_system
```cpp
// Current (exception-based)
std::future<void> submit(std::function<void()> task);

// Future (Result-based)
common::Result<std::future<void>> submit_safe(std::function<void()> task);
```

### logger_system
```cpp
// Current
void log(LogLevel level, const std::string& message);

// Future
common::Result<void> log_safe(LogLevel level, const std::string& message);
```

### network_system
```cpp
// Current
void send_message(const Message& msg);

// Future
common::Result<void> send_message_safe(const Message& msg);
```

### database_system
```cpp
// Current
QueryResult execute_query(const std::string& sql);

// Future
common::Result<QueryResult> execute_query_safe(const std::string& sql);
```

## Error Code Ranges

To avoid conflicts, each module should use specific error code ranges:

| Module            | Error Code Range | Description                  |
|-------------------|------------------|------------------------------|
| Common            | -1 to -99        | Common/generic errors        |
| thread_system     | -100 to -199     | Threading errors             |
| logger_system     | -200 to -299     | Logging errors               |
| monitoring_system | -300 to -399     | Monitoring errors            |
| container_system  | -400 to -499     | Container/serialization errors |
| database_system   | -500 to -599     | Database errors              |
| network_system    | -600 to -699     | Network errors               |

## Common Error Codes

```cpp
namespace common {
namespace error_codes {
    constexpr int SUCCESS = 0;
    constexpr int INVALID_ARGUMENT = -1;
    constexpr int NOT_FOUND = -2;
    constexpr int PERMISSION_DENIED = -3;
    constexpr int TIMEOUT = -4;
    constexpr int CANCELLED = -5;
    constexpr int NOT_INITIALIZED = -6;
    constexpr int ALREADY_EXISTS = -7;
    constexpr int OUT_OF_MEMORY = -8;
    constexpr int INTERNAL_ERROR = -99;
}
}
```

## Best Practices

1. **Use Result<T> at API boundaries** - Public functions should return Result
2. **Allow exceptions internally** - Implementation details can use exceptions
3. **Convert at boundaries** - Transform exceptions to Results at module interfaces
4. **Provide clear error messages** - Include context and suggested fixes
5. **Log errors appropriately** - Use structured logging for errors
6. **Document error conditions** - List possible error codes in function docs
7. **Test error paths** - Include error cases in unit tests

## Performance Considerations

- Result<T> has minimal overhead (similar to std::variant)
- No dynamic allocation for error information
- Branch prediction friendly when errors are rare
- Consider `noexcept` for performance-critical paths

## Future Enhancements

When C++23's `std::expected` becomes widely available:
```cpp
template<typename T>
using Result = std::expected<T, error_info>;
```

This will provide better ergonomics with monadic operations:
```cpp
auto result = read_config("app.conf")
    .and_then(parse_config)
    .or_else(load_default_config);
```
---

*Last Updated: 2025-10-20*
