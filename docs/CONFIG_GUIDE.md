# Configuration Subsystem Guide

> **Language:** **English** | [한국어](CONFIG_GUIDE.kr.md)

**Complete Guide**: Unified Configuration, Hot-Reload File Watching, CLI Argument Parsing, and Configuration Loading

**Status**: ✅ **Complete**

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
- [Config Watcher](#3-config-watcher-config_watcherh)
  - [File System Monitoring](#file-system-monitoring)
  - [Debouncing and Rate Limiting](#debouncing-and-rate-limiting)
  - [Callback Registration](#callback-registration)
  - [Watched Path Management](#watched-path-management)
  - [Thread Safety](#thread-safety)
  - [Version Tracking](#version-tracking)
  - [Rollback Support](#rollback-support)
- [CLI Config Parser](#4-cli-config-parser-cli_config_parserh)
  - [Argument Format](#argument-format)
  - [Boolean Flags](#boolean-flags)
  - [List Arguments](#list-arguments)
  - [Help Text Generation](#help-text-generation)
  - [Required vs Optional Arguments](#required-vs-optional-arguments)
- [Config Loader](#5-config-loader-config_loaderh)
  - [File Format Support](#file-format-support)
  - [Error Handling](#error-handling)
  - [Schema Validation](#schema-validation)
  - [Environment Variable Substitution](#environment-variable-substitution)
- [Complete Usage Example](#6-complete-usage-example)
  - [Basic Example](#basic-example)
  - [Full-Featured Example](#full-featured-example)
  - [Hot-Reload Callback Patterns](#hot-reload-callback-patterns)
- [Environment Variable Reference](#7-environment-variable-reference)
- [YAML Configuration Examples](#8-yaml-configuration-examples)
  - [Minimal Configuration](#minimal-configuration)
  - [Production Configuration](#production-configuration)
  - [Environment Variable Substitution](#environment-variable-substitution-1)
- [Hot-Reload Patterns](#9-hot-reload-patterns)
  - [Basic Hot-Reload](#basic-hot-reload)
  - [Selective Hot-Reload](#selective-hot-reload)
  - [Hot-Reload with Validation](#hot-reload-with-validation)
- [Troubleshooting](#10-troubleshooting)
- [Related Documentation](#related-documentation)

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

**Hot-Reloadable**: ❌ (Requires restart)

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

**Hot-Reloadable**: ✅ `level`, `file_path` only

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

**Hot-Reloadable**: ✅ `metrics_interval`, `tracing.sampling_rate`

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

**Hot-Reloadable**: ❌ (Requires restart)

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

**Hot-Reloadable**: ❌ (Requires restart)

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

## 3. Config Watcher (`config_watcher.h`)

The `config_watcher` provides automatic configuration hot-reload by monitoring configuration files for changes.

### File System Monitoring

The watcher uses platform-native file system event APIs:

| Platform | API | Supported Events |
|----------|-----|------------------|
| **Linux** | `inotify` | `IN_MODIFY`, `IN_CREATE`, `IN_MOVED_TO`, `IN_CLOSE_WRITE` |
| **macOS/BSD** | `kqueue` | `NOTE_WRITE`, `NOTE_EXTEND`, `NOTE_RENAME`, `NOTE_DELETE` |
| **Windows** | `ReadDirectoryChangesW` | `FILE_NOTIFY_CHANGE_LAST_WRITE`, `FILE_NOTIFY_CHANGE_FILE_NAME` |

**Cross-platform support**: The watcher automatically selects the appropriate API for the current platform.

### Debouncing and Rate Limiting

To prevent excessive reloads from rapid file system events (e.g., text editor auto-save), the watcher implements:

1. **100ms debounce delay**: Waits 100ms after a file change before reloading
2. **Event coalescing**: Multiple events within the debounce window trigger only one reload

Example timeline:

```
t=0ms    → File modified (event 1)
t=50ms   → File modified (event 2, debounce resets)
t=100ms  → File modified (event 3, debounce resets)
t=200ms  → Debounce timeout, reload triggered (only once)
```

### Callback Registration

Register callbacks to be notified of configuration changes:

```cpp
#include <kcenon/common/config/config_watcher.h>

using namespace kcenon::common::config;

config_watcher watcher("config.yaml");

// Register change callback
watcher.on_change([](const unified_config& old_cfg,
                     const unified_config& new_cfg) {
    std::cout << "Configuration updated\n";

    // Compare configurations
    if (old_cfg.logger.level != new_cfg.logger.level) {
        std::cout << "Log level changed: "
                  << old_cfg.logger.level << " → "
                  << new_cfg.logger.level << "\n";
    }

    // Apply changes
    // ...
});

// Register error callback
watcher.on_error([](const std::string& error_message) {
    std::cerr << "Config reload failed: " << error_message << "\n";
});

// Start watching
auto result = watcher.start();
if (result.is_err()) {
    std::cerr << "Failed to start watcher: "
              << result.error().message << "\n";
}
```

**Multiple callbacks**: You can register multiple callbacks. All callbacks are invoked in the order they were registered.

### Watched Path Management

The watcher monitors a single configuration file specified at construction:

```cpp
// Watch a specific file
config_watcher watcher("/etc/myapp/config.yaml");

// Get the watched path
std::string path = watcher.config_path();
```

**Directory watching**: The watcher monitors the parent directory to handle file deletion and recreation (common with atomic file writes).

### Thread Safety

The `config_watcher` is thread-safe:

- **Configuration access**: Uses `std::shared_mutex` for reader-writer locking
- **Callback invocation**: Callbacks are invoked sequentially in a separate thread
- **Multiple threads can read** the current configuration simultaneously
- **Only the watch thread can update** the configuration

```cpp
// Thread-safe read access
const unified_config& current = watcher.current();

// Thread-safe version access
uint64_t version = watcher.version();
```

### Version Tracking

Each configuration change increments a version counter:

```cpp
// Get current version
uint64_t version = watcher.version();

// Get configuration history
std::vector<config_snapshot> history = watcher.history(5);  // Last 5 versions

for (const auto& snapshot : history) {
    std::cout << "Version " << snapshot.version << ": "
              << std::chrono::system_clock::to_time_t(snapshot.timestamp)
              << "\n";
}
```

**History retention**: By default, the watcher keeps the last 10 configuration snapshots. This can be configured at construction:

```cpp
config_watcher watcher("config.yaml", 20);  // Keep last 20 versions
```

### Rollback Support

Rollback to a previous configuration version:

```cpp
// Get configuration history
auto history = watcher.history();

if (!history.empty()) {
    uint64_t target_version = history[1].version;  // Previous version

    auto result = watcher.rollback(target_version);
    if (result.is_ok()) {
        std::cout << "Rolled back to version " << target_version << "\n";
    } else {
        std::cerr << "Rollback failed: " << result.error().message << "\n";
    }
}
```

**Note**: Rollback triggers `on_change` callbacks with the old configuration restored.

### Recent Change Events

Track recent configuration change events:

```cpp
// Get last 10 change events
std::vector<config_change_event> events = watcher.recent_events(10);

for (const auto& event : events) {
    std::cout << "Version " << event.version << ": ";

    if (event.success) {
        std::cout << "Success (";
        for (const auto& field : event.changed_fields) {
            std::cout << field << " ";
        }
        std::cout << ")\n";
    } else {
        std::cout << "Failed: " << event.error_message << "\n";
    }
}
```

---

## 4. CLI Config Parser (`cli_config_parser.h`)

The `cli_config_parser` provides command-line argument parsing with configuration override support.

### Argument Format

Supported argument formats:

```bash
# Configuration file
--config=config.yaml
--config config.yaml

# Configuration overrides
--set logger.level=debug
--set key=value

# Help and version
--help
-h
--version
-v
```

**Key-value format**: `--set` accepts `key=value` format where:
- `key` is a dot-separated configuration path (e.g., `logger.level`)
- `value` is the value to set

### Boolean Flags

Boolean values can be specified in multiple formats:

```bash
# True values
--set logger.async=true
--set logger.async=1
--set logger.async=yes
--set logger.async=on

# False values
--set logger.async=false
--set logger.async=0
--set logger.async=no
--set logger.async=off
```

### List Arguments

List values (e.g., `logger.writers`) are specified as comma-separated values via environment variables:

```bash
# Via environment variable
export UNIFIED_LOGGER_WRITERS="console,file,json"

# In YAML file
logger:
  writers:
    - console
    - file
    - json
```

**CLI limitation**: The `--set` flag does not support list values directly. Use environment variables or YAML for lists.

### Help Text Generation

The parser automatically generates help text from configuration metadata:

```cpp
#include <kcenon/common/config/cli_config_parser.h>

using namespace kcenon::common::config;

// Print help message
cli_config_parser::print_help("myapp");
```

**Output**:

```
Usage: myapp [OPTIONS]

Options:
  --config=<path>     Load configuration from YAML file
  --set <key>=<value> Override a configuration value
  --help, -h          Show this help message
  --version, -v       Show version information

Configuration keys:
  thread.pool_size
    Number of worker threads (0 for auto) [env: UNIFIED_THREAD_POOL_SIZE]
  logger.level (trace|debug|info|warn|error|critical|off)
    Log level [env: UNIFIED_LOGGER_LEVEL]
  ...

Examples:
  myapp --config=config.yaml
  myapp --set logger.level=debug
  myapp --config=config.yaml --set thread.pool_size=16
```

### Required vs Optional Arguments

All CLI arguments are **optional**:

- If `--config` is not provided, configuration is loaded from environment variables and defaults
- If `--set` overrides are not provided, configuration from file/environment is used

**Positional arguments** are collected in `parsed_args::positional_args`:

```cpp
auto parse_result = cli_config_parser::parse(argc, argv);
if (parse_result.is_ok()) {
    auto args = parse_result.value();

    for (const auto& arg : args.positional_args) {
        std::cout << "Positional arg: " << arg << "\n";
    }
}
```

### Complete CLI Parsing Example

```cpp
#include <kcenon/common/config/cli_config_parser.h>

int main(int argc, char** argv) {
    using namespace kcenon::common::config;

    // Load configuration with CLI overrides
    auto result = cli_config_parser::load_with_cli_overrides(argc, argv);

    if (result.is_err()) {
        // Check for help/version requests
        if (result.error().message == "help_requested") {
            cli_config_parser::print_help(argv[0]);
            return 0;
        } else if (result.error().message == "version_requested") {
            cli_config_parser::print_version("1.0.0");
            return 0;
        }

        // Actual error
        std::cerr << "Configuration error: "
                  << result.error().message << "\n";
        return 1;
    }

    auto config = result.value();

    // Use configuration
    std::cout << "Log level: " << config.logger.level << "\n";
    std::cout << "Thread pool size: " << config.thread.pool_size << "\n";

    return 0;
}
```

---

## 5. Config Loader (`config_loader.h`)

The `config_loader` provides YAML file loading, environment variable substitution, and configuration validation.

### File Format Support

The loader supports YAML configuration files when built with `BUILD_WITH_YAML_CPP=ON`:

```cmake
# Enable YAML support
cmake -DBUILD_WITH_YAML_CPP=ON ..
```

**Without YAML support**: The loader can still load configuration from environment variables and defaults.

### Error Handling

The loader provides detailed error reporting via `Result<T>`:

```cpp
#include <kcenon/common/config/config_loader.h>

using namespace kcenon::common::config;

auto result = config_loader::load("config.yaml");

if (result.is_err()) {
    const auto& error = result.error();

    std::cerr << "Error loading configuration:\n"
              << "  Code: " << error.code << "\n"
              << "  Message: " << error.message << "\n"
              << "  Source: " << error.source << "\n";

    // Error code ranges (from config_error_codes namespace)
    if (error.code == config_error_codes::file_not_found) {
        std::cerr << "Configuration file not found\n";
    } else if (error.code == config_error_codes::parse_error) {
        std::cerr << "YAML parsing failed\n";
    } else if (error.code == config_error_codes::validation_error) {
        std::cerr << "Configuration validation failed\n";
    }
}
```

**Error codes**:

| Code | Value | Description |
|------|-------|-------------|
| `file_not_found` | `1001` | Configuration file not found |
| `parse_error` | `1002` | YAML parsing error |
| `validation_error` | `1003` | Configuration validation failed |
| `invalid_value` | `1004` | Invalid configuration value |
| `io_error` | `1005` | I/O error reading file |

### Schema Validation

The loader validates all configuration values against the schema:

```cpp
auto result = config_loader::load("config.yaml");

if (result.is_ok()) {
    auto config = result.value();
    // Configuration is valid
} else {
    // Validation failed
    std::cerr << "Validation error: " << result.error().message << "\n";
}
```

**Validation rules**:

1. **Enum values**: String fields with allowed values (e.g., `logger.level` must be `"trace"`, `"debug"`, etc.)
2. **Range checks**: Numeric fields with range constraints (e.g., `monitoring.tracing.sampling_rate` must be 0.0-1.0)
3. **Pool constraints**: Database pool `min_size` cannot exceed `max_size`
4. **Required fields**: Some fields must be set (e.g., `database.backend` when using database)

**Get validation issues** (including warnings):

```cpp
auto config = unified_config::defaults();
config.logger.buffer_size = 512;  // Very small buffer

auto issues = config_loader::get_validation_issues(config);

for (const auto& issue : issues) {
    if (issue.is_warning) {
        std::cout << "Warning: ";
    } else {
        std::cout << "Error: ";
    }
    std::cout << issue.field_path << ": " << issue.message << "\n";
}

// Output:
// Warning: logger.buffer_size: Buffer size is very small for async logging. Consider using at least 1024 bytes.
```

### Environment Variable Substitution

The loader supports environment variable substitution in YAML files using `${VAR_NAME}` syntax:

**YAML file**:

```yaml
logger:
  file_path: ${LOG_DIR}/app.log  # Expands to environment variable
  level: ${LOG_LEVEL:-info}      # With default value (not supported yet)
```

**Shell**:

```bash
export LOG_DIR=/var/log/myapp
./myapp --config=config.yaml
```

**Result**: `logger.file_path` becomes `/var/log/myapp/app.log`

**Variable expansion**:

```cpp
std::string expanded = config_loader::expand_env_vars(
    "Log path: ${LOG_DIR}/app.log"
);
// expanded = "Log path: /var/log/myapp/app.log"
```

**Undefined variables**: If an environment variable is not set, the pattern is left unchanged:

```yaml
file_path: ${UNDEFINED_VAR}/app.log
```

Result: `file_path = "${UNDEFINED_VAR}/app.log"` (literal string)

---

## 6. Complete Usage Example

### Basic Example

Load configuration from file and environment:

```cpp
#include <kcenon/common/config/config_loader.h>
#include <iostream>

int main() {
    using namespace kcenon::common::config;

    // Load configuration from file
    auto result = config_loader::load("config.yaml");

    if (result.is_err()) {
        std::cerr << "Error: " << result.error().message << "\n";
        return 1;
    }

    auto config = result.value();

    // Use configuration
    std::cout << "Log level: " << config.logger.level << "\n";
    std::cout << "Thread pool size: " << config.thread.pool_size << "\n";
    std::cout << "Monitoring enabled: "
              << (config.monitoring.enabled ? "yes" : "no") << "\n";

    return 0;
}
```

### Full-Featured Example

Complete example with hot-reload, CLI parsing, and callbacks:

```cpp
#include <kcenon/common/config/config_watcher.h>
#include <kcenon/common/config/cli_config_parser.h>
#include <iostream>
#include <thread>
#include <chrono>

using namespace kcenon::common::config;

class Application {
public:
    Application(const unified_config& initial_config)
        : config_(initial_config)
        , watcher_(nullptr) {}

    void start_with_hot_reload(const std::string& config_path) {
        // Create config watcher
        watcher_ = std::make_unique<config_watcher>(config_path);

        // Register change callback
        watcher_->on_change([this](const unified_config& old_cfg,
                                   const unified_config& new_cfg) {
            handle_config_change(old_cfg, new_cfg);
        });

        // Register error callback
        watcher_->on_error([](const std::string& error_message) {
            std::cerr << "Config reload failed: " << error_message << "\n";
        });

        // Start watching
        auto result = watcher_->start();
        if (result.is_err()) {
            std::cerr << "Failed to start config watcher: "
                      << result.error().message << "\n";
        } else {
            std::cout << "Configuration hot-reload enabled\n";
        }
    }

    void run() {
        std::cout << "Application running with:\n"
                  << "  Log level: " << config_.logger.level << "\n"
                  << "  Thread pool: " << config_.thread.pool_size << "\n";

        // Simulate application running
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            // Do work...
        }
    }

    void shutdown() {
        if (watcher_) {
            watcher_->stop();
            std::cout << "Configuration watcher stopped\n";
        }
    }

private:
    void handle_config_change(const unified_config& old_cfg,
                              const unified_config& new_cfg) {
        std::cout << "Configuration changed (version "
                  << watcher_->version() << "):\n";

        // Apply hot-reloadable changes
        if (old_cfg.logger.level != new_cfg.logger.level) {
            std::cout << "  Log level: "
                      << old_cfg.logger.level << " → "
                      << new_cfg.logger.level << "\n";
            apply_log_level_change(new_cfg.logger.level);
        }

        if (old_cfg.monitoring.metrics_interval != new_cfg.monitoring.metrics_interval) {
            std::cout << "  Metrics interval: "
                      << old_cfg.monitoring.metrics_interval.count() << "ms → "
                      << new_cfg.monitoring.metrics_interval.count() << "ms\n";
            apply_metrics_interval_change(new_cfg.monitoring.metrics_interval);
        }

        // Warn about non-hot-reloadable changes
        if (old_cfg.thread.pool_size != new_cfg.thread.pool_size) {
            std::cout << "  Warning: Thread pool size changed, "
                      << "restart required to take effect\n";
        }

        // Update internal config
        config_ = new_cfg;
    }

    void apply_log_level_change(const std::string& new_level) {
        // Update logger level in logger_system
        // (Implementation depends on logger_system integration)
    }

    void apply_metrics_interval_change(std::chrono::milliseconds interval) {
        // Update metrics collection interval in monitoring_system
        // (Implementation depends on monitoring_system integration)
    }

    unified_config config_;
    std::unique_ptr<config_watcher> watcher_;
};

int main(int argc, char** argv) {
    // Parse CLI arguments
    auto config_result = cli_config_parser::load_with_cli_overrides(argc, argv);

    if (config_result.is_err()) {
        // Handle --help and --version
        if (config_result.error().message == "help_requested") {
            cli_config_parser::print_help(argv[0]);
            return 0;
        } else if (config_result.error().message == "version_requested") {
            cli_config_parser::print_version("1.0.0");
            return 0;
        }

        std::cerr << "Configuration error: "
                  << config_result.error().message << "\n";
        return 1;
    }

    auto config = config_result.value();

    // Create application
    Application app(config);

    // Start with hot-reload (if --config was provided)
    auto parse_result = cli_config_parser::parse(argc, argv);
    if (parse_result.is_ok() && !parse_result.value().config_path.empty()) {
        app.start_with_hot_reload(parse_result.value().config_path);
    }

    // Run application
    try {
        app.run();
    } catch (const std::exception& e) {
        std::cerr << "Application error: " << e.what() << "\n";
        app.shutdown();
        return 1;
    }

    app.shutdown();
    return 0;
}
```

**Run the example**:

```bash
# With configuration file and hot-reload
./app --config=config.yaml

# With CLI overrides
./app --config=config.yaml --set logger.level=debug --set thread.pool_size=16

# Environment variable overrides
export UNIFIED_LOGGER_LEVEL=debug
./app --config=config.yaml
```

### Hot-Reload Callback Patterns

#### Pattern 1: Selective Hot-Reload

Only apply changes to hot-reloadable fields:

```cpp
watcher.on_change([](const unified_config& old_cfg,
                     const unified_config& new_cfg) {
    // Check if hot-reloadable field changed
    if (old_cfg.logger.level != new_cfg.logger.level) {
        // Apply change immediately
        apply_log_level(new_cfg.logger.level);
    }

    // Ignore non-hot-reloadable changes
    if (old_cfg.thread.pool_size != new_cfg.thread.pool_size) {
        std::cout << "Warning: Thread pool size changed, restart required\n";
    }
});
```

#### Pattern 2: Validation Before Apply

Validate new configuration before applying:

```cpp
watcher.on_change([&app](const unified_config& old_cfg,
                         const unified_config& new_cfg) {
    // Validate configuration with application state
    if (!app.validate_config(new_cfg)) {
        std::cerr << "New configuration is incompatible with current state\n";
        // Configuration is still updated, but application doesn't apply it
        return;
    }

    // Apply valid configuration
    app.apply_config(new_cfg);
});
```

#### Pattern 3: Gradual Rollout

Apply configuration changes gradually to avoid disruption:

```cpp
watcher.on_change([&scheduler](const unified_config& old_cfg,
                               const unified_config& new_cfg) {
    if (old_cfg.monitoring.metrics_interval != new_cfg.monitoring.metrics_interval) {
        // Schedule gradual change
        scheduler.schedule_change([interval = new_cfg.monitoring.metrics_interval]() {
            apply_metrics_interval(interval);
        }, std::chrono::seconds(5));  // Apply after 5 seconds
    }
});
```

---

## 7. Environment Variable Reference

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

## 8. YAML Configuration Examples

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

## 9. Hot-Reload Patterns

### Basic Hot-Reload

Enable hot-reload for a configuration file:

```cpp
#include <kcenon/common/config/config_watcher.h>

using namespace kcenon::common::config;

config_watcher watcher("config.yaml");

watcher.on_change([](const unified_config& old_cfg,
                     const unified_config& new_cfg) {
    std::cout << "Configuration reloaded\n";
});

watcher.start();

// Application runs...
// When config.yaml is modified, the callback is invoked

watcher.stop();
```

### Selective Hot-Reload

Only react to specific configuration changes:

```cpp
watcher.on_change([&logger](const unified_config& old_cfg,
                            const unified_config& new_cfg) {
    // Only handle hot-reloadable fields

    if (old_cfg.logger.level != new_cfg.logger.level) {
        logger.set_level(new_cfg.logger.level);
        std::cout << "Log level updated to " << new_cfg.logger.level << "\n";
    }

    if (old_cfg.monitoring.metrics_interval != new_cfg.monitoring.metrics_interval) {
        monitoring.set_interval(new_cfg.monitoring.metrics_interval);
        std::cout << "Metrics interval updated to "
                  << new_cfg.monitoring.metrics_interval.count() << "ms\n";
    }
});
```

### Hot-Reload with Validation

Validate configuration before applying changes:

```cpp
watcher.on_change([&app](const unified_config& old_cfg,
                         const unified_config& new_cfg) {
    // Validate new configuration
    auto issues = config_loader::get_validation_issues(new_cfg);

    bool has_errors = false;
    for (const auto& issue : issues) {
        if (!issue.is_warning) {
            std::cerr << "Validation error: " << issue.field_path
                      << ": " << issue.message << "\n";
            has_errors = true;
        }
    }

    if (has_errors) {
        std::cerr << "Configuration validation failed, not applying changes\n";

        // Rollback to previous version
        uint64_t prev_version = watcher.version() - 1;
        auto rollback_result = watcher.rollback(prev_version);

        if (rollback_result.is_ok()) {
            std::cout << "Rolled back to version " << prev_version << "\n";
        }

        return;
    }

    // Apply valid configuration
    app.apply_config(new_cfg);
});
```

### Manual Reload

Trigger a manual configuration reload:

```cpp
// Manually reload configuration (without file system event)
auto result = watcher.reload();

if (result.is_ok()) {
    std::cout << "Configuration manually reloaded\n";
} else {
    std::cerr << "Manual reload failed: " << result.error().message << "\n";
}
```

---

## 10. Troubleshooting

### Configuration File Not Found

**Error**:

```
Error loading configuration:
  Code: 1001
  Message: Configuration file not found: config.yaml
  Source: config_loader
```

**Solution**:

1. Check that the configuration file exists:
   ```bash
   ls -l config.yaml
   ```

2. Use an absolute path:
   ```cpp
   auto result = config_loader::load("/etc/myapp/config.yaml");
   ```

3. Verify working directory:
   ```bash
   pwd
   ./myapp --config=$(pwd)/config.yaml
   ```

### YAML Parsing Error

**Error**:

```
Error loading configuration:
  Code: 1002
  Message: YAML parse error: yaml-cpp: error at line 5, column 3: illegal map value
  Source: config_loader
```

**Solution**:

1. Validate YAML syntax:
   ```bash
   # Install yamllint
   pip install yamllint

   # Check YAML file
   yamllint config.yaml
   ```

2. Common YAML errors:
   - Inconsistent indentation (use spaces, not tabs)
   - Missing colon after key
   - Incorrect list syntax

3. Test YAML online: https://www.yamllint.com/

### Validation Error

**Error**:

```
Error loading configuration:
  Code: 1003
  Message: Validation failed for logger.level: Invalid log level: dbug. Valid values: trace, debug, info, warn, error, critical, off
  Source: config_loader
```

**Solution**:

1. Check allowed values in error message
2. Refer to [Sub-Structs and Fields](#sub-structs-and-fields) section for valid values
3. Use `get_validation_issues()` to see all validation problems:

```cpp
auto config = unified_config::defaults();
config.logger.level = "dbug";  // Invalid

auto issues = config_loader::get_validation_issues(config);
for (const auto& issue : issues) {
    std::cout << issue.field_path << ": " << issue.message << "\n";
}
```

### Hot-Reload Not Working

**Symptoms**:

- Configuration file is modified, but application doesn't reload
- `on_change` callback is not invoked

**Solution**:

1. Verify watcher is started:
   ```cpp
   auto result = watcher.start();
   if (result.is_err()) {
       std::cerr << "Failed to start watcher: "
                 << result.error().message << "\n";
   }

   // Check if running
   if (watcher.is_running()) {
       std::cout << "Watcher is running\n";
   }
   ```

2. Check file system permissions:
   ```bash
   ls -l config.yaml
   # Ensure file is readable
   chmod 644 config.yaml
   ```

3. Verify platform support:
   - Linux: `inotify` support
   - macOS: `kqueue` support
   - Windows: `ReadDirectoryChangesW` support

4. Test manual reload:
   ```cpp
   auto result = watcher.reload();
   if (result.is_ok()) {
       std::cout << "Manual reload succeeded\n";
   }
   ```

### Environment Variable Not Applied

**Symptoms**:

- Environment variable is set, but configuration doesn't use it

**Solution**:

1. Verify environment variable name:
   ```bash
   # Check environment variable
   echo $UNIFIED_LOGGER_LEVEL

   # List all UNIFIED_* variables
   env | grep UNIFIED_
   ```

2. Ensure variable is exported:
   ```bash
   export UNIFIED_LOGGER_LEVEL=debug
   ```

3. Check priority order: CLI arguments > Environment variables > YAML file
   - If CLI argument is set, it overrides environment variable

4. Test environment loading:
   ```cpp
   auto result = config_loader::load_from_env();
   if (result.is_ok()) {
       std::cout << "Log level from env: "
                 << result.value().logger.level << "\n";
   }
   ```

### YAML Support Not Available

**Error**:

```
Error loading configuration:
  Code: 1002
  Message: YAML support not available. Build with -DBUILD_WITH_YAML_CPP=ON
  Source: config_loader
```

**Solution**:

1. Rebuild with YAML support:
   ```bash
   cmake -DBUILD_WITH_YAML_CPP=ON ..
   make
   ```

2. Install yaml-cpp dependency:
   ```bash
   # Ubuntu/Debian
   sudo apt-get install libyaml-cpp-dev

   # macOS
   brew install yaml-cpp

   # Windows (vcpkg)
   vcpkg install yaml-cpp
   ```

3. Alternative: Use environment variables only:
   ```cpp
   auto result = config_loader::load_from_env();
   ```

---

## Related Documentation

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
