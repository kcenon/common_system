# Runtime Binding Architecture

## Overview

The Runtime Binding Pattern is a fundamental architectural approach in common_system that decouples interface definitions from their implementations at runtime. This pattern enables:

- **Dependency Inversion**: High-level modules depend on abstractions, not concrete implementations
- **Late Binding**: Implementation selection occurs at runtime rather than compile time
- **Testability**: Easy substitution of mock implementations for unit testing
- **Flexibility**: Different implementations can be used in different contexts

## Motivation

### The Circular Dependency Problem

In traditional architectures, `thread_system` and `logger_system` often have circular dependencies:

```
thread_system ──depends on──> logger_system (for logging thread operations)
logger_system ──depends on──> thread_system (for async logging with thread pools)
```

This creates compilation issues and tight coupling between modules.

### The Solution: Runtime Binding

By moving the logging interface to `common_system` and using runtime binding through `GlobalLoggerRegistry`, we break this cycle:

```
common_system (interfaces only, Tier 0)
       ↑
       │ depends on
       │
┌──────┴──────────────┐
│                     │
thread_system    logger_system
(Tier 1)         (Tier 1)
```

## Tier Architecture

The system uses a tiered dependency model:

### Tier 0: common_system (Foundation)

**Contains:**
- Core interfaces (`ILogger`, `ILoggerRegistry`, `IExecutor`, etc.)
- `GlobalLoggerRegistry` singleton
- `SystemBootstrapper` for application initialization
- Logging macros and utility functions
- Common patterns (`Result`, `EventBus`, etc.)

**Dependencies:** None (only C++ standard library)

### Tier 1: Core Systems

Systems that directly depend on common_system:

| System | Purpose |
|--------|---------|
| `thread_system` | Thread pools, job queues, async execution |
| `logger_system` | Logging backends (console, file, rotating) |
| `container_system` | High-performance containers |

### Tier 2+: Higher-Level Systems

Systems that depend on Tier 1:

| System | Dependencies |
|--------|--------------|
| `monitoring_system` | common, thread, logger |
| `network_system` | common, thread, logger |
| `database_system` | common, thread, logger |
| `messaging_system` | common, thread, logger, network |

## Key Components

### GlobalLoggerRegistry

A thread-safe singleton that manages logger instances across all subsystems.

```
┌─────────────────────────────────────────────────────────────┐
│                    GlobalLoggerRegistry                      │
├─────────────────────────────────────────────────────────────┤
│  ┌─────────────┐   ┌─────────────┐   ┌─────────────┐        │
│  │   default   │   │  "network"  │   │ "database"  │  ...   │
│  │   logger    │   │   logger    │   │   logger    │        │
│  └─────────────┘   └─────────────┘   └─────────────┘        │
├─────────────────────────────────────────────────────────────┤
│  Thread Safety: std::shared_mutex (read/write locking)      │
│  Fallback: NullLogger (safe no-op when unregistered)        │
│  Factory Support: Lazy initialization via factory functions │
└─────────────────────────────────────────────────────────────┘
```

**Key Features:**

1. **Thread-Safe Access**: Uses `std::shared_mutex` for concurrent read access with exclusive write access
2. **NullLogger Fallback**: Returns a safe no-op logger when no logger is registered
3. **Factory Support**: Supports lazy initialization through factory functions
4. **Named Loggers**: Supports multiple named loggers for different subsystems

**Usage:**

```cpp
#include <kcenon/common/interfaces/global_logger_registry.h>

using namespace kcenon::common::interfaces;

// Get the global registry
auto& registry = GlobalLoggerRegistry::instance();

// Register a default logger
registry.set_default_logger(my_logger);

// Register a named logger
registry.register_logger("network", network_logger);

// Retrieve loggers
auto logger = registry.get_default_logger();
auto network_log = registry.get_logger("network");
```

### SystemBootstrapper

A fluent API for application initialization that integrates with GlobalLoggerRegistry.

```
┌─────────────────────────────────────────────────────────────┐
│                    SystemBootstrapper                        │
├─────────────────────────────────────────────────────────────┤
│  Configuration Phase (fluent API):                          │
│  ┌───────────────┐  ┌───────────────┐  ┌─────────────────┐  │
│  │with_default_  │  │ with_logger() │  │ on_initialize() │  │
│  │logger()       │  │               │  │ on_shutdown()   │  │
│  └───────────────┘  └───────────────┘  └─────────────────┘  │
├─────────────────────────────────────────────────────────────┤
│  Lifecycle:                                                  │
│  initialize() ──────────────────────> shutdown()             │
│       │                                     │                │
│       ▼                                     ▼                │
│  1. Create loggers                    1. Run shutdown hooks  │
│  2. Register with GlobalLoggerRegistry 2. Clear registry     │
│  3. Run init callbacks               3. Reset state          │
├─────────────────────────────────────────────────────────────┤
│  RAII: Destructor calls shutdown() automatically            │
└─────────────────────────────────────────────────────────────┘
```

**Key Features:**

1. **Fluent API**: Method chaining for expressive configuration
2. **Factory-Based**: Supports lazy logger creation via factory functions
3. **Lifecycle Hooks**: `on_initialize()` and `on_shutdown()` callbacks
4. **RAII Support**: Automatic cleanup on destruction

**Usage:**

```cpp
#include <kcenon/common/bootstrap/system_bootstrapper.h>

using namespace kcenon::common::bootstrap;

int main() {
    SystemBootstrapper bootstrapper;
    bootstrapper
        .with_default_logger([] {
            return std::make_shared<ConsoleLogger>();
        })
        .with_logger("file", [] {
            return std::make_shared<FileLogger>("app.log");
        })
        .on_initialize([] {
            LOG_INFO("Application started");
        })
        .on_shutdown([] {
            LOG_INFO("Application shutting down");
        });

    auto result = bootstrapper.initialize();
    if (result.is_err()) {
        std::cerr << "Initialization failed: "
                  << result.error().message << std::endl;
        return 1;
    }

    // Application logic here...
    // Shutdown is automatic via RAII
    return 0;
}
```

### ILogger Interface

The standard logging interface that all logger implementations must follow.

```cpp
class ILogger {
public:
    virtual ~ILogger() = default;

    // Basic logging
    virtual VoidResult log(log_level level, const std::string& message) = 0;

    // Logging with source location (C++20, preferred)
    virtual VoidResult log(
        log_level level,
        std::string_view message,
        const source_location& loc = source_location::current());

    // Level management
    virtual bool is_enabled(log_level level) const = 0;
    virtual VoidResult set_level(log_level level) = 0;
    virtual log_level get_level() const = 0;

    // Buffer management
    virtual VoidResult flush() = 0;
};
```

### Logging Macros

Convenient macros that integrate with GlobalLoggerRegistry.

```cpp
#include <kcenon/common/logging/log_macros.h>

// Basic logging (uses default logger)
LOG_TRACE("Detailed trace info");
LOG_DEBUG("Debug information");
LOG_INFO("Application event");
LOG_WARNING("Warning condition");
LOG_ERROR("Error occurred");
LOG_CRITICAL("Critical failure");

// Named logger logging
LOG_INFO_TO("network", "Connection established");
LOG_ERROR_TO("database", "Query failed");

// Conditional logging (avoids message construction if disabled)
LOG_IF(log_level::debug, expensive_to_string(data));

// Flush
LOG_FLUSH();
```

## Data Flow

### Logging Flow

```
┌──────────────────────────────────────────────────────────────┐
│  Application Code                                            │
│  LOG_INFO("Message");                                        │
└──────────────────────┬───────────────────────────────────────┘
                       │
                       ▼
┌──────────────────────────────────────────────────────────────┐
│  log_macros.h                                                │
│  Calls log_info(msg) from log_functions.h                    │
└──────────────────────┬───────────────────────────────────────┘
                       │
                       ▼
┌──────────────────────────────────────────────────────────────┐
│  log_functions.h                                             │
│  Gets default logger from GlobalLoggerRegistry               │
│  Calls logger->log(level, message, source_location)          │
└──────────────────────┬───────────────────────────────────────┘
                       │
                       ▼
┌──────────────────────────────────────────────────────────────┐
│  GlobalLoggerRegistry                                        │
│  Returns registered logger or NullLogger                     │
└──────────────────────┬───────────────────────────────────────┘
                       │
                       ▼
┌──────────────────────────────────────────────────────────────┐
│  ILogger Implementation (e.g., ConsoleLogger, FileLogger)    │
│  Formats and outputs the log message                         │
└──────────────────────────────────────────────────────────────┘
```

### Initialization Flow

```
┌─────────────────────────────────────────────────────────────┐
│  main()                                                      │
│  SystemBootstrapper bootstrapper;                            │
│  bootstrapper.with_default_logger(...).initialize();         │
└────────────────────────┬────────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────────┐
│  SystemBootstrapper::initialize()                            │
│  1. Validate not already initialized                         │
│  2. register_loggers()                                       │
│  3. execute_init_callbacks()                                 │
│  4. Set initialized_ = true                                  │
└────────────────────────┬────────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────────┐
│  register_loggers()                                          │
│  1. Call default_logger_factory_()                           │
│  2. registry.set_default_logger(logger)                      │
│  3. For each named factory:                                  │
│     - Call factory()                                         │
│     - registry.register_logger(name, logger)                 │
└────────────────────────┬────────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────────┐
│  GlobalLoggerRegistry                                        │
│  Stores loggers in thread-safe maps                          │
│  Available for use by all subsystems                         │
└─────────────────────────────────────────────────────────────┘
```

## Thread Safety

### GlobalLoggerRegistry

- Uses `std::shared_mutex` for read-write locking
- Multiple readers can access loggers concurrently
- Writers get exclusive access for registration/removal
- Factory invocation is protected against race conditions

### SystemBootstrapper

- Configuration methods are NOT thread-safe (single-thread setup)
- `initialize()` and `shutdown()` are protected by `std::mutex`
- Once initialized, registered loggers are safely accessible globally

### Logging Macros

- All macros are thread-safe
- Delegate to thread-safe GlobalLoggerRegistry methods
- Safe to call from any thread after initialization

## Best Practices

### 1. Initialize Early

```cpp
int main() {
    SystemBootstrapper bootstrapper;
    // Configure and initialize before any other operations
    bootstrapper.with_default_logger(...).initialize();

    // Now safe to use logging throughout the application
    LOG_INFO("Application started");
}
```

### 2. Use Named Loggers for Subsystems

```cpp
bootstrapper
    .with_default_logger(create_console_logger)
    .with_logger("network", create_network_logger)
    .with_logger("database", create_database_logger)
    .with_logger("security", create_security_logger);

// In network code
LOG_INFO_TO("network", "Connection established");

// In database code
LOG_ERROR_TO("database", "Query failed: " + error_msg);
```

### 3. Use Factory Functions for Deferred Initialization

```cpp
// Logger created only when first needed
registry.register_factory("expensive", []() {
    return std::make_shared<ExpensiveLogger>();
});
```

### 4. Leverage RAII for Cleanup

```cpp
{
    SystemBootstrapper bootstrapper;
    bootstrapper.with_default_logger(...).initialize();

    run_application();

}  // Automatic shutdown here via RAII
```

## Migration from Legacy Logging

See [Migration Guide](../guides/MIGRATION_RUNTIME_BINDING.md) for detailed migration instructions.

## Related Documentation

- [Migration Guide](../guides/MIGRATION_RUNTIME_BINDING.md)
- [Logging Best Practices](../guides/LOGGING_BEST_PRACTICES.md)
- [Troubleshooting](../guides/TROUBLESHOOTING_LOGGING.md)
- [API Reference](../API_REFERENCE.md)

## Version History

- **v2.0.0**: Initial runtime binding implementation
  - GlobalLoggerRegistry (Issue #174)
  - Logging macros (Issue #175)
  - SystemBootstrapper (Issue #176)
  - C++20 source_location support (Issue #177)
  - Cross-system integration tests (Issue #178)
