# Production Deployment Guide

> **Status**: ✅ **Complete**

This guide provides comprehensive instructions for deploying kcenon ecosystem applications to production environments, covering configuration, deployment patterns, containerization, monitoring, troubleshooting, security, and upgrade procedures.

---

## Table of Contents

- [Overview](#overview)
- [Production Configuration](#1-production-configuration)
  - [common_system Configuration](#common_system-configuration)
  - [thread_system Configuration](#thread_system-configuration)
  - [container_system Configuration](#container_system-configuration)
  - [logger_system Configuration](#logger_system-configuration)
  - [monitoring_system Configuration](#monitoring_system-configuration)
  - [database_system Configuration](#database_system-configuration)
  - [network_system Configuration](#network_system-configuration)
- [Deployment Patterns](#2-deployment-patterns)
  - [Pattern 1: Monolith Deployment](#pattern-1-monolith-deployment)
  - [Pattern 2: Microservice Deployment](#pattern-2-microservice-deployment)
  - [Pattern 3: Sidecar Deployment](#pattern-3-sidecar-deployment)
  - [Pattern 4: Hybrid Deployment](#pattern-4-hybrid-deployment)
- [Container Deployment](#3-container-deployment)
  - [Docker Configuration](#docker-configuration)
  - [Kubernetes Deployment](#kubernetes-deployment)
  - [Container Optimization](#container-optimization)
- [Monitoring and Alerting](#4-monitoring-and-alerting)
  - [Metrics Collection](#metrics-collection)
  - [Health Checks](#health-checks)
  - [Alerting Rules](#alerting-rules)
  - [Observability Integration](#observability-integration)
- [Troubleshooting Guide](#5-troubleshooting-guide)
  - [Common Issues](#common-issues)
  - [Diagnostic Tools](#diagnostic-tools)
  - [Performance Debugging](#performance-debugging)
  - [Log Analysis](#log-analysis)
- [Security Hardening](#6-security-hardening)
  - [Security Checklist](#security-checklist)
  - [Network Security](#network-security)
  - [Authentication and Authorization](#authentication-and-authorization)
  - [Secrets Management](#secrets-management)
- [Upgrade and Rollback](#7-upgrade-and-rollback)
  - [Version Compatibility](#version-compatibility)
  - [Upgrade Procedures](#upgrade-procedures)
  - [Rollback Procedures](#rollback-procedures)
  - [Zero-Downtime Upgrades](#zero-downtime-upgrades)
- [Related Documentation](#related-documentation)

---

## Overview

### Production Readiness

The kcenon ecosystem is designed for production workloads with:
- **High performance**: Million+ ops/sec throughput
- **Reliability**: Zero-copy operations, RAII memory safety
- **Observability**: Structured logging, metrics, health checks
- **Scalability**: Thread pool sizing, resource pooling
- **Maintainability**: Graceful shutdown, rolling upgrades

### Deployment Considerations

Production deployments require careful planning for:
1. **Configuration**: Environment-specific settings for all systems
2. **Architecture**: Monolith vs. microservices trade-offs
3. **Infrastructure**: Bare metal, containers, orchestration
4. **Operations**: Monitoring, alerting, incident response
5. **Security**: Hardening, secrets, compliance
6. **Maintenance**: Upgrades, rollbacks, disaster recovery

---

## 1. Production Configuration

### common_system Configuration

**File**: `config/common.yaml`

```yaml
common:
  error_handling:
    # Production error reporting
    detailed_errors: false  # Disable stack traces in errors
    error_registry:
      persist: true
      path: /var/log/app/error_registry.json

  result_type:
    # Enable Result<T> performance optimizations
    move_semantics: true
    copy_elision: true

  adapters:
    # Adapter validation
    strict_type_checking: true
    enable_smart_adapters: true

  # Logging integration (used by all systems)
  logging:
    level: info  # Production: info, Debug: debug
    format: json  # Structured logging for log aggregation
```

**Environment Variables**:
```bash
# Override config file settings
COMMON_ERROR_LEVEL=info
COMMON_ENABLE_REGISTRY=true
COMMON_REGISTRY_PATH=/var/log/app/error_registry.json
```

**Best Practices**:
- **Error levels**: Use `info` in production, `debug` only for troubleshooting
- **Error registry**: Enable to track error patterns and frequencies
- **Adapter strictness**: Keep strict type checking enabled to catch integration bugs

---

### thread_system Configuration

**File**: `config/thread.yaml`

```yaml
thread:
  # Thread pool configuration
  pool_size: auto  # Auto-detect CPU cores, or specify count

  # CPU core allocation
  cpu_affinity:
    enabled: true
    cores: [0-15]  # Pin threads to specific cores

  # Queue configuration
  queue:
    type: lock_free  # lock_free | mutex_based
    capacity: 10000  # Max queued jobs
    overflow_policy: reject  # reject | block | drop_oldest

  # Job scheduling
  scheduler:
    priority_levels: 3  # Number of priority levels
    preemption: false   # Disable preemption in production
    steal_policy: random  # random | fifo | lifo

  # Resource limits
  limits:
    max_job_duration: 30s  # Kill jobs exceeding this
    stack_size: 2MB        # Thread stack size
    max_cpu_usage: 95%     # Throttle if CPU > 95%

  # Monitoring
  metrics:
    enabled: true
    interval: 60s  # Metric collection interval
    export_queue_depth: true
    export_cpu_usage: true
```

**Sizing Formula**:
```cpp
// CPU-bound workloads
pool_size = num_cores

// I/O-bound workloads (database, network)
pool_size = num_cores * 2

// Mixed workloads
pool_size = num_cores * 1.5
```

**Production Settings**:
- **Pool size**: Start with `auto`, tune based on workload
- **CPU affinity**: Enable for consistent latency (NUMA-aware)
- **Queue capacity**: Set 100-1000x expected concurrent jobs
- **Overflow policy**: `reject` to fail fast, `block` for back-pressure

**Environment Variables**:
```bash
THREAD_POOL_SIZE=16
THREAD_QUEUE_CAPACITY=10000
THREAD_CPU_AFFINITY=0-15
THREAD_METRICS_ENABLED=true
```

---

### container_system Configuration

**File**: `config/container.yaml`

```yaml
container:
  # Serialization format
  default_format: messagepack  # json | messagepack | binary

  # Performance optimization
  zero_copy:
    enabled: true
    min_size: 4KB  # Use zero-copy for payloads > 4KB

  # Memory management
  memory:
    pooling:
      enabled: true
      pool_size: 1000  # Pre-allocate containers
      recycle_threshold: 100KB  # Recycle containers < 100KB

    limits:
      max_container_size: 10MB  # Reject containers > 10MB
      max_total_memory: 1GB     # Total memory for all containers

  # Compression
  compression:
    enabled: true
    algorithm: lz4  # lz4 | zstd | gzip
    min_size: 1KB   # Compress payloads > 1KB
    level: 3        # Compression level (1=fast, 9=best)

  # Validation
  validation:
    schema_validation: true
    checksum_verification: true
    max_nesting_depth: 10
```

**Best Practices**:
- **Format**: Use `messagepack` for performance, `json` for debugging
- **Zero-copy**: Enable for large payloads to reduce allocations
- **Pooling**: Pre-allocate containers for hot paths
- **Compression**: Use `lz4` for speed, `zstd` for compression ratio

**Environment Variables**:
```bash
CONTAINER_FORMAT=messagepack
CONTAINER_ZERO_COPY=true
CONTAINER_COMPRESSION=lz4
CONTAINER_POOL_SIZE=1000
```

---

### logger_system Configuration

**File**: `config/logger.yaml`

```yaml
logger:
  # Global logging settings
  default_level: info  # trace | debug | info | warn | error | critical

  # Async logging (production recommended)
  async:
    enabled: true
    queue_size: 8192
    flush_interval: 1s
    overflow_policy: drop  # drop | block

  # Sinks configuration
  sinks:
    # Console output (structured JSON)
    - type: console
      level: warn  # Only warnings and errors to console
      format: json
      colorize: false

    # File output (rotated logs)
    - type: file
      level: info
      path: /var/log/app/application.log
      format: json
      rotation:
        max_size: 100MB
        max_files: 10
        compress: true

    # Error-only file
    - type: file
      level: error
      path: /var/log/app/errors.log
      format: json
      rotation:
        max_size: 50MB
        max_files: 20

    # Syslog integration
    - type: syslog
      level: warn
      facility: local0
      hostname: app-server-01

  # Structured logging fields
  fields:
    service_name: my-application
    environment: production
    region: us-west-2
    version: 1.2.3

  # Performance
  performance:
    buffer_size: 4KB
    max_message_size: 64KB
    drop_on_overflow: true  # Don't block on full queue
```

**Log Aggregation Integration**:
```yaml
# For Elasticsearch/ELK
sinks:
  - type: file
    path: /var/log/app/json.log
    format: json
    fields:
      @timestamp: true  # Add timestamp field
      @version: 1
```

**Best Practices**:
- **Async logging**: Always enable in production (4-5x faster)
- **Log levels**: `info` for production, `debug` only when troubleshooting
- **Rotation**: Prevent disk space exhaustion
- **Structured logs**: Use JSON format for log aggregation (ELK, Splunk)

**Environment Variables**:
```bash
LOGGER_LEVEL=info
LOGGER_ASYNC=true
LOGGER_FILE_PATH=/var/log/app/application.log
LOGGER_FORMAT=json
```

---

### monitoring_system Configuration

**File**: `config/monitoring.yaml`

```yaml
monitoring:
  # Metrics collection
  metrics:
    enabled: true
    collection_interval: 15s  # Scrape interval

    # Metric types
    types:
      - counter    # Monotonically increasing
      - gauge      # Point-in-time value
      - histogram  # Distribution
      - summary    # Quantiles

    # Retention
    retention:
      in_memory_duration: 5m   # Keep 5 min in memory
      persist_to_disk: false   # Disable disk persistence

  # Health checks
  health:
    enabled: true
    endpoint: /health
    port: 9090

    checks:
      - name: liveness
        interval: 10s
        timeout: 2s

      - name: readiness
        interval: 30s
        timeout: 5s
        checks:
          - database_connection
          - external_api_reachability

  # Prometheus integration
  prometheus:
    enabled: true
    port: 9091
    endpoint: /metrics

    # Metric prefixes
    namespace: app
    subsystem: kcenon

    # Labels
    labels:
      service: my-service
      environment: production
      region: us-west-2

  # Alerting
  alerts:
    enabled: true
    webhook_url: https://alertmanager:9093/api/v1/alerts

    # Alert rules
    rules:
      - name: high_error_rate
        expr: error_count_total > 100
        duration: 5m
        severity: critical

      - name: high_memory_usage
        expr: memory_usage_bytes > 8GB
        duration: 10m
        severity: warning
```

**Prometheus Metrics Exposed**:
```
# Thread pool metrics
thread_pool_queue_depth{priority="high"} 42
thread_pool_active_threads{pool="main"} 16
thread_pool_job_duration_seconds{quantile="0.99"} 0.005

# Logger metrics
logger_messages_total{level="error"} 123
logger_queue_depth 45

# Network metrics
network_connections_active 1024
network_bytes_sent_total 1073741824
network_request_duration_seconds{quantile="0.99"} 0.120

# Database metrics
database_query_duration_seconds{quantile="0.95"} 0.050
database_connection_pool_size 20
database_connection_pool_active 15
```

**Best Practices**:
- **Collection interval**: 15s for production, 5s for debugging
- **Health checks**: Separate liveness (is process alive?) from readiness (can it serve traffic?)
- **Prometheus**: Standard metrics format for Kubernetes, Grafana
- **Alerting**: Define SLOs and alert on violations

**Environment Variables**:
```bash
MONITORING_ENABLED=true
MONITORING_PROMETHEUS_PORT=9091
MONITORING_HEALTH_PORT=9090
```

---

### database_system Configuration

**File**: `config/database.yaml`

```yaml
database:
  # Connection pool
  connection_pool:
    min_size: 5
    max_size: 20
    idle_timeout: 300s    # Close idle connections after 5 min
    max_lifetime: 1h      # Rotate connections every hour
    connection_timeout: 10s

  # Query execution
  execution:
    max_retry_attempts: 3
    retry_backoff: exponential  # exponential | linear
    initial_backoff: 100ms
    max_backoff: 5s

    # Timeouts
    query_timeout: 30s
    transaction_timeout: 60s

  # Performance
  performance:
    prepared_statements: true
    statement_cache_size: 1000
    batch_size: 100  # Batch INSERT/UPDATE

  # High availability
  ha:
    read_replicas:
      enabled: true
      endpoints:
        - db-replica-01:5432
        - db-replica-02:5432
      load_balancing: round_robin  # round_robin | random | least_connections

    failover:
      enabled: true
      health_check_interval: 10s
      max_failover_attempts: 3

  # Connection string (per environment)
  connections:
    primary:
      host: db-primary.example.com
      port: 5432
      database: production_db
      user: app_user
      password: ${DB_PASSWORD}  # From environment variable
      ssl_mode: require

    replicas:
      - host: db-replica-01.example.com
        port: 5432
        database: production_db
        user: app_user
        password: ${DB_PASSWORD}
        ssl_mode: require
```

**Connection Pool Sizing**:
```
# Formula
pool_size = ((core_count * 2) + effective_spindle_count)

# Example: 16 cores, SSD storage
pool_size = (16 * 2) + 1 = 33
```

**Best Practices**:
- **Pool size**: Start conservative (10-20), tune based on connection utilization
- **Timeouts**: Set query timeout < transaction timeout < connection timeout
- **Prepared statements**: Always enable for performance and SQL injection prevention
- **Read replicas**: Offload read-heavy workloads from primary
- **SSL**: Always use `ssl_mode: require` in production

**Environment Variables**:
```bash
DB_PRIMARY_HOST=db-primary.example.com
DB_PRIMARY_PORT=5432
DB_PRIMARY_NAME=production_db
DB_PRIMARY_USER=app_user
DB_PASSWORD=<secret>  # Never hardcode
DB_POOL_SIZE=20
DB_SSL_MODE=require
```

---

### network_system Configuration

**File**: `config/network.yaml`

```yaml
network:
  # TCP server configuration
  tcp:
    bind_address: 0.0.0.0
    port: 8080
    backlog: 1024  # Listen queue size

    # Connection limits
    max_connections: 10000
    max_connections_per_ip: 100

    # Timeouts
    connection_timeout: 30s
    read_timeout: 60s
    write_timeout: 60s
    keepalive_timeout: 120s

    # TCP options
    tcp_nodelay: true      # Disable Nagle's algorithm
    tcp_quickack: true     # Enable quick ACK
    so_reuseaddr: true
    so_reuseport: true     # Enable multi-process binding

  # HTTP server configuration
  http:
    max_request_size: 10MB
    max_header_size: 8KB
    max_uri_length: 2KB

    # Compression
    compression:
      enabled: true
      algorithms: [gzip, deflate, br]
      min_size: 1KB
      level: 6

    # CORS
    cors:
      enabled: true
      allowed_origins: ["https://example.com"]
      allowed_methods: [GET, POST, PUT, DELETE]
      allowed_headers: [Content-Type, Authorization]
      max_age: 3600

  # TLS/SSL configuration
  tls:
    enabled: true
    cert_file: /etc/ssl/certs/server.crt
    key_file: /etc/ssl/private/server.key
    ca_file: /etc/ssl/certs/ca.crt

    # TLS version
    min_version: TLSv1.2
    max_version: TLSv1.3

    # Cipher suites (strong ciphers only)
    ciphers:
      - TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384
      - TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256
      - TLS_ECDHE_RSA_WITH_CHACHA20_POLY1305

    # Certificate rotation
    auto_reload: true
    reload_interval: 24h

  # Load balancing (if multi-process)
  load_balancing:
    strategy: round_robin  # round_robin | least_connections | ip_hash
    health_check:
      enabled: true
      interval: 10s
      timeout: 2s
      unhealthy_threshold: 3
      healthy_threshold: 2

  # Rate limiting
  rate_limit:
    enabled: true
    requests_per_second: 1000
    burst: 2000
    strategy: token_bucket  # token_bucket | leaky_bucket
```

**Best Practices**:
- **Connection limits**: Set based on system resources (file descriptors, memory)
- **Timeouts**: Prevent resource exhaustion from slow clients
- **TCP options**: Enable `TCP_NODELAY` for low latency, `SO_REUSEPORT` for multi-process
- **TLS**: Always use TLS 1.2+ in production, strong cipher suites only
- **Rate limiting**: Protect against DoS attacks

**Environment Variables**:
```bash
NETWORK_BIND_ADDRESS=0.0.0.0
NETWORK_PORT=8080
NETWORK_MAX_CONNECTIONS=10000
NETWORK_TLS_ENABLED=true
NETWORK_TLS_CERT=/etc/ssl/certs/server.crt
NETWORK_TLS_KEY=/etc/ssl/private/server.key
```

---

## 2. Deployment Patterns

### Pattern 1: Monolith Deployment

**Description**: All 7 systems in a single process/container.

**Pros**:
- Simple deployment and operations
- Low latency (in-process communication)
- Easy debugging and tracing
- No network overhead

**Cons**:
- Scaling all systems together (resource waste)
- Single point of failure
- Difficult to update individual systems

**Use Cases**:
- Small to medium applications (<10K RPS)
- Internal tools and dashboards
- Proof-of-concept deployments

**Architecture Diagram**:
```
┌─────────────────────────────────────────┐
│       Monolith Process                  │
│  ┌────────────────────────────────┐     │
│  │  Application Code              │     │
│  └────────────────────────────────┘     │
│            ▲                            │
│            │                            │
│  ┌─────────┴──────────────────────┐     │
│  │  Unified Bootstrapper          │     │
│  └────────────────────────────────┘     │
│            ▲                            │
│            │ initializes                │
│  ┌─────────┴──────────────────────┐     │
│  │  All 7 Systems (in-process)    │     │
│  │  • network_system               │     │
│  │  • database_system              │     │
│  │  • monitoring_system            │     │
│  │  • logger_system                │     │
│  │  • container_system             │     │
│  │  • thread_system                │     │
│  │  • common_system                │     │
│  └────────────────────────────────┘     │
└─────────────────────────────────────────┘
         │
         │ (single deployment unit)
         ▼
   [Container/VM/Bare Metal]
```

**Configuration Example**:
```yaml
# config/production.yaml
deployment:
  pattern: monolith

  # All systems configured in single file
  systems:
    - common
    - thread
    - container
    - logger
    - monitoring
    - database
    - network

  # Resource allocation
  resources:
    cpu: 4 cores
    memory: 8GB

  # Scaling
  replicas: 3  # Deploy 3 instances behind load balancer
```

**Deployment Steps**:
```bash
# 1. Build single binary
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target my_app

# 2. Create container image
docker build -t my-app:1.0.0 .

# 3. Deploy to production
docker run -d \
  --name my-app \
  -p 8080:8080 \
  -v /etc/app/config:/etc/app/config:ro \
  -v /var/log/app:/var/log/app \
  --restart unless-stopped \
  my-app:1.0.0
```

**Monitoring**:
```yaml
# Prometheus scrape config
scrape_configs:
  - job_name: 'monolith'
    static_configs:
      - targets: ['app-01:9091', 'app-02:9091', 'app-03:9091']
```

---

### Pattern 2: Microservice Deployment

**Description**: Each system (or group of systems) in separate processes/containers.

**Pros**:
- Independent scaling per system
- Fault isolation (logger crash doesn't affect database)
- Technology flexibility (different languages per service)
- Team autonomy

**Cons**:
- Network latency between services
- Complex deployment and orchestration
- Distributed tracing required
- Higher operational overhead

**Use Cases**:
- Large-scale applications (>100K RPS)
- Polyglot architectures
- Teams with microservice expertise

**Architecture Diagram**:
```
┌──────────────┐  ┌──────────────┐  ┌──────────────┐
│  API Gateway │  │  Web Service │  │ Worker Service│
└──────┬───────┘  └──────┬───────┘  └──────┬───────┘
       │                 │                 │
       │ HTTP/gRPC       │ HTTP/gRPC       │ Message Queue
       ▼                 ▼                 ▼
┌──────────────┐  ┌──────────────┐  ┌──────────────┐
│Network Service│  │Database Svc  │  │Logger Service│
│(network_sys)  │  │(database_sys)│  │(logger_sys)  │
└──────────────┘  └──────────────┘  └──────────────┘
       │                 │                 │
       │                 │                 │
       ▼                 ▼                 ▼
┌──────────────┐  ┌──────────────┐  ┌──────────────┐
│Thread Service │  │Container Svc │  │Monitor Svc   │
│(thread_sys)   │  │(container_sys)│ │(monitor_sys) │
└──────────────┘  └──────────────┘  └──────────────┘
                         │
                         │ (all depend on)
                         ▼
                  ┌──────────────┐
                  │Common Service│
                  │(common_sys)  │
                  └──────────────┘
```

**Service Grouping Strategy**:
```yaml
# Group services by tier
services:
  # API tier (stateless)
  - name: api-service
    systems: [network, logger, thread, common]
    replicas: 10

  # Data tier (stateful)
  - name: database-service
    systems: [database, logger, thread, common]
    replicas: 3

  # Background jobs tier
  - name: worker-service
    systems: [container, logger, thread, common]
    replicas: 5

  # Observability tier
  - name: monitoring-service
    systems: [monitoring, logger, thread, common]
    replicas: 2
```

**Inter-Service Communication**:
```cpp
// Use network_system for service-to-service calls
#include <network_system/http_client.hpp>

// Call database service from API service
auto db_client = kcenon::HttpClient("http://database-service:8081");
auto response = db_client.post("/query", query_data);

// Use service mesh (Istio/Linkerd) for:
// - mTLS encryption
// - Load balancing
// - Circuit breaking
// - Retries and timeouts
```

**Deployment (Kubernetes)**:
```yaml
# api-service.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: api-service
spec:
  replicas: 10
  template:
    spec:
      containers:
      - name: api
        image: my-app/api-service:1.0.0
        ports:
        - containerPort: 8080
        env:
        - name: DATABASE_SERVICE_URL
          value: "http://database-service:8081"
        resources:
          requests:
            cpu: 500m
            memory: 512Mi
          limits:
            cpu: 2000m
            memory: 2Gi

---
# database-service.yaml
apiVersion: apps/v1
kind: StatefulSet  # Stateful for persistent storage
metadata:
  name: database-service
spec:
  replicas: 3
  serviceName: database-service
  template:
    spec:
      containers:
      - name: database
        image: my-app/database-service:1.0.0
        volumeMounts:
        - name: data
          mountPath: /var/lib/app/data
  volumeClaimTemplates:
  - metadata:
      name: data
    spec:
      accessModes: ["ReadWriteOnce"]
      resources:
        requests:
          storage: 100Gi
```

---

### Pattern 3: Sidecar Deployment

**Description**: Common systems (logger, monitoring) as sidecars, business logic in main container.

**Pros**:
- Separation of concerns (business logic vs. infrastructure)
- Shared infrastructure code across applications
- Easier to update infrastructure without touching app code
- Standard observability across all apps

**Cons**:
- Higher resource usage (multiple containers per pod)
- Inter-container communication overhead
- More complex pod configuration

**Use Cases**:
- Kubernetes environments with service mesh
- Standardized logging/monitoring across organization
- Legacy applications needing observability

**Architecture Diagram**:
```
┌─────────────────────────────────────────────┐
│              Kubernetes Pod                 │
│                                             │
│  ┌─────────────────────────────────────┐    │
│  │   Main Application Container        │    │
│  │  (Business Logic + network/db/thread)│   │
│  └─────────────────────────────────────┘    │
│            │                                │
│            │ stdout/stderr                  │
│            │ HTTP (metrics)                 │
│            ▼                                │
│  ┌─────────────────────────────────────┐    │
│  │   Logger Sidecar Container          │    │
│  │  • Collects logs from main app      │    │
│  │  • Forwards to log aggregator       │    │
│  │  • File rotation and buffering      │    │
│  └─────────────────────────────────────┘    │
│            │                                │
│            ▼ (send to Elasticsearch)       │
│                                             │
│  ┌─────────────────────────────────────┐    │
│  │  Monitoring Sidecar Container       │    │
│  │  • Scrapes /metrics from main app   │    │
│  │  • Forwards to Prometheus           │    │
│  │  • Health check proxy               │    │
│  └─────────────────────────────────────┘    │
│            │                                │
│            ▼ (send to Prometheus)          │
└─────────────────────────────────────────────┘
```

**Pod Configuration**:
```yaml
# sidecar-pod.yaml
apiVersion: v1
kind: Pod
metadata:
  name: app-with-sidecars
spec:
  containers:
  # Main application
  - name: app
    image: my-app:1.0.0
    ports:
    - containerPort: 8080
      name: http
    - containerPort: 9091
      name: metrics
    volumeMounts:
    - name: logs
      mountPath: /var/log/app
    resources:
      requests:
        cpu: 1000m
        memory: 2Gi
      limits:
        cpu: 2000m
        memory: 4Gi

  # Logger sidecar
  - name: logger
    image: logger-sidecar:1.0.0
    volumeMounts:
    - name: logs
      mountPath: /var/log/app
      readOnly: true
    env:
    - name: LOG_DESTINATION
      value: "elasticsearch:9200"
    - name: LOG_INDEX
      value: "app-logs"
    resources:
      requests:
        cpu: 100m
        memory: 128Mi
      limits:
        cpu: 200m
        memory: 256Mi

  # Monitoring sidecar (Prometheus exporter)
  - name: metrics-exporter
    image: prometheus-exporter:1.0.0
    ports:
    - containerPort: 9090
      name: exporter-metrics
    env:
    - name: APP_METRICS_URL
      value: "http://localhost:9091/metrics"
    resources:
      requests:
        cpu: 50m
        memory: 64Mi
      limits:
        cpu: 100m
        memory: 128Mi

  # Shared volume for logs
  volumes:
  - name: logs
    emptyDir: {}
```

**Sidecar Implementation**:
```cpp
// Main application exposes metrics endpoint
#include <network_system/http_server.hpp>

void setup_metrics_endpoint() {
    auto server = kcenon::HttpServer(9091);

    // Expose metrics in Prometheus format
    server.route("/metrics", [](const auto& req) {
        std::stringstream metrics;
        metrics << "# HELP app_requests_total Total requests\n";
        metrics << "# TYPE app_requests_total counter\n";
        metrics << "app_requests_total " << get_request_count() << "\n";

        return kcenon::HttpResponse(200, metrics.str());
    });

    server.start();
}
```

---

### Pattern 4: Hybrid Deployment

**Description**: Combination of monolith + microservices + sidecars.

**Use Cases**:
- Migration from monolith to microservices
- Gradual extraction of services
- Different scaling needs per component

**Architecture Example**:
```
┌─────────────────────────────────────┐
│     Monolith (Core Business Logic)  │  ← Most traffic here
│  (all 7 systems)                    │
└─────────────┬───────────────────────┘
              │
              │ Calls specialized services
              │
    ┌─────────┼─────────┬─────────────┐
    │         │         │             │
    ▼         ▼         ▼             ▼
┌──────┐  ┌──────┐  ┌────────┐  ┌─────────┐
│ML Svc│  │Search│  │Billing │  │ Logger  │ ← Extracted services
│      │  │ Svc  │  │  Svc   │  │ Sidecar │
└──────┘  └──────┘  └────────┘  └─────────┘
```

**Migration Strategy**:
1. **Start**: Monolith with all systems
2. **Extract**: High-load systems (e.g., search, ML inference)
3. **Stabilize**: Run hybrid for months
4. **Iterate**: Extract more services based on need
5. **End goal**: Microservices (optional)

---

## 3. Container Deployment

### Docker Configuration

**Multi-Stage Dockerfile**:
```dockerfile
# Stage 1: Build environment
FROM ubuntu:22.04 AS builder

# Install build dependencies
RUN apt-get update && apt-get install -y \
    g++-11 \
    cmake \
    ninja-build \
    git \
    && rm -rf /var/lib/apt/lists/*

# Copy source code
WORKDIR /app
COPY . .

# Build application (static linking for portability)
RUN cmake -B build \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_COMPILER=g++-11 \
    -DCMAKE_EXE_LINKER_FLAGS="-static-libgcc -static-libstdc++" \
    -G Ninja
RUN cmake --build build --target my_app

# Stage 2: Runtime environment (minimal)
FROM ubuntu:22.04

# Install runtime dependencies only
RUN apt-get update && apt-get install -y \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Create non-root user
RUN groupadd -r appuser && useradd -r -g appuser appuser

# Copy binary from builder
COPY --from=builder /app/build/my_app /usr/local/bin/my_app

# Copy configuration
COPY config/production.yaml /etc/app/config.yaml

# Create log directory
RUN mkdir -p /var/log/app && chown appuser:appuser /var/log/app

# Switch to non-root user
USER appuser

# Expose ports
EXPOSE 8080 9090 9091

# Health check
HEALTHCHECK --interval=30s --timeout=3s --start-period=5s --retries=3 \
  CMD curl -f http://localhost:9090/health || exit 1

# Run application
ENTRYPOINT ["/usr/local/bin/my_app"]
CMD ["--config", "/etc/app/config.yaml"]
```

**Build and Push**:
```bash
# Build image
docker build -t my-app:1.0.0 .

# Tag for registry
docker tag my-app:1.0.0 registry.example.com/my-app:1.0.0
docker tag my-app:1.0.0 registry.example.com/my-app:latest

# Push to registry
docker push registry.example.com/my-app:1.0.0
docker push registry.example.com/my-app:latest
```

**Docker Compose (Local Testing)**:
```yaml
# docker-compose.yml
version: '3.8'

services:
  app:
    image: my-app:1.0.0
    ports:
      - "8080:8080"   # Application
      - "9090:9090"   # Health check
      - "9091:9091"   # Metrics
    volumes:
      - ./config:/etc/app/config:ro
      - logs:/var/log/app
    environment:
      - LOG_LEVEL=info
      - DB_HOST=postgres
    depends_on:
      - postgres
      - redis
    restart: unless-stopped

  postgres:
    image: postgres:15
    environment:
      - POSTGRES_DB=production_db
      - POSTGRES_USER=app_user
      - POSTGRES_PASSWORD=secret
    volumes:
      - pgdata:/var/lib/postgresql/data

  redis:
    image: redis:7
    volumes:
      - redisdata:/data

  prometheus:
    image: prom/prometheus:latest
    ports:
      - "9092:9090"
    volumes:
      - ./prometheus.yml:/etc/prometheus/prometheus.yml:ro
      - prometheus-data:/prometheus

  grafana:
    image: grafana/grafana:latest
    ports:
      - "3000:3000"
    volumes:
      - grafana-data:/var/lib/grafana

volumes:
  logs:
  pgdata:
  redisdata:
  prometheus-data:
  grafana-data:
```

---

### Kubernetes Deployment

**Namespace Setup**:
```yaml
# namespace.yaml
apiVersion: v1
kind: Namespace
metadata:
  name: production
  labels:
    environment: production
```

**ConfigMap for Configuration**:
```yaml
# configmap.yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: app-config
  namespace: production
data:
  production.yaml: |
    common:
      error_handling:
        detailed_errors: false

    thread:
      pool_size: 16

    logger:
      default_level: info
      async:
        enabled: true

    monitoring:
      prometheus:
        enabled: true
        port: 9091

    # ... (rest of config)
```

**Secret for Sensitive Data**:
```yaml
# secret.yaml
apiVersion: v1
kind: Secret
metadata:
  name: app-secrets
  namespace: production
type: Opaque
stringData:
  DB_PASSWORD: "super-secret-password"
  API_KEY: "api-key-value"
```

**Deployment**:
```yaml
# deployment.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: my-app
  namespace: production
  labels:
    app: my-app
    version: v1.0.0
spec:
  replicas: 3
  strategy:
    type: RollingUpdate
    rollingUpdate:
      maxSurge: 1
      maxUnavailable: 0  # Zero-downtime deployment

  selector:
    matchLabels:
      app: my-app

  template:
    metadata:
      labels:
        app: my-app
        version: v1.0.0
      annotations:
        prometheus.io/scrape: "true"
        prometheus.io/port: "9091"
        prometheus.io/path: "/metrics"

    spec:
      # Security context
      securityContext:
        runAsNonRoot: true
        runAsUser: 1000
        fsGroup: 1000

      containers:
      - name: app
        image: registry.example.com/my-app:1.0.0
        imagePullPolicy: IfNotPresent

        ports:
        - name: http
          containerPort: 8080
          protocol: TCP
        - name: health
          containerPort: 9090
          protocol: TCP
        - name: metrics
          containerPort: 9091
          protocol: TCP

        # Environment variables
        env:
        - name: LOG_LEVEL
          value: "info"
        - name: DB_PASSWORD
          valueFrom:
            secretKeyRef:
              name: app-secrets
              key: DB_PASSWORD
        - name: POD_NAME
          valueFrom:
            fieldRef:
              fieldPath: metadata.name
        - name: POD_NAMESPACE
          valueFrom:
            fieldRef:
              fieldPath: metadata.namespace

        # Volume mounts
        volumeMounts:
        - name: config
          mountPath: /etc/app/config
          readOnly: true
        - name: logs
          mountPath: /var/log/app

        # Resource limits
        resources:
          requests:
            cpu: 1000m      # 1 CPU core
            memory: 2Gi
          limits:
            cpu: 2000m      # 2 CPU cores
            memory: 4Gi

        # Liveness probe (is container alive?)
        livenessProbe:
          httpGet:
            path: /health
            port: 9090
          initialDelaySeconds: 10
          periodSeconds: 10
          timeoutSeconds: 3
          failureThreshold: 3

        # Readiness probe (can it serve traffic?)
        readinessProbe:
          httpGet:
            path: /health
            port: 9090
          initialDelaySeconds: 5
          periodSeconds: 5
          timeoutSeconds: 2
          failureThreshold: 2

        # Graceful shutdown
        lifecycle:
          preStop:
            exec:
              command: ["/bin/sh", "-c", "sleep 15"]  # Allow time for connection draining

      # Volumes
      volumes:
      - name: config
        configMap:
          name: app-config
      - name: logs
        emptyDir: {}

      # Pod anti-affinity (spread across nodes)
      affinity:
        podAntiAffinity:
          preferredDuringSchedulingIgnoredDuringExecution:
          - weight: 100
            podAffinityTerm:
              labelSelector:
                matchLabels:
                  app: my-app
              topologyKey: kubernetes.io/hostname
```

**Service (Load Balancer)**:
```yaml
# service.yaml
apiVersion: v1
kind: Service
metadata:
  name: my-app
  namespace: production
spec:
  type: LoadBalancer
  selector:
    app: my-app
  ports:
  - name: http
    port: 80
    targetPort: 8080
    protocol: TCP
  sessionAffinity: ClientIP  # Sticky sessions
```

**Horizontal Pod Autoscaler**:
```yaml
# hpa.yaml
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: my-app-hpa
  namespace: production
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: my-app
  minReplicas: 3
  maxReplicas: 20
  metrics:
  - type: Resource
    resource:
      name: cpu
      target:
        type: Utilization
        averageUtilization: 70  # Scale when CPU > 70%
  - type: Resource
    resource:
      name: memory
      target:
        type: Utilization
        averageUtilization: 80  # Scale when memory > 80%
  behavior:
    scaleDown:
      stabilizationWindowSeconds: 300  # Wait 5 min before scaling down
      policies:
      - type: Percent
        value: 50
        periodSeconds: 60  # Max 50% scale-down per minute
```

**Deployment Commands**:
```bash
# Create namespace
kubectl apply -f namespace.yaml

# Create ConfigMap and Secret
kubectl apply -f configmap.yaml
kubectl apply -f secret.yaml

# Deploy application
kubectl apply -f deployment.yaml
kubectl apply -f service.yaml
kubectl apply -f hpa.yaml

# Check status
kubectl -n production get pods
kubectl -n production get svc
kubectl -n production get hpa

# View logs
kubectl -n production logs -f deployment/my-app

# Scale manually
kubectl -n production scale deployment my-app --replicas=5
```

---

### Container Optimization

**Image Size Optimization**:
```dockerfile
# Use Alpine for minimal size (trade-off: musl libc compatibility)
FROM alpine:3.18 AS runtime

# Or use distroless (no shell, smallest attack surface)
FROM gcr.io/distroless/cc-debian11
COPY --from=builder /app/build/my_app /app
ENTRYPOINT ["/app"]
```

**Build Cache Optimization**:
```dockerfile
# Order layers from least to most frequently changing
# 1. OS dependencies (changes rarely)
RUN apt-get update && apt-get install -y ...

# 2. Third-party libraries (changes occasionally)
COPY third_party/ /app/third_party/
RUN cd /app/third_party && make install

# 3. Application dependencies (changes frequently)
COPY CMakeLists.txt /app/
RUN cmake -B /app/build ...

# 4. Source code (changes most frequently)
COPY src/ /app/src/
RUN cmake --build /app/build
```

**Security Hardening**:
```dockerfile
# Scan image for vulnerabilities
# Use tools: trivy, snyk, clair

# Run as non-root
USER 1000:1000

# Read-only root filesystem
# (requires writable /tmp and /var/log via volumes)
```

**Resource Limits (Kubernetes)**:
```yaml
resources:
  requests:
    cpu: "1"       # Guaranteed CPU
    memory: 2Gi    # Guaranteed memory
  limits:
    cpu: "2"       # Max CPU (can burst up to 2 cores)
    memory: 4Gi    # Max memory (OOMKilled if exceeded)
```

---

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
1. **Inefficient algorithm**: O(n²) in hot path
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

## 6. Security Hardening

### Security Checklist

**Application Security**:
- [ ] All inputs validated (SQL injection, XSS, command injection)
- [ ] Parameterized SQL queries (no string concatenation)
- [ ] Output encoding for HTML/JavaScript/SQL
- [ ] CSRF protection enabled
- [ ] Rate limiting on all endpoints
- [ ] Authentication required for sensitive endpoints
- [ ] Authorization checks on all resources
- [ ] Secrets never hardcoded (use env vars or secret managers)
- [ ] Error messages don't leak sensitive information
- [ ] Logging doesn't log passwords, tokens, or PII

**Container Security**:
- [ ] Non-root user (USER 1000)
- [ ] Read-only root filesystem (where possible)
- [ ] Minimal base image (Alpine/distroless)
- [ ] No secrets in image layers
- [ ] Image scanned for vulnerabilities (Trivy, Snyk)
- [ ] Drop all capabilities, add only required ones
- [ ] Resource limits set (CPU, memory)
- [ ] Security context configured

**Network Security**:
- [ ] TLS 1.2+ for all external connections
- [ ] Strong cipher suites only
- [ ] Certificate validation enabled
- [ ] CORS configured properly
- [ ] Firewall rules restrict access
- [ ] Network policies in Kubernetes
- [ ] Service mesh for mTLS (Istio/Linkerd)

**Kubernetes Security**:
- [ ] RBAC enabled and configured
- [ ] Pod Security Standards enforced (restricted)
- [ ] Network policies restrict pod-to-pod traffic
- [ ] Secrets encrypted at rest
- [ ] No privileged pods
- [ ] AppArmor/SELinux profiles applied
- [ ] Audit logging enabled
- [ ] Admission controllers configured (OPA/Kyverno)

---

### Network Security

**TLS Configuration**:
```cpp
#include <network_system/tls_server.hpp>

auto server = kcenon::TlsServer({
    .bind_address = "0.0.0.0",
    .port = 8443,

    // Certificate and key
    .cert_file = "/etc/ssl/certs/server.crt",
    .key_file = "/etc/ssl/private/server.key",
    .ca_file = "/etc/ssl/certs/ca.crt",  // Client cert validation

    // TLS version
    .min_version = kcenon::TlsVersion::TLS_1_2,
    .max_version = kcenon::TlsVersion::TLS_1_3,

    // Cipher suites (strong only)
    .ciphers = {
        "TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384",
        "TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256",
        "TLS_ECDHE_RSA_WITH_CHACHA20_POLY1305"
    },

    // Client certificate validation
    .verify_client = true,
    .verify_depth = 3
});

server.start();
```

**Kubernetes Network Policy**:
```yaml
# network-policy.yaml
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: my-app-policy
  namespace: production
spec:
  podSelector:
    matchLabels:
      app: my-app

  policyTypes:
  - Ingress
  - Egress

  # Ingress: only from ingress controller
  ingress:
  - from:
    - namespaceSelector:
        matchLabels:
          name: ingress-nginx
    ports:
    - protocol: TCP
      port: 8080

  # Egress: only to database and external APIs
  egress:
  # Allow DNS
  - to:
    - namespaceSelector: {}
    ports:
    - protocol: UDP
      port: 53

  # Allow database
  - to:
    - podSelector:
        matchLabels:
          app: postgres
    ports:
    - protocol: TCP
      port: 5432

  # Allow external HTTPS
  - to:
    - namespaceSelector: {}
    ports:
    - protocol: TCP
      port: 443
```

---

### Authentication and Authorization

**JWT Authentication**:
```cpp
#include <jwt-cpp/jwt.h>

class AuthMiddleware {
public:
    bool verify_token(const std::string& token) {
        try {
            auto decoded = jwt::decode(token);

            auto verifier = jwt::verify()
                .allow_algorithm(jwt::algorithm::hs256{"secret"})
                .with_issuer("my-app")
                .with_audience("api");

            verifier.verify(decoded);

            // Check expiration
            auto exp = decoded.get_expires_at();
            if (exp < std::chrono::system_clock::now()) {
                return false;
            }

            return true;
        } catch (const std::exception& e) {
            logger->error("Token verification failed", {{"error", e.what()}});
            return false;
        }
    }

    std::optional<std::string> get_user_id(const std::string& token) {
        auto decoded = jwt::decode(token);
        return decoded.get_payload_claim("user_id").as_string();
    }
};

// HTTP middleware
void handle_request(const Request& req) {
    auto auth_header = req.get_header("Authorization");
    if (!auth_header || !auth_header->starts_with("Bearer ")) {
        return HttpResponse(401, "Unauthorized");
    }

    auto token = auth_header->substr(7);  // Remove "Bearer "
    if (!auth_middleware->verify_token(token)) {
        return HttpResponse(401, "Invalid token");
    }

    auto user_id = auth_middleware->get_user_id(token);

    // ... process request with user_id ...
}
```

**RBAC (Role-Based Access Control)**:
```cpp
class AuthorizationService {
public:
    bool has_permission(const std::string& user_id, const std::string& resource, const std::string& action) {
        // Get user roles from database
        auto roles = db_->execute("SELECT role FROM user_roles WHERE user_id = ?", user_id);

        for (const auto& role : roles) {
            // Check if role has permission
            auto perms = db_->execute(
                "SELECT 1 FROM role_permissions "
                "WHERE role = ? AND resource = ? AND action = ?",
                role, resource, action
            );

            if (!perms.empty()) {
                return true;
            }
        }

        return false;
    }
};

// Check permission before action
if (!authz->has_permission(user_id, "orders", "delete")) {
    return HttpResponse(403, "Forbidden");
}
```

---

### Secrets Management

**Kubernetes Secrets**:
```yaml
# secret.yaml
apiVersion: v1
kind: Secret
metadata:
  name: app-secrets
  namespace: production
type: Opaque
stringData:
  DATABASE_PASSWORD: "super-secret-password"
  API_KEY: "api-key-value"
  JWT_SECRET: "jwt-signing-secret"
```

```yaml
# Use in deployment
env:
- name: DATABASE_PASSWORD
  valueFrom:
    secretKeyRef:
      name: app-secrets
      key: DATABASE_PASSWORD
```

**HashiCorp Vault Integration**:
```cpp
#include <vault/vault.hpp>

class SecretsManager {
public:
    std::string get_secret(const std::string& path) {
        // Authenticate with Vault
        auto vault_token = std::getenv("VAULT_TOKEN");
        auto vault_addr = std::getenv("VAULT_ADDR");

        vault::Client client(vault_addr, vault_token);

        // Read secret
        auto response = client.read_secret(path);
        return response["data"]["value"];
    }
};

// Usage
auto db_password = secrets->get_secret("database/production/password");
auto api_key = secrets->get_secret("external/api/key");
```

**External Secrets Operator (Kubernetes)**:
```yaml
# external-secret.yaml
apiVersion: external-secrets.io/v1beta1
kind: ExternalSecret
metadata:
  name: app-secrets
  namespace: production
spec:
  refreshInterval: 1h
  secretStoreRef:
    name: vault-backend
    kind: SecretStore

  target:
    name: app-secrets
    creationPolicy: Owner

  data:
  - secretKey: DATABASE_PASSWORD
    remoteRef:
      key: database/production
      property: password

  - secretKey: API_KEY
    remoteRef:
      key: external/api
      property: key
```

---

## 7. Upgrade and Rollback

### Version Compatibility

**Semantic Versioning**:
```
MAJOR.MINOR.PATCH

MAJOR: Breaking changes (incompatible API changes)
MINOR: New features (backward compatible)
PATCH: Bug fixes (backward compatible)

Examples:
1.0.0 → 1.0.1  Safe (patch)
1.0.1 → 1.1.0  Safe (minor)
1.1.0 → 2.0.0  Breaking (major) - requires migration
```

**Compatibility Matrix**:
```
| Version | Compatible With | Breaking Changes | Migration Required |
|---------|----------------|------------------|-------------------|
| 1.0.x   | 1.0.0+         | None             | No                |
| 1.1.x   | 1.0.0+         | None             | No                |
| 2.0.x   | 2.0.0+         | Config schema    | Yes (config)      |
| 2.1.x   | 2.0.0+         | None             | No                |
```

**Upgrade Path**:
```
1.0.x → 1.1.x: Direct upgrade
1.0.x → 2.0.x: Requires config migration
1.0.x → 2.1.x: Upgrade to 2.0.x first, then 2.1.x
```

---

### Upgrade Procedures

**Pre-Upgrade Checklist**:
- [ ] Review changelog for breaking changes
- [ ] Backup database
- [ ] Backup configuration files
- [ ] Test upgrade in staging environment
- [ ] Notify users of planned maintenance (if downtime)
- [ ] Prepare rollback plan
- [ ] Monitor resources (disk space, memory)

**Blue-Green Deployment (Zero Downtime)**:
```bash
# Step 1: Deploy new version (green) alongside old (blue)
kubectl apply -f deployment-v2.yaml

# deployment-v2.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: my-app-v2  # Different name
  labels:
    version: v2
spec:
  replicas: 3
  template:
    metadata:
      labels:
        app: my-app
        version: v2
    spec:
      containers:
      - name: app
        image: my-app:2.0.0  # New version

# Step 2: Wait for new pods to be ready
kubectl wait --for=condition=Ready pod -l version=v2 --timeout=300s

# Step 3: Switch traffic to new version
kubectl patch service my-app -p '{"spec":{"selector":{"version":"v2"}}}'

# Step 4: Monitor for errors
# If all good:
kubectl delete deployment my-app-v1  # Remove old version

# If errors:
kubectl patch service my-app -p '{"spec":{"selector":{"version":"v1"}}}'  # Rollback
```

**Rolling Update (Kubernetes Default)**:
```yaml
# deployment.yaml
spec:
  replicas: 10
  strategy:
    type: RollingUpdate
    rollingUpdate:
      maxSurge: 2        # Create 2 extra pods during update
      maxUnavailable: 1  # Max 1 pod can be unavailable

# Update image
kubectl set image deployment/my-app app=my-app:2.0.0

# Monitor rollout
kubectl rollout status deployment/my-app

# Pause rollout if issues
kubectl rollout pause deployment/my-app

# Resume
kubectl rollout resume deployment/my-app
```

**Database Migration**:
```bash
# Step 1: Backup database
pg_dump production_db > backup_$(date +%Y%m%d).sql

# Step 2: Run migrations
./migrate --database postgresql://user:pass@host/db up

# Step 3: Verify migration
./migrate --database postgresql://user:pass@host/db version

# Step 4: Deploy application

# Rollback migration if needed
./migrate --database postgresql://user:pass@host/db down 1
```

---

### Rollback Procedures

**Kubernetes Rollback**:
```bash
# View rollout history
kubectl rollout history deployment/my-app

# Rollback to previous version
kubectl rollout undo deployment/my-app

# Rollback to specific revision
kubectl rollout undo deployment/my-app --to-revision=3

# Check rollback status
kubectl rollout status deployment/my-app
```

**Database Rollback**:
```bash
# Restore from backup
psql production_db < backup_20250101.sql

# Or rollback specific migration
./migrate --database postgresql://user:pass@host/db down 1
```

**Configuration Rollback**:
```bash
# Git-based configuration (recommended)
# Rollback ConfigMap
kubectl apply -f config/v1.0.0/configmap.yaml

# Restart pods to pick up old config
kubectl rollout restart deployment/my-app
```

---

### Zero-Downtime Upgrades

**Strategy 1: Blue-Green Deployment**

See [Upgrade Procedures](#upgrade-procedures) above.

**Strategy 2: Canary Deployment**

```yaml
# Canary deployment (10% traffic to new version)
apiVersion: v1
kind: Service
metadata:
  name: my-app
spec:
  selector:
    app: my-app  # Matches both v1 and v2
  ports:
  - port: 80
    targetPort: 8080

---
# Old version (90% of pods)
apiVersion: apps/v1
kind: Deployment
metadata:
  name: my-app-v1
spec:
  replicas: 9  # 90%
  template:
    metadata:
      labels:
        app: my-app
        version: v1

---
# New version (10% of pods)
apiVersion: apps/v1
kind: Deployment
metadata:
  name: my-app-v2
spec:
  replicas: 1  # 10%
  template:
    metadata:
      labels:
        app: my-app
        version: v2

# Monitor metrics for canary
# If good: increase v2 replicas, decrease v1 replicas
# Repeat until 100% v2
```

**Strategy 3: Feature Flags**

```cpp
class FeatureFlags {
public:
    bool is_enabled(const std::string& feature, const std::string& user_id) {
        // Check if feature is enabled for user
        auto percentage = get_rollout_percentage(feature);
        auto user_hash = std::hash<std::string>{}(user_id) % 100;

        return user_hash < percentage;
    }

private:
    int get_rollout_percentage(const std::string& feature) {
        // Read from config or feature flag service
        return config_->get<int>("features." + feature + ".rollout_percentage", 0);
    }
};

// Usage
if (feature_flags->is_enabled("new_algorithm", user_id)) {
    return new_algorithm(data);  // New version
} else {
    return old_algorithm(data);  // Old version
}
```

---

## Related Documentation

- [INTEGRATION_GUIDE.md](INTEGRATION_GUIDE.md) - Cross-system integration patterns
- [performance/E2E_BENCHMARKS.md](performance/E2E_BENCHMARKS.md) - End-to-end performance benchmarks
- [PRODUCTION_QUALITY.md](PRODUCTION_QUALITY.md) - Production quality standards
- [ADAPTER_GUIDE.md](ADAPTER_GUIDE.md) - Adapter framework for service integration
- [ERROR_CODES.md](ERROR_CODES.md) - Error handling and codes

**System-Specific Documentation**:
- [thread_system/README.md](../../thread_system/README.md) - Thread pool configuration
- [logger_system/README.md](../../logger_system/README.md) - Logging configuration
- [database_system/README.md](../../database_system/README.md) - Database connection pooling
- [network_system/README.md](../../network_system/README.md) - Network server configuration
- [monitoring_system/README.md](../../monitoring_system/README.md) - Metrics and health checks

---

**Version**: 1.0.0
**Last Updated**: 2025-12-03
**Maintainer**: kcenon team
