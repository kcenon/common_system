---
doc_id: "COM-FEAT-002b"
doc_title: "Common System - Dependency Injection & Configuration"
doc_version: "1.0.0"
doc_date: "2026-04-04"
doc_status: "Released"
project: "common_system"
category: "FEAT"
---

# Common System - Dependency Injection & Configuration

> This document covers the dependency injection container, bootstrapping utilities, and configuration features of the Common System.
> Split from [FEATURES.md](FEATURES.md) for readability.

---

## Dependency Injection & Bootstrap

The common_system provides a dependency injection (DI) container and bootstrapping utilities for managing service lifetimes, wiring dependencies, and controlling application lifecycle.

### Service Container

The `service_container` (`kcenon::common::di`) is a thread-safe DI container supporting factory-based registration, multiple lifetime policies, scoped containers, and circular dependency detection.

**Service Lifetimes (`service_lifetime`):**

| Lifetime | Behavior | Use Case |
|----------|----------|----------|
| `singleton` | One instance shared globally; created on first resolution. | Loggers, configuration, stateless services. |
| `transient` | New instance created for every resolution request. | Stateful per-consumer services. |
| `scoped` | One instance per `IServiceScope`; shared within that scope. | Request-scoped services, unit-of-work patterns. |

**Registration API:**

```cpp
auto& container = service_container::global();

// Register implementation type (default-constructible)
container.register_type<ILogger, ConsoleLogger>(service_lifetime::singleton);

// Register factory with dependency resolution
container.register_factory<IDatabase>(
    [](IServiceContainer& c) {
        auto logger = c.resolve<ILogger>().value();
        return std::make_shared<PostgresDatabase>(logger);
    },
    service_lifetime::scoped
);

// Register simple factory (no container access needed)
container.register_simple_factory<ICache>(
    [] { return std::make_shared<InMemoryCache>(); },
    service_lifetime::singleton
);

// Register pre-existing instance
auto config = std::make_shared<AppConfig>("config.yaml");
container.register_instance<IConfig>(config);
```

**Resolution API:**

```cpp
// Resolve with error handling
auto result = container.resolve<ILogger>();
if (result.is_ok()) {
    auto logger = result.value();
    logger->info("Resolved successfully");
}

// Resolve or nullptr (for optional services)
auto cache = container.resolve_or_null<ICache>();
if (cache) {
    cache->set("key", "value");
}

// Introspection
bool has_logger = container.is_registered<ILogger>();
auto services = container.registered_services();
```

**Scoped Containers:**

Scopes provide request-level isolation for scoped services while sharing singletons with the parent:

```cpp
auto& container = service_container::global();

void handle_request() {
    auto scope = container.create_scope();

    // Each scope gets its own IDatabase instance
    auto db = scope->resolve<IDatabase>().value();
    auto db2 = scope->resolve<IDatabase>().value();
    // db == db2 (same scoped instance within this scope)

    // Singletons are shared with the parent
    auto logger = scope->resolve<ILogger>().value();
    // Same ILogger instance as parent container

} // Scope destroyed, scoped instances disposed
```

**Security Controls:**

```cpp
// Freeze the container after initialization to prevent tampering
container.freeze();

// Subsequent registration attempts return an error
auto result = container.register_type<IFoo, FooImpl>();
// result.is_err() == true, error code: REGISTRY_FROZEN

bool frozen = container.is_frozen();  // true
```

**Circular Dependency Detection:**

The container uses a thread-local resolution stack to detect circular dependencies at runtime:

```cpp
// If A depends on B, and B depends on A:
auto result = container.resolve<A>();
// result.is_err() == true
// Error: "Circular dependency detected: A -> B -> A"
```

**Thread Safety:**
- All public methods are thread-safe using `std::shared_mutex` (read/write locking).
- Singleton instantiation uses double-checked locking.
- Circular dependency detection uses thread-local storage to avoid false positives.

### System Bootstrapper

The `SystemBootstrapper` (`kcenon::common::bootstrap`) provides a fluent API for application initialization, integrating logger registration with lifecycle management.

**Key Features:**
- Fluent method chaining for expressive configuration
- Factory-based lazy initialization of loggers
- RAII support with automatic shutdown on destruction
- Prevention of duplicate initialization/shutdown
- Integration with `GlobalLoggerRegistry` for thread-safe logger access

**API:**

```cpp
namespace kcenon::common::bootstrap {

class SystemBootstrapper {
public:
    SystemBootstrapper();
    ~SystemBootstrapper();  // Calls shutdown() automatically

    // Fluent configuration
    SystemBootstrapper& with_default_logger(LoggerFactory factory);
    SystemBootstrapper& with_logger(const std::string& name, LoggerFactory factory);
    SystemBootstrapper& on_initialize(std::function<void()> callback);
    SystemBootstrapper& on_shutdown(std::function<void()> callback);
    SystemBootstrapper& with_auto_freeze(
        bool freeze_logger_registry = true,
        bool freeze_service_container = true);

    // Lifecycle
    VoidResult initialize();
    void shutdown();
    bool is_initialized() const noexcept;
    void reset();
};

}
```

**Usage Example:**

```cpp
#include <kcenon/common/bootstrap/system_bootstrapper.h>

using namespace kcenon::common::bootstrap;

int main() {
    SystemBootstrapper bootstrapper;
    bootstrapper
        .with_default_logger([] { return create_console_logger(); })
        .with_logger("database", [] { return create_file_logger("db.log"); })
        .with_auto_freeze()  // Freeze registries after init
        .on_initialize([] { LOG_INFO("System started"); })
        .on_shutdown([] { LOG_INFO("System stopping"); });

    auto result = bootstrapper.initialize();
    if (result.is_err()) {
        std::cerr << "Init failed: " << result.error().message << "\n";
        return 1;
    }

    // Application logic...

    return 0;
    // ~SystemBootstrapper calls shutdown() automatically
}
```

**Initialization Order:**
1. Create and register the default logger (if configured)
2. Create and register all named loggers
3. Execute initialization callbacks in registration order
4. Freeze registries (if `with_auto_freeze()` was called)

**Shutdown Order:**
1. Execute shutdown callbacks in reverse registration order (LIFO)
2. Clear all loggers from `GlobalLoggerRegistry`

### Unified Bootstrapper

The `unified_bootstrapper` (`kcenon::common::di`) is a static utility class that coordinates system-wide initialization and shutdown through the service container. It provides signal handling, shutdown hooks, and conditional subsystem registration.

**Configuration (`bootstrapper_options`):**

```cpp
namespace kcenon::common::di {

struct bootstrapper_options {
    bool enable_logging = true;           // Enable logging services
    bool enable_monitoring = true;        // Enable monitoring services
    bool enable_database = false;         // Enable database services
    bool enable_network = false;          // Enable network services
    std::string config_path;              // Path to config file (optional)
    std::chrono::milliseconds shutdown_timeout{30000};
    bool register_signal_handlers = true; // Handle SIGTERM/SIGINT
};

}
```

**API:**

```cpp
namespace kcenon::common::di {

class unified_bootstrapper {
public:
    // Lifecycle
    static VoidResult initialize(const bootstrapper_options& opts = {});
    static VoidResult shutdown(
        std::chrono::milliseconds timeout = std::chrono::seconds(30));

    // Service access
    static service_container& services();

    // State queries
    static bool is_initialized();
    static bool is_shutdown_requested();

    // Shutdown hooks (called in LIFO order during shutdown)
    static VoidResult register_shutdown_hook(
        const std::string& name, shutdown_hook hook);
    static VoidResult unregister_shutdown_hook(const std::string& name);

    // Signal handling
    static void request_shutdown(bool trigger_shutdown = false);

    // Configuration
    static bootstrapper_options get_options();
};

}
```

**Usage Example:**

```cpp
#include <kcenon/common/di/unified_bootstrapper.h>

using namespace kcenon::common::di;

int main() {
    auto result = unified_bootstrapper::initialize({
        .enable_logging = true,
        .enable_monitoring = true,
        .config_path = "config.yaml",
        .register_signal_handlers = true
    });

    if (result.is_err()) {
        std::cerr << "Init failed: " << result.error().message << "\n";
        return 1;
    }

    // Register custom shutdown hook
    unified_bootstrapper::register_shutdown_hook("flush_cache",
        [](std::chrono::milliseconds remaining) {
            flush_all_caches();
        });

    // Access services
    auto& services = unified_bootstrapper::services();
    auto logger = services.resolve<ILogger>();

    // Main loop - check for shutdown signal
    while (!unified_bootstrapper::is_shutdown_requested()) {
        process_next_request();
    }

    unified_bootstrapper::shutdown();
    return 0;
}
```

**Signal Handling:**
- Automatically registers handlers for `SIGTERM` and `SIGINT` (configurable).
- On signal receipt, sets the shutdown flag (`is_shutdown_requested()` returns `true`).
- Application code should poll `is_shutdown_requested()` for cooperative shutdown.

**Shutdown Hooks:**
- Hooks are executed in reverse registration order (LIFO) during shutdown.
- Each hook receives the remaining timeout duration for budget-aware cleanup.
- Exceptions in hooks are silently caught to ensure all hooks execute.

---

**Last Updated**: 2026-02-08
**Version**: 0.2.0
