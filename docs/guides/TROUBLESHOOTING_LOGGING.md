# Troubleshooting Guide

This guide helps diagnose and resolve common issues with the logging system in common_system.

## Quick Diagnostic Checklist

Before diving into specific issues, run through this checklist:

1. [ ] Is `SystemBootstrapper::initialize()` called before logging?
2. [ ] Is a default logger registered?
3. [ ] Is the log level set appropriately?
4. [ ] Is the logger backend configured correctly?
5. [ ] Are you linking against `kcenon::common_system`?

## Common Issues

### "No Logger Registered" / Silent Logging

**Symptoms:**
- Log statements produce no output
- `get_default_logger()` returns NullLogger
- Application works but no logs appear

**Cause:** GlobalLoggerRegistry has no logger registered.

**Solutions:**

**Option 1: Use SystemBootstrapper (Recommended)**
```cpp
#include <kcenon/common/bootstrap/system_bootstrapper.h>
#include <kcenon/logger/console_logger.h>

int main() {
    SystemBootstrapper bootstrapper;
    bootstrapper
        .with_default_logger([] {
            return std::make_shared<ConsoleLogger>();
        })
        .initialize();  // Don't forget this!

    LOG_INFO("Now logging works");
}
```

**Option 2: Register Manually**
```cpp
#include <kcenon/common/interfaces/global_logger_registry.h>

auto& registry = GlobalLoggerRegistry::instance();
auto logger = std::make_shared<ConsoleLogger>();
registry.set_default_logger(logger);

LOG_INFO("Now logging works");
```

**Verification:**
```cpp
if (GlobalLoggerRegistry::instance().has_default_logger()) {
    LOG_INFO("Logger is registered");
} else {
    std::cerr << "No logger registered!" << std::endl;
}
```

---

### Logs Not Appearing (Log Level Too High)

**Symptoms:**
- Some log statements work, others don't
- DEBUG and TRACE logs are missing
- Only ERROR and above appear

**Cause:** Logger's minimum level is set higher than the log statements.

**Solution:**
```cpp
// Check current level
auto logger = GlobalLoggerRegistry::instance().get_default_logger();
std::cout << "Current level: "
          << to_string(logger->get_level()) << std::endl;

// Set to desired level
logger->set_level(log_level::debug);

// Or configure at registration
bootstrapper.with_default_logger([] {
    auto logger = std::make_shared<ConsoleLogger>();
    logger->set_level(log_level::trace);  // Enable all levels
    return logger;
});
```

**Debug Specific Logger:**
```cpp
// For named loggers
auto network_logger = GlobalLoggerRegistry::instance().get_logger("network");
network_logger->set_level(log_level::debug);
```

---

### Named Logger Not Found

**Symptoms:**
- `LOG_INFO_TO("mylogger", ...)` produces no output
- `get_logger("mylogger")` returns NullLogger

**Cause:** Named logger wasn't registered.

**Solution:**
```cpp
// Register the named logger
bootstrapper.with_logger("mylogger", [] {
    return std::make_shared<FileLogger>("mylogger.log");
});

// Or manually
auto& registry = GlobalLoggerRegistry::instance();
auto logger = std::make_shared<FileLogger>("mylogger.log");
registry.register_logger("mylogger", logger);

// Verify registration
if (registry.has_logger("mylogger")) {
    LOG_INFO_TO("mylogger", "Logger is registered");
}
```

**Check Typos:**
```cpp
// Logger names are case-sensitive
LOG_INFO_TO("MyLogger", "...");   // Different from
LOG_INFO_TO("mylogger", "...");   // This
```

---

### Link Errors

**Symptoms:**
```
undefined reference to `kcenon::common::interfaces::GlobalLoggerRegistry::instance()'
undefined reference to `kcenon::common::logging::log_info(...)'
```

**Cause:** Not linking against common_system.

**Solution (CMake):**
```cmake
find_package(common_system REQUIRED)
target_link_libraries(myapp PUBLIC kcenon::common_system)
```

**Verify with:**
```cmake
# Debug: print linked libraries
get_target_property(LINKED_LIBS myapp LINK_LIBRARIES)
message(STATUS "Linked libraries: ${LINKED_LIBS}")
```

---

### Deprecation Warnings

**Symptoms:**
```
warning: 'log_level' is deprecated: Use kcenon::common::interfaces::log_level
warning: 'THREAD_LOG_INFO' is deprecated: Use LOG_INFO
```

**Cause:** Using legacy compatibility aliases.

**Solution:**

Update code to use new types and macros:

| Old | New |
|-----|-----|
| `kcenon::logger::log_level` | `kcenon::common::interfaces::log_level` |
| `THREAD_LOG_INFO(msg)` | `LOG_INFO(msg)` |
| `logger_interface` | `ILogger` |

See [Migration Guide](MIGRATION_RUNTIME_BINDING.md) for complete mapping.

---

### Thread Safety Issues

**Symptoms:**
- Garbled or interleaved log output
- Crashes during concurrent logging
- Missing log messages under high concurrency

**Cause:** Usually related to logger implementation, not the registry.

**Verification:**

The GlobalLoggerRegistry is thread-safe. Check your logger implementation:

```cpp
class MyLogger : public ILogger {
    std::mutex mutex_;  // Need this for thread safety

public:
    VoidResult log(log_level level, const std::string& message) override {
        std::lock_guard<std::mutex> lock(mutex_);  // Protect output
        // ... logging implementation
        return VoidResult::ok({});
    }
};
```

**Use Standard Loggers:**
The loggers in `logger_system` (ConsoleLogger, FileLogger, etc.) are thread-safe. If you have issues, ensure you're using them correctly or check your custom implementation.

---

### Initialization Order Problems

**Symptoms:**
- Logs before `main()` don't appear
- Static initialization logging fails
- Crashes during static initialization

**Cause:** Logging before SystemBootstrapper is initialized.

**Solution:**

Avoid logging during static initialization. If unavoidable:

```cpp
// Option 1: Defer logging
class MyComponent {
    std::string deferred_log_;
public:
    MyComponent() {
        // Don't log in constructor during static init
        deferred_log_ = "MyComponent initialized";
    }

    void late_init() {
        // Call after SystemBootstrapper::initialize()
        LOG_INFO(deferred_log_);
    }
};

// Option 2: Check if logger is available
class SafeLogger {
public:
    static void log(log_level level, const std::string& msg) {
        if (GlobalLoggerRegistry::instance().has_default_logger()) {
            GlobalLoggerRegistry::instance().get_default_logger()
                ->log(level, msg);
        }
        // Silently skip if no logger (during static init)
    }
};
```

---

### File Logger Issues

**Symptoms:**
- File logger not writing
- Log file is empty
- Permission denied errors

**Solutions:**

**Check file path:**
```cpp
// Use absolute path
auto logger = std::make_shared<FileLogger>("/var/log/myapp/app.log");

// Ensure directory exists
std::filesystem::create_directories("/var/log/myapp");
```

**Check permissions:**
```bash
# Verify write permissions
ls -la /var/log/myapp/
touch /var/log/myapp/test.txt
```

**Flush after critical logs:**
```cpp
LOG_ERROR("Critical error occurred");
LOG_FLUSH();  // Ensure it's written before potential crash
```

---

### Duplicate Initialization

**Symptoms:**
```
Error: SystemBootstrapper already initialized
Error: Default logger already registered
```

**Cause:** Calling `initialize()` or `set_default_logger()` multiple times.

**Solution:**

Check initialization state before calling:
```cpp
if (!bootstrapper.is_initialized()) {
    bootstrapper.initialize();
}

// Or for manual registration
auto& registry = GlobalLoggerRegistry::instance();
if (!registry.has_default_logger()) {
    registry.set_default_logger(my_logger);
}
```

**For test isolation:**
```cpp
class MyTest : public ::testing::Test {
protected:
    void SetUp() override {
        GlobalLoggerRegistry::instance().clear();  // Reset first
        GlobalLoggerRegistry::instance().set_default_logger(test_logger);
    }

    void TearDown() override {
        GlobalLoggerRegistry::instance().clear();
    }
};
```

---

### Memory Leaks

**Symptoms:**
- Memory leak reports mentioning loggers
- Loggers not being destroyed at shutdown

**Cause:** GlobalLoggerRegistry holding references after shutdown.

**Solution:**

Clear the registry at shutdown:
```cpp
// Automatic with SystemBootstrapper (RAII)
{
    SystemBootstrapper bootstrapper;
    bootstrapper.initialize();
    // ...
}  // Automatic cleanup here

// Manual cleanup
GlobalLoggerRegistry::instance().clear();
```

---

### Compile-Time Level Stripping Not Working

**Symptoms:**
- `KCENON_MIN_LOG_LEVEL` defined but logs still appearing
- Debug logs in release builds

**Cause:** Header included before defining the macro.

**Solution:**

Define macro before including headers:
```cpp
// MUST be before include
#define KCENON_MIN_LOG_LEVEL 2  // Strip TRACE and DEBUG

#include <kcenon/common/logging/log_macros.h>
```

Or define in CMake:
```cmake
target_compile_definitions(myapp PRIVATE KCENON_MIN_LOG_LEVEL=2)
```

---

## Diagnostic Tools

### Check Registry State

```cpp
#include <kcenon/common/interfaces/global_logger_registry.h>
#include <iostream>

void diagnose_registry() {
    auto& registry = GlobalLoggerRegistry::instance();

    std::cout << "=== Logger Registry Diagnostic ===" << std::endl;
    std::cout << "Has default logger: "
              << (registry.has_default_logger() ? "yes" : "no") << std::endl;
    std::cout << "Registered loggers: " << registry.size() << std::endl;

    if (registry.has_default_logger()) {
        auto logger = registry.get_default_logger();
        std::cout << "Default logger level: "
                  << to_string(logger->get_level()) << std::endl;
    }
}
```

### Test Logger Output

```cpp
void test_logging() {
    LOG_TRACE("TRACE level test");
    LOG_DEBUG("DEBUG level test");
    LOG_INFO("INFO level test");
    LOG_WARNING("WARNING level test");
    LOG_ERROR("ERROR level test");
    LOG_CRITICAL("CRITICAL level test");
    LOG_FLUSH();

    std::cout << "If no output above, logger may not be registered "
              << "or level is too high" << std::endl;
}
```

### Verbose Startup

```cpp
int main() {
    std::cout << "[STARTUP] Initializing logger..." << std::endl;

    SystemBootstrapper bootstrapper;
    bootstrapper.with_default_logger([] {
        std::cout << "[STARTUP] Creating console logger" << std::endl;
        return std::make_shared<ConsoleLogger>();
    });

    auto result = bootstrapper.initialize();
    if (result.is_err()) {
        std::cerr << "[STARTUP] Failed: " << result.error().message << std::endl;
        return 1;
    }

    std::cout << "[STARTUP] Logger initialized successfully" << std::endl;
    LOG_INFO("First log message");

    // ... rest of application
}
```

## Getting Help

If you're still having issues:

1. Check the [FAQ](FAQ.md) for additional solutions
2. Review [Best Practices](LOGGING_BEST_PRACTICES.md)
3. Open an issue on GitHub with:
   - common_system version
   - Compiler and OS
   - Minimal reproduction code
   - Actual vs expected behavior

## Related Documentation

- [Runtime Binding Architecture](../architecture/RUNTIME_BINDING.md)
- [Migration Guide](MIGRATION_RUNTIME_BINDING.md)
- [Logging Best Practices](LOGGING_BEST_PRACTICES.md)
- [API Reference](../API_REFERENCE.md)
