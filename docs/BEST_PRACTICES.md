# Result<T> Best Practices

This guide documents the recommended patterns for creating and using `Result<T>` objects in the Common System.

## Header Structure

The Result pattern is organized into modular headers for improved compilation times and maintainability:

```
include/kcenon/common/patterns/
├── result.h                      # Umbrella header (include this for full functionality)
└── result/
    ├── fwd.h                     # Forward declarations
    ├── error_info.h              # error_info struct
    ├── result_core.h             # Result<T> class definition
    ├── optional.h                # Optional<T> class
    ├── result_funcs.h            # Factory and helper functions
    ├── error_codes_compat.h      # Backward compatibility aliases
    ├── exception_conversion.h    # try_catch utilities
    └── result_macros.h           # Convenience macros
```

### Include Strategies

**Full Include (Recommended for most cases):**
```cpp
#include <kcenon/common/patterns/result.h>
```

**Selective Include (For faster compilation):**
```cpp
// Minimal: just Result class and error_info
#include <kcenon/common/patterns/result/result_core.h>

// Add factory functions
#include <kcenon/common/patterns/result/result_funcs.h>

// Add exception conversion
#include <kcenon/common/patterns/result/exception_conversion.h>
```

## Recommended Patterns

### Creating Successful Results

Use the free function `ok()` for creating successful results:

```cpp
// Recommended: Use ok() free function
auto result = ok(42);
auto string_result = ok(std::string("hello"));

// For void operations, use ok() without arguments
return ok();  // Returns VoidResult
```

Avoid using the static method `Result<T>::ok()` directly unless necessary for type deduction:

```cpp
// Less preferred (verbose)
auto result = Result<int>::ok(42);

// Use when type deduction is needed
Result<int> get_value() {
    return Result<int>::ok(compute());  // Type explicit
}
```

### Creating Error Results

Use `make_error<T>()` for creating error results:

```cpp
// Recommended: Use make_error<T>()
return make_error<int>(error_codes::INVALID_ARGUMENT, "Value must be positive");

// With module name
return make_error<int>(
    error_codes::NOT_FOUND,
    "Resource not found",
    "database_system"
);

// With details
return make_error<int>(
    error_codes::IO_ERROR,
    "Failed to read file",
    "file_system",
    "Path: /etc/config.json"
);
```

Alternative: Use `Result<T>::err()` static factory:

```cpp
// Also acceptable
return Result<int>::err(error_info{code, "message", "module"});
return Result<int>::err(1001, "Error message", "module");
```

### Exception Wrapping

Use `try_catch<T>()` to wrap code that may throw exceptions:

```cpp
// Recommended: Wrap exception-throwing code
auto result = try_catch<int>([&]() {
    return std::stoi(input);  // May throw std::invalid_argument
}, "parser");

// For void operations
auto void_result = try_catch_void([&]() {
    save_to_file(data);  // May throw std::runtime_error
}, "file_system");
```

### Chaining Operations

Use member functions for chaining:

```cpp
// Recommended: Use method chaining
auto result = get_config()
    .map([](const Config& c) { return c.value; })
    .and_then([](int v) -> Result<std::string> {
        return ok(std::to_string(v));
    })
    .or_else([](const error_info& err) {
        log_error(err.message);
        return ok("default");
    });
```

## Patterns to Avoid

### Direct Construction (Discouraged)

```cpp
// Avoid: Direct construction with value
Result<int> result{42};  // Works but not explicit

// Prefer: Factory function
auto result = ok(42);
```

### Uppercase Factories (Legacy)

```cpp
// Avoid: Uppercase factories (legacy compatibility only)
auto result = Ok(42);      // Use ok() instead
auto error = Err<int>(1, "error");  // Use make_error<T>() instead
```

### Default Construction (Prohibited)

```cpp
// Error: Default construction is deleted
Result<int> result;  // Compile error!

// Use explicit uninitialized if needed (rare)
auto result = Result<int>::uninitialized();
```

## Summary Table

| Pattern | Recommended | Usage |
|---------|-------------|-------|
| `ok(value)` | Yes | Create successful result |
| `ok()` | Yes | Create successful VoidResult |
| `make_error<T>(code, msg)` | Yes | Create error result |
| `make_error<T>(code, msg, module)` | Yes | Create error with module |
| `make_error<T>(code, msg, mod, details)` | Yes | Create error with details |
| `try_catch<T>([&]{ ... })` | Yes | Wrap exception-throwing code |
| `try_catch_void([&]{ ... })` | Yes | Wrap void functions |
| `.map()`, `.and_then()`, `.or_else()` | Yes | Chain operations |
| `Result<T>::ok(value)` | Acceptable | When type explicit needed |
| `Result<T>::err(error_info)` | Acceptable | Alternative to make_error |
| `Ok(value)` | Legacy | Use `ok()` instead |
| `Err<T>(...)` | Legacy | Use `make_error<T>()` instead |
| `Result<T> r{value}` | Discouraged | Use `ok()` instead |
| `Result<T> r;` | Prohibited | Deleted default constructor |

## Error Handling Examples

### Basic Error Handling

```cpp
Result<Config> load_config(const std::string& path) {
    if (path.empty()) {
        return make_error<Config>(
            error_codes::INVALID_ARGUMENT,
            "Path cannot be empty",
            "config_loader"
        );
    }

    return try_catch<Config>([&]() {
        return parse_config_file(path);
    }, "config_loader");
}

void process() {
    auto result = load_config("/etc/app.json");

    if (result.is_ok()) {
        use_config(result.value());
    } else {
        log_error(result.error().message);
    }
}
```

### Chained Operations

```cpp
Result<std::string> process_user_input(const std::string& input) {
    return try_catch<int>([&]() {
        return std::stoi(input);
    }, "parser")
    .and_then([](int value) -> Result<int> {
        if (value < 0) {
            return make_error<int>(
                error_codes::INVALID_ARGUMENT,
                "Value must be non-negative"
            );
        }
        return ok(value * 2);
    })
    .map([](int value) {
        return std::to_string(value);
    });
}
```

### Using Helper Functions

```cpp
#include <kcenon/common/patterns/result_helpers.h>

Result<int> complex_operation() {
    // Use combine_results for multiple operations
    auto combined = helpers::combine_results(
        get_a(),
        get_b(),
        get_c()
    );

    if (combined.is_err()) {
        return make_error<int>(combined.error());
    }

    auto [a, b, c] = combined.value();
    return ok(a + b + c);
}
```

## Migration Guide

If you have code using deprecated patterns:

### From Direct Construction

```cpp
// Before
Result<int> result{42};
Result<int> error_result{error_info{1, "error"}};

// After
auto result = ok(42);
auto error_result = make_error<int>(1, "error");
```

### From Uppercase Factories

```cpp
// Before
auto result = Ok(42);
auto error = Err<int>(1, "error");

// After
auto result = ok(42);
auto error = make_error<int>(1, "error");
```

### From Uninitialized Default

```cpp
// Before (if you had old code with default construction)
Result<int> result;  // Was uninitialized

// After
auto result = Result<int>::uninitialized();  // Explicit
// Or better: initialize properly
auto result = ok(default_value);
```
