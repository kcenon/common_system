# Common System - Improvement Plan

> **Language:** **English** | [한국어](IMPROVEMENTS_KO.md)

## Current Status

**Version:** 1.0.0
**Last Review:** 2025-01-20
**Overall Score:** 4.0/5

### Strengths
- Excellent Result<T> pattern implementation (Rust-like)
- Clean interface definitions
- Strong type safety
- Comprehensive error code system

### Areas for Improvement
- Exception mapping relies on RTTI
- Some namespace pollution in examples
- Limited C++20 features usage

---

## Critical Issues

None identified in common_system core.

---

## High Priority Improvements

### 1. Exception Mapper - Remove RTTI Dependency

**Location:** `include/kcenon/common/patterns/result.h:558-593`

**Current Issue:**
```cpp
static error_info map_exception(const std::exception& e, const std::string& module = "") {
    // Uses dynamic_cast - requires RTTI
    if (dynamic_cast<const std::bad_alloc*>(&e)) { ... }
    if (dynamic_cast<const std::system_error*>(&e)) { ... }
    // ...
}
```

**Problems:**
- Breaks with `-fno-rtti` compiler flag
- Performance overhead from dynamic_cast
- Not suitable for embedded systems

**Proposed Solution:**
```cpp
// Option 1: Type-erased exception wrapper
class typed_exception_wrapper {
    std::exception_ptr ptr_;
    std::type_index type_;
    std::function<error_info(const std::exception&)> mapper_;

public:
    template<typename E>
    typed_exception_wrapper(const E& e)
        : ptr_(std::make_exception_ptr(e))
        , type_(typeid(E))
        , mapper_([](const std::exception& ex) -> error_info {
            return map_specific_exception(static_cast<const E&>(ex));
        }) {}

    error_info map_to_error_info() const;
};

// Option 2: Compile-time dispatch
template<typename E>
error_info map_exception_impl(const E& e, std::string_view module) {
    if constexpr (std::is_same_v<E, std::bad_alloc>) {
        return {error_codes::OUT_OF_MEMORY, e.what(), std::string(module)};
    } else if constexpr (std::is_base_of_v<std::system_error, E>) {
        return {e.code().value(), e.what(), std::string(module)};
    } else if constexpr (std::is_base_of_v<std::invalid_argument, E>) {
        return {error_codes::INVALID_ARGUMENT, e.what(), std::string(module)};
    }
    // ... more conditions ...
    return {error_codes::INTERNAL_ERROR, e.what(), std::string(module)};
}

// Usage
try {
    // ...
} catch (const std::bad_alloc& e) {
    return Result<T>(map_exception_impl(e, "module_name"));
} catch (const std::system_error& e) {
    return Result<T>(map_exception_impl(e, "module_name"));
} catch (const std::exception& e) {
    return Result<T>(map_exception_impl(e, "module_name"));
}
```

**Priority:** P1
**Effort:** 2-3 days
**Impact:** High (enables RTTI-free builds)

---

### 2. Add C++20 Concepts for Type Constraints

**Location:** `include/kcenon/common/patterns/result.h`

**Current Issue:**
Template methods lack explicit constraints, leading to unclear error messages.

**Proposed Solution:**
```cpp
// Add concepts header
#if __cplusplus >= 202002L
#include <concepts>

namespace common::concepts {

template<typename T>
concept Mappable = requires(T t) {
    typename std::invoke_result_t<T>;
};

template<typename F, typename T>
concept UnaryFunction = std::invocable<F, T>;

template<typename F, typename T>
concept ReturnsResult = requires(F f, T t) {
    { f(t) } -> std::same_as<Result<typename std::invoke_result_t<F, T>::value_type>>;
};

} // namespace common::concepts

// Use in Result<T>
template<typename F>
    requires common::concepts::UnaryFunction<F, T>
auto map(F&& func) const -> Result<std::invoke_result_t<F, T>> {
    using ReturnType = std::invoke_result_t<F, T>;

    if (is_ok()) {
        return Result<ReturnType>(func(std::get<T>(value_)));
    } else {
        return Result<ReturnType>(std::get<error_info>(value_));
    }
}

template<typename F>
    requires common::concepts::ReturnsResult<F, T>
auto and_then(F&& func) const -> std::invoke_result_t<F, T> {
    using ReturnType = std::invoke_result_t<F, T>;

    if (is_ok()) {
        return func(std::get<T>(value_));
    } else {
        return ReturnType(std::get<error_info>(value_));
    }
}
#endif // C++20
```

**Priority:** P2
**Effort:** 1-2 days
**Impact:** Medium (better compile-time errors, future-proofing)

---

## Medium Priority Improvements

### 3. Add Result<T>::expect() for Better Error Messages

**Proposed Addition:**
```cpp
// In Result<T> class
template<typename T>
class Result {
    // ... existing code ...

    /**
     * @brief Unwrap with custom panic message
     * @param msg Custom error message
     * @throws std::runtime_error with custom message if error
     */
    const T& expect(std::string_view msg) const {
        if (is_err()) {
            const auto& err = std::get<error_info>(value_);
            std::string full_msg = std::string(msg) + ": " + err.message;
            throw std::runtime_error(full_msg);
        }
        return std::get<T>(value_);
    }

    T& expect(std::string_view msg) {
        if (is_err()) {
            const auto& err = std::get<error_info>(value_);
            std::string full_msg = std::string(msg) + ": " + err.message;
            throw std::runtime_error(full_msg);
        }
        return std::get<T>(value_);
    }
};

// Usage
auto config = load_config().expect("Failed to load configuration");
// Instead of generic "Called unwrap on error"
```

**Priority:** P3
**Effort:** 0.5 day
**Impact:** Low (developer experience improvement)

---

### 4. Add Result<T> Monadic Operators

**Proposed Addition:**
```cpp
// Add operator| for or_else
template<typename T, typename F>
Result<T> operator|(Result<T>&& result, F&& alternative) {
    return result.or_else(std::forward<F>(alternative));
}

// Add operator& for and_then
template<typename T, typename F>
auto operator&(Result<T>&& result, F&& func)
    -> decltype(result.and_then(std::forward<F>(func))) {
    return result.and_then(std::forward<F>(func));
}

// Usage - pipeline style
auto result = load_file("config.json")
    & parse_json
    & validate_schema
    | [] { return load_default_config(); };
```

**Priority:** P3
**Effort:** 1 day
**Impact:** Low (syntactic sugar, may reduce readability for some)

---

### 5. Improve Error Context Propagation

**Current Issue:**
Error context can be lost through multiple layers.

**Proposed Solution:**
```cpp
struct error_info {
    int code;
    std::string message;
    std::string module;
    std::optional<std::string> details;

    // NEW: Error chain support
    std::vector<error_info> causes;  // Chain of causes

    /**
     * @brief Add a cause to the error chain
     */
    error_info& caused_by(const error_info& cause) {
        causes.push_back(cause);
        return *this;
    }

    /**
     * @brief Get full error chain as string
     */
    std::string full_chain() const {
        std::string result = message;
        for (const auto& cause : causes) {
            result += "\n  Caused by: " + cause.message;
        }
        return result;
    }
};

// Usage
Result<Config> load_config() {
    auto result = read_file("config.json");
    if (!result) {
        return error_info{-1, "Failed to load config", "config_loader"}
            .caused_by(result.error());
    }
    // ...
}
```

**Priority:** P3
**Effort:** 2 days
**Impact:** Medium (better debugging)

---

## Low Priority Enhancements

### 6. Add Convenient Conversion Functions

**Proposed Addition:**
```cpp
namespace common {

// Convert Optional<T> to Result<T>
template<typename T>
Result<T> from_optional(const Optional<T>& opt,
                       const error_info& error_on_none) {
    if (opt.is_some()) {
        return Result<T>::ok(opt.value());
    }
    return Result<T>::err(error_on_none);
}

// Convert std::optional to Result<T>
template<typename T>
Result<T> from_std_optional(const std::optional<T>& opt,
                            const error_info& error_on_none) {
    if (opt.has_value()) {
        return Result<T>::ok(*opt);
    }
    return Result<T>::err(error_on_none);
}

// Convert Result<T> to std::optional
template<typename T>
std::optional<T> to_optional(const Result<T>& result) {
    if (result.is_ok()) {
        return std::make_optional(result.value());
    }
    return std::nullopt;
}

} // namespace common
```

**Priority:** P4
**Effort:** 0.5 day
**Impact:** Low (convenience)

---

## Testing Recommendations

### Required Test Coverage

1. **Exception Mapper Tests:**
   ```cpp
   TEST(ExceptionMapper, MapsStdExceptions) {
       auto err = exception_mapper::map_exception(
           std::bad_alloc(), "test_module");
       EXPECT_EQ(err.code, error_codes::OUT_OF_MEMORY);
   }

   TEST(ExceptionMapper, WorksWithoutRTTI) {
       // Compile with -fno-rtti
       auto err = map_exception_impl(std::bad_alloc(), "test");
       EXPECT_EQ(err.code, error_codes::OUT_OF_MEMORY);
   }
   ```

2. **Result<T> Monadic Operations:**
   ```cpp
   TEST(Result, MapPreservesError) {
       Result<int> err = error_info{-1, "error"};
       auto mapped = err.map([](int x) { return x * 2; });
       EXPECT_TRUE(mapped.is_err());
       EXPECT_EQ(mapped.error().code, -1);
   }

   TEST(Result, AndThenChainsResults) {
       auto result = ok<int>(5)
           .and_then([](int x) { return ok<int>(x * 2); })
           .and_then([](int x) { return ok<int>(x + 1); });
       EXPECT_EQ(result.value(), 11);
   }
   ```

---

## Implementation Timeline

| Phase | Tasks | Duration | Dependencies |
|-------|-------|----------|--------------|
| Phase 1 | Remove RTTI from exception mapper | 2-3 days | None |
| Phase 2 | Add C++20 concepts | 1-2 days | C++20 compiler |
| Phase 3 | Add expect() method | 0.5 day | None |
| Phase 4 | Error chain support | 2 days | None |
| Phase 5 | Monadic operators & conversions | 1.5 days | None |

**Total Estimated Effort:** 7-9 days

---

## Breaking Changes

### Version 2.0.0 (Proposed)

**Breaking Changes:**
- Exception mapper signature may change if RTTI removal approach is adopted
- Consider deprecating lowercase factory functions (ok, error) in favor of static methods (Result<T>::ok)

**Migration Guide:**
```cpp
// Old (1.x)
auto result = ok<int>(42);
auto err = error<int>(-1, "message", "module");

// New (2.0) - both work, but static preferred
auto result = Result<int>::ok(42);
auto err = Result<int>::err(-1, "message", "module");
```

---

## Documentation Improvements Needed

1. **Add Architecture Decision Records (ADR):**
   - Why Result<T> over exceptions?
   - Why std::variant over inheritance?
   - RTTI dependency decision

2. **Add Migration Guides:**
   - From exception-based to Result<T>
   - From bool return to Result<void>

3. **Add Best Practices:**
   - When to use Result vs exception
   - Error code organization
   - Module-specific error ranges

---

## Compatibility Matrix

| Compiler | Version | Status | Notes |
|----------|---------|--------|-------|
| GCC | 10+ | ✅ Full | C++17/20 support |
| GCC | 9 | ⚠️ Limited | C++17 only |
| Clang | 12+ | ✅ Full | C++17/20 support |
| Clang | 10-11 | ⚠️ Limited | C++17 only |
| MSVC | 2019+ | ✅ Full | C++17/20 support |
| Apple Clang | 13+ | ✅ Full | C++17/20 support |

**Minimum Requirements:**
- C++17 (current)
- C++20 (recommended for future features)

---

## References

- [Result<T> Design Document](./docs/result_design.md)
- [Error Handling Guidelines](./docs/error_handling.md)
- [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/)
- [Rust Result<T,E> Documentation](https://doc.rust-lang.org/std/result/)
