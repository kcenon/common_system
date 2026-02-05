# Error Code Guidelines

**Version**: 0.2.0.0
**Last Updated**: 2025-11-08
**Status**: Active

## Overview

This document defines the error code allocation strategy and usage guidelines for all systems in the ecosystem. Error codes are centrally managed in `include/kcenon/common/error/error_codes.h` with compile-time validation to prevent conflicts.

## Error Code Ranges

| Range | System | Status |
|-------|--------|--------|
| 0 | Success | Reserved |
| -1 to -99 | common_system | Active |
| -100 to -199 | thread_system | Active |
| -200 to -299 | logger_system | Active |
| -300 to -399 | monitoring_system | Active |
| -400 to -499 | container_system | Active |
| -500 to -599 | database_system | Active |
| -600 to -699 | network_system | Active |
| -1000+ | Reserved | Future |

## Adding New Error Codes

### Step 1: Choose the Correct Range

Identify which system the error belongs to and use its designated range.

### Step 2: Add the Error Code

Add the code in `error_codes.h` within the appropriate namespace:

```cpp
namespace codes {
namespace your_system {
    constexpr int base = -XXX;  // Your system's base

    // Group by category with comments
    // Example: Connection errors (-500 to -519)
    constexpr int your_new_error = base - N;  // N is offset
}
}
```

### Step 3: Add Validation

Add compile-time validation in the `validation` namespace:

```cpp
namespace validation {
    static_assert(codes::your_system::your_new_error >= -XXX
                  && codes::your_system::your_new_error <= -YYY,
                  "your_system error codes must be in range [-XXX, -YYY]");
}
```

### Step 4: Add Error Message

Update `get_error_message()` function:

```cpp
case codes::your_system::your_new_error:
    return "Human-readable error message";
```

### Step 5: Verify

Compile and verify no assertion failures:

```bash
clang++ -std=c++20 -I. -fsyntax-only test_error_codes.cpp
```

## Usage in Code

### Creating Errors

```cpp
#include <kcenon/common/error/error_codes.h>
#include <kcenon/common/patterns/result.h>

using namespace kcenon::common;
using namespace kcenon::common::error;

// Method 1: Using error_info directly
auto error = error_info{
    codes::thread_system::pool_full,
    "Thread pool has reached maximum capacity",
    "thread_pool::enqueue"
};

// Method 2: Using get_error_message for standard message
auto error2 = error_info{
    codes::logger_system::file_open_failed,
    std::string(get_error_message(codes::logger_system::file_open_failed)),
    "file_writer::open"
};

// Method 3: With additional details
auto error3 = error_info{
    codes::monitoring_system::storage_full,
    "Metric storage capacity exceeded",
    "metrics_collector",
    "Current usage: 10000/10000 metrics"
};
```

### Returning Errors in Result<T>

```cpp
Result<int> calculate_something() {
    if (invalid_input) {
        return Result<int>::err(
            codes::common_errors::invalid_argument,
            "Input must be positive",
            "calculator"
        );
    }

    return Result<int>::ok(42);
}
```

### Error Handling

```cpp
auto result = some_operation();
if (result.is_err()) {
    const auto& err = result.error();

    // Check specific error codes
    if (err.code == codes::thread_system::pool_full) {
        // Handle pool full condition
    } else if (err.code == codes::common_errors::timeout) {
        // Handle timeout
    }

    // Or get category
    auto category = get_category_name(err.code);
    // Handle by category
}
```

## Best Practices

### DO

✅ **Use compile-time constants**: Always use the defined constants from `error_codes.h`

```cpp
// Good
return Result<int>::err(codes::thread_system::pool_full, "Pool full", "enqueue");
```

✅ **Provide context**: Include module/function name in error_info

```cpp
// Good
error_info{-100, "Pool full", "thread_pool::enqueue", "size: 1000/1000"}
```

✅ **Group related errors**: Keep error codes logically organized

```cpp
// Connection errors (-500 to -519)
constexpr int connection_failed = base - 0;
constexpr int connection_timeout = base - 1;

// Pool errors (-520 to -539)
constexpr int pool_exhausted = base - 20;
```

### DON'T

❌ **Use magic numbers**: Never use literal error codes

```cpp
// Bad
return Result<int>::err(-100, "Error");

// Good
return Result<int>::err(codes::thread_system::pool_full, "Pool full");
```

❌ **Exceed range boundaries**: Stay within your system's allocated range

```cpp
// Bad - thread_system using logger range
constexpr int my_error = -250;  // Belongs to logger_system!

// Good
constexpr int my_error = base - 80;  // Within thread_system range
```

❌ **Skip validation**: Always add static_assert for new codes

```cpp
// Bad - no validation
constexpr int new_error = base - 99;

// Good - with validation
constexpr int new_error = base - 99;
static_assert(new_error >= -199 && new_error <= -100, "Range check");
```

## Debugging Error Codes

### Print Error Information

```cpp
void print_error_info(const error_info& err) {
    std::cout << "Error Code: " << err.code << "\n"
              << "Category: " << get_category_name(err.code) << "\n"
              << "Message: " << err.message << "\n"
              << "Module: " << err.module << "\n";
    if (err.details) {
        std::cout << "Details: " << *err.details << "\n";
    }
}
```

### Find Error Code Owner

Use `get_category_name()` to identify which system owns an error code:

```cpp
int code = -155;
std::cout << "Owner: " << get_category_name(code) << std::endl;
// Output: "ThreadSystem"
```

## Migration from Old Code

### Migrating from error_codes namespace in result.h

Old code in `result.h` (deprecated):

```cpp
using kcenon::common::error_codes::INVALID_ARGUMENT;
```

New code:

```cpp
using kcenon::common::error::codes::common_errors::invalid_argument;
```

### Batch Migration Script

```bash
# Replace old error code references
find . -name "*.cpp" -o -name "*.h" | xargs sed -i '' \
  's/error_codes::INVALID_ARGUMENT/error::codes::common_errors::invalid_argument/g'
```

## Error Code Registry

The current registry is maintained in:
- **Header**: `include/kcenon/common/error/error_codes.h`
- **Test**: `test_error_codes.cpp`
- **This Document**: `docs/ERROR_CODE_GUIDELINES.md`

To view all registered error codes:

```bash
grep -r "constexpr int" include/kcenon/common/error/error_codes.h
```

## Compile-Time Validation

The error code system uses C++20 `static_assert` for compile-time validation:

```cpp
// Range validation
static_assert(codes::thread_system::pool_full >= -199
              && codes::thread_system::pool_full <= -100,
              "thread_system error codes must be in range [-199, -100]");
```

If an error code is added outside its designated range, **compilation will fail** with a clear error message.

## Future Extensions

### Planned Features

- [ ] Error code to exception mapping
- [ ] Internationalization (i18n) support for error messages
- [ ] Error code telemetry and analytics
- [ ] JSON schema for error code registry

### Adding New System Ranges

To add a new system (e.g., `auth_system` with range -700 to -799):

1. Update the category enum:
```cpp
enum class category : int {
    // ...
    auth_system = -700,
};
```

2. Add error code namespace:
```cpp
namespace auth_system {
    constexpr int base = static_cast<int>(category::auth_system);
    constexpr int authentication_failed = base - 0;
    // ...
}
```

3. Add validation:
```cpp
static_assert(codes::auth_system::base == -700, "auth_system base must be -700");
```

4. Update `get_category_name()` and `get_error_message()`

## References

- [Result Pattern](./ERROR_HANDLING.md)
- [Architecture](../ARCHITECTURE.md)
- [Common System README](../README.md)

---

**Maintainer**: Architecture Team
**Review Cycle**: Each major version release
**Contact**: architecture@kcenon.com
