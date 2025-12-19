# Migration Guide: Runtime Binding Pattern

This guide provides step-by-step instructions for migrating existing code to use the runtime binding pattern introduced in common_system v2.0.

## Prerequisites

Before starting the migration:

1. **Compiler Requirements**: C++20 compatible compiler
   - GCC 11+
   - Clang 14+
   - MSVC 2019 16.10+
   - Apple Clang 14+

2. **Dependencies**: common_system v2.0.0 or later

3. **Backup**: Ensure your code is committed to version control

## Migration Overview

The migration involves these main steps:

1. Update dependencies in CMake
2. Update include statements
3. Replace direct logger usage with logging macros
4. Update application initialization
5. Handle breaking changes

## Step 1: Update Dependencies

### Before (CMakeLists.txt)

```cmake
# Old approach: direct dependency on logger_system
find_package(logger_system REQUIRED)
target_link_libraries(myapp PRIVATE logger_system)

# Or direct dependency on thread_system's logging
find_package(thread_system REQUIRED)
target_link_libraries(myapp PRIVATE thread_system)
```

### After (CMakeLists.txt)

```cmake
# New approach: depend on common_system for interfaces
find_package(common_system REQUIRED)
target_link_libraries(myapp PUBLIC kcenon::common_system)

# If you need logger implementations, also link logger_system
find_package(logger_system REQUIRED)
target_link_libraries(myapp PRIVATE kcenon::logger_system)
```

### Key Changes

| Old | New |
|-----|-----|
| `logger_system` for interfaces | `common_system` for interfaces |
| `thread_system` logging | `common_system` logging macros |
| Direct logger linking | Separate interface and implementation |

## Step 2: Update Include Statements

### Header Migration Table

> **Important (thread_system v3.0)**: The legacy headers listed below are **removed** in thread_system v3.0. If you are upgrading to thread_system v3.0 or later, you must migrate to the new common_system headers.

| Old Header | New Header | Status |
|------------|------------|--------|
| `<kcenon/logger/logger.h>` | `<kcenon/common/logging/log_macros.h>` | Deprecated |
| `<kcenon/logger/log_level.h>` | `<kcenon/common/interfaces/logger_interface.h>` | Deprecated |
| (new) | `<kcenon/common/interfaces/global_logger_registry.h>` | Current |
| (new) | `<kcenon/common/bootstrap/system_bootstrapper.h>` | Current |

#### Removed in thread_system v3.0

The following headers have been **removed** in thread_system v3.0 and must be migrated:

| Removed Header | Replacement |
|----------------|-------------|
| `<kcenon/thread/interfaces/logger_interface.h>` | `<kcenon/common/interfaces/logger_interface.h>` |

### Example Migration

**Before (pre-v3.0 - these headers are removed in thread_system v3.0):**

```cpp
// WARNING: These headers are REMOVED in thread_system v3.0
// Migrate to common_system headers immediately
#include <kcenon/logger/logger.h>
// REMOVED in thread_system v3.0 - use <kcenon/common/interfaces/logger_interface.h>
// #include <kcenon/thread/interfaces/logger_interface.h>

// Using old logger interface (deprecated)
class MyClass {
    std::shared_ptr<kcenon::thread::interfaces::logger_interface> logger_;
public:
    void do_work() {
        logger_->log(kcenon::logger::log_level::info, "Working...");
    }
};
```

**After:**

```cpp
#include <kcenon/common/logging/log_macros.h>

// Using new logging macros
class MyClass {
public:
    void do_work() {
        LOG_INFO("Working...");
    }
};
```

## Step 3: Replace Logger Usage

### Using Logging Macros

The simplest migration path is to use the new logging macros:

**Before:**

```cpp
// Direct logger usage
auto logger = get_logger();
logger->log(log_level::info, "Processing item: " + std::to_string(id));
logger->log(log_level::error, "Error: " + error_msg);
logger->log(log_level::debug, "Debug data: " + data.to_string());
```

**After:**

```cpp
// Using macros
LOG_INFO("Processing item: " + std::to_string(id));
LOG_ERROR("Error: " + error_msg);
LOG_DEBUG("Debug data: " + data.to_string());
```

### Macro Reference

| Log Level | Macro |
|-----------|-------|
| trace | `LOG_TRACE(msg)` |
| debug | `LOG_DEBUG(msg)` |
| info | `LOG_INFO(msg)` |
| warning | `LOG_WARNING(msg)` |
| error | `LOG_ERROR(msg)` |
| critical | `LOG_CRITICAL(msg)` |

### Named Loggers

For subsystem-specific logging:

**Before:**

```cpp
auto network_logger = get_network_logger();
network_logger->log(log_level::info, "Connection established");
```

**After:**

```cpp
LOG_INFO_TO("network", "Connection established");
```

### Conditional Logging

For expensive message construction:

**Before:**

```cpp
if (logger->is_enabled(log_level::debug)) {
    logger->log(log_level::debug, expensive_to_string(data));
}
```

**After:**

```cpp
LOG_IF(log_level::debug, expensive_to_string(data));
```

## Step 4: Update Application Initialization

### Basic Initialization

**Before:**

```cpp
#include <kcenon/logger/console_logger.h>

int main() {
    auto logger = std::make_shared<kcenon::logger::ConsoleLogger>();
    // Somehow inject logger into components...

    run_application();
    return 0;
}
```

**After:**

```cpp
#include <kcenon/common/bootstrap/system_bootstrapper.h>
#include <kcenon/logger/console_logger.h>

using namespace kcenon::common::bootstrap;

int main() {
    SystemBootstrapper bootstrapper;
    bootstrapper
        .with_default_logger([] {
            return std::make_shared<kcenon::logger::ConsoleLogger>();
        })
        .on_initialize([] {
            LOG_INFO("Application started");
        })
        .on_shutdown([] {
            LOG_INFO("Application stopping");
        });

    auto result = bootstrapper.initialize();
    if (result.is_err()) {
        std::cerr << "Init failed: " << result.error().message << std::endl;
        return 1;
    }

    run_application();

    // Shutdown is automatic via RAII
    return 0;
}
```

### Multiple Loggers

**After:**

```cpp
SystemBootstrapper bootstrapper;
bootstrapper
    .with_default_logger(create_console_logger)
    .with_logger("network", [] {
        return std::make_shared<FileLogger>("network.log");
    })
    .with_logger("database", [] {
        return std::make_shared<FileLogger>("database.log");
    })
    .with_logger("security", [] {
        auto logger = std::make_shared<FileLogger>("security.log");
        logger->set_level(log_level::warning);  // Higher threshold
        return logger;
    });
```

### Manual Registry Usage

If you need more control, use GlobalLoggerRegistry directly:

```cpp
#include <kcenon/common/interfaces/global_logger_registry.h>

using namespace kcenon::common::interfaces;

int main() {
    auto& registry = GlobalLoggerRegistry::instance();

    // Register loggers directly
    auto console = std::make_shared<ConsoleLogger>();
    registry.set_default_logger(console);

    auto file = std::make_shared<FileLogger>("app.log");
    registry.register_logger("file", file);

    // Use factory for lazy initialization
    registry.register_factory("expensive", []() {
        return std::make_shared<ExpensiveLogger>();
    });

    run_application();

    // Manual cleanup
    registry.clear();
    return 0;
}
```

## Step 5: Handle Breaking Changes

### Namespace Changes

> **Note**: Items marked with ⚠️ are **removed** in thread_system v3.0.

| Old Namespace | New Namespace | Status |
|---------------|---------------|--------|
| `kcenon::logger::log_level` | `kcenon::common::interfaces::log_level` | Deprecated |
| `kcenon::thread::interfaces::logger_interface` | `kcenon::common::interfaces::ILogger` | ⚠️ Removed in v3.0 |

### Type Renames

| Old Type | New Type | Status |
|----------|----------|--------|
| `logger_interface` | `ILogger` | ⚠️ Removed in v3.0 |
| `log_level` enum values | Same values, different namespace | Deprecated |

### Removed in thread_system v3.0

The following legacy items are **removed** in thread_system v3.0:

#### Legacy Headers
```cpp
// REMOVED - use common_system headers instead
#include <kcenon/thread/interfaces/logger_interface.h>  // → <kcenon/common/interfaces/logger_interface.h>
```

#### Legacy Types
```cpp
// REMOVED - use ILogger instead
kcenon::thread::interfaces::logger_interface  // → kcenon::common::interfaces::ILogger
```

**Migration action**: Replace all `thread/interfaces/` includes with `common/interfaces/` equivalents before upgrading to thread_system v3.0.

### Removed in v2.0.0

The following deprecated items have been **removed** in v2.0.0:

#### Result Factory Functions
```cpp
// REMOVED - use lowercase versions instead
Ok<int>(42);              // → ok<int>(42)
Err<int>("error");        // → make_error<int>(-1, "error")
```

#### Legacy Logging Macros
```cpp
// REMOVED - use LOG_* macros instead
THREAD_LOG_INFO("message");   // → LOG_INFO("message")
THREAD_LOG_ERROR("message");  // → LOG_ERROR("message")
```

#### Legacy Macro Aliases
```cpp
// REMOVED - use COMMON_* prefixed versions instead
RETURN_IF_ERROR(expr);        // → COMMON_RETURN_IF_ERROR(expr)
ASSIGN_OR_RETURN(decl, expr); // → COMMON_ASSIGN_OR_RETURN(decl, expr)
```

## Step 6: Update Custom Logger Implementations

If you have custom logger implementations:

### Before (pre-v3.0 - this interface is removed in thread_system v3.0):

```cpp
// WARNING: kcenon::thread::interfaces::logger_interface is REMOVED in thread_system v3.0
// Migrate to kcenon::common::interfaces::ILogger immediately
class MyLogger : public kcenon::thread::interfaces::logger_interface {
public:
    void log(kcenon::logger::log_level level,
             const std::string& message) override {
        // Implementation
    }
};
```

### After:

```cpp
#include <kcenon/common/interfaces/logger_interface.h>

using namespace kcenon::common::interfaces;

class MyLogger : public ILogger {
public:
    VoidResult log(log_level level, const std::string& message) override {
        // Implementation
        return VoidResult::ok({});
    }

    // New: source_location-based logging (C++20)
    VoidResult log(log_level level,
                   std::string_view message,
                   const source_location& loc = source_location::current()) override {
        // Implementation with source location
        return VoidResult::ok({});
    }

    // Deprecated: legacy file/line/function logging
    VoidResult log(log_level level,
                   const std::string& message,
                   const std::string& file,
                   int line,
                   const std::string& function) override {
        // Legacy implementation
        return VoidResult::ok({});
    }

    VoidResult log(const log_entry& entry) override {
        // Structured logging
        return VoidResult::ok({});
    }

    bool is_enabled(log_level level) const override {
        return level >= current_level_;
    }

    VoidResult set_level(log_level level) override {
        current_level_ = level;
        return VoidResult::ok({});
    }

    log_level get_level() const override {
        return current_level_;
    }

    VoidResult flush() override {
        // Flush implementation
        return VoidResult::ok({});
    }

private:
    log_level current_level_ = log_level::info;
};
```

## Step 7: Update Tests

### Mocking with NullLogger

```cpp
#include <kcenon/common/interfaces/global_logger_registry.h>
#include <gtest/gtest.h>

class MyTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Use NullLogger for tests (no output, thread-safe)
        auto& registry = GlobalLoggerRegistry::instance();
        registry.set_default_logger(GlobalLoggerRegistry::null_logger());
    }

    void TearDown() override {
        GlobalLoggerRegistry::instance().clear();
    }
};
```

### Testing with Mock Logger

```cpp
class MockLogger : public ILogger {
public:
    std::vector<std::pair<log_level, std::string>> logged_messages;

    VoidResult log(log_level level, const std::string& message) override {
        logged_messages.emplace_back(level, message);
        return VoidResult::ok({});
    }

    // Implement other required methods...
};

TEST_F(MyTest, LogsCorrectMessages) {
    auto mock = std::make_shared<MockLogger>();
    GlobalLoggerRegistry::instance().set_default_logger(mock);

    my_function_that_logs();

    ASSERT_EQ(mock->logged_messages.size(), 1);
    EXPECT_EQ(mock->logged_messages[0].first, log_level::info);
    EXPECT_EQ(mock->logged_messages[0].second, "Expected message");
}
```

## Migration Checklist

Use this checklist to track your migration progress:

### CMake/Build System
- [ ] Update find_package calls to use common_system
- [ ] Update target_link_libraries to link kcenon::common_system
- [ ] Verify build succeeds with new dependencies

### Source Code
- [ ] Update include statements to new headers
- [ ] Replace old logger types with ILogger
- [ ] Replace old log_level namespace
- [ ] Replace direct logging calls with LOG_* macros
- [ ] Update custom logger implementations to implement ILogger

### Initialization
- [ ] Add SystemBootstrapper to application entry point
- [ ] Register default logger using with_default_logger()
- [ ] Add any named loggers using with_logger()
- [ ] Add initialization and shutdown callbacks as needed

### Tests
- [ ] Update test setup to use GlobalLoggerRegistry
- [ ] Replace mock loggers with ILogger implementations
- [ ] Clear registry in test teardown

### Verification
- [ ] Run all tests
- [ ] Verify logging output in development
- [ ] Check for deprecation warnings and address them
- [ ] Test in production-like environment

## Troubleshooting

If you encounter issues during migration, see [Troubleshooting Guide](TROUBLESHOOTING_LOGGING.md).

Common issues:

1. **"No logger registered"**: Call `SystemBootstrapper::initialize()` before logging
2. **Deprecation warnings**: Update to new types/macros as indicated
3. **Link errors**: Ensure kcenon::common_system is linked

## Getting Help

- Check the [FAQ](FAQ.md)
- Review [Best Practices](LOGGING_BEST_PRACTICES.md)
- Open an issue on GitHub for migration-specific problems

## Related Documentation

- [Runtime Binding Architecture](../architecture/RUNTIME_BINDING.md)
- [Logging Best Practices](LOGGING_BEST_PRACTICES.md)
- [Troubleshooting](TROUBLESHOOTING_LOGGING.md)
- [API Reference](../API_REFERENCE.md)
