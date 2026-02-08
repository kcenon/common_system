# Cross-System Integration Guide

> **Language:** **English** | [한국어](INTEGRATION_GUIDE.kr.md)

**Parts 1-2 of 3**: Dependency Map, Integration Patterns, and Scenarios

**Status**: ✅ **Parts 1-2 Complete** | Part 3 Planned

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
- [Common Integration Scenarios](#3-common-integration-scenarios)
  - [Scenario 1: Web API Server](#scenario-1-web-api-server)
  - [Scenario 2: Data Pipeline](#scenario-2-data-pipeline)
  - [Scenario 3: Monitoring Stack](#scenario-3-monitoring-stack)
  - [Scenario 4: Full Stack Application](#scenario-4-full-stack-application)
- [Configuration Across Systems](#4-configuration-across-systems)
  - [Unified Configuration Strategy](#unified-configuration-strategy)
  - [Configuration Structure](#configuration-structure)
  - [Configuration Examples by Scenario](#configuration-examples-by-scenario)
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

## 3. Common Integration Scenarios

This section demonstrates real-world integration patterns using multiple kcenon systems together.

### Scenario 1: Web API Server

**Systems Used**: common_system + thread_system + logger_system + network_system

**Use Case**: HTTP REST API server with async request handling and structured logging.

#### Required Systems

| System | Role | Key Features Used |
|--------|------|-------------------|
| common_system | Foundation | Result<T>, ILogger, IExecutor interfaces |
| thread_system | Request handling | Thread pool for async request processing |
| logger_system | Observability | Structured logging, async file sinks |
| network_system | Communication | HTTP server, routing, middleware |

#### Code Example

```cpp
#include <kcenon/common/di/unified_bootstrapper.h>
#include <kcenon/network/http/http_server.h>
#include <kcenon/thread/core/thread_pool.h>
#include <kcenon/logger/core/logger.h>

using namespace kcenon;

int main() {
    // Initialize systems
    common::di::bootstrapper_options opts;
    opts.enable_logging = true;
    opts.enable_network = true;
    opts.config_path = "api_server.yaml";

    auto init_result = common::di::unified_bootstrapper::initialize(opts);
    if (init_result.is_err()) {
        std::cerr << "Initialization failed\n";
        return 1;
    }

    auto& services = common::di::unified_bootstrapper::services();
    auto logger = services.resolve<common::interfaces::ILogger>();
    auto executor = services.resolve<common::interfaces::IExecutor>();

    // Create HTTP server
    network::http::server_config config;
    config.port = 8080;
    config.thread_pool_size = 8;

    network::http::http_server server(config, logger, executor);

    // Register routes
    server.route("GET", "/api/users", [logger](const auto& req) {
        logger->log(common::log_level::info, "GET /api/users");

        // Async database query would go here
        return network::http::response{
            .status = 200,
            .body = R"({"users": [{"id": 1, "name": "Alice"}]})",
            .content_type = "application/json"
        };
    });

    server.route("POST", "/api/users", [logger, executor](const auto& req) {
        logger->log(common::log_level::info, "POST /api/users");

        // Async processing in thread pool
        executor->submit([logger, body = req.body]() {
            // Process user creation
            logger->log(common::log_level::debug, "Processing user creation");
        });

        return network::http::response{
            .status = 201,
            .body = R"({"id": 2, "name": "Bob"})",
            .content_type = "application/json"
        };
    });

    // Start server
    logger->log(common::log_level::info, "Server starting on port 8080");
    server.start();

    // Wait for shutdown signal
    while (!common::di::unified_bootstrapper::is_shutdown_requested()) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    server.stop();
    common::di::unified_bootstrapper::shutdown();
    return 0;
}
```

#### Configuration

```yaml
# api_server.yaml
common:
  log_level: info

logging:
  enabled: true
  async: true
  sinks:
    - type: console
      level: info
    - type: file
      path: logs/api_server.log
      level: debug
      rotation:
        max_size_mb: 100
        max_files: 10

thread:
  pool_size: 8
  queue_capacity: 1000

network:
  http:
    port: 8080
    max_connections: 1000
    request_timeout_sec: 30
    keep_alive: true
```

#### Common Pitfalls

| Pitfall | Solution |
|---------|----------|
| **Blocking I/O in request handlers** | Use executor->submit() for long operations |
| **Shared state without locking** | Use std::mutex or atomic types |
| **Memory leaks in lambda captures** | Use weak_ptr for cyclic references |
| **Unbounded request queues** | Set queue_capacity in thread pool config |

---

### Scenario 2: Data Pipeline

**Systems Used**: common_system + thread_system + database_system + container_system

**Use Case**: ETL pipeline processing data from database with serialization.

#### Required Systems

| System | Role | Key Features Used |
|--------|------|-------------------|
| common_system | Foundation | Result<T> for error handling |
| thread_system | Parallelism | Thread pool for batch processing |
| database_system | Data source | Async queries, connection pooling |
| container_system | Serialization | JSON/binary serialization, typed containers |

#### Code Example

```cpp
#include <kcenon/common/di/unified_bootstrapper.h>
#include <kcenon/database/core/database.h>
#include <kcenon/thread/core/thread_pool.h>
#include <kcenon/container/serialization/json_serializer.h>

using namespace kcenon;

struct UserRecord {
    int id;
    std::string name;
    std::string email;
};

int main() {
    // Initialize systems
    common::di::bootstrapper_options opts;
    opts.enable_database = true;
    opts.config_path = "pipeline.yaml";

    auto init_result = common::di::unified_bootstrapper::initialize(opts);
    if (init_result.is_err()) {
        std::cerr << "Init failed: " << init_result.error().message << "\n";
        return 1;
    }

    auto& services = common::di::unified_bootstrapper::services();
    auto executor = services.resolve<common::interfaces::IExecutor>();

    // Setup database connection
    database::connection_config db_config;
    db_config.type = database::db_type::sqlite;
    db_config.path = "data.db";
    db_config.pool_size = 4;

    auto db_result = database::database::connect(db_config);
    if (db_result.is_err()) {
        std::cerr << "DB connection failed\n";
        return 1;
    }
    auto db = db_result.value();

    // ETL Pipeline: Extract -> Transform -> Load

    // 1. Extract: Query data in batches
    std::string query = "SELECT id, name, email FROM users LIMIT 1000";
    auto rows_result = db->query(query);

    if (rows_result.is_err()) {
        std::cerr << "Query failed\n";
        return 1;
    }
    auto rows = rows_result.value();

    // 2. Transform: Process in parallel batches
    const size_t batch_size = 100;
    std::vector<std::future<std::vector<UserRecord>>> futures;

    for (size_t i = 0; i < rows.size(); i += batch_size) {
        size_t end = std::min(i + batch_size, rows.size());

        auto future = std::async(std::launch::async, [&rows, i, end]() {
            std::vector<UserRecord> batch;
            for (size_t j = i; j < end; ++j) {
                UserRecord record;
                record.id = rows[j].get<int>("id");
                record.name = rows[j].get<std::string>("name");
                record.email = rows[j].get<std::string>("email");

                // Transform: Normalize email to lowercase
                std::transform(record.email.begin(), record.email.end(),
                             record.email.begin(), ::tolower);

                batch.push_back(record);
            }
            return batch;
        });

        futures.push_back(std::move(future));
    }

    // Collect transformed batches
    std::vector<UserRecord> transformed_records;
    for (auto& future : futures) {
        auto batch = future.get();
        transformed_records.insert(transformed_records.end(),
                                  batch.begin(), batch.end());
    }

    // 3. Load: Serialize to JSON and save
    container::json_serializer serializer;
    auto json_result = serializer.serialize(transformed_records);

    if (json_result.is_ok()) {
        std::ofstream outfile("output.json");
        outfile << json_result.value();
        std::cout << "Pipeline completed: " << transformed_records.size()
                  << " records processed\n";
    }

    common::di::unified_bootstrapper::shutdown();
    return 0;
}
```

#### Configuration

```yaml
# pipeline.yaml
common:
  log_level: info

database:
  type: sqlite
  path: data.db
  pool_size: 4
  timeout_sec: 10

thread:
  pool_size: 4

container:
  json:
    pretty_print: true
    indent_size: 2
```

#### Common Pitfalls

| Pitfall | Solution |
|---------|----------|
| **Transaction scope too large** | Commit in batches, not all at once |
| **Memory exhaustion on large datasets** | Stream processing instead of loading all data |
| **Deadlocks with parallel writes** | Use single-threaded writes or row-level locking |
| **Serialization bottleneck** | Parallelize serialization per batch |

---

### Scenario 3: Monitoring Stack

**Systems Used**: common_system + thread_system + monitoring_system + logger_system

**Use Case**: Real-time application monitoring with metrics collection and health checks.

#### Required Systems

| System | Role | Key Features Used |
|--------|------|-------------------|
| common_system | Foundation | Event bus, IMonitor interface |
| thread_system | Background tasks | Scheduled metric collection |
| monitoring_system | Metrics | Prometheus metrics, health checks, circuit breakers |
| logger_system | Alerting | Structured logging for alerts |

#### Code Example

```cpp
#include <kcenon/common/di/unified_bootstrapper.h>
#include <kcenon/monitoring/core/metrics.h>
#include <kcenon/monitoring/health/health_checker.h>
#include <kcenon/thread/core/thread_pool.h>

using namespace kcenon;

int main() {
    // Initialize systems
    common::di::bootstrapper_options opts;
    opts.enable_logging = true;
    opts.enable_monitoring = true;
    opts.config_path = "monitoring.yaml";

    auto init_result = common::di::unified_bootstrapper::initialize(opts);
    if (init_result.is_err()) {
        return 1;
    }

    auto& services = common::di::unified_bootstrapper::services();
    auto logger = services.resolve<common::interfaces::ILogger>();
    auto executor = services.resolve<common::interfaces::IExecutor>();

    // Setup metrics
    monitoring::metrics_registry metrics;

    auto request_counter = metrics.create_counter(
        "http_requests_total",
        "Total HTTP requests",
        {"method", "endpoint", "status"}
    );

    auto request_duration = metrics.create_histogram(
        "http_request_duration_seconds",
        "HTTP request latency",
        {0.001, 0.01, 0.1, 0.5, 1.0, 5.0}  // Buckets
    );

    auto active_connections = metrics.create_gauge(
        "active_connections",
        "Number of active connections"
    );

    // Setup health checks
    monitoring::health_checker health;

    health.register_check("database", []() -> monitoring::health_status {
        // Check database connectivity
        // Return: monitoring::health_status::healthy or ::unhealthy
        return monitoring::health_status::healthy;
    });

    health.register_check("cache", []() -> monitoring::health_status {
        // Check cache availability
        return monitoring::health_status::healthy;
    });

    // Periodic metric collection (every 10 seconds)
    executor->submit_periodic(std::chrono::seconds(10), [&metrics, logger]() {
        // Collect system metrics
        auto cpu_usage = get_cpu_usage();  // Hypothetical function
        auto memory_usage = get_memory_usage();

        metrics.create_gauge("system_cpu_usage", "CPU usage")->set(cpu_usage);
        metrics.create_gauge("system_memory_usage", "Memory usage")->set(memory_usage);

        logger->log(common::log_level::debug,
                   "Metrics collected - CPU: " + std::to_string(cpu_usage) + "%");
    });

    // Simulate application workload
    for (int i = 0; i < 100; ++i) {
        auto start = std::chrono::steady_clock::now();

        // Simulate HTTP request processing
        active_connections->increment();

        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        active_connections->decrement();

        auto duration = std::chrono::duration<double>(
            std::chrono::steady_clock::now() - start
        ).count();

        request_duration->observe(duration);
        request_counter->inc({"GET", "/api/users", "200"});
    }

    // Export metrics to Prometheus format
    std::cout << "=== Metrics Export ===\n";
    std::cout << metrics.export_prometheus() << "\n";

    // Run health checks
    auto health_report = health.check_all();
    std::cout << "=== Health Status ===\n";
    std::cout << "Overall: " << (health_report.is_healthy() ? "Healthy" : "Unhealthy") << "\n";

    common::di::unified_bootstrapper::shutdown();
    return 0;
}
```

#### Configuration

```yaml
# monitoring.yaml
common:
  log_level: info

monitoring:
  enabled: true
  metrics:
    export_port: 9090
    export_path: /metrics
  health:
    check_interval_sec: 30
  circuit_breaker:
    failure_threshold: 5
    timeout_sec: 60

logging:
  enabled: true
  sinks:
    - type: console
      level: info
```

#### Common Pitfalls

| Pitfall | Solution |
|---------|----------|
| **Metric cardinality explosion** | Limit label values, use label guidelines |
| **Health check false positives** | Add grace periods, retry logic |
| **Blocking health checks** | Run checks async with timeouts |
| **Missing metrics on errors** | Always record metrics in finally blocks |

---

### Scenario 4: Full Stack Application

**Systems Used**: All 7 systems (common + thread + container + logger + monitoring + database + network)

**Use Case**: Production-ready web application with full observability and persistence.

#### Required Systems

| System | Role |
|--------|------|
| common_system | Foundation for all interfaces |
| thread_system | Async request handling, background jobs |
| container_system | Configuration parsing, data serialization |
| logger_system | Application logging, audit trails |
| monitoring_system | Metrics, health checks, circuit breakers |
| database_system | User data persistence |
| network_system | HTTP API, WebSocket connections |

#### Code Example (Simplified)

```cpp
#include <kcenon/common/di/unified_bootstrapper.h>
// Include all system headers...

using namespace kcenon;

class FullStackApp {
    std::shared_ptr<common::interfaces::ILogger> logger_;
    std::shared_ptr<network::http::http_server> server_;
    std::shared_ptr<database::database> db_;
    std::shared_ptr<monitoring::metrics_registry> metrics_;

public:
    FullStackApp(/* inject dependencies */) {
        // Constructor injection of all services
    }

    void start() {
        logger_->log(common::log_level::info, "Starting full stack application");

        // Setup routes
        server_->route("GET", "/api/health", [this](const auto& req) {
            return this->handle_health_check(req);
        });

        server_->route("POST", "/api/data", [this](const auto& req) {
            return this->handle_data_submission(req);
        });

        server_->start();
    }

private:
    network::http::response handle_health_check(const auto& req) {
        // Use monitoring_system for health status
        auto status = monitoring::health_checker::check_all();

        metrics_->create_counter("health_checks_total", "")->inc();

        return network::http::response{
            .status = status.is_healthy() ? 200 : 503,
            .body = status.to_json(),
            .content_type = "application/json"
        };
    }

    network::http::response handle_data_submission(const auto& req) {
        // Parse request using container_system
        auto data_result = container::json_serializer::parse(req.body);

        if (data_result.is_err()) {
            logger_->log(common::log_level::warn, "Invalid JSON");
            metrics_->create_counter("invalid_requests_total", "")->inc();
            return network::http::response{.status = 400};
        }

        // Store in database_system (async via thread_system)
        auto insert_result = db_->execute(
            "INSERT INTO data (payload) VALUES (?)",
            {data_result.value()}
        );

        if (insert_result.is_ok()) {
            logger_->log(common::log_level::info, "Data stored successfully");
            metrics_->create_counter("data_stored_total", "")->inc();
            return network::http::response{.status = 201};
        } else {
            logger_->log(common::log_level::error,
                        "Database error: " + insert_result.error().message);
            return network::http::response{.status = 500};
        }
    }
};

int main() {
    // Initialize ALL systems
    common::di::bootstrapper_options opts;
    opts.enable_logging = true;
    opts.enable_monitoring = true;
    opts.enable_database = true;
    opts.enable_network = true;
    opts.config_path = "fullstack.yaml";

    auto init_result = common::di::unified_bootstrapper::initialize(opts);
    if (init_result.is_err()) {
        return 1;
    }

    auto& services = common::di::unified_bootstrapper::services();

    // Resolve all services and wire together
    FullStackApp app(/* pass services */);
    app.start();

    // Wait for shutdown
    while (!common::di::unified_bootstrapper::is_shutdown_requested()) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    common::di::unified_bootstrapper::shutdown();
    return 0;
}
```

#### Configuration

```yaml
# fullstack.yaml
common:
  log_level: info
  shutdown_timeout_ms: 30000

logging:
  enabled: true
  async: true
  sinks:
    - type: console
      level: info
    - type: file
      path: logs/app.log
      level: debug

thread:
  pool_size: 16

database:
  type: sqlite
  path: app.db
  pool_size: 10

network:
  http:
    port: 8080
    max_connections: 1000

monitoring:
  enabled: true
  metrics:
    export_port: 9090
  health:
    check_interval_sec: 30
```

#### Common Pitfalls

| Pitfall | Solution |
|---------|----------|
| **Initialization order errors** | Use unified_bootstrapper for correct ordering |
| **Service lifetime mismatches** | Use shared_ptr for all services, avoid raw pointers |
| **Configuration drift** | Validate config on startup, fail fast |
| **Graceful shutdown failures** | Register shutdown hooks for each system |

---

## 4. Configuration Across Systems

### Unified Configuration Strategy

When integrating multiple kcenon systems, a unified configuration approach prevents duplication and ensures consistency.

#### Design Principles

1. **Single Source of Truth**: One configuration file per deployment environment
2. **Hierarchical Structure**: Common settings at top level, system-specific nested
3. **Environment Overrides**: Support for dev/staging/production variants
4. **Validation on Load**: Fail fast if configuration is invalid
5. **Sensible Defaults**: Minimize required configuration

### Configuration Structure

#### Top-Level Schema

```yaml
# Standard configuration structure for kcenon ecosystem

common:                    # Shared settings across all systems
  log_level: info         # Global log level
  shutdown_timeout_ms: 30000
  environment: production  # dev | staging | production

logging:                   # logger_system configuration
  enabled: true
  async: true
  sinks:
    - type: console
      level: info
    - type: file
      path: logs/app.log
      level: debug
      rotation:
        max_size_mb: 100
        max_files: 10

thread:                    # thread_system configuration
  pool_size: 8             # Number of worker threads
  queue_capacity: 1000     # Max queued tasks
  autoscale:
    enabled: false
    min_threads: 4
    max_threads: 32

database:                  # database_system configuration
  type: sqlite             # sqlite | postgres | mysql
  host: localhost          # For remote databases
  port: 5432
  path: data.db            # For SQLite
  pool_size: 10
  timeout_sec: 10

network:                   # network_system configuration
  http:
    port: 8080
    host: 0.0.0.0
    max_connections: 1000
    request_timeout_sec: 30
    keep_alive: true

monitoring:                # monitoring_system configuration
  enabled: true
  metrics:
    export_port: 9090
    export_path: /metrics
  health:
    check_interval_sec: 30

container:                 # container_system configuration
  json:
    pretty_print: false
    indent_size: 2
  binary:
    compression: true
```

#### Shared Settings

Settings that apply across multiple systems:

| Setting | Systems Affected | Purpose |
|---------|------------------|---------|
| `common.log_level` | All systems | Default log verbosity |
| `common.shutdown_timeout_ms` | All systems | Max time for graceful shutdown |
| `common.environment` | All systems | Deployment environment identifier |
| `thread.pool_size` | thread, database, network | Worker thread count |

#### System-Specific Overrides

Systems can override common settings:

```yaml
common:
  log_level: info  # Default for all systems

logging:
  level: debug     # Override for logger_system only

database:
  log_level: warn  # Override for database_system only
```

### Configuration Examples by Scenario

#### Web API Server Configuration

```yaml
common:
  log_level: info
  environment: production

logging:
  enabled: true
  async: true
  sinks:
    - type: console
      level: info
    - type: file
      path: logs/api.log
      level: debug

thread:
  pool_size: 16            # Handle concurrent requests
  queue_capacity: 5000

network:
  http:
    port: 8080
    max_connections: 2000  # High concurrency
    request_timeout_sec: 30

monitoring:
  enabled: true
  metrics:
    export_port: 9090
```

#### Data Pipeline Configuration

```yaml
common:
  log_level: debug         # Verbose logging for debugging

database:
  type: postgres
  host: db.example.com
  port: 5432
  pool_size: 20            # High parallelism for ETL
  timeout_sec: 60          # Long-running queries

thread:
  pool_size: 32            # Parallel batch processing

container:
  json:
    pretty_print: true     # Human-readable output files
```

#### Monitoring Stack Configuration

```yaml
common:
  log_level: info

logging:
  enabled: true
  sinks:
    - type: console
      level: info

monitoring:
  enabled: true
  metrics:
    export_port: 9090
    export_path: /metrics
  health:
    check_interval_sec: 10  # Frequent checks
  circuit_breaker:
    failure_threshold: 5
    timeout_sec: 60

thread:
  pool_size: 4             # Light workload
```

### Environment Variable Mapping

Override configuration with environment variables:

| Environment Variable | Configuration Path | Example |
|---------------------|-------------------|---------|
| `KCENON_LOG_LEVEL` | `common.log_level` | `export KCENON_LOG_LEVEL=debug` |
| `KCENON_HTTP_PORT` | `network.http.port` | `export KCENON_HTTP_PORT=3000` |
| `KCENON_DB_HOST` | `database.host` | `export KCENON_DB_HOST=prod-db.internal` |
| `KCENON_THREAD_POOL_SIZE` | `thread.pool_size` | `export KCENON_THREAD_POOL_SIZE=32` |

**Priority Order**: Environment Variables > Config File > Defaults

### Configuration Validation

```cpp
#include <kcenon/common/config/config_reader.h>

auto config_result = common::config::read_yaml("app.yaml");
if (config_result.is_err()) {
    std::cerr << "Config error: " << config_result.error().message << "\n";
    return 1;
}

auto cfg = config_result.value();

// Validate required settings
if (!cfg.has("network.http.port")) {
    std::cerr << "Missing required config: network.http.port\n";
    return 1;
}

// Validate value ranges
int port = cfg.get_int("network.http.port");
if (port < 1024 || port > 65535) {
    std::cerr << "Invalid port: " << port << " (must be 1024-65535)\n";
    return 1;
}
```

---

## Next Steps

This completes **Parts 1-2** of the integration guide. The following part will cover:

### Part 3: Error Handling, Lifecycle, and Complete Example (Planned)
- Error code ranges by system (1000-7999 allocation)
- Cross-system error propagation patterns
- Result<T> composition for multi-system operations
- Complete 100-line multi-system example application
- CMakeLists.txt setup for multi-system projects
- Initialization/shutdown sequence with timeout handling
- Korean translation of the complete guide

---

## Related Documentation

- [Quick Start Guide](guides/QUICK_START.md) - Getting started with a single system
- [Simple Integration Examples](guides/INTEGRATION.md) - Basic 2-3 system integrations
- [Architecture Overview](ARCHITECTURE.md) - System design philosophy
- [Dependency Matrix](advanced/DEPENDENCY_MATRIX.md) - Detailed dependency analysis
- [API Reference](API_REFERENCE.md) - Interface specifications
- [Rust/C++ Parity Matrix](RUST_PARITY.md) - Feature comparison between C++ and Rust ports

---

**Document Version**: 2.0.0 (Parts 1-2)
**Last Updated**: 2026-02-08
**Authors**: kcenon team
**Related Issues**: kcenon/common_system#329, #334, #335
