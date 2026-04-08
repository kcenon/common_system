# Production Deployment Guide - Production Configuration

> **SSOT**: This document is part of the [Production Deployment Guide](PRODUCTION_GUIDE.md).

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

**Version**: 1.0.0
**Last Updated**: 2025-12-03
**Maintainer**: kcenon team
