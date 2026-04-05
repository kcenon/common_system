# API Quick Reference

One-page cheat sheet for the most common **common_system** APIs.
All types live under the `kcenon::common` namespace unless noted otherwise.

---

## Result<T> / Unwrap

```cpp
#include <kcenon/common/patterns/result.h>
using namespace kcenon::common;

// Create
Result<int> good = ok(42);
Result<int> bad  = make_error<int>(error_codes::INVALID_ARGUMENT, "msg", "mod");

// Query
good.is_ok();           // true
bad.is_err();           // true

// Access
int v  = good.value();            // throws if error
int v2 = bad.unwrap_or(-1);       // returns default on error
int v3 = good.unwrap();           // throws with source_location on error

// Transform
auto doubled = good.map([](int x) { return x * 2; });          // Result<int>
auto chained = good.and_then([](int x) { return ok(x + 1); }); // Result<int>
auto fixed   = bad.or_else([](const error_info&) { return ok(0); });

// Wrap exceptions
auto safe = try_catch<int>([] { return may_throw(); }, "module");

// VoidResult (success-or-error, no value)
VoidResult vr(std::monostate{});                       // success
VoidResult ve = make_error<std::monostate>(-1, "fail", "mod"); // error
```

---

## Service Container (DI)

```cpp
#include <kcenon/common/di/service_container.h>
using namespace kcenon::common::di;

auto& c = service_container::global();

// Register interface -> implementation
c.register_type<IFoo, FooImpl>(service_lifetime::singleton);

// Register with factory (receives container for resolving dependencies)
c.register_factory<IFoo>(
    [](IServiceContainer& c) -> std::shared_ptr<IFoo> {
        auto dep = c.resolve<IBar>().value();
        return std::make_shared<FooImpl>(dep);
    },
    service_lifetime::transient
);

// Register existing instance
c.register_instance<IFoo>(std::make_shared<FooImpl>());

// Resolve
auto result = c.resolve<IFoo>();    // Result<shared_ptr<IFoo>>
auto ptr    = c.resolve_or_null<IFoo>(); // shared_ptr or nullptr

// Scoped container
auto scope = c.create_scope();
auto scoped_svc = scope->resolve<IFoo>();

// Security: freeze after initialization
c.freeze();      // prevents further registrations
c.is_frozen();   // true
```

**Lifetimes**: `singleton` | `transient` | `scoped`

---

## Event Bus (Publish / Subscribe)

```cpp
#include <kcenon/common/patterns/event_bus.h>
using namespace kcenon::common;

// Define a custom event (must satisfy concepts::EventType)
struct user_created_event {
    std::string name;
    std::string get_type() const { return "user_created"; }
};

simple_event_bus bus;

// Subscribe -- returns a subscription ID
auto id = bus.subscribe<user_created_event>(
    [](const user_created_event& e) {
        std::cout << "User: " << e.name << "\n";
    });

// Publish
bus.publish(user_created_event{"Alice"});
bus.publish(user_created_event{"Bob"}, event_priority::high);

// Unsubscribe
bus.unsubscribe(id);

// Access the global bus
auto& global = get_event_bus();
```

---

## Circuit Breaker

```cpp
#include <kcenon/common/resilience/circuit_breaker.h>
using namespace kcenon::common::resilience;

// Configure
circuit_breaker_config cfg{
    .failure_threshold      = 5,
    .success_threshold      = 2,
    .failure_window         = std::chrono::seconds(60),
    .timeout                = std::chrono::seconds(30),
    .half_open_max_requests = 3,
};

circuit_breaker breaker(cfg);

// Check before calling
if (!breaker.allow_request()) {
    // Circuit is OPEN -- fail fast
}

// RAII guard (records failure on destruction unless committed)
{
    auto guard = breaker.make_guard();
    auto result = call_external_service();
    if (result.is_ok()) {
        guard.record_success();
    }
    // If guard is destroyed without record_success(), failure is recorded
}

// Manual recording
breaker.record_success();
breaker.record_failure();

// Inspect state
circuit_state state = breaker.get_state(); // CLOSED, OPEN, or HALF_OPEN
```

**States**: `CLOSED` (normal) -> `OPEN` (blocking) -> `HALF_OPEN` (testing recovery)

---

## Feature Flags

```cpp
#include <kcenon/common/config/feature_flags.h>

// Compile-time capability checks
#if KCENON_HAS_SOURCE_LOCATION
    // std::source_location is available
#endif

#if KCENON_HAS_JTHREAD
    // std::jthread / std::stop_token available
#endif

#if KCENON_HAS_FORMAT
    // std::format available
#endif

// Platform detection
#if KCENON_PLATFORM_LINUX
    // Linux-specific code
#elif KCENON_PLATFORM_MACOS
    // macOS-specific code
#elif KCENON_PLATFORM_WINDOWS
    // Windows-specific code
#endif
```

---

## Config Loader / Watcher

```cpp
#include <kcenon/common/config/config_loader.h>
#include <kcenon/common/config/config_watcher.h>
#include <kcenon/common/config/cli_config_parser.h>
using namespace kcenon::common::config;

// Load from YAML (requires BUILD_WITH_YAML_CPP=ON)
auto result = config_loader::load("config.yaml");    // Result<unified_config>
auto env    = config_loader::load_from_env();         // environment-only
auto defs   = config_loader::defaults();              // built-in defaults

// Load from string
auto from_str = config_loader::load_from_string("key: value");

// CLI parser -- merges --config file + --set overrides + env
auto cli = cli_config_parser::load_with_cli_overrides(argc, argv);

// Parse args only (no config loading)
auto args = cli_config_parser::parse(argc, argv);  // Result<parsed_args>
// args.value().config_path, args.value().overrides, args.value().show_help

// Hot-reload watcher
// config_watcher monitors file changes (inotify / kqueue / ReadDirectoryChangesW)
// and invokes callbacks with old and new configuration for comparison.
```

**Priority** (highest wins): CLI args > env vars (`UNIFIED_*`) > YAML file > defaults

---

## CLI Parser

```cpp
#include <kcenon/common/config/cli_config_parser.h>
using namespace kcenon::common::config;

// Supported arguments:
//   --config=<path>    Load YAML config file
//   --set key=value    Override a config value
//   --help, -h         Show help
//   --version, -v      Show version

int main(int argc, char** argv) {
    auto result = cli_config_parser::load_with_cli_overrides(argc, argv);
    if (result.is_err()) {
        if (result.error().code == cli_error_codes::invalid_argument) {
            cli_config_parser::print_help(argv[0]);
        }
        return 1;
    }
    auto config = result.value();
    // ...
}
```

---

## Common Includes

| Header | Provides |
|--------|----------|
| `<kcenon/common/patterns/result.h>` | `Result<T>`, `VoidResult`, `Optional<T>`, `error_info` |
| `<kcenon/common/di/service_container.h>` | `service_container`, `service_lifetime` |
| `<kcenon/common/patterns/event_bus.h>` | `simple_event_bus`, `event`, `event_priority` |
| `<kcenon/common/resilience/circuit_breaker.h>` | `circuit_breaker`, `circuit_breaker_config` |
| `<kcenon/common/config/feature_flags.h>` | `KCENON_HAS_*`, `KCENON_PLATFORM_*` macros |
| `<kcenon/common/config/config_loader.h>` | `config_loader`, `unified_config` |
| `<kcenon/common/config/config_watcher.h>` | `config_watcher` |
| `<kcenon/common/config/cli_config_parser.h>` | `cli_config_parser`, `parsed_args` |
| `<kcenon/common/error/error_codes.h>` | `error_codes`, `error::category` |
| `<kcenon/common/interfaces/executor_interface.h>` | `IExecutor`, `IJob`, `IExecutorProvider` |
| `<kcenon/common/interfaces/logger_interface.h>` | `ILogger`, `log_level` |
| `<kcenon/common/common.h>` | Umbrella header -- includes everything |
