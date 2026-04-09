---
doc_id: "COM-COOK-001"
doc_title: "Ecosystem Cookbook"
doc_version: "1.0.0"
doc_date: "2026-04-08"
doc_status: "Draft"
project: "common_system"
category: "COOK"
---

# Ecosystem Cookbook

> **Copy-paste recipes for common cross-system patterns.**

This cookbook provides working code snippets for integrating multiple kcenon ecosystem systems to solve common problems.

---

## Table of Contents

- [1. Async Task with Retry](#1-async-task-with-retry)
- [2. Structured Log Correlation](#2-structured-log-correlation)
- [3. Type-Safe Network Messaging](#3-type-safe-network-messaging)
- [4. DB Connection Pool Monitoring](#4-db-connection-pool-monitoring)
- [5. Hot-Reload Config Update](#5-hot-reload-config-update)
- [6. Circuit Breaker for External Calls](#6-circuit-breaker-for-external-calls)
- [7. Multi-Format Data Serialization](#7-multi-format-data-serialization)
- [8. Health Check Endpoint](#8-health-check-endpoint)

---

## 1. Async Task with Retry

**Systems**: common_system + thread_system
**Use case**: Execute an operation asynchronously with exponential backoff on failure.

```cpp
#include <kcenon/common/patterns/result.h>
#include <kcenon/thread/thread_pool.h>
#include <chrono>
#include <thread>

using namespace kcenon::common;

auto pool = std::make_shared<kcenon::thread::thread_pool>(4);

template<typename F>
Result<int> retry_with_backoff(F operation, int max_retries = 3) {
    int delay_ms = 100;
    for (int attempt = 0; attempt < max_retries; ++attempt) {
        auto result = operation();
        if (result.is_ok()) return result;

        if (attempt == max_retries - 1) return result; // Final failure

        std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
        delay_ms *= 2; // Exponential backoff
    }
    return make_error<int>(-1, "max retries exceeded", "retry");
}

// Submit retry task to thread pool
auto future = pool->submit([] {
    return retry_with_backoff([]() -> Result<int> {
        // Your potentially-failing operation here
        return ok(42);
    });
});
```

---

## 2. Structured Log Correlation

**Systems**: logger_system + monitoring_system
**Use case**: Correlate log entries with distributed trace IDs.

```cpp
#include <kcenon/logger/core/logger.h>
#include <kcenon/network/tracing/tracing.h>

auto trace_ctx = kcenon::network::tracing::trace_context::create("handle_request");
auto trace_id = trace_ctx.to_headers().front().second; // Extract trace ID

logger->info("Processing request", {
    {"trace_id", trace_id},
    {"user_id", "42"},
    {"action", "get_profile"}
});
```

---

## 3. Type-Safe Network Messaging

**Systems**: container_system + network_system
**Use case**: Send typed messages over TCP with binary serialization.

```cpp
#include "container.h"
#include <kcenon/network/facade/tcp_facade.h>
#include <kcenon/container/serializers/serializer_factory.h>

using namespace kcenon::container;

// Build a typed message
value_container request;
request.set("action", std::string("login"));
request.set("username", std::string("alice"));
request.set("timestamp", static_cast<int64_t>(std::time(nullptr)));

// Serialize to binary
auto serializer = serializer_factory::create(serialization_format::binary);
auto result = serializer->serialize(request);
auto bytes = result.value();

// Send over TCP
auto client = kcenon::network::facade::tcp_facade::create_client({"localhost", 9090});
client->send(std::span<const std::byte>(
    reinterpret_cast<const std::byte*>(bytes.data()), bytes.size()));
```

---

## 4. DB Connection Pool Monitoring

**Systems**: database_system + monitoring_system
**Use case**: Track pool health and query latency.

```cpp
#include <kcenon/database/database_manager.h>
#include <kcenon/monitoring/core/performance_monitor.h>

auto& db = kcenon::database::database_manager::handle();
auto monitor = std::make_shared<kcenon::monitoring::performance_monitor>();

auto pool = db.get_connection_pool(kcenon::database::database_types::postgres);

// Periodically report pool stats
auto stats = pool->get_statistics();
monitor->record_gauge("db.pool.active", stats.active_connections);
monitor->record_gauge("db.pool.available", stats.available_connections);
monitor->record_histogram("db.pool.acquire_time_ns",
                          stats.avg_acquisition_time.count());

// Alert on unhealthy pool
if (!pool->is_healthy()) {
    logger->error("Database pool unhealthy",
                  {{"failed_checks", std::to_string(stats.failed_health_checks)}});
}
```

---

## 5. Hot-Reload Config Update

**Systems**: common_system (config)
**Use case**: Watch a config file and apply changes at runtime.

```cpp
#include <kcenon/common/config/config_watcher.h>

using namespace kcenon::common::config;

config_watcher watcher("app.yaml");

watcher.on_change([&logger](const unified_config& old_cfg,
                             const unified_config& new_cfg) {
    logger->info("Config reloaded", {
        {"old_pool_size", std::to_string(old_cfg.thread.pool_size)},
        {"new_pool_size", std::to_string(new_cfg.thread.pool_size)}
    });
    // Apply new config to running thread pool
    if (old_cfg.thread.pool_size != new_cfg.thread.pool_size) {
        pool->resize(new_cfg.thread.pool_size);
    }
});

watcher.start();
```

---

## 6. Circuit Breaker for External Calls

**Systems**: common_system (resilience) + network_system
**Use case**: Protect HTTP client calls with circuit breaker.

```cpp
#include <kcenon/common/resilience/circuit_breaker.h>
#include <kcenon/network/facade/http_facade.h>

using namespace kcenon::common::resilience;

circuit_breaker_config cb_config;
cb_config.failure_threshold = 5;
cb_config.timeout = std::chrono::seconds(30);

circuit_breaker breaker(cb_config);

auto http_client = kcenon::network::facade::http_facade::create_client({});

auto call_api = [&]() -> Result<std::string> {
    if (!breaker.allow_request()) {
        return make_error<std::string>(-1, "circuit open", "api");
    }

    auto guard = breaker.make_guard();
    // Make HTTP call...
    // On success:
    guard.record_success();
    return ok(std::string("response"));
};
```

---

## 7. Multi-Format Data Serialization

**Systems**: container_system
**Use case**: Compare JSON/binary/XML roundtrips for the same data.

```cpp
#include "container.h"
#include <kcenon/container/serializers/serializer_factory.h>

using namespace kcenon::container;

value_container data;
data.set("name", std::string("Alice"));
data.set("age", static_cast<int64_t>(30));
data.set("score", 95.5);

// Binary (compact, fast)
auto binary = serializer_factory::create(serialization_format::binary);
auto binary_bytes = binary->serialize(data).value();
std::cout << "Binary size: " << binary_bytes.size() << " bytes\n";

// JSON (human-readable)
auto json = serializer_factory::create(serialization_format::json);
auto json_bytes = json->serialize(data).value();
std::cout << "JSON size: " << json_bytes.size() << " bytes\n";

// Typical results: binary ~30%, json ~100%, xml ~150%
```

---

## 8. Health Check Endpoint

**Systems**: common_system + network_system + monitoring_system
**Use case**: HTTP health endpoint aggregating subsystem health.

```cpp
#include <kcenon/common/interfaces/monitoring/health_check.h>
#include <kcenon/monitoring/health/health_monitor.h>
#include <kcenon/network/facade/http_facade.h>

using namespace kcenon::common::interfaces;

auto health_monitor = std::make_shared<kcenon::monitoring::health_monitor>();

// Register individual checks
health_monitor->register_check("database", db_health_check);
health_monitor->register_check("cache", cache_health_check);
health_monitor->register_check("queue", queue_health_check);

// HTTP endpoint
auto server = kcenon::network::facade::http_facade::create_server({.port = 8080});
server->on_request("/health", [&](const auto& req, auto& resp) {
    auto status = health_monitor->get_overall_status();
    if (status == health_status::healthy) {
        resp.status = 200;
        resp.body = R"({"status": "healthy"})";
    } else {
        resp.status = 503;
        resp.body = R"({"status": "unhealthy"})";
    }
});
```

---

## Related Documentation

- [Ecosystem Integration Tutorial](../tutorials/ECOSYSTEM_INTEGRATION_TUTORIAL.md) — Step-by-step walkthrough
- [Integration Guide](../INTEGRATION_GUIDE.md) — In-depth integration reference
