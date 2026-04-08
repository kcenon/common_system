# Production Deployment Guide - Monitoring, Alerting, and Troubleshooting

> **SSOT**: This document is part of the [Production Deployment Guide](PRODUCTION_GUIDE.md).

## 4. Monitoring and Alerting

### Metrics Collection

**Application Instrumentation**:
```cpp
#include <monitoring_system/prometheus_exporter.hpp>

// Define metrics
auto request_counter = kcenon::Counter("http_requests_total", "Total HTTP requests",
    {{"method", "GET"}, {"status", "200"}});

auto latency_histogram = kcenon::Histogram("http_request_duration_seconds",
    "HTTP request latency", {0.001, 0.01, 0.1, 1.0, 10.0});

auto active_connections = kcenon::Gauge("http_active_connections", "Active connections");

// Instrument code
void handle_request(const Request& req) {
    auto start = std::chrono::steady_clock::now();
    active_connections.inc();

    // ... process request ...

    active_connections.dec();
    auto duration = std::chrono::steady_clock::now() - start;
    latency_histogram.observe(duration.count() / 1e9);  // Convert to seconds
    request_counter.inc({{"method", req.method}, {"status", std::to_string(response.status)}});
}

// Start Prometheus exporter
int main() {
    kcenon::PrometheusExporter exporter(9091);
    exporter.start();

    // ... run application ...
}
```

**Prometheus Scrape Configuration**:
```yaml
# prometheus.yml
global:
  scrape_interval: 15s
  evaluation_interval: 15s

scrape_configs:
  # Scrape application metrics
  - job_name: 'my-app'
    kubernetes_sd_configs:
    - role: pod
      namespaces:
        names:
        - production
    relabel_configs:
    # Only scrape pods with prometheus.io/scrape annotation
    - source_labels: [__meta_kubernetes_pod_annotation_prometheus_io_scrape]
      action: keep
      regex: true
    # Use custom port from annotation
    - source_labels: [__meta_kubernetes_pod_annotation_prometheus_io_port]
      action: replace
      target_label: __address__
      regex: (.+)
      replacement: $1:${1}
    # Use custom path from annotation
    - source_labels: [__meta_kubernetes_pod_annotation_prometheus_io_path]
      action: replace
      target_label: __metrics_path__
      regex: (.+)
```

**Key Metrics to Collect**:
```yaml
# Application metrics
- http_requests_total{method, status}        # Request count
- http_request_duration_seconds{quantile}    # Latency distribution
- http_active_connections                    # Current connections

# Thread pool metrics
- thread_pool_queue_depth{priority}          # Queue backlog
- thread_pool_active_threads                 # Active threads
- thread_pool_job_duration_seconds{quantile} # Job latency

# Database metrics
- database_query_duration_seconds{quantile}  # Query latency
- database_connection_pool_size              # Total connections
- database_connection_pool_active            # Active connections
- database_errors_total{type}                # Error count

# Logger metrics
- logger_messages_total{level}               # Log volume
- logger_queue_depth                         # Async queue depth
- logger_dropped_messages_total              # Dropped messages

# System metrics (node_exporter)
- node_cpu_seconds_total                     # CPU usage
- node_memory_MemAvailable_bytes             # Available memory
- node_disk_io_time_seconds_total            # Disk I/O
- node_network_receive_bytes_total           # Network traffic
```

---

### Health Checks

**Health Check Endpoint**:
```cpp
#include <network_system/http_server.hpp>

class HealthChecker {
public:
    // Liveness check: is the process alive?
    bool liveness_check() {
        // Simple check: can we allocate memory?
        try {
            std::vector<int> test(100);
            return true;
        } catch (...) {
            return false;
        }
    }

    // Readiness check: can we serve traffic?
    bool readiness_check() {
        bool db_ready = check_database_connection();
        bool cache_ready = check_cache_connection();
        bool thread_pool_ready = thread_pool_->queue_depth() < 1000;

        return db_ready && cache_ready && thread_pool_ready;
    }

private:
    bool check_database_connection() {
        // Try simple query with timeout
        try {
            auto result = db_->execute("SELECT 1", std::chrono::seconds(2));
            return result.is_ok();
        } catch (...) {
            return false;
        }
    }
};

// HTTP endpoints
void setup_health_endpoints() {
    auto server = kcenon::HttpServer(9090);
    auto checker = std::make_shared<HealthChecker>();

    // Liveness endpoint
    server.route("/health/live", [checker](const auto& req) {
        if (checker->liveness_check()) {
            return kcenon::HttpResponse(200, R"({"status":"UP"})");
        } else {
            return kcenon::HttpResponse(503, R"({"status":"DOWN"})");
        }
    });

    // Readiness endpoint
    server.route("/health/ready", [checker](const auto& req) {
        if (checker->readiness_check()) {
            return kcenon::HttpResponse(200, R"({"status":"READY"})");
        } else {
            return kcenon::HttpResponse(503, R"({"status":"NOT_READY"})");
        }
    });

    server.start();
}
```

**Kubernetes Health Probes**:
```yaml
livenessProbe:
  httpGet:
    path: /health/live
    port: 9090
  initialDelaySeconds: 10   # Wait for startup
  periodSeconds: 10         # Check every 10s
  timeoutSeconds: 3         # Timeout after 3s
  failureThreshold: 3       # Restart after 3 failures

readinessProbe:
  httpGet:
    path: /health/ready
    port: 9090
  initialDelaySeconds: 5
  periodSeconds: 5
  timeoutSeconds: 2
  failureThreshold: 2       # Remove from service after 2 failures
  successThreshold: 1       # Add to service after 1 success
```

---

### Alerting Rules

**Prometheus Alerting Rules**:
```yaml
# alerts.yml
groups:
- name: application_alerts
  interval: 30s
  rules:

  # High error rate
  - alert: HighErrorRate
    expr: |
      (
        sum(rate(http_requests_total{status=~"5.."}[5m])) /
        sum(rate(http_requests_total[5m]))
      ) > 0.05
    for: 5m
    labels:
      severity: critical
      team: backend
    annotations:
      summary: "High error rate detected"
      description: "Error rate is {{ $value | humanizePercentage }} (threshold: 5%)"

  # High latency
  - alert: HighLatency
    expr: |
      histogram_quantile(0.99,
        sum(rate(http_request_duration_seconds_bucket[5m])) by (le)
      ) > 1.0
    for: 10m
    labels:
      severity: warning
      team: backend
    annotations:
      summary: "High P99 latency"
      description: "P99 latency is {{ $value }}s (threshold: 1.0s)"

  # Database connection pool exhausted
  - alert: DatabasePoolExhausted
    expr: |
      (
        database_connection_pool_active /
        database_connection_pool_size
      ) > 0.9
    for: 5m
    labels:
      severity: warning
      team: database
    annotations:
      summary: "Database connection pool near capacity"
      description: "Pool utilization: {{ $value | humanizePercentage }}"

  # High memory usage
  - alert: HighMemoryUsage
    expr: |
      (
        process_resident_memory_bytes /
        container_spec_memory_limit_bytes
      ) > 0.9
    for: 10m
    labels:
      severity: warning
      team: sre
    annotations:
      summary: "High memory usage"
      description: "Memory usage: {{ $value | humanizePercentage }}"

  # Thread pool queue backlog
  - alert: ThreadPoolBacklog
    expr: thread_pool_queue_depth > 1000
    for: 5m
    labels:
      severity: warning
      team: backend
    annotations:
      summary: "Thread pool queue backlog"
      description: "Queue depth: {{ $value }}"

  # Service down
  - alert: ServiceDown
    expr: up{job="my-app"} == 0
    for: 1m
    labels:
      severity: critical
      team: sre
    annotations:
      summary: "Service {{ $labels.instance }} is down"
      description: "Service has been down for 1 minute"
```

**Alertmanager Configuration**:
```yaml
# alertmanager.yml
global:
  resolve_timeout: 5m

route:
  group_by: ['alertname', 'severity']
  group_wait: 10s
  group_interval: 10s
  repeat_interval: 12h
  receiver: 'default'

  routes:
  # Critical alerts to PagerDuty
  - match:
      severity: critical
    receiver: pagerduty
    continue: true

  # Warning alerts to Slack
  - match:
      severity: warning
    receiver: slack

  # Database alerts to database team
  - match:
      team: database
    receiver: database-team

receivers:
- name: 'default'
  email_configs:
  - to: 'team@example.com'

- name: 'pagerduty'
  pagerduty_configs:
  - service_key: '<pagerduty_service_key>'

- name: 'slack'
  slack_configs:
  - api_url: '<slack_webhook_url>'
    channel: '#alerts'
    title: 'Alert: {{ .GroupLabels.alertname }}'
    text: '{{ range .Alerts }}{{ .Annotations.description }}{{ end }}'

- name: 'database-team'
  email_configs:
  - to: 'database-team@example.com'
```

---

### Observability Integration

**Distributed Tracing (OpenTelemetry)**:
```cpp
#include <opentelemetry/trace/provider.h>
#include <opentelemetry/exporters/jaeger/jaeger_exporter.h>

// Initialize tracer
void setup_tracing() {
    auto exporter = std::make_unique<opentelemetry::exporter::jaeger::JaegerExporter>(
        opentelemetry::exporter::jaeger::JaegerExporterOptions{
            .endpoint = "http://jaeger:14268/api/traces",
            .service_name = "my-app"
        }
    );

    auto processor = std::make_shared<opentelemetry::sdk::trace::SimpleSpanProcessor>(
        std::move(exporter)
    );

    auto provider = std::make_shared<opentelemetry::sdk::trace::TracerProvider>(processor);
    opentelemetry::trace::Provider::SetTracerProvider(provider);
}

// Instrument request handling
void handle_request(const Request& req) {
    auto tracer = opentelemetry::trace::Provider::GetTracerProvider()->GetTracer("my-app");

    // Start span
    auto span = tracer->StartSpan("handle_request",
        {{"http.method", req.method},
         {"http.url", req.url}});

    // Database query (child span)
    {
        auto db_span = tracer->StartSpan("database_query",
            {{"db.statement", "SELECT * FROM users"}},
            {span->GetContext()});

        auto result = db_->execute("SELECT * FROM users");

        db_span->SetAttribute("db.rows", result.row_count());
        db_span->End();
    }

    // External API call (child span)
    {
        auto api_span = tracer->StartSpan("external_api_call",
            {{"http.url", "https://api.example.com"}},
            {span->GetContext()});

        auto response = http_client_->get("https://api.example.com/data");

        api_span->SetAttribute("http.status_code", response.status);
        api_span->End();
    }

    span->SetAttribute("http.status_code", 200);
    span->End();
}
```

**Log Aggregation (ELK Stack)**:
```yaml
# filebeat.yml
filebeat.inputs:
- type: container
  paths:
    - '/var/log/containers/*-my-app-*.log'
  json.keys_under_root: true
  json.add_error_key: true

output.elasticsearch:
  hosts: ["elasticsearch:9200"]
  index: "app-logs-%{+yyyy.MM.dd}"

# Kibana dashboard queries
# - Error rate: status:5* AND @timestamp:[now-1h TO now]
# - Slow queries: duration:>1000 AND component:database
# - User activity: user_id:* AND action:login
```

---

## 5. Troubleshooting Guide

### Common Issues

#### Issue 1: High CPU Usage

**Symptoms**:
- CPU usage consistently >90%
- Slow response times
- Thread pool queue backlog

**Diagnosis**:
```bash
# Check CPU usage per process
top -p $(pgrep my_app)

# Profile with perf
perf record -p $(pgrep my_app) -g -- sleep 30
perf report

# Check thread pool queue depth
curl http://localhost:9091/metrics | grep thread_pool_queue_depth
```

**Root Causes**:
1. **Inefficient algorithm**: O(n^2) in hot path
2. **Busy loop**: Polling instead of event-driven
3. **Lock contention**: Too much time in mutex
4. **Excessive logging**: High-frequency debug logs

**Solutions**:
```cpp
// 1. Use profiler to find hot spots
// 2. Optimize algorithm
std::sort(data.begin(), data.end());  // O(n log n) instead of bubble sort

// 3. Replace polling with event notification
condition_variable.wait(lock, [&]{ return data_ready; });

// 4. Reduce lock scope
{
    std::lock_guard<std::mutex> lock(mutex);
    // Critical section only
}

// 5. Lower log level in production
logger->set_level(kcenon::LogLevel::INFO);  // Disable DEBUG
```

---

#### Issue 2: Memory Leak

**Symptoms**:
- Memory usage growing over time
- Eventually OOMKilled by Kubernetes

**Diagnosis**:
```bash
# Monitor memory over time
watch -n 1 'ps aux | grep my_app'

# Get heap profile (if enabled)
curl http://localhost:9091/debug/pprof/heap > heap.prof
go tool pprof heap.prof

# Use Valgrind (development only, 10-30x slowdown)
valgrind --leak-check=full --show-leak-kinds=all ./my_app

# AddressSanitizer (better for production)
cmake -DCMAKE_CXX_FLAGS="-fsanitize=address"
./my_app 2>&1 | grep "LeakSanitizer"
```

**Root Causes**:
1. **Circular shared_ptr**: Reference cycles preventing deallocation
2. **Forgotten cleanup**: Resources not freed in destructor
3. **Event handler not unregistered**: Lambda capturing `this`
4. **Connection pool not releasing**: Connections leaked

**Solutions**:
```cpp
// 1. Break circular references with weak_ptr
class Node {
    std::shared_ptr<Node> next;
    std::weak_ptr<Node> prev;  // Use weak_ptr for back-reference
};

// 2. Use RAII everywhere
class Resource {
    ~Resource() {
        cleanup();  // Always cleanup in destructor
    }
};

// 3. Unregister callbacks
event_bus->subscribe("event", handler_id);
// Later:
event_bus->unsubscribe(handler_id);

// 4. Return connections to pool
{
    auto conn = pool->acquire();  // RAII wrapper auto-returns
    conn->execute("SELECT 1");
}  // Connection returned to pool
```

---

#### Issue 3: Slow Database Queries

**Symptoms**:
- High P99 latency
- Database CPU usage high
- Connection pool exhausted

**Diagnosis**:
```sql
-- PostgreSQL: Find slow queries
SELECT query, mean_exec_time, calls
FROM pg_stat_statements
ORDER BY mean_exec_time DESC
LIMIT 10;

-- Check for missing indexes
SELECT schemaname, tablename, attname, n_distinct, correlation
FROM pg_stats
WHERE schemaname NOT IN ('pg_catalog', 'information_schema')
  AND (n_distinct > 1000 OR correlation < 0.1);

-- Check connection pool utilization
curl http://localhost:9091/metrics | grep database_connection_pool
```

**Root Causes**:
1. **Missing index**: Full table scan
2. **N+1 queries**: Fetching related data in loop
3. **Large result set**: Returning too many rows
4. **Lock contention**: Long-running transactions

**Solutions**:
```sql
-- 1. Add index
CREATE INDEX idx_users_email ON users(email);

-- 2. Use JOIN instead of N+1
-- Bad: SELECT * FROM orders WHERE user_id = ?; (in loop)
-- Good: SELECT * FROM orders JOIN users ON orders.user_id = users.id;

-- 3. Add pagination
SELECT * FROM orders ORDER BY created_at DESC LIMIT 100 OFFSET 0;

-- 4. Keep transactions short
BEGIN;
UPDATE accounts SET balance = balance - 100 WHERE id = 1;
-- Don't do expensive work here
COMMIT;
```

```cpp
// Use read replicas for read-heavy workloads
auto primary = db_pool->acquire_primary();  // Write
auto replica = db_pool->acquire_replica();  // Read

// Batch operations
db->execute_batch([](auto& tx) {
    for (const auto& order : orders) {
        tx.insert("orders", order);
    }
});
```

---

#### Issue 4: Network Connection Failures

**Symptoms**:
- `Connection refused` errors
- Timeouts
- Intermittent failures

**Diagnosis**:
```bash
# Check if port is listening
netstat -tulpn | grep 8080

# Test connection
curl -v http://localhost:8080/health

# Check firewall rules
iptables -L -n

# DNS resolution
nslookup database-service

# Network connectivity
ping database-service
traceroute database-service
```

**Root Causes**:
1. **Service not started**: Application crashed
2. **Firewall blocking**: Port not open
3. **DNS failure**: Service name not resolving
4. **Connection pool exhausted**: All connections in use

**Solutions**:
```bash
# 1. Check service status
systemctl status my-app
kubectl -n production get pods

# 2. Open firewall port
iptables -A INPUT -p tcp --dport 8080 -j ACCEPT

# 3. Verify DNS
kubectl -n production get svc

# 4. Increase connection pool size
# config/database.yaml
connection_pool:
  max_size: 50  # Increase from 20
```

---

### Diagnostic Tools

**Performance Profiling**:
```bash
# CPU profiling (Linux perf)
perf record -F 99 -p $(pgrep my_app) -g -- sleep 30
perf script | ./flamegraph.pl > flamegraph.svg

# Heap profiling (gperftools)
LD_PRELOAD=/usr/lib/libprofiler.so CPUPROFILE=/tmp/prof.out ./my_app
google-pprof --text ./my_app /tmp/prof.out

# System calls tracing (strace)
strace -c -p $(pgrep my_app)  # Count syscalls
strace -e open,read,write -p $(pgrep my_app)  # Trace specific syscalls
```

**Network Debugging**:
```bash
# Packet capture
tcpdump -i any -n port 8080 -w capture.pcap

# HTTP traffic inspection
tcpdump -i any -A -s 0 port 8080

# Connection tracking
ss -tunap | grep my_app  # Show connections
netstat -anp | grep my_app
```

**Log Analysis**:
```bash
# Find errors in last hour
journalctl -u my-app --since "1 hour ago" | grep ERROR

# Count errors by type
grep ERROR /var/log/app/application.log | awk '{print $5}' | sort | uniq -c

# Parse JSON logs
cat /var/log/app/application.log | jq 'select(.level == "ERROR")'

# Slow query log
grep "duration:[0-9]\{4,\}" /var/log/app/application.log
```

---

### Performance Debugging

**Latency Breakdown**:
```cpp
#include <chrono>

void handle_request(const Request& req) {
    auto t0 = std::chrono::steady_clock::now();

    // Parse request
    auto body = parse_body(req);
    auto t1 = std::chrono::steady_clock::now();

    // Database query
    auto result = db_->execute("SELECT ...");
    auto t2 = std::chrono::steady_clock::now();

    // External API call
    auto api_response = http_client_->get("https://api.example.com");
    auto t3 = std::chrono::steady_clock::now();

    // Serialize response
    auto response = serialize(result, api_response);
    auto t4 = std::chrono::steady_clock::now();

    // Log latency breakdown
    logger->info("Latency breakdown",
        {{"parse_ms", duration(t0, t1)},
         {"database_ms", duration(t1, t2)},
         {"api_ms", duration(t2, t3)},
         {"serialize_ms", duration(t3, t4)},
         {"total_ms", duration(t0, t4)}});
}

auto duration(auto t1, auto t2) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
}
```

**Memory Profiling**:
```cpp
// Track allocations per component
class AllocationTracker {
public:
    void record_allocation(const char* component, size_t bytes) {
        std::lock_guard<std::mutex> lock(mutex_);
        allocations_[component] += bytes;
    }

    void report() {
        for (const auto& [component, bytes] : allocations_) {
            logger->info("Memory usage", {{"component", component}, {"bytes", bytes}});
        }
    }
};

// Use custom allocator to track allocations
template<typename T>
class TrackingAllocator {
    T* allocate(size_t n) {
        tracker->record_allocation("my_component", n * sizeof(T));
        return std::allocator<T>{}.allocate(n);
    }
};
```

---

### Log Analysis

**Structured Log Parsing**:
```bash
# Find all ERROR logs with status code 500
cat /var/log/app/application.log | jq 'select(.level == "ERROR" and .status == 500)'

# Count errors by endpoint
cat /var/log/app/application.log | jq -r 'select(.level == "ERROR") | .endpoint' | sort | uniq -c

# Find slow requests (>1s)
cat /var/log/app/application.log | jq 'select(.duration_ms > 1000)'

# Get P50, P95, P99 latency
cat /var/log/app/application.log | jq -r '.duration_ms' | st --percentile 50,95,99
```

**Log Aggregation Queries (Kibana)**:
```
# Error rate over time
{
  "query": {
    "bool": {
      "must": [
        {"match": {"level": "ERROR"}},
        {"range": {"@timestamp": {"gte": "now-1h"}}}
      ]
    }
  },
  "aggs": {
    "errors_over_time": {
      "date_histogram": {
        "field": "@timestamp",
        "interval": "1m"
      }
    }
  }
}
```

---

**Version**: 1.0.0
**Last Updated**: 2025-12-03
**Maintainer**: kcenon team
