# Cross-System Integration Guide

> **Language:** **English** | [한국어](INTEGRATION_GUIDE.kr.md)

**Part 1 of 3**: Dependency Map and Integration Patterns

**Status**: ⚠️ **Part 1 Complete** | Part 2 Planned | Part 3 Planned

This guide provides a comprehensive overview of how the 7 kcenon ecosystem systems work together. It covers dependency management, initialization sequences, and integration patterns for building applications that leverage multiple systems.

---

## Table of Contents

- [Overview](#overview)
- [Ecosystem Dependency Map](#1-ecosystem-dependency-map)
  - [Visual Dependency Tree](#visual-dependency-tree)
  - [Tier-Based Initialization Order](#tier-based-initialization-order)
  - [Dependency Rationale](#dependency-rationale)
- [Integration Patterns](#2-integration-patterns)
  - [Using Unified Bootstrapper](#using-unified-bootstrapper)
  - [Manual Adapter Wiring](#manual-adapter-wiring)
  - [Service Container Integration](#service-container-integration)
  - [Lifecycle Management](#lifecycle-management)
- [Next Steps](#next-steps)

---

## Overview

The kcenon ecosystem consists of **7 core C++ systems** (plus Rust ports), each providing specialized functionality while maintaining loose coupling through interface-driven design. This guide explains how to integrate multiple systems into a cohesive application.

### The 7 Core Systems

| System | Tier | Description | Key Interfaces |
|--------|------|-------------|----------------|
| **common_system** | 0 | Foundation layer providing interfaces and patterns | `ILogger`, `IExecutor`, `Result<T>` |
| **thread_system** | 1 | Execution and job scheduling | Implements `IExecutor` |
| **container_system** | 1 | Data storage and serialization | Uses `Result<T>` |
| **logger_system** | 2 | Structured logging and diagnostics | Implements `ILogger` |
| **monitoring_system** | 2 | Metrics, health checks, and observability (Rust) | Uses event bus, `IMonitor` |
| **database_system** | 2 | Persistent data storage | Uses `Result<T>`, `IExecutor` |
| **network_system** | 3 | TCP/IP communication and messaging | Uses `IExecutor`, `ILogger` |

### Integration Challenges Addressed

When building applications with multiple systems, developers face:
- **Initialization order complexity**: Which system to initialize first?
- **Dependency resolution**: How to wire services together?
- **Configuration management**: How to configure multiple systems consistently?
- **Lifecycle coordination**: How to gracefully shutdown interconnected systems?

This guide provides battle-tested patterns for solving these challenges.

---

## 1. Ecosystem Dependency Map

### Visual Dependency Tree

The ecosystem follows a strict **tier-based hierarchy** to prevent circular dependencies and ensure clean architecture:

```
Tier 0: Foundation Layer (No Dependencies)
┌─────────────────────────────────────────────────┐
│         common_system                           │
│  • Interfaces: ILogger, IExecutor, IMonitor     │
│  • Patterns: Result<T>, VoidResult              │
│  • Error handling: ErrorInfo, error_codes       │
│  • Adapters: typed_adapter, smart_adapter       │
└─────────────────────────────────────────────────┘
                    ▲
                    │ provides interfaces to
        ┌───────────┴───────────┐
        │                       │
Tier 1: Core Systems (Depend on Tier 0 only)
┌─────────────────┐     ┌─────────────────┐
│  thread_system  │     │container_system │
│                 │     │                 │
│ • Implements    │     │ • Uses Result<T>│
│   IExecutor     │     │ • JSON/Binary   │
│ • Job queuing   │     │   serialization │
└─────────────────┘     └─────────────────┘
        ▲                       ▲
        │                       │
        └───────────┬───────────┘
                    │ build upon
        ┌───────────┴───────────┬───────────────┐
        │                       │               │
Tier 2: Service Systems (Depend on Tier 0-1)
┌─────────────────┐ ┌─────────────────┐ ┌─────────────────┐
│  logger_system  │ │monitoring_system│ │ database_system │
│                 │ │     (Rust)      │ │                 │
│ • Implements    │ │ • Event bus     │ │ • Uses thread_  │
│   ILogger       │ │ • Metrics       │ │   system for    │
│ • Async logging │ │ • Health checks │ │   async queries │
│ • Thread-safe   │ │ • Circuit       │ │ • Result<T> for │
│   sinks         │ │   breakers      │ │   error handling│
└─────────────────┘ └─────────────────┘ └─────────────────┘
                    ▲
                    │ build upon
                    │
Tier 3: Integration Systems (Depend on Tier 0-2)
        ┌───────────────────────┐
        │   network_system      │
        │                       │
        │ • TCP/IP communication│
        │ • Uses IExecutor from │
        │   thread_system       │
        │ • Uses ILogger from   │
        │   logger_system       │
        └───────────────────────┘
```

### Tier-Based Initialization Order

**Rule**: Initialize systems in **ascending tier order**, respecting dependencies within each tier.

#### Phase 1: Foundation (Tier 0)
```
1. common_system (header-only, no initialization needed)
```

#### Phase 2: Core Systems (Tier 1)
```
2. thread_system    }
3. container_system } → Can initialize in parallel (no dependencies between them)
```

#### Phase 3: Service Systems (Tier 2)
```
4. logger_system      }
5. monitoring_system  } → Can initialize in parallel after thread_system is ready
6. database_system    }
```

#### Phase 4: Integration Systems (Tier 3)
```
7. network_system (requires thread_system + logger_system)
```

**Shutdown Order**: **Reverse** of initialization (Tier 3 → Tier 2 → Tier 1).

#### Rationale for Reverse Shutdown

Shutting down in reverse dependency order ensures:
1. **Active connections close first**: network_system shuts down before thread_system
2. **Logging remains available**: logger_system stays alive until higher tiers finish cleanup
3. **No dangling references**: Higher-tier systems don't access deallocated lower-tier resources

### Dependency Rationale

#### Why This Hierarchy?

| Dependency | Reason | Alternative Considered |
|------------|--------|------------------------|
| thread_system → common_system | Needs `IExecutor` interface | ❌ Embed executor directly → Tight coupling |
| logger_system → thread_system | Async logging requires job queue | ❌ Sync logging only → Performance penalty |
| database_system → thread_system | Async queries without blocking | ❌ Blocking queries → Poor scalability |
| network_system → logger_system | Network events need logging | ❌ Custom logging → Duplicate effort |
| monitoring_system → event bus | Decoupled metric collection | ❌ Direct coupling → Hard to extend |

#### Circular Dependency Mitigation

**Potential Cycle**: common_system ↔ monitoring_system

**Solution**: Conditional compilation in `event_bus.h`:
```cpp
#if defined(ENABLE_MONITORING_INTEGRATION)
    #include <kcenon/monitoring/core/event_bus.h>
    using event_bus = monitoring_system::event_bus;
#else
    // Provide stub implementation
    class null_event_bus { /* no-op */ };
    using event_bus = null_event_bus;
#endif
```

**Result**: common_system can be built **without** monitoring_system as a hard dependency.

---

## 2. Integration Patterns

### Using Unified Bootstrapper

The **unified_bootstrapper** provides a high-level API for initializing and shutting down the entire ecosystem with minimal boilerplate.

#### Basic Example

```cpp
#include <kcenon/common/di/unified_bootstrapper.h>
#include <iostream>

using namespace kcenon::common::di;

int main() {
    // Configure which systems to enable
    bootstrapper_options opts;
    opts.enable_logging = true;
    opts.enable_monitoring = true;
    opts.enable_database = false;  // Not needed for this app
    opts.enable_network = false;
    opts.config_path = "config.yaml";
    opts.shutdown_timeout = std::chrono::seconds(30);

    // Initialize all enabled systems
    auto result = unified_bootstrapper::initialize(opts);
    if (result.is_err()) {
        std::cerr << "Initialization failed: "
                  << result.error().message << "\n";
        return 1;
    }

    // Get the service container
    auto& services = unified_bootstrapper::services();

    // Resolve services
    auto logger = services.resolve<interfaces::ILogger>();
    logger->log(log_level::info, "Application started");

    // Application logic...

    // Graceful shutdown (signal handlers will also trigger this)
    unified_bootstrapper::shutdown();
    return 0;
}
```

#### Initialization Sequence

The bootstrapper follows this sequence internally:

```
1. Parse configuration file (if provided)
2. Register core services (always enabled)
   → service_container
   → ErrorInfo factory
3. Register optional services based on options
   → logger_system (if enable_logging = true)
   → monitoring_system (if enable_monitoring = true)
   → database_system (if enable_database = true)
   → network_system (if enable_network = true)
4. Resolve inter-service dependencies
   → Wire logger to thread pool
   → Wire database to executor
5. Register signal handlers (SIGTERM, SIGINT)
6. Call initialization hooks
7. Return VoidResult (ok or error)
```

#### Advanced: Shutdown Hooks

Register custom cleanup logic to run during shutdown:

```cpp
unified_bootstrapper::register_shutdown_hook("my_cleanup",
    [](std::chrono::milliseconds remaining_timeout) {
        // Custom cleanup logic
        std::cout << "Cleaning up resources with "
                  << remaining_timeout.count() << "ms remaining\n";

        // Ensure cleanup completes before timeout
        my_resource_manager.close_all();
    });
```

**Hook Execution Order**: Reverse order of registration (LIFO).

#### Error Handling

```cpp
auto result = unified_bootstrapper::initialize(opts);
if (result.is_err()) {
    switch (result.error().code) {
        case error_codes::ALREADY_EXISTS:
            // Already initialized with different options
            std::cerr << "Already initialized\n";
            break;
        case error_codes::INTERNAL_ERROR:
            // Service registration failed
            std::cerr << "Service registration error: "
                      << result.error().message << "\n";
            break;
        default:
            std::cerr << "Unknown error\n";
    }
    return 1;
}
```

### Manual Adapter Wiring

For fine-grained control, you can manually wire systems using **adapters** and the **service container**.

#### Pattern 1: Interface Adapter

**Use Case**: Adapt a concrete implementation to an interface.

```cpp
#include <kcenon/common/adapters/adapter.h>
#include <kcenon/common/di/service_container.h>

using namespace kcenon::common;

// Custom logger implementation
class MyCustomLogger : public interfaces::ILogger {
public:
    void log(log_level level, const std::string& message) override {
        // Custom logging logic
    }
};

// Manual wiring
di::service_container container;

// Register custom logger
auto logger_impl = std::make_shared<MyCustomLogger>();
auto logger_adapter = adapters::interface_adapter<interfaces::ILogger, MyCustomLogger>(logger_impl);

container.register_instance<interfaces::ILogger>(
    std::make_shared<decltype(logger_adapter)>(logger_adapter)
);

// Resolve anywhere in the app
auto logger = container.resolve<interfaces::ILogger>();
logger->log(log_level::info, "Using manual adapter");
```

#### Pattern 2: Cross-System Adapter

**Use Case**: Connect two systems with different interfaces.

```cpp
// Example: Adapt database executor to thread pool
#include <kcenon/thread/core/thread_pool.h>
#include <kcenon/database/core/async_executor.h>

class DatabaseExecutorAdapter : public interfaces::IExecutor {
    std::shared_ptr<database::async_executor> db_exec_;

public:
    explicit DatabaseExecutorAdapter(std::shared_ptr<database::async_executor> exec)
        : db_exec_(exec) {}

    void submit(std::function<void()> task) override {
        db_exec_->enqueue_query([task]() {
            task();
            return database::query_result{};  // Adapt signature
        });
    }
};

// Register adapted executor
auto db_exec = std::make_shared<database::async_executor>(pool_size);
auto adapted = std::make_shared<DatabaseExecutorAdapter>(db_exec);
container.register_instance<interfaces::IExecutor>(adapted);
```

#### Pattern 3: Smart Adapter (Deprecated)

**Legacy Code**: `typed_adapter` and `smart_adapter` are **deprecated**.

**Migration Path**:
```cpp
// Old (deprecated)
auto old_adapter = typed_adapter<ILogger, MyLogger>(impl);

// New (recommended)
auto new_adapter = interface_adapter<ILogger, MyLogger>(impl);
```

**Reason for Deprecation**: `interface_adapter` provides better type safety and eliminates RTTI dependency.

### Service Container Integration

The **service_container** is the central DI container used by all systems.

#### Lifecycle

```cpp
#include <kcenon/common/di/service_container.h>

using namespace kcenon::common::di;

// 1. Create container
service_container container;

// 2. Register services
//    - Singleton: One instance shared globally
container.register_singleton<ILogger>([]() {
    return std::make_shared<ConsoleLogger>();
});

//    - Transient: New instance per resolve
container.register_transient<IExecutor>([]() {
    return std::make_shared<ThreadPool>(4);
});

//    - Instance: Pre-existing instance
auto existing_logger = std::make_shared<FileLogger>("app.log");
container.register_instance<ILogger>(existing_logger);

// 3. Resolve services
auto logger = container.resolve<ILogger>();
auto executor = container.resolve<IExecutor>();

// 4. Clear all registrations (during shutdown)
container.clear();
```

#### Dependency Injection

**Constructor Injection** (recommended):

```cpp
class MyService {
    std::shared_ptr<interfaces::ILogger> logger_;
    std::shared_ptr<interfaces::IExecutor> executor_;

public:
    MyService(std::shared_ptr<interfaces::ILogger> logger,
              std::shared_ptr<interfaces::IExecutor> executor)
        : logger_(logger), executor_(executor) {}

    void do_work() {
        logger_->log(log_level::info, "Starting work");
        executor_->submit([this]() {
            // Async work
        });
    }
};

// Register with factory function
container.register_singleton<MyService>([&container]() {
    return std::make_shared<MyService>(
        container.resolve<interfaces::ILogger>(),
        container.resolve<interfaces::IExecutor>()
    );
});
```

**Property Injection** (fallback):

```cpp
class LegacyService {
public:
    void set_logger(std::shared_ptr<interfaces::ILogger> logger) {
        logger_ = logger;
    }

private:
    std::shared_ptr<interfaces::ILogger> logger_;
};

// Wire after construction
auto service = std::make_shared<LegacyService>();
service->set_logger(container.resolve<interfaces::ILogger>());
```

### Lifecycle Management

#### Application Lifecycle Stages

```
┌─────────────────────────────────────────────────┐
│ Stage 1: Configuration Loading                 │
│  → Parse YAML/JSON config                      │
│  → Validate settings                           │
│  → Set environment variables                   │
└─────────────────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────────────┐
│ Stage 2: Bootstrapping                         │
│  → unified_bootstrapper::initialize()          │
│  → Service registration                        │
│  → Dependency resolution                       │
└─────────────────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────────────┐
│ Stage 3: Running                               │
│  → Application logic executes                  │
│  → Services interact via interfaces            │
│  → Event bus handles cross-system messages     │
└─────────────────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────────────┐
│ Stage 4: Graceful Shutdown                     │
│  → Signal received (SIGTERM/SIGINT)            │
│  → unified_bootstrapper::shutdown()            │
│  → Shutdown hooks called (reverse order)       │
│  → Services cleared from container             │
└─────────────────────────────────────────────────┘
```

#### Complete Lifecycle Example

```cpp
#include <kcenon/common/di/unified_bootstrapper.h>
#include <kcenon/common/config/config_reader.h>
#include <csignal>
#include <iostream>

using namespace kcenon::common;

int main(int argc, char** argv) {
    // Stage 1: Load Configuration
    auto config_result = config::read_yaml("config.yaml");
    if (config_result.is_err()) {
        std::cerr << "Config error: " << config_result.error().message << "\n";
        return 1;
    }
    auto cfg = config_result.value();

    // Stage 2: Bootstrap
    di::bootstrapper_options opts;
    opts.enable_logging = cfg.get_bool("logging.enabled", true);
    opts.enable_monitoring = cfg.get_bool("monitoring.enabled", true);
    opts.enable_database = cfg.get_bool("database.enabled", false);
    opts.enable_network = cfg.get_bool("network.enabled", false);
    opts.shutdown_timeout = std::chrono::milliseconds(
        cfg.get_int("shutdown_timeout_ms", 30000)
    );

    auto init_result = di::unified_bootstrapper::initialize(opts);
    if (init_result.is_err()) {
        std::cerr << "Init error: " << init_result.error().message << "\n";
        return 1;
    }

    // Register custom shutdown hook
    di::unified_bootstrapper::register_shutdown_hook("app_cleanup",
        [](std::chrono::milliseconds timeout) {
            std::cout << "App shutting down with "
                      << timeout.count() << "ms timeout\n";
            // Custom cleanup logic here
        });

    // Stage 3: Run Application
    auto& services = di::unified_bootstrapper::services();
    auto logger = services.resolve<interfaces::ILogger>();
    logger->log(log_level::info, "Application started");

    // Application logic...
    std::cout << "Press Ctrl+C to shutdown\n";

    // Wait for signal (signal handlers are auto-registered by bootstrapper)
    while (!di::unified_bootstrapper::is_shutdown_requested()) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    // Stage 4: Graceful Shutdown
    // (signal handler already triggered shutdown, but we can call it explicitly too)
    auto shutdown_result = di::unified_bootstrapper::shutdown(
        std::chrono::seconds(30)
    );

    if (shutdown_result.is_err()) {
        std::cerr << "Shutdown error: " << shutdown_result.error().message << "\n";
        return 1;
    }

    logger->log(log_level::info, "Application stopped");
    return 0;
}
```

---

## Next Steps

This completes **Part 1** of the integration guide. The following parts will cover:

### Part 2: Integration Scenarios and Configuration (Planned)
- Common integration scenarios:
  - Web API Server (common + thread + logger + network)
  - Data Pipeline (common + thread + database + container)
  - Monitoring Stack (common + thread + monitoring + logger)
  - Full Stack (all 7 systems)
- Configuration management across systems
- Shared settings and system-specific overrides

### Part 3: Error Handling, Lifecycle, and Complete Example (Planned)
- Error code ranges by system
- Cross-system error propagation
- Complete 100-line multi-system example
- CMakeLists.txt setup for multi-system projects

---

## Related Documentation

- [Quick Start Guide](guides/QUICK_START.md) - Getting started with a single system
- [Simple Integration Examples](guides/INTEGRATION.md) - Basic 2-3 system integrations
- [Architecture Overview](ARCHITECTURE.md) - System design philosophy
- [Dependency Matrix](advanced/DEPENDENCY_MATRIX.md) - Detailed dependency analysis
- [API Reference](API_REFERENCE.md) - Interface specifications

---

**Document Version**: 1.0.0 (Part 1)
**Last Updated**: 2026-02-08
**Authors**: kcenon team
**Related Issues**: kcenon/common_system#329, #334
