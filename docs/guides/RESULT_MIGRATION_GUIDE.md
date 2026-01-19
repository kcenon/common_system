# Result\<T\> Migration Guide

> **Language:** **English** | [한국어](RESULT_MIGRATION_GUIDE.kr.md)

## Table of Contents

- [Overview](#overview)
- [Problem Statement](#problem-statement)
- [Standard API Reference](#standard-api-reference)
  - [Core Types](#core-types)
  - [Factory Functions](#factory-functions)
  - [Member Functions](#member-functions)
  - [Monadic Operations](#monadic-operations)
  - [Helper Functions (Deprecated)](#helper-functions-deprecated)
- [Free Function Deprecation](#free-function-deprecation)
- [Migration Steps](#migration-steps)
  - [Phase 1: Add Deprecated Aliases](#phase-1-add-deprecated-aliases)
  - [Phase 2: Update Internal Usage](#phase-2-update-internal-usage)
  - [Phase 3: Remove Deprecated Types](#phase-3-remove-deprecated-types)
- [System-Specific Migration](#system-specific-migration)
  - [thread_system](#thread_system)
  - [database_system](#database_system)
  - [network_system](#network_system)
  - [monitoring_system](#monitoring_system)
- [Backward Compatibility](#backward-compatibility)
- [Code Examples](#code-examples)
- [Testing Migration](#testing-migration)
- [FAQ](#faq)
- [References](#references)

**Version**: 1.1
**Last Updated**: 2026-01-03

---

## Overview

This guide provides comprehensive instructions for migrating all KCENON ecosystem systems to use the standardized `Result<T>` type from `common_system`. The goal is to ensure type consistency and seamless interoperability across all systems.

**Target Audience**: Developers maintaining or contributing to KCENON ecosystem systems.

---

## Problem Statement

Multiple `Result<T>` implementations currently exist across the ecosystem:

| System | File | Type Name | Key Methods |
|--------|------|-----------|-------------|
| **common_system** | `patterns/result.h` | `Result<T>` | `is_ok()`, `value()`, `unwrap()` |
| thread_system | `core/error_handling.h` | `result<T>` | `is_success()`, `value()` |
| database_system | `core/result.h` | `result<T>` | `is_ok()`, `get_error()` |
| network_system | `utils/result_types.h` | Custom result | Various |
| monitoring_system | `core/result_types.h` | Custom result | Various |

**Issues with Multiple Implementations**:

1. **Type Name Inconsistency**: `Result` vs `result` (case sensitivity matters in C++)
2. **Method Name Differences**: `is_ok` vs `is_success`, `value` vs `unwrap`
3. **Type Conversion Required**: Passing results between systems requires conversion
4. **Compile-Time Safety Compromised**: Different types break template deduction

---

## Standard API Reference

The standard `Result<T>` type is defined in:

```cpp
#include <kcenon/common/patterns/result.h>
```

Namespace: `kcenon::common`

### Core Types

```cpp
namespace kcenon::common {

// Main Result type
template<typename T>
class Result;

// Error information structure
struct error_info {
    int code;
    std::string message;
    std::string module;
    std::optional<std::string> details;
};

// Alias for backward compatibility
using error_code = error_info;

// Result for void operations
using VoidResult = Result<std::monostate>;

}
```

### Factory Functions

```cpp
// Create successful result
template<typename T>
Result<T> ok(T value);

// Create successful void result
VoidResult ok();

// Create error result
template<typename T>
Result<T> make_error(int code, const std::string& message,
                     const std::string& module = "");

// Create error result with details
template<typename T>
Result<T> make_error(int code, const std::string& message,
                     const std::string& module, const std::string& details);

// Create error result from error_info
template<typename T>
Result<T> make_error(const error_info& err);
```

**Static Factory Methods** (on Result class):

```cpp
// Create successful result
Result<T>::ok(value);

// Create error result
Result<T>::err(error_info);
Result<T>::err(code, message, module);

// Create explicitly uninitialized result (use sparingly)
Result<T>::uninitialized();
```

### Member Functions

```cpp
template<typename T>
class Result {
public:
    // State checking
    bool is_ok() const;      // Returns true if contains value
    bool is_err() const;     // Returns true if contains error

    // Value access
    const T& value() const;  // Get value reference (undefined if error)
    T& value();              // Get mutable value reference
    const T& unwrap() const; // Get value or throw if error
    T& unwrap();             // Get mutable value or throw if error
    T unwrap_or(T default_value) const;  // Get value or default
    T value_or(T default_value) const;   // C++23 std::expected compatible

    // Error access
    const error_info& error() const;  // Get error reference
};
```

### Monadic Operations

```cpp
// Transform success value
template<typename F>
auto map(F&& func) const -> Result<decltype(func(value))>;

// Chain operations that return Result
template<typename F>
auto and_then(F&& func) const -> decltype(func(value));

// Provide alternative if error
template<typename F>
Result<T> or_else(F&& func) const;
```

### Helper Functions (Deprecated)

> ⚠️ **Deprecation Notice**: Free functions are deprecated in favor of member methods.
> See [API Standardization](#free-function-deprecation) for migration details.

```cpp
// Free functions (DEPRECATED - use member methods instead)
template<typename T> bool is_ok(const Result<T>& result);       // Use result.is_ok()
template<typename T> bool is_error(const Result<T>& result);    // Use result.is_err()
template<typename T> const T& get_value(const Result<T>& result);  // Use result.value()
template<typename T> const error_info& get_error(const Result<T>& result);  // Use result.error()
template<typename T> T value_or(const Result<T>& result, T default_value);  // Use result.unwrap_or()
template<typename T> const T* get_if_ok(const Result<T>& result);
template<typename T> const error_info* get_if_error(const Result<T>& result);
```

---

## Free Function Deprecation

As part of the API standardization effort (see [Issue #258](https://github.com/kcenon/common_system/issues/258)), free functions for `Result<T>` operations are being deprecated in favor of member methods.

### Rationale

Analysis of the codebase shows that member methods are used **5.6x more frequently** than free functions:

| API Style | Usage Count | Percentage |
|-----------|-------------|------------|
| Member Methods | 510 | **84.9%** |
| Free Functions | 91 | 15.1% |

### Migration Table

| Free Function | Replacement Member Method |
|---------------|--------------------------|
| `is_ok(result)` | `result.is_ok()` |
| `is_error(result)` | `result.is_err()` |
| `get_value(result)` | `result.value()` |
| `get_error(result)` | `result.error()` |
| `value_or(result, def)` | `result.unwrap_or(def)` or `result.value_or(def)` |
| `map(result, func)` | `result.map(func)` |
| `and_then(result, func)` | `result.and_then(func)` |
| `or_else(result, func)` | `result.or_else(func)` |

### Deprecation Timeline

| Phase | Version | Status | Action |
|-------|---------|--------|--------|
| Documentation | v1.x | ✅ Completed | Update docs to recommend member methods |
| Deprecation Attributes | v1.x | ✅ Completed | Add `[[deprecated]]` to free functions |
| Removal | v2.0.0 | 🔜 Planned | Remove deprecated free functions |

> **Note**: As of the current release, all free functions in `utilities.h` are marked with
> `[[deprecated]]` attributes. Using them will generate compiler warnings. Plan to migrate
> to member methods before v2.0.0 to avoid breaking changes.

### When Free Functions Are Still Appropriate

Free functions should only be used in these specific contexts:

1. **ADL Requirements**: Generic templates that require argument-dependent lookup
2. **Functional Composition**: When working with higher-order functions that expect free functions

> **Note**: The `COMMON_ASSIGN_OR_RETURN` and `COMMON_RETURN_IF_ERROR` macros now use
> member methods internally, so they no longer trigger deprecation warnings.

### Code Migration Example

```cpp
// BEFORE (free function style)
if (is_ok(result)) {
    auto value = get_value(result);
    process(value);
} else {
    log_error(get_error(result));
}

// AFTER (member method style - RECOMMENDED)
if (result.is_ok()) {
    auto value = result.value();
    process(value);
} else {
    log_error(result.error());
}
```

For more details, see [Best Practices - Recommended API Style](BEST_PRACTICES.md#recommended-api-style).

---

## Migration Steps

### Phase 1: Add Deprecated Aliases

Add backward-compatible aliases that point to the standard type while marking old types as deprecated.

**Example for thread_system**:

```cpp
// In thread_system/core/error_handling.h

// Import the standard Result type
#include <kcenon/common/patterns/result.h>

namespace kcenon::thread {

// Deprecated alias - will be removed in next major version
template<typename T>
using result [[deprecated("Use kcenon::common::Result<T> instead")]]
    = kcenon::common::Result<T>;

// Method compatibility wrapper (if needed)
template<typename T>
[[deprecated("Use is_ok() instead")]]
inline bool is_success(const common::Result<T>& r) {
    return r.is_ok();
}

} // namespace kcenon::thread
```

### Phase 2: Update Internal Usage

Replace all internal usage of the old type with `kcenon::common::Result<T>`.

**Before**:
```cpp
namespace kcenon::thread {

result<void> thread_pool::submit(std::function<void()> task) {
    if (!running_) {
        return result<void>::error(-1, "Pool not running");
    }
    // ...
    return result<void>::ok();
}

} // namespace kcenon::thread
```

**After**:
```cpp
#include <kcenon/common/patterns/result.h>

namespace kcenon::thread {

using common::Result;
using common::VoidResult;
using common::ok;
using common::make_error;
using common::error_codes::INVALID_STATE;

VoidResult thread_pool::submit(std::function<void()> task) {
    if (!running_) {
        return make_error<std::monostate>(INVALID_STATE,
            "Pool not running", "thread_pool");
    }
    // ...
    return ok();
}

} // namespace kcenon::thread
```

### Phase 3: Remove Deprecated Types

After a deprecation period (typically one major version), remove the deprecated aliases and require all users to use the standard type directly.

**Timeline Recommendation**:
- **Patch Release (x.y.z)**: Add aliases with `[[deprecated]]`
- **Minor Release (x.Y.0)**: Log deprecation warnings at runtime
- **Major Release (X.0.0)**: Remove deprecated types

---

## System-Specific Migration

### thread_system

**Current Location**: `core/error_handling.h`
**Current Type**: `result<T>` with `is_success()` method

**Migration Checklist**:
- [ ] Add `#include <kcenon/common/patterns/result.h>`
- [ ] Create deprecated alias: `template<typename T> using result = common::Result<T>;`
- [ ] Add `is_success()` compatibility wrapper
- [ ] Update all internal `result<T>` to `common::Result<T>`
- [ ] Update factory calls: `result::ok()` → `common::ok()`
- [ ] Update factory calls: `result::error()` → `common::make_error<T>()`
- [ ] Update tests

### database_system

**Current Location**: `core/result.h`
**Current Type**: `result<T>` with `get_error()` method

**Migration Checklist**:
- [ ] Add `#include <kcenon/common/patterns/result.h>`
- [ ] Create deprecated alias
- [ ] Verify `get_error()` compatibility (standard uses `error()`)
- [ ] Update all internal usage
- [ ] Update tests

### network_system

**Current Location**: `utils/result_types.h`
**Current Type**: Custom result type

**Migration Checklist**:
- [ ] Analyze current API differences
- [ ] Add `#include <kcenon/common/patterns/result.h>`
- [ ] Create deprecated aliases for all custom types
- [ ] Create compatibility wrappers for custom methods
- [ ] Update all internal usage
- [ ] Update tests

### monitoring_system

**Current Location**: `core/result_types.h`
**Current Type**: Custom result type

**Migration Checklist**:
- [ ] Analyze current API differences
- [ ] Add `#include <kcenon/common/patterns/result.h>`
- [ ] Create deprecated aliases
- [ ] Update all internal usage
- [ ] Update tests

---

## Backward Compatibility

To maintain backward compatibility during migration, use these patterns:

### 1. Namespace Alias

```cpp
namespace kcenon::your_system {
    // Make common types easily accessible
    using common::Result;
    using common::VoidResult;
    using common::error_info;
    using common::ok;
    using common::make_error;
}
```

### 2. Method Wrappers

If your system used different method names:

```cpp
namespace kcenon::your_system::compat {

template<typename T>
[[deprecated("Use is_ok() instead")]]
inline bool is_success(const common::Result<T>& r) {
    return r.is_ok();
}

template<typename T>
[[deprecated("Use error() instead")]]
inline const common::error_info& get_error(const common::Result<T>& r) {
    return r.error();
}

} // namespace kcenon::your_system::compat
```

### 3. Macro Migration

If you had custom macros, map them to standard macros:

```cpp
// Old macro
#define THREAD_RETURN_IF_ERROR(expr) /* ... */

// Deprecation mapping
#define THREAD_RETURN_IF_ERROR(expr) \
    _Pragma("message(\"THREAD_RETURN_IF_ERROR is deprecated, use COMMON_RETURN_IF_ERROR\")") \
    COMMON_RETURN_IF_ERROR(expr)
```

---

## Code Examples

### Example 1: Basic Migration

**Before** (thread_system style):
```cpp
result<int> parse_number(const std::string& s) {
    try {
        return result<int>::ok(std::stoi(s));
    } catch (...) {
        return result<int>::error(-1, "Parse failed");
    }
}

void example() {
    auto r = parse_number("42");
    if (r.is_success()) {
        std::cout << r.value() << "\n";
    }
}
```

**After** (common_system style):
```cpp
#include <kcenon/common/patterns/result.h>

using namespace kcenon::common;

Result<int> parse_number(const std::string& s) {
    return try_catch<int>([&]() {
        return std::stoi(s);
    }, "parser");
}

void example() {
    auto r = parse_number("42");
    if (r.is_ok()) {
        std::cout << r.value() << "\n";
    }
}
```

### Example 2: Cross-System Interoperability

```cpp
#include <kcenon/common/patterns/result.h>
#include <kcenon/database/database.h>
#include <kcenon/network/client.h>

using namespace kcenon::common;

// All systems now return the same Result<T> type
Result<UserData> fetch_user(int user_id) {
    // Database query - returns common::Result
    auto db_result = database::query_user(user_id);
    if (db_result.is_err()) {
        return db_result;  // No conversion needed!
    }

    // Network request - also returns common::Result
    auto net_result = network::fetch_profile(db_result.value().profile_url);
    if (net_result.is_err()) {
        return net_result;  // Seamless error propagation
    }

    return ok(UserData{db_result.value(), net_result.value()});
}
```

### Example 3: Using Monadic Operations

```cpp
Result<ProcessedData> process_pipeline(const RawData& input) {
    return validate(input)
        .and_then([](const ValidData& v) { return transform(v); })
        .and_then([](const TransformedData& t) { return enrich(t); })
        .map([](const EnrichedData& e) { return ProcessedData(e); })
        .or_else([](const error_info& err) -> Result<ProcessedData> {
            log_error("Pipeline failed: {}", err.message);
            return make_error<ProcessedData>(err);
        });
}
```

---

## Testing Migration

### Unit Test Updates

**Before**:
```cpp
TEST(ThreadPool, SubmitTask) {
    auto result = pool.submit(task);
    EXPECT_TRUE(result.is_success());
}
```

**After**:
```cpp
TEST(ThreadPool, SubmitTask) {
    auto result = pool.submit(task);
    EXPECT_TRUE(result.is_ok());  // Method name change
}
```

### Compilation Verification

After migration, verify that deprecation warnings appear:

```bash
# Build with warnings enabled
cmake -DCMAKE_CXX_FLAGS="-Wall -Wdeprecated" ..
make 2>&1 | grep -i deprecated
```

### Integration Testing

Test cross-system interoperability:

```cpp
TEST(CrossSystem, ResultCompatibility) {
    // Results from different systems should be compatible
    common::Result<int> common_result = common::ok(42);

    // Can be used interchangeably
    EXPECT_TRUE(common_result.is_ok());
    EXPECT_EQ(common_result.value(), 42);
}
```

---

## FAQ

### Q: Why standardize on common_system's Result<T>?

**A**: The common_system's `Result<T>` implementation:
- Provides a complete Rust-like API (`map`, `and_then`, `or_else`)
- Is C++23 `std::expected` compatible (`value_or`)
- Has comprehensive error information (`error_info`)
- Includes helper macros and functions
- Is well-tested and documented

### Q: What about performance?

**A**: The standard `Result<T>` uses `std::optional<T>` internally, which has zero overhead for value storage. Error paths may have slight overhead due to `std::optional<error_info>`, but this is negligible in practice.

### Q: Can I keep using my system's custom methods?

**A**: Yes, during the migration period. Create deprecated wrappers that map to the standard methods. Plan to remove these in the next major version.

### Q: How do I handle type-erased results?

**A**: Use `VoidResult` (`Result<std::monostate>`) for void operations, or consider using `Result<std::any>` for type-erased values (with appropriate documentation).

### Q: What if I need custom error types?

**A**: The `error_info` struct includes a `details` field (`std::optional<std::string>`) for additional context. For structured custom data, serialize to the details field or extend error_info in your module (not recommended for interoperability).

---

## References

- [Error Handling Guidelines](ERROR_HANDLING.md) - Comprehensive error handling patterns
- [API Reference](../API_REFERENCE.md) - Full API documentation
- [Error Code Registry](../ERROR_CODE_REGISTRY.md) - Centralized error codes
- [Best Practices](BEST_PRACTICES.md) - General best practices

---

*For questions or issues with this migration guide, please open an issue on the common_system repository.*
