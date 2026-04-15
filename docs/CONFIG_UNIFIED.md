# Configuration Subsystem Guide

> **SSOT**: This document is the single source of truth for **Configuration Subsystem Guide**.

<!-- TODO: target file does not exist -->
> **Language:** **English** | [한국어](CONFIG_GUIDE.kr.md)

**Complete Guide**: Unified Configuration, Hot-Reload File Watching, CLI Argument Parsing, and Configuration Loading

**Status**: Complete

This guide provides comprehensive documentation for the configuration subsystem in common_system, including hot-reload file watching (`config_watcher`), unified configuration management (`unified_config`), and CLI argument parsing (`cli_config_parser`).

---

## Table of Contents

- [Overview](#overview)
- [Configuration Architecture](#1-configuration-architecture)
  - [Configuration Source Priority](#configuration-source-priority)
  - [Hot-Reload Mechanism](#hot-reload-mechanism)
  - [Architecture Diagram](#architecture-diagram)
- [Unified Config](#2-unified-config-unified_configh)
  - [Configuration Schema](#configuration-schema)
  - [Sub-Structs and Fields](#sub-structs-and-fields)
  - [Default Values](#default-values)
  - [Hot-Reloadable Fields](#hot-reloadable-fields)
- [Environment Variable Reference](#3-environment-variable-reference)
- [YAML Configuration Examples](#4-yaml-configuration-examples)
  - [Minimal Configuration](#minimal-configuration)
  - [Production Configuration](#production-configuration)
  - [Environment Variable Substitution](#environment-variable-substitution)

---

## Overview

The configuration subsystem provides a robust, production-ready solution for managing application configuration with the following features:

### Key Features

| Feature | Description | Header File |
|---------|-------------|-------------|
| **Unified Configuration** | Type-safe hierarchical configuration schema | `unified_config.h` |
| **Hot-Reload** | Automatic configuration reload on file changes | `config_watcher.h` |
| **CLI Parsing** | Command-line argument parsing and overrides | `cli_config_parser.h` |
| **YAML Loading** | YAML file loading with validation | `config_loader.h` |
| **Environment Variables** | Environment variable overrides and substitution | `config_loader.h` |
| **Validation** | Schema validation with error reporting | `config_loader.h` |
| **Version Tracking** | Configuration version history and rollback | `config_watcher.h` |

### Configuration Files

The configuration subsystem is located at `include/kcenon/common/config/`:

- `unified_config.h` — Unified configuration schema (7 sub-structs)
- `config_watcher.h` — File system monitoring for config hot-reload
- `cli_config_parser.h` — Command-line argument parser
- `config_loader.h` — Configuration file loading and validation

---

## 1. Configuration Architecture

### Configuration Source Priority

Configuration is loaded and merged from multiple sources with the following priority (highest to lowest):

```
┌──────────────────────────────────────────────────┐
│ 1. CLI Arguments (--set key=value)              │  ← Highest Priority
├──────────────────────────────────────────────────┤
│ 2. Environment Variables (UNIFIED_*)            │
├──────────────────────────────────────────────────┤
│ 3. Configuration File (YAML)                    │
├──────────────────────────────────────────────────┤
│ 4. Default Values (in code)                     │  ← Lowest Priority
└──────────────────────────────────────────────────┘
```

**Example**: If `logger.level` is set via CLI (`--set logger.level=debug`), environment variable (`UNIFIED_LOGGER_LEVEL=info`), and YAML file (`level: warn`), the CLI value (`debug`) takes precedence.

### Hot-Reload Mechanism

The `config_watcher` monitors configuration files for changes and automatically reloads the configuration:

```
┌─────────────────────────────────────────────────┐
│  File System Events                             │
│  (inotify/kqueue/ReadDirectoryChangesW)         │
└────────────────┬────────────────────────────────┘
                 │ File Modified
                 ▼
┌─────────────────────────────────────────────────┐
│  Debouncing (100ms delay)                       │
└────────────────┬────────────────────────────────┘
                 │ Stable Change Detected
                 ▼
┌─────────────────────────────────────────────────┐
│  Load New Configuration                         │
└────────────────┬────────────────────────────────┘
                 │ Validation
                 ▼
┌─────────────────────────────────────────────────┐
│  Check Hot-Reloadable Fields                    │
│  (Only some fields can be hot-reloaded)         │
└────────────────┬────────────────────────────────┘
                 │ Validation Passed
                 ▼
┌─────────────────────────────────────────────────┐
│  Swap Configuration (Thread-Safe)               │
└────────────────┬────────────────────────────────┘
                 │ Notify Callbacks
                 ▼
┌─────────────────────────────────────────────────┐
│  Application Callbacks Invoked                  │
│  (with old_config and new_config)               │
└─────────────────────────────────────────────────┘
```

**Note**: Some configuration fields are **not hot-reloadable** (e.g., thread pool size) because they require application restart to take effect.

### Architecture Diagram

The configuration subsystem integrates with the broader kcenon ecosystem:

```
┌────────────────────────────────────────────────────────────┐
│                    Application                             │
└──────────────┬─────────────────────────────────────────────┘
               │ Uses
               ▼
┌────────────────────────────────────────────────────────────┐
│              unified_config                                │
│  ┌──────────┬──────────┬──────────┬──────────┬──────────┐ │
│  │  thread  │  logger  │monitoring│ database │ network  │ │
│  └──────────┴──────────┴──────────┴──────────┴──────────┘ │
└──────────────┬─────────────────────────────────────────────┘
               │ Loaded by
               ▼
┌────────────────────────────────────────────────────────────┐
│              config_loader                                 │
│  • YAML file parsing                                       │
│  • Environment variable substitution                       │
│  • Validation                                              │
└──────────────┬─────────────────────────────────────────────┘
               │ Merges with
        ┌──────┴──────┬───────────────────┐
        ▼             ▼                   ▼
┌───────────────┐ ┌───────────────┐ ┌───────────────┐
│   YAML File   │ │   Env Vars    │ │  CLI Args     │
└───────────────┘ └───────────────┘ └───────────────┘
        │                               ▲
        │ Watched by                    │ Parsed by
        ▼                               │
┌───────────────────────────────────────┴─────────────────┐
│              config_watcher          cli_config_parser  │
│  • File system monitoring                               │
│  • Hot-reload on file change                            │
│  • Version tracking                                     │
│  • Rollback support                                     │
└─────────────────────────────────────────────────────────┘
```

---

## 2. Unified Config (`unified_config.h`)

### Configuration Schema

The `unified_config` structure provides a type-safe schema for all subsystem configurations:

```cpp
namespace kcenon::common::config {

struct unified_config {
    thread_config thread;          // Thread pool configuration
    logger_config logger;          // Logging system configuration
    monitoring_config monitoring;  // Monitoring and tracing
    database_config database;      // Database connection
    network_config network;        // Network and TLS
};

}  // namespace kcenon::common::config
```

### Sub-Structs and Fields

#### 1. `thread_config` — Thread Pool Configuration

Controls the thread pool behavior for concurrent execution.

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `pool_size` | `size_t` | `0` (auto-detect) | Number of worker threads |
| `queue_type` | `std::string` | `"lockfree"` | Queue type: `"mutex"`, `"lockfree"`, `"bounded"` |
| `max_queue_size` | `size_t` | `10000` | Maximum queue size (for bounded queue) |
| `thread_name_prefix` | `std::string` | `"worker"` | Thread naming prefix |

**Hot-Reloadable**: No (Requires restart)

#### 2. `logger_config` — Logging System Configuration

Controls logging behavior and output destinations.

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `level` | `std::string` | `"info"` | Log level: `"trace"`, `"debug"`, `"info"`, `"warn"`, `"error"`, `"critical"`, `"off"` |
| `writers` | `std::vector<std::string>` | `{"console"}` | Writers: `"console"`, `"file"`, `"rotating_file"`, `"network"`, `"json"` |
| `async` | `bool` | `true` | Enable async logging |
| `buffer_size` | `size_t` | `8192` | Async buffer size in bytes |
| `file_path` | `std::string` | `"./logs/app.log"` | Log file path (for file writers) |
| `max_file_size` | `size_t` | `10485760` (10MB) | Maximum file size (for rotating_file) |
| `max_backup_files` | `size_t` | `5` | Maximum backup files (for rotating_file) |
| `format_pattern` | `std::string` | `"[%Y-%m-%d %H:%M:%S.%e] [%l] [%t] %v"` | Log format pattern |

**Hot-Reloadable**: Yes — `level`, `file_path` only

#### 3. `monitoring_config` — Monitoring and Tracing

Controls metrics collection and distributed tracing.

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `enabled` | `bool` | `true` | Enable monitoring |
| `metrics_interval` | `std::chrono::milliseconds` | `5000` | Metrics collection interval |
| `health_check_interval` | `std::chrono::milliseconds` | `30000` | Health check interval |
| `prometheus_port` | `uint16_t` | `9090` | Prometheus metrics port (0 to disable) |
| `prometheus_path` | `std::string` | `"/metrics"` | Prometheus metrics path |
| `tracing` | `tracing_config` | — | Distributed tracing configuration (see below) |

##### `tracing_config` — Distributed Tracing

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `enabled` | `bool` | `false` | Enable tracing |
| `sampling_rate` | `double` | `0.1` | Sampling rate (0.0 to 1.0) |
| `exporter` | `std::string` | `"otlp"` | Exporter type: `"otlp"`, `"jaeger"`, `"zipkin"`, `"console"` |
| `endpoint` | `std::string` | `"http://localhost:4317"` | Exporter endpoint |

**Hot-Reloadable**: Yes — `metrics_interval`, `tracing.sampling_rate`

#### 4. `database_config` — Database Configuration

Controls database connections and connection pooling.

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `backend` | `std::string` | `""` | Backend: `"postgresql"`, `"mysql"`, `"sqlite"`, `"mongodb"`, `"redis"` |
| `connection_string` | `std::string` | `""` | Connection string or URI |
| `log_queries` | `bool` | `false` | Enable query logging |
| `slow_query_threshold` | `std::chrono::milliseconds` | `1000` | Slow query threshold |
| `pool` | `pool_config` | — | Connection pool configuration (see below) |

##### `pool_config` — Connection Pool

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `min_size` | `size_t` | `5` | Minimum pool size |
| `max_size` | `size_t` | `20` | Maximum pool size |
| `idle_timeout` | `std::chrono::milliseconds` | `60000` | Idle connection timeout |
| `acquire_timeout` | `std::chrono::milliseconds` | `5000` | Connection acquisition timeout |

**Hot-Reloadable**: No (Requires restart)

#### 5. `network_config` — Network and TLS

Controls network communication and TLS security.

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `compression` | `std::string` | `"lz4"` | Compression: `"none"`, `"lz4"`, `"gzip"`, `"deflate"`, `"zstd"` |
| `buffer_size` | `size_t` | `65536` | Send/receive buffer size |
| `connect_timeout` | `std::chrono::milliseconds` | `5000` | Connection timeout |
| `io_timeout` | `std::chrono::milliseconds` | `30000` | Read/write timeout |
| `keepalive_interval` | `std::chrono::milliseconds` | `15000` | Keep-alive interval |
| `max_connections` | `size_t` | `10000` | Maximum concurrent connections (server) |
| `tls` | `tls_config` | — | TLS configuration (see below) |

##### `tls_config` — TLS/SSL Configuration

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `enabled` | `bool` | `true` | Enable TLS |
| `version` | `std::string` | `"1.3"` | TLS version: `"1.2"`, `"1.3"` |
| `cert_path` | `std::string` | `""` | Certificate file path |
| `key_path` | `std::string` | `""` | Private key file path |
| `ca_path` | `std::string` | `""` | CA certificate path (for client verification) |
| `verify_peer` | `bool` | `true` | Verify peer certificate |

**Hot-Reloadable**: No (Requires restart)

### Default Values

Get default configuration programmatically:

```cpp
#include <kcenon/common/config/unified_config.h>

using namespace kcenon::common::config;

// Get default configuration
unified_config defaults = unified_config::defaults();
```

### Hot-Reloadable Fields

Only specific fields support hot-reload without application restart:

```cpp
inline bool is_hot_reloadable(const std::string& field_path) {
    static const std::vector<std::string> hot_reloadable_fields = {
        "logger.level",                      // ✅ Log level
        "logger.file_path",                  // ✅ Log file path
        "monitoring.metrics_interval",       // ✅ Metrics interval
        "monitoring.tracing.sampling_rate",  // ✅ Tracing sampling rate
    };
    // All other fields require restart
}
```

**Non-hot-reloadable fields** can still be changed in the configuration file, but they won't take effect until the application restarts.

---

## 3. Environment Variable Reference

All configuration fields can be overridden via environment variables with the `UNIFIED_` prefix:

### Thread Configuration

| Environment Variable | Configuration Field | Example |
|---------------------|---------------------|---------|
| `UNIFIED_THREAD_POOL_SIZE` | `thread.pool_size` | `16` |
| `UNIFIED_THREAD_QUEUE_TYPE` | `thread.queue_type` | `lockfree` |
| `UNIFIED_THREAD_MAX_QUEUE_SIZE` | `thread.max_queue_size` | `10000` |
| `UNIFIED_THREAD_NAME_PREFIX` | `thread.thread_name_prefix` | `worker` |

### Logger Configuration

| Environment Variable | Configuration Field | Example |
|---------------------|---------------------|---------|
| `UNIFIED_LOGGER_LEVEL` | `logger.level` | `debug` |
| `UNIFIED_LOGGER_ASYNC` | `logger.async` | `true` |
| `UNIFIED_LOGGER_BUFFER_SIZE` | `logger.buffer_size` | `8192` |
| `UNIFIED_LOGGER_FILE_PATH` | `logger.file_path` | `/var/log/app.log` |
| `UNIFIED_LOGGER_MAX_FILE_SIZE` | `logger.max_file_size` | `10485760` |
| `UNIFIED_LOGGER_MAX_BACKUP_FILES` | `logger.max_backup_files` | `5` |
| `UNIFIED_LOGGER_FORMAT_PATTERN` | `logger.format_pattern` | `[%H:%M:%S] %v` |
| `UNIFIED_LOGGER_WRITERS` | `logger.writers` | `console,file,json` (comma-separated) |

### Monitoring Configuration

| Environment Variable | Configuration Field | Example |
|---------------------|---------------------|---------|
| `UNIFIED_MONITORING_ENABLED` | `monitoring.enabled` | `true` |
| `UNIFIED_MONITORING_METRICS_INTERVAL_MS` | `monitoring.metrics_interval` | `5000` |
| `UNIFIED_MONITORING_HEALTH_CHECK_INTERVAL_MS` | `monitoring.health_check_interval` | `30000` |
| `UNIFIED_MONITORING_PROMETHEUS_PORT` | `monitoring.prometheus_port` | `9090` |
| `UNIFIED_MONITORING_PROMETHEUS_PATH` | `monitoring.prometheus_path` | `/metrics` |
| `UNIFIED_MONITORING_TRACING_ENABLED` | `monitoring.tracing.enabled` | `true` |
| `UNIFIED_MONITORING_TRACING_SAMPLING_RATE` | `monitoring.tracing.sampling_rate` | `0.1` |
| `UNIFIED_MONITORING_TRACING_EXPORTER` | `monitoring.tracing.exporter` | `otlp` |
| `UNIFIED_MONITORING_TRACING_ENDPOINT` | `monitoring.tracing.endpoint` | `http://localhost:4317` |

### Database Configuration

| Environment Variable | Configuration Field | Example |
|---------------------|---------------------|---------|
| `UNIFIED_DATABASE_BACKEND` | `database.backend` | `postgresql` |
| `UNIFIED_DATABASE_CONNECTION_STRING` | `database.connection_string` | `postgresql://localhost/mydb` |
| `UNIFIED_DATABASE_LOG_QUERIES` | `database.log_queries` | `false` |
| `UNIFIED_DATABASE_SLOW_QUERY_THRESHOLD_MS` | `database.slow_query_threshold` | `1000` |
| `UNIFIED_DATABASE_POOL_MIN_SIZE` | `database.pool.min_size` | `5` |
| `UNIFIED_DATABASE_POOL_MAX_SIZE` | `database.pool.max_size` | `20` |
| `UNIFIED_DATABASE_POOL_IDLE_TIMEOUT_MS` | `database.pool.idle_timeout` | `60000` |
| `UNIFIED_DATABASE_POOL_ACQUIRE_TIMEOUT_MS` | `database.pool.acquire_timeout` | `5000` |

### Network Configuration

| Environment Variable | Configuration Field | Example |
|---------------------|---------------------|---------|
| `UNIFIED_NETWORK_COMPRESSION` | `network.compression` | `lz4` |
| `UNIFIED_NETWORK_BUFFER_SIZE` | `network.buffer_size` | `65536` |
| `UNIFIED_NETWORK_CONNECT_TIMEOUT_MS` | `network.connect_timeout` | `5000` |
| `UNIFIED_NETWORK_IO_TIMEOUT_MS` | `network.io_timeout` | `30000` |
| `UNIFIED_NETWORK_KEEPALIVE_INTERVAL_MS` | `network.keepalive_interval` | `15000` |
| `UNIFIED_NETWORK_MAX_CONNECTIONS` | `network.max_connections` | `10000` |
| `UNIFIED_NETWORK_TLS_ENABLED` | `network.tls.enabled` | `true` |
| `UNIFIED_NETWORK_TLS_VERSION` | `network.tls.version` | `1.3` |
| `UNIFIED_NETWORK_TLS_CERT_PATH` | `network.tls.cert_path` | `/etc/ssl/cert.pem` |
| `UNIFIED_NETWORK_TLS_KEY_PATH` | `network.tls.key_path` | `/etc/ssl/key.pem` |
| `UNIFIED_NETWORK_TLS_CA_PATH` | `network.tls.ca_path` | `/etc/ssl/ca.pem` |
| `UNIFIED_NETWORK_TLS_VERIFY_PEER` | `network.tls.verify_peer` | `true` |

**Example usage**:

```bash
# Set environment variables
export UNIFIED_LOGGER_LEVEL=debug
export UNIFIED_THREAD_POOL_SIZE=16
export UNIFIED_MONITORING_ENABLED=true

# Run application (loads from environment)
./myapp
```

---

## 4. YAML Configuration Examples

### Minimal Configuration

```yaml
# config.yaml — Minimal configuration (uses defaults for most fields)

logger:
  level: info
  writers:
    - console

monitoring:
  enabled: true
```

### Production Configuration

```yaml
# config.yaml — Production configuration

# Thread system
thread:
  pool_size: 16              # 16 worker threads
  queue_type: lockfree       # Lock-free queue for performance
  max_queue_size: 50000      # Large queue for burst traffic

# Logger system
logger:
  level: info
  writers:
    - rotating_file          # Use rotating file writer
    - json                   # JSON format for log aggregation
  async: true
  buffer_size: 16384         # 16KB buffer
  file_path: /var/log/myapp/app.log
  max_file_size: 104857600   # 100MB
  max_backup_files: 10
  format_pattern: "[%Y-%m-%d %H:%M:%S.%e] [%l] [%t] %v"

# Monitoring system
monitoring:
  enabled: true
  metrics_interval_ms: 5000
  health_check_interval_ms: 30000
  prometheus_port: 9090
  prometheus_path: /metrics

  tracing:
    enabled: true
    sampling_rate: 0.1       # Sample 10% of requests
    exporter: otlp           # OpenTelemetry Protocol
    endpoint: http://jaeger:4317

# Database system
database:
  backend: postgresql
  connection_string: postgresql://user:pass@localhost:5432/mydb
  log_queries: false
  slow_query_threshold_ms: 1000

  pool:
    min_size: 10
    max_size: 50
    idle_timeout_ms: 300000  # 5 minutes
    acquire_timeout_ms: 5000

# Network system
network:
  compression: lz4
  buffer_size: 65536
  connect_timeout_ms: 5000
  io_timeout_ms: 30000
  keepalive_interval_ms: 15000
  max_connections: 10000

  tls:
    enabled: true
    version: "1.3"
    cert_path: /etc/ssl/certs/server.crt
    key_path: /etc/ssl/private/server.key
    ca_path: /etc/ssl/certs/ca.crt
    verify_peer: true
```

### Environment Variable Substitution

```yaml
# config.yaml — Using environment variable substitution

logger:
  level: ${LOG_LEVEL}        # Expands to UNIFIED_LOGGER_LEVEL env var
  file_path: ${LOG_DIR}/app.log

database:
  connection_string: ${DATABASE_URL}

network:
  tls:
    cert_path: ${SSL_CERT_PATH}
    key_path: ${SSL_KEY_PATH}
    ca_path: ${SSL_CA_PATH}
```

**Shell**:

```bash
export LOG_LEVEL=debug
export LOG_DIR=/var/log/myapp
export DATABASE_URL=postgresql://localhost/mydb
export SSL_CERT_PATH=/etc/ssl/certs/server.crt
export SSL_KEY_PATH=/etc/ssl/private/server.key
export SSL_CA_PATH=/etc/ssl/certs/ca.crt

./myapp --config=config.yaml
```

---

## Related Documentation

- [Config Watcher](CONFIG_WATCHER.md) — Hot-reload file watching and patterns
- [CLI Config Parser](CONFIG_CLI_PARSER.md) — Command-line argument parsing
- [Config Loader](CONFIG_LOADER.md) — YAML loading, validation, and troubleshooting
- [API Reference](API_REFERENCE.md) — Complete API documentation for all systems
- [Integration Guide](INTEGRATION_GUIDE.md) — Cross-system integration patterns
- [Architecture](ARCHITECTURE.md) — System architecture and design principles
- [Best Practices](BEST_PRACTICES.md) — Recommended usage patterns

### Configuration-Related Headers

- `include/kcenon/common/config/unified_config.h` — Configuration schema
- `include/kcenon/common/config/config_watcher.h` — Hot-reload file watching
- `include/kcenon/common/config/cli_config_parser.h` — CLI argument parsing
- `include/kcenon/common/config/config_loader.h` — YAML loading and validation

### External Resources

- [YAML Specification](https://yaml.org/spec/) — YAML format reference
- [yaml-cpp Documentation](https://github.com/jbeder/yaml-cpp) — YAML parser library
- [OpenTelemetry](https://opentelemetry.io/) — Distributed tracing

---

**Version**: 1.0.0
**Last Updated**: 2026-02-08
**License**: BSD 3-Clause (see LICENSE file)
