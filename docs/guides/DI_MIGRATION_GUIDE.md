# DI Container Migration Guide

> **Language:** **English**

## Table of Contents

- [Overview](#overview)
- [What Changed](#what-changed)
- [Unified Registration API](#unified-registration-api)
  - [service_container Basics](#service_container-basics)
  - [Per-System Registration Functions](#per-system-registration-functions)
- [Migration by System](#migration-by-system)
  - [logger_system](#logger_system)
  - [monitoring_system](#monitoring_system)
  - [database_system](#database_system)
- [Common Patterns](#common-patterns)
- [FAQ](#faq)
- [References](#references)

**Version**: 1.0.0
**Last Updated**: 2026-02-23
**Related Issue**: kcenon/common_system#363

---

## Overview

The KCENON ecosystem previously maintained multiple DI container implementations
across different systems. As of this consolidation, **`common_system::di::service_container`**
is the single, unified DI container used across all systems.

**Target Audience**: Developers maintaining or contributing to KCENON ecosystem systems,
and downstream consumers integrating multiple KCENON systems.

### Before: Multiple DI Containers

| System | Old DI Implementation | Status |
|--------|----------------------|--------|
| common_system | `service_container` | **Kept** (canonical) |
| logger_system | `lightweight_di_container` | **Removed** |
| monitoring_system | Mock `service_container_interface` in tests | **Removed** |
| database_system | N/A (coordinator-based lifecycle) | **No change** |

### After: Single DI Container

All systems now use `common_system::di::service_container` through
system-specific `service_registration.h` convenience functions.

---

## What Changed

### logger_system

- **Removed**: `lightweight_di_container`, `di_container_interface`,
  `lightweight_container`, `thread_system_di_adapter` (4 files, ~587 LOC)
- **Added**: 20 unit tests validating `service_container` integration
- **Kept**: `include/kcenon/logger/di/service_registration.h` (unchanged, already used `service_container`)

### monitoring_system

- **Removed**: Mock `service_container_interface` and `service_locator` (~490 LOC of test stubs)
- **Added**: 14 unit tests using real `service_container`
- **Kept**: `include/kcenon/monitoring/di/service_registration.h` (unchanged, already used `service_container`)

### database_system

- **Fixed**: Broken include paths in `service_registration.h` and `common_system_database_adapter.h`
- **Fixed**: Renamed enum `database_types::postgresql` -> `database_types::postgres`
- **Added**: 25 unit tests validating `service_container` integration
- **Kept**: `database_coordinator` as internal lifecycle manager (not a DI container)

### common_system

- **Added**: `ModuleRegistrar` concept and `register_module()` helper (PR #370)

---

## Unified Registration API

### service_container Basics

```cpp
#include <kcenon/common/di/service_container.h>

using namespace kcenon::common::di;

// Get the global singleton container
auto& container = service_container::global();

// Or create a local container
service_container container;

// Register a service
container.register_simple_factory<IMyService>(
    []() { return std::make_shared<MyServiceImpl>(); },
    service_lifetime::singleton
);

// Resolve a service
auto result = container.resolve<IMyService>();
if (result.is_ok()) {
    auto service = result.value();
    // use service...
}

// Or use resolve_or_null for optional services
auto service = container.resolve_or_null<IMyService>();
```

**Service Lifetimes:**

| Lifetime | Behavior |
|----------|----------|
| `singleton` | One instance shared across all resolve() calls |
| `transient` | New instance created on each resolve() call |
| `scoped` | One instance per scope (via `create_scope()`) |

### Per-System Registration Functions

Each system provides convenience functions in its `service_registration.h` header:

```cpp
// Logger
#include <kcenon/logger/di/service_registration.h>
kcenon::logger::di::register_logger_services(container, config);

// Monitoring
#include <kcenon/monitoring/di/service_registration.h>
kcenon::monitoring::di::register_monitor_services(container, config);

// Database
#include <kcenon/database/di/service_registration.h>
kcenon::database::di::register_database_services(container, config);
```

---

## Migration by System

### logger_system

**Before** (lightweight_di_container):

```cpp
// Old pattern — NO LONGER AVAILABLE
#include "impl/di/lightweight_di_container.h"

lightweight_di_container container;
container.register_service<ILogWriter>(std::make_shared<FileWriter>());
auto writer = container.resolve<ILogWriter>();
```

**After** (service_container):

```cpp
#include <kcenon/common/di/service_container.h>
#include <kcenon/logger/di/service_registration.h>

using namespace kcenon::common::di;
using namespace kcenon::logger::di;

service_container container;

// Option 1: Use convenience function
logger_registration_config config;
config.lifetime = service_lifetime::singleton;
register_logger_services(container, config);

// Option 2: Register a pre-created instance
auto writer = std::make_shared<FileWriter>("app.log");
register_logger_instance(container, writer);

// Resolve
auto logger = container.resolve<ILogger>().value();
```

### monitoring_system

**Before** (mock service_container_interface in tests):

```cpp
// Old test pattern — NO LONGER NEEDED
class mock_service_container : public service_container_interface {
    // 200+ lines of mock implementation
};
```

**After** (real service_container):

```cpp
#include <kcenon/common/di/service_container.h>
#include <kcenon/monitoring/di/service_registration.h>

using namespace kcenon::common::di;
using namespace kcenon::monitoring::di;

service_container container;

// Register with config
monitor_registration_config config;
config.lifetime = service_lifetime::singleton;
register_monitor_services(container, config);

// Or register a pre-created instance
auto monitor = std::make_shared<performance_monitor>();
register_monitor_instance(container, monitor);

// Resolve
auto mon = container.resolve<IMonitor>().value();

// Access underlying implementation if needed
auto perf_mon = get_underlying_performance_monitor(mon);
```

### database_system

The `database_coordinator` remains unchanged — it manages internal adapter
lifecycle with phase-aware initialization ordering (Logger -> Monitor -> Thread).
This is by design and is not a DI container.

For `IDatabase` registration via `service_container`:

```cpp
#include <kcenon/common/di/service_container.h>
#include <kcenon/database/di/service_registration.h>

using namespace kcenon::common::di;
using namespace kcenon::database::di;

service_container container;

// Register with config
database_registration_config config;
config.db_type = ::database::database_types::postgres;
config.connection_string = "host=localhost dbname=mydb";
config.connect_on_register = true;
config.lifetime = service_lifetime::singleton;
register_database_services(container, config);

// Resolve
auto db = container.resolve<IDatabase>().value();
auto connected = db->is_connected();

// Access underlying database_manager if needed
auto manager = get_underlying_database_manager(db);
```

---

## Common Patterns

### Multi-System Bootstrap

```cpp
#include <kcenon/common/di/service_container.h>
#include <kcenon/logger/di/service_registration.h>
#include <kcenon/monitoring/di/service_registration.h>
#include <kcenon/database/di/service_registration.h>

auto& container = service_container::global();

// Register all systems
kcenon::logger::di::register_logger_services(container);
kcenon::monitoring::di::register_monitor_services(container);
kcenon::database::di::register_database_services(container);

// Freeze to prevent further modifications
container.freeze();

// Resolve services anywhere in the application
auto logger = container.resolve<ILogger>().value();
auto monitor = container.resolve<IMonitor>().value();
auto database = container.resolve<IDatabase>().value();
```

### Scoped Containers

```cpp
// Create a scope for request-level isolation
auto scope = container.create_scope();

// Scoped containers inherit parent registrations
auto logger = scope->resolve<ILogger>().value();
```

### Error Handling

```cpp
auto result = container.resolve<IDatabase>();
if (result.is_err()) {
    // Service not registered or factory threw an exception
    auto& error = result.error();
    // error.code, error.message, error.source
}
```

### Thread Safety

`service_container` is fully thread-safe:
- Concurrent `resolve()` calls are safe
- Singleton factories are invoked exactly once (even under contention)
- Registration and resolution can happen concurrently (with appropriate locking)

---

## FAQ

### Q: Do I need to change my code if I was already using service_registration.h?

**No.** The `service_registration.h` APIs are unchanged. Only the internal
implementations (lightweight_di_container, mock containers) were removed.

### Q: Can I still use a local container instead of the global one?

**Yes.** `service_container container;` creates a standalone container.
The global singleton is available via `service_container::global()` but is not required.

### Q: What about database_coordinator? Was it refactored?

**No.** The `database_coordinator` manages internal adapter lifecycle with
strict initialization ordering (Logger -> Monitor -> Thread). It is a lifecycle
manager, not a DI container, and was intentionally left unchanged.

### Q: How do I write tests with the DI container?

Create a local `service_container` in your test fixture and register mock
implementations:

```cpp
class MyTest : public ::testing::Test {
protected:
    service_container container_;

    void TearDown() override { container_.clear(); }
};

TEST_F(MyTest, ResolvesService) {
    container_.register_simple_factory<IService>(
        []() { return std::make_shared<MockService>(); },
        service_lifetime::transient);

    auto svc = container_.resolve<IService>();
    ASSERT_TRUE(svc.is_ok());
}
```

---

## References

- [FEATURES.md — Dependency Injection & Bootstrap](../FEATURES.md)
- [INTEGRATION_GUIDE.md — Service Container Integration](../INTEGRATION_GUIDE.md)
- [CONCEPTS_GUIDE.md — Service Concepts](CONCEPTS_GUIDE.md)
- Parent issue: [kcenon/common_system#363](https://github.com/kcenon/common_system/issues/363)

### Implementation PRs

| Phase | PR | Repository |
|-------|-----|-----------|
| Phase 1+2: ModuleRegistrar | [#370](https://github.com/kcenon/common_system/pull/370) | common_system |
| Phase 3: logger_system | [#447](https://github.com/kcenon/logger_system/pull/447) | logger_system |
| Phase 4: monitoring_system | [#485](https://github.com/kcenon/monitoring_system/pull/485) | monitoring_system |
| Phase 5: database_system | [#384](https://github.com/kcenon/database_system/pull/384) | database_system |
