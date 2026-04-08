# Config Watcher (`config_watcher.h`)

The `config_watcher` provides automatic configuration hot-reload by monitoring configuration files for changes.

---

## Table of Contents

- [File System Monitoring](#file-system-monitoring)
- [Debouncing and Rate Limiting](#debouncing-and-rate-limiting)
- [Callback Registration](#callback-registration)
- [Watched Path Management](#watched-path-management)
- [Thread Safety](#thread-safety)
- [Version Tracking](#version-tracking)
- [Rollback Support](#rollback-support)
- [Recent Change Events](#recent-change-events)
- [Hot-Reload Patterns](#hot-reload-patterns)
  - [Basic Hot-Reload](#basic-hot-reload)
  - [Selective Hot-Reload](#selective-hot-reload)
  - [Hot-Reload with Validation](#hot-reload-with-validation)
  - [Manual Reload](#manual-reload)

---

## File System Monitoring

The watcher uses platform-native file system event APIs:

| Platform | API | Supported Events |
|----------|-----|------------------|
| **Linux** | `inotify` | `IN_MODIFY`, `IN_CREATE`, `IN_MOVED_TO`, `IN_CLOSE_WRITE` |
| **macOS/BSD** | `kqueue` | `NOTE_WRITE`, `NOTE_EXTEND`, `NOTE_RENAME`, `NOTE_DELETE` |
| **Windows** | `ReadDirectoryChangesW` | `FILE_NOTIFY_CHANGE_LAST_WRITE`, `FILE_NOTIFY_CHANGE_FILE_NAME` |

**Cross-platform support**: The watcher automatically selects the appropriate API for the current platform.

## Debouncing and Rate Limiting

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

## Callback Registration

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

## Watched Path Management

The watcher monitors a single configuration file specified at construction:

```cpp
// Watch a specific file
config_watcher watcher("/etc/myapp/config.yaml");

// Get the watched path
std::string path = watcher.config_path();
```

**Directory watching**: The watcher monitors the parent directory to handle file deletion and recreation (common with atomic file writes).

## Thread Safety

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

## Version Tracking

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

## Rollback Support

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

## Recent Change Events

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

## Hot-Reload Patterns

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

## Related Documentation

- [Configuration Subsystem Guide](CONFIG_UNIFIED.md) — Overview, schema, and environment variables
- [CLI Config Parser](CONFIG_CLI_PARSER.md) — Command-line argument parsing
- [Config Loader](CONFIG_LOADER.md) — YAML loading, validation, and troubleshooting

---

**Version**: 1.0.0
**Last Updated**: 2026-02-08
**License**: BSD 3-Clause (see LICENSE file)
