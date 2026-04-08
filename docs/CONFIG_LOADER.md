# Config Loader (`config_loader.h`)

The `config_loader` provides YAML file loading, environment variable substitution, and configuration validation.

---

## Table of Contents

- [File Format Support](#file-format-support)
- [Error Handling](#error-handling)
- [Schema Validation](#schema-validation)
- [Environment Variable Substitution](#environment-variable-substitution)
- [Complete Usage Example](#complete-usage-example)
  - [Basic Example](#basic-example)
  - [Full-Featured Example](#full-featured-example)
  - [Hot-Reload Callback Patterns](#hot-reload-callback-patterns)
- [Troubleshooting](#troubleshooting)

---

## File Format Support

The loader supports YAML configuration files when built with `BUILD_WITH_YAML_CPP=ON`:

```cmake
# Enable YAML support
cmake -DBUILD_WITH_YAML_CPP=ON ..
```

**Without YAML support**: The loader can still load configuration from environment variables and defaults.

## Error Handling

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

## Schema Validation

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

## Environment Variable Substitution

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

## Complete Usage Example

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

## Troubleshooting

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
2. Refer to [Sub-Structs and Fields](CONFIG_UNIFIED.md#sub-structs-and-fields) section for valid values
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

- [Configuration Subsystem Guide](CONFIG_UNIFIED.md) — Overview, schema, and environment variables
- [Config Watcher](CONFIG_WATCHER.md) — Hot-reload file watching and patterns
- [CLI Config Parser](CONFIG_CLI_PARSER.md) — Command-line argument parsing
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
