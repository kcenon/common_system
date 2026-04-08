---
doc_id: "COM-INTR-002b"
doc_title: "Cross-System Integration Guide - Patterns"
doc_version: "1.0.0"
doc_date: "2026-04-04"
doc_status: "Released"
project: "common_system"
category: "INTR"
---

# Cross-System Integration Guide - Patterns

> **Parent document**: [Cross-System Integration Guide](INTEGRATION_GUIDE.md)

This document covers common integration scenarios, configuration management, and error handling across the kcenon ecosystem.

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

## 5. Error Handling Across Systems

### Error Code Ranges

Each kcenon system has a dedicated error code range to prevent collisions and enable error source identification.

| System | Error Code Range | Purpose |
|--------|------------------|---------|
| common_system | 1000-1999 | Foundation errors (invalid arguments, not found, etc.) |
| thread_system | 2000-2999 | Thread pool errors (queue full, shutdown, etc.) |
| logger_system | 3000-3999 | Logging errors (sink failure, file I/O, etc.) |
| database_system | 4000-4999 | Database errors (connection, query, transaction, etc.) |
| network_system | 5000-5999 | Network errors (connection, timeout, protocol, etc.) |
| container_system | 6000-6999 | Container errors (serialization, validation, etc.) |
| monitoring_system | 7000-7999 | Monitoring errors (metric collection, health check, etc.) |

#### Common Error Codes

**common_system (1000-1999)**:
```cpp
namespace kcenon::common::error_codes {
    constexpr int OK = 0;
    constexpr int UNKNOWN_ERROR = 1000;
    constexpr int INVALID_ARGUMENT = 1001;
    constexpr int NOT_FOUND = 1002;
    constexpr int ALREADY_EXISTS = 1003;
    constexpr int PERMISSION_DENIED = 1004;
    constexpr int TIMEOUT = 1005;
    constexpr int INTERNAL_ERROR = 1006;
    constexpr int NOT_INITIALIZED = 1007;
    constexpr int ALREADY_INITIALIZED = 1008;
}
```

**thread_system (2000-2999)**:
```cpp
namespace kcenon::thread::error_codes {
    constexpr int QUEUE_FULL = 2001;
    constexpr int EXECUTOR_SHUTDOWN = 2002;
    constexpr int TASK_REJECTED = 2003;
    constexpr int INVALID_THREAD_COUNT = 2004;
    constexpr int THREAD_CREATION_FAILED = 2005;
}
```

**database_system (4000-4999)**:
```cpp
namespace kcenon::database::error_codes {
    constexpr int CONNECTION_FAILED = 4001;
    constexpr int QUERY_FAILED = 4002;
    constexpr int TRANSACTION_FAILED = 4003;
    constexpr int INVALID_QUERY = 4004;
    constexpr int CONSTRAINT_VIOLATION = 4005;
    constexpr int DEADLOCK = 4006;
}
```

**network_system (5000-5999)**:
```cpp
namespace kcenon::network::error_codes {
    constexpr int CONNECTION_REFUSED = 5001;
    constexpr int CONNECTION_TIMEOUT = 5002;
    constexpr int SOCKET_ERROR = 5003;
    constexpr int INVALID_PROTOCOL = 5004;
    constexpr int REQUEST_TIMEOUT = 5005;
    constexpr int SERVER_ERROR = 5006;
}
```

### Cross-System Error Propagation

When errors cross system boundaries, preserve context while allowing higher-level systems to add their own error information.

#### Pattern 1: Error Context Preservation

```cpp
#include <kcenon/common/patterns/result.h>
#include <kcenon/database/core/database.h>
#include <kcenon/network/http/http_server.h>

using namespace kcenon;

// Low-level database error
auto query_users() -> common::Result<std::vector<User>> {
    auto result = db->execute("SELECT * FROM users");
    if (result.is_err()) {
        // Database error (4000-4999 range)
        return common::make_error<std::vector<User>>(
            result.error()  // Preserve original error
        );
    }
    return common::ok(parse_users(result.value()));
}

// Mid-level service error
auto get_users_service() -> common::Result<UserList> {
    auto users_result = query_users();
    if (users_result.is_err()) {
        // Add service-layer context while preserving DB error
        return common::make_error<UserList>(
            common::error_codes::INTERNAL_ERROR,
            "Failed to retrieve users: " + users_result.error().message,
            "user_service",
            users_result.error()  // Chain original error
        );
    }

    return common::ok(UserList{users_result.value()});
}

// High-level HTTP handler
auto handle_get_users(const auto& req) -> network::http::response {
    auto result = get_users_service();
    if (result.is_err()) {
        const auto& err = result.error();

        // Map internal errors to HTTP status codes
        int http_status = 500;
        if (err.code == common::error_codes::NOT_FOUND) {
            http_status = 404;
        } else if (err.code == common::error_codes::PERMISSION_DENIED) {
            http_status = 403;
        }

        // Log full error chain
        logger->log(common::log_level::error,
                   "Request failed: " + err.to_string());

        // Return user-friendly error
        return network::http::response{
            .status = http_status,
            .body = R"({"error": "Failed to retrieve users"})",
            .content_type = "application/json"
        };
    }

    return network::http::response{
        .status = 200,
        .body = serialize_users(result.value()),
        .content_type = "application/json"
    };
}
```

#### Pattern 2: Error Translation

Some systems need to translate errors from one domain to another:

```cpp
// Translate database errors to network errors
auto save_user_http(const User& user) -> common::VoidResult {
    auto db_result = db->insert("users", user);

    if (db_result.is_err()) {
        switch (db_result.error().code) {
            case database::error_codes::CONSTRAINT_VIOLATION:
                // Translate to common error
                return common::make_error(
                    common::error_codes::ALREADY_EXISTS,
                    "User already exists",
                    "http_handler"
                );

            case database::error_codes::CONNECTION_FAILED:
                // Translate to network error
                return common::make_error(
                    network::error_codes::SERVER_ERROR,
                    "Database unavailable",
                    "http_handler"
                );

            default:
                // Generic fallback
                return common::make_error(
                    common::error_codes::INTERNAL_ERROR,
                    "Failed to save user",
                    "http_handler",
                    db_result.error()  // Preserve original
                );
        }
    }

    return common::ok();
}
```

### Result Composition Patterns

Combine Results from multiple systems using monadic operations.

#### Sequential Composition

```cpp
auto process_order(const Order& order) -> common::Result<Receipt> {
    return validate_order(order)           // common_system
        .and_then([](const auto& validated) {
            return save_to_database(validated);  // database_system
        })
        .and_then([](const auto& saved) {
            return send_confirmation_email(saved);  // network_system
        })
        .and_then([](const auto& confirmed) {
            return generate_receipt(confirmed);
        });
}

// Usage
auto result = process_order(order);
if (result.is_err()) {
    logger->log(common::log_level::error,
               "Order processing failed at: " + result.error().source);
    return result.error();
}
```

#### Parallel Composition

```cpp
auto fetch_user_dashboard(int user_id) -> common::Result<Dashboard> {
    // Launch parallel queries
    auto user_future = std::async(std::launch::async, [=]() {
        return db->query_user(user_id);
    });

    auto orders_future = std::async(std::launch::async, [=]() {
        return db->query_orders(user_id);
    });

    auto notifications_future = std::async(std::launch::async, [=]() {
        return fetch_notifications(user_id);
    });

    // Collect results
    auto user_result = user_future.get();
    auto orders_result = orders_future.get();
    auto notifications_result = notifications_future.get();

    // Combine results (all must succeed)
    if (user_result.is_err()) {
        return common::make_error<Dashboard>(user_result.error());
    }
    if (orders_result.is_err()) {
        return common::make_error<Dashboard>(orders_result.error());
    }
    if (notifications_result.is_err()) {
        return common::make_error<Dashboard>(notifications_result.error());
    }

    // Success: combine all data
    return common::ok(Dashboard{
        .user = user_result.value(),
        .orders = orders_result.value(),
        .notifications = notifications_result.value()
    });
}
```

#### Fallback Composition

```cpp
auto get_config() -> common::Result<Config> {
    // Try primary config source
    auto result = load_from_database();
    if (result.is_ok()) {
        return result;
    }

    logger->log(common::log_level::warn,
               "Database config failed, trying file");

    // Fallback to file
    result = load_from_file("config.yaml");
    if (result.is_ok()) {
        return result;
    }

    logger->log(common::log_level::warn,
               "File config failed, using defaults");

    // Final fallback to defaults
    return common::ok(Config::defaults());
}
```

---

**Document Version**: 3.0.0 (Complete Guide)
**Last Updated**: 2026-02-08
**Authors**: kcenon team
**Related Issues**: kcenon/common_system#329, #334, #335, #336
