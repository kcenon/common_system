# Deprecated APIs

This document lists all deprecated APIs in the Common System library, their replacements, and migration guidance.

> **Language:** **English** | [한국어](DEPRECATION_KO.md)

---

## Overview

The Common System library follows semantic versioning. Deprecated APIs are marked with the `[[deprecated]]` attribute and will be removed in the next major version.

### Deprecation Timeline

| Phase | Version | Description |
|-------|---------|-------------|
| Deprecation | v2.0.0 | APIs marked deprecated with compiler warnings |
| Grace Period | v2.x | Deprecated APIs remain functional |
| Removal | v3.0.0 | Deprecated APIs removed |

---

## Currently Deprecated APIs

No APIs are currently deprecated.

---

## APIs Removed in v3.0.0

### 1. Legacy Logger Method with File/Line/Function Parameters

**File:** `include/kcenon/common/interfaces/logger_interface.h`

**Removed in:** v3.0.0 (Issue #217)

**Previous Declaration:**
```cpp
[[deprecated("Use log(log_level, std::string_view, const source_location&) instead.")]]
virtual VoidResult log(log_level level,
                       const std::string& message,
                       const std::string& file,
                       int line,
                       const std::string& function) = 0;
```

**Reason for Removal:**
- Replaced by C++20 `source_location`-based API
- Type safety: `source_location` provides compile-time verification
- Automatic capture: Source location is captured at call site without manual parameters
- Cleaner API: Reduced parameter count improves usability

**Replacement API:**
```cpp
virtual VoidResult log(log_level level,
                       std::string_view message,
                       const source_location& loc = source_location::current());
```

**Migration Guide:**

<details>
<summary>Before (Removed)</summary>

```cpp
// Direct call with manual parameters
logger->log(log_level::info, "Operation completed", __FILE__, __LINE__, __func__);

// Custom wrapper function
void my_log(ILogger* logger, log_level level, const std::string& msg) {
    logger->log(level, msg, __FILE__, __LINE__, __func__);
}
```
</details>

<details>
<summary>After (Current)</summary>

```cpp
// Direct call - source_location auto-captured
logger->log(log_level::info, "Operation completed");

// Using convenience functions (preferred)
#include <kcenon/common/logging/log_functions.h>
log_info("Operation completed");

// Using macros (preferred)
#include <kcenon/common/logging/log_macros.h>
LOG_INFO("Operation completed");
```
</details>

**Implementation Migration:**

If you have a custom `ILogger` implementation, update as follows:

<details>
<summary>Before (Removed Implementation)</summary>

```cpp
class MyLogger : public ILogger {
public:
    VoidResult log(log_level level,
                   const std::string& message,
                   const std::string& file,
                   int line,
                   const std::string& function) override {
        // Log with file/line/function
        std::cout << file << ":" << line << " [" << function << "] " << message;
        return VoidResult::ok();
    }
};
```
</details>

<details>
<summary>After (Current Implementation)</summary>

```cpp
class MyLogger : public ILogger {
public:
    // Implement the source_location-based method
    VoidResult log(log_level level,
                   std::string_view message,
                   const source_location& loc = source_location::current()) override {
        std::cout << loc.file_name() << ":" << loc.line()
                  << " [" << loc.function_name() << "] " << message;
        return VoidResult::ok();
    }
};
```
</details>

---

## APIs Removed in v2.0.0

The following APIs were deprecated in earlier versions and have been removed in v2.0.0:

### 1. Result::is_uninitialized()

**Removed in:** v2.0.0

**Reason:** The default constructor now initializes Result to an error state, making the "uninitialized" concept obsolete.

**Migration:**
```cpp
// Before
if (result.is_uninitialized()) { ... }

// After - Check for specific error state
if (!result.is_ok()) { ... }
```

### 2. THREAD_LOG_* Macros (Re-added as Deprecated)

**Status:** Re-added in v2.x for backward compatibility, deprecated, removal planned for v3.0.0

**Note:** These macros were removed in v2.0.0 but have been re-added to ease migration.
They now redirect to the standard LOG_* macros but are marked as deprecated.

**Migration:**
```cpp
// Before (deprecated)
THREAD_LOG_INFO("Message");

// After (recommended)
LOG_INFO("Message");
```

---

## Compiler Warning Suppression

If you need to use deprecated APIs during migration, you can suppress warnings:

### GCC/Clang
```cpp
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
// deprecated API usage
#pragma GCC diagnostic pop
```

### MSVC
```cpp
#pragma warning(push)
#pragma warning(disable: 4996)
// deprecated API usage
#pragma warning(pop)
```

---

## Enabling Deprecation Warnings in CI

To catch deprecated API usage early, enable deprecation warnings in your build:

### CMake
```cmake
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(${TARGET} PRIVATE -Wdeprecated-declarations)
elseif(MSVC)
    target_compile_options(${TARGET} PRIVATE /W4)
endif()
```

### Command Line
```bash
# GCC/Clang
cmake -DCMAKE_CXX_FLAGS="-Wdeprecated-declarations" ..

# Scan for deprecated usage
make 2>&1 | grep -i deprecated
```

---

## Downstream System Migration Status

All dependent systems have been notified and have completed migration to the new APIs:

| System | Repository | Notification Issue | Migration Status |
|--------|------------|-------------------|------------------|
| thread_system | [kcenon/thread_system](https://github.com/kcenon/thread_system) | [#331](https://github.com/kcenon/thread_system/issues/331) | ✅ Completed |
| logger_system | [kcenon/logger_system](https://github.com/kcenon/logger_system) | [#248](https://github.com/kcenon/logger_system/issues/248) | ✅ Completed |
| monitoring_system | [kcenon/monitoring_system](https://github.com/kcenon/monitoring_system) | [#269](https://github.com/kcenon/monitoring_system/issues/269) | ✅ Completed |
| pacs_system | [kcenon/pacs_system](https://github.com/kcenon/pacs_system) | [#399](https://github.com/kcenon/pacs_system/issues/399) | ✅ Completed |
| database_system | [kcenon/database_system](https://github.com/kcenon/database_system) | [#276](https://github.com/kcenon/database_system/issues/276) | ✅ Completed |

For tracking the overall deprecation plan, see [#213](https://github.com/kcenon/common_system/issues/213).

---

## Related Documentation

- [CHANGELOG](CHANGELOG.md) - Version history with deprecation notices
- [API Reference](API_REFERENCE.md) - Complete API documentation
- [Migration Guide](advanced/MIGRATION.md) - General migration guidance
- [Logging Best Practices](guides/LOGGING_BEST_PRACTICES.md) - Recommended logging patterns

---

## Questions?

If you have questions about migrating from deprecated APIs, please:
1. Check the related documentation above
2. Search existing [GitHub Issues](https://github.com/kcenon/common_system/issues)
3. Open a new issue with the `question` label
