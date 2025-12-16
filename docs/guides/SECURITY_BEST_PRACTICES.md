# Security Best Practices

This guide documents security features and best practices for the Common System, focusing on registry protection and audit logging.

## Overview

The Common System provides security controls to protect global registries from unauthorized modifications after system initialization. These controls help prevent:

- **Service Hijacking**: Unauthorized replacement of registered services
- **Logger Tampering**: Modification of loggers after system startup
- **Audit Trail Manipulation**: Unauthorized clearing of audit logs

## Registry Freeze Mechanism

### What is Registry Freezing?

Registry freezing is a one-way security mechanism that prevents modifications to global registries after system initialization. Once frozen, a registry cannot be unfrozen, ensuring that critical system components remain stable throughout the application lifecycle.

### Freezable Registries

The following registries support freezing:

1. **GlobalLoggerRegistry**: Central registry for logger instances
2. **service_container**: Dependency injection container for services

### Using the Freeze Mechanism

#### Manual Freezing

```cpp
#include <kcenon/common/interfaces/global_logger_registry.h>
#include <kcenon/common/di/service_container.h>

// Freeze logger registry
GlobalLoggerRegistry::instance().freeze();

// Freeze service container
service_container::global().freeze();
```

#### Automatic Freezing with SystemBootstrapper

The recommended approach is to use `SystemBootstrapper` with auto-freeze enabled:

```cpp
#include <kcenon/common/bootstrap/system_bootstrapper.h>

int main() {
    SystemBootstrapper bootstrapper;

    auto result = bootstrapper
        .with_default_logger(create_console_logger)
        .with_logger("database", create_file_logger)
        .with_auto_freeze()  // Freeze after initialization
        .initialize();

    if (result.is_err()) {
        return 1;
    }

    // Registries are now frozen - no modifications allowed
    // Application logic here...

    return 0;
}
```

#### Selective Freezing

You can freeze registries selectively:

```cpp
// Freeze only logger registry
bootstrapper.with_auto_freeze(true, false);

// Freeze only service container
bootstrapper.with_auto_freeze(false, true);

// Freeze both (default)
bootstrapper.with_auto_freeze(true, true);
```

### Frozen Registry Behavior

When a registry is frozen:

- **Registration attempts** return an error with code `REGISTRY_FROZEN`
- **Unregistration attempts** return an error with code `REGISTRY_FROZEN`
- **Clear operations** silently fail (for API compatibility)
- **Resolution/lookup operations** continue to work normally

Example error handling:

```cpp
auto result = registry.register_logger("new-logger", logger);

if (result.is_err()) {
    if (result.error().code == error_codes::REGISTRY_FROZEN) {
        // Handle frozen registry - expected after initialization
        LOG_WARN("Attempted to modify frozen registry");
    }
}
```

### Checking Frozen State

```cpp
if (GlobalLoggerRegistry::instance().is_frozen()) {
    // Registry is frozen
}

if (service_container::global().is_frozen()) {
    // Container is frozen
}
```

## Audit Logging

### What is Registry Audit Logging?

The `RegistryAuditLog` captures all mutation operations on global registries, providing a tamper-evident audit trail for security monitoring and compliance.

### Captured Events

The audit log captures the following actions:

| Action | Description |
|--------|-------------|
| `register_logger` | Logger registration |
| `unregister_logger` | Logger unregistration |
| `set_default_logger` | Default logger assignment |
| `register_factory` | Logger factory registration |
| `set_default_factory` | Default factory assignment |
| `clear_loggers` | Clear all loggers |
| `freeze_logger_registry` | Freeze logger registry |
| `register_service` | Service registration |
| `unregister_service` | Service unregistration |
| `clear_services` | Clear all services |
| `freeze_service_container` | Freeze service container |

### Accessing Audit Events

```cpp
#include <kcenon/common/interfaces/registry_audit_log.h>

using namespace kcenon::common::interfaces;

// Get all events
auto all_events = RegistryAuditLog::get_events();

// Filter by action type
auto register_events = RegistryAuditLog::get_events_by_action(
    registry_action::register_logger);

// Filter by time range
auto start = std::chrono::system_clock::now() - std::chrono::hours(1);
auto end = std::chrono::system_clock::now();
auto recent_events = RegistryAuditLog::get_events_in_range(start, end);

// Get event count
size_t count = RegistryAuditLog::event_count();
```

### Event Properties

Each `registry_event` contains:

- `action`: The type of registry operation
- `target_name`: Name of the service/logger (empty for clear/freeze)
- `file`: Source file where operation occurred
- `line`: Line number where operation occurred
- `function`: Function where operation occurred
- `timestamp`: When the event occurred
- `success`: Whether the operation succeeded
- `error_message`: Error message if operation failed

### Analyzing Audit Events

```cpp
for (const auto& event : RegistryAuditLog::get_events()) {
    std::cout << "Action: " << to_string(event.action) << "\n"
              << "Target: " << event.target_name << "\n"
              << "Location: " << event.file << ":" << event.line << "\n"
              << "Function: " << event.function << "\n"
              << "Success: " << (event.success ? "yes" : "no") << "\n";

    if (!event.success) {
        std::cout << "Error: " << event.error_message << "\n";
    }
    std::cout << "\n";
}
```

### Controlling Audit Logging

Audit logging is enabled by default. You can control it:

```cpp
// Disable audit logging (security-sensitive operation)
RegistryAuditLog::set_enabled(false);

// Re-enable audit logging
RegistryAuditLog::set_enabled(true);

// Check if enabled
bool is_logging = RegistryAuditLog::is_enabled();
```

> **Warning**: Disabling audit logging should be done with caution and proper authorization.

## Security Patterns

### Pattern 1: Secure Initialization

```cpp
int main() {
    // Phase 1: Configure system
    SystemBootstrapper bootstrapper;
    bootstrapper
        .with_default_logger([] { return create_secure_logger(); })
        .with_auto_freeze();  // Enable auto-freeze

    // Phase 2: Initialize and freeze
    auto result = bootstrapper.initialize();
    if (result.is_err()) {
        std::cerr << "Initialization failed: " << result.error().message;
        return 1;
    }

    // Phase 3: Verify frozen state
    assert(GlobalLoggerRegistry::instance().is_frozen());
    assert(service_container::global().is_frozen());

    // Phase 4: Normal operation (registries immutable)
    run_application();

    return 0;
}
```

### Pattern 2: Audit Log Verification

```cpp
void verify_initialization_integrity() {
    auto events = RegistryAuditLog::get_events();

    // Check for any failed registrations
    for (const auto& event : events) {
        if (!event.success) {
            LOG_ERROR("Registration failed: {} - {}",
                     event.target_name, event.error_message);
        }
    }

    // Verify freeze events exist
    auto freeze_events = RegistryAuditLog::get_events_by_action(
        registry_action::freeze_logger_registry);

    if (freeze_events.empty()) {
        LOG_WARN("Logger registry was not frozen during initialization");
    }
}
```

### Pattern 3: Security Monitoring

```cpp
class SecurityMonitor {
public:
    void check_unauthorized_access() {
        auto events = RegistryAuditLog::get_events();

        for (const auto& event : events) {
            // Check for failed attempts after freeze
            if (!event.success &&
                event.error_message.find("frozen") != std::string::npos) {
                LOG_SECURITY("Unauthorized modification attempt: {} at {}:{}",
                            to_string(event.action),
                            event.file,
                            event.line);
            }
        }
    }
};
```

## Thread Safety

All security controls are thread-safe:

- `freeze()` uses atomic operations
- `is_frozen()` uses memory ordering for visibility
- `RegistryAuditLog` uses mutex for event storage
- Multiple threads can safely check frozen state

## Error Codes

| Error Code | Value | Description |
|------------|-------|-------------|
| `REGISTRY_FROZEN` | -11 | Registry is frozen and cannot be modified |

## Best Practices Summary

1. **Always use auto-freeze in production** - Configure `with_auto_freeze()` to prevent post-initialization modifications

2. **Freeze early** - Freeze registries immediately after initialization completes

3. **Monitor audit logs** - Regularly review audit events for unauthorized access attempts

4. **Handle frozen errors gracefully** - Expect `REGISTRY_FROZEN` errors for late modifications

5. **Don't disable audit logging in production** - Keep audit trails for security compliance

6. **Verify frozen state** - Assert registries are frozen before entering normal operation

7. **Use SystemBootstrapper** - Centralized initialization ensures consistent security controls

## See Also

- [SystemBootstrapper](../architecture/RUNTIME_BINDING.md) - System initialization
- [GlobalLoggerRegistry](./LOGGING_BEST_PRACTICES.md) - Logger management
- [Error Handling](./ERROR_HANDLING.md) - Result<T> error handling
- [Error Code Registry](../ERROR_CODE_REGISTRY.md) - Complete error code list
