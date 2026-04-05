# Getting Started with common_system

This guide walks you through installing **common_system**, writing your first
program, and exploring the core patterns you will use every day.

---

## 1. Prerequisites

| Requirement | Minimum Version |
|-------------|----------------|
| C++ standard | C++20 |
| GCC | 12+ |
| Clang | 15+ |
| Apple Clang | 14+ |
| MSVC | 2022 (17.4+) |
| CMake | 3.20+ |

Optional:

* **yaml-cpp** -- required only if you enable `BUILD_WITH_YAML_CPP` for
  YAML-based configuration loading.

---

## 2. Installation

### Method A -- CMake FetchContent (recommended)

Add the following to your project's `CMakeLists.txt`:

```cmake
include(FetchContent)

FetchContent_Declare(
    common_system
    GIT_REPOSITORY https://github.com/kcenon/common_system.git
    GIT_TAG        main   # or a specific release tag, e.g. v0.2.0
)
FetchContent_MakeAvailable(common_system)

# Link against the library
target_link_libraries(my_app PRIVATE kcenon::common_system)
```

### Method B -- vcpkg

If you maintain a local vcpkg overlay:

```bash
# Register the overlay port, then:
vcpkg install common-system
```

Then in CMake:

```cmake
find_package(common_system CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE kcenon::common_system)
```

### Method C -- Manual build and install

```bash
git clone https://github.com/kcenon/common_system.git
cd common_system

# Configure (header-only by default)
cmake --preset default

# Build
cmake --build build

# Install system-wide (optional)
cmake --install build --prefix /usr/local
```

To build with tests and examples:

```bash
cmake --preset debug
cmake --build build-debug
cd build-debug && ctest --output-on-failure
```

---

## 3. Your First Program

The `Result<T>` type is the primary error-handling mechanism in
common_system. It replaces exceptions with an explicit success-or-error
value, inspired by Rust's `Result`.

### 3.1 CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.20)
project(hello_result LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    common_system
    GIT_REPOSITORY https://github.com/kcenon/common_system.git
    GIT_TAG        main
)
FetchContent_MakeAvailable(common_system)

add_executable(hello_result main.cpp)
target_link_libraries(hello_result PRIVATE kcenon::common_system)
```

### 3.2 main.cpp

```cpp
#include <kcenon/common/patterns/result.h>
#include <iostream>
#include <string>

using namespace kcenon::common;

// A function that can fail returns Result<T> instead of throwing.
Result<int> parse_port(const std::string& input) {
    try {
        int port = std::stoi(input);
        if (port < 1 || port > 65535) {
            return make_error<int>(
                error_codes::INVALID_ARGUMENT,
                "Port out of range: " + input,
                "config"
            );
        }
        return ok(port);
    } catch (const std::exception& e) {
        return make_error<int>(
            error_codes::INVALID_ARGUMENT,
            std::string("Cannot parse port: ") + e.what(),
            "config"
        );
    }
}

int main() {
    // --- Success path ---
    auto result = parse_port("8080");
    if (result.is_ok()) {
        std::cout << "Listening on port " << result.value() << "\n";
    }

    // --- Error path ---
    auto bad = parse_port("abc");
    if (bad.is_err()) {
        const auto& err = bad.error();
        std::cout << "Error: " << err.message
                  << " (code " << err.code << ")\n";
    }

    // --- Default value ---
    int port = parse_port("xyz").unwrap_or(3000);
    std::cout << "Fallback port: " << port << "\n";

    // --- Monadic chaining ---
    auto doubled = parse_port("4000")
        .map([](int p) { return p * 2; })
        .unwrap_or(0);
    std::cout << "Doubled port: " << doubled << "\n";

    return 0;
}
```

Build and run:

```bash
cmake -B build -DCMAKE_CXX_STANDARD=20
cmake --build build
./build/hello_result
```

Expected output:

```
Listening on port 8080
Error: Cannot parse port: stoi (code -1)
Fallback port: 3000
Doubled port: 8000
```

---

## 4. Dependency Injection

The `service_container` provides a thread-safe dependency injection
container with singleton, transient, and scoped lifetimes.

```cpp
#include <kcenon/common/di/service_container.h>
#include <iostream>
#include <memory>

using namespace kcenon::common;
using namespace kcenon::common::di;

// 1. Define an interface
class IGreeter {
public:
    virtual ~IGreeter() = default;
    virtual std::string greet(const std::string& name) const = 0;
};

// 2. Implement it
class FriendlyGreeter : public IGreeter {
public:
    std::string greet(const std::string& name) const override {
        return "Hello, " + name + "!";
    }
};

int main() {
    // 3. Get the global container
    auto& container = service_container::global();

    // 4. Register: interface -> implementation, singleton lifetime
    container.register_type<IGreeter, FriendlyGreeter>(
        service_lifetime::singleton
    );

    // 5. Resolve
    auto result = container.resolve<IGreeter>();
    if (result.is_ok()) {
        auto greeter = result.value();
        std::cout << greeter->greet("World") << "\n";
    } else {
        std::cerr << "Resolve failed: " << result.error().message << "\n";
    }

    // 6. Factory registration (when you need constructor arguments)
    container.register_factory<IGreeter>(
        [](IServiceContainer&) -> std::shared_ptr<IGreeter> {
            return std::make_shared<FriendlyGreeter>();
        },
        service_lifetime::transient
    );

    // 7. Scoped containers isolate per-request state
    auto scope = container.create_scope();
    auto scoped = scope->resolve<IGreeter>();

    // 8. Freeze to prevent further registration (security best practice)
    container.freeze();

    return 0;
}
```

### Lifetime summary

| Lifetime | Instances | Typical use |
|----------|-----------|-------------|
| `singleton` | One for the entire application | Loggers, configuration |
| `transient` | New instance per `resolve()` call | Stateful per-consumer services |
| `scoped` | One per `IServiceScope` | Request-scoped / unit-of-work |

---

## 5. Configuration

### 5.1 Config loader

Load configuration from a YAML file (requires `BUILD_WITH_YAML_CPP=ON`):

```cpp
#include <kcenon/common/config/config_loader.h>
#include <iostream>

using namespace kcenon::common::config;

int main() {
    // Load from file -- environment variables (UNIFIED_*) override file values.
    auto result = config_loader::load("app.yaml");
    if (result.is_err()) {
        std::cerr << "Config error: " << result.error().message << "\n";
        // Fall back to defaults
        auto config = config_loader::defaults();
    }

    auto config = result.value();
    // Access config fields...

    return 0;
}
```

Configuration priority (highest wins):

1. CLI arguments (`--set key=value`)
2. Environment variables (`UNIFIED_*` prefix)
3. YAML configuration file
4. Built-in defaults

### 5.2 CLI config parser

```cpp
#include <kcenon/common/config/cli_config_parser.h>

int main(int argc, char** argv) {
    auto result = cli_config_parser::load_with_cli_overrides(argc, argv);
    if (result.is_err()) {
        cli_config_parser::print_help(argv[0]);
        return 1;
    }
    auto config = result.value();
    // Use config...
    return 0;
}
```

### 5.3 Config watcher (hot-reload)

```cpp
#include <kcenon/common/config/config_watcher.h>

// config_watcher monitors the file system and reloads on changes.
// Supports inotify (Linux), kqueue (macOS), and ReadDirectoryChangesW (Windows).
```

### 5.4 Feature flags

Feature flags are compile-time macros for detecting platform and C++
library capabilities:

```cpp
#include <kcenon/common/config/feature_flags.h>

#if KCENON_HAS_SOURCE_LOCATION
    #include <source_location>
#endif

#if KCENON_HAS_JTHREAD
    std::jthread worker([](std::stop_token st) { /* ... */ });
#endif
```

---

## 6. Error Handling

### 6.1 Error codes and categories

Error codes are organized by system into non-overlapping ranges:

| Range | System |
|-------|--------|
| 0 | Success |
| -1 to -99 | common_system |
| -100 to -199 | thread_system |
| -200 to -299 | logger_system |
| -300 to -399 | monitoring_system |
| -400 to -499 | container_system |
| -500 to -599 | database_system |
| -600 to -699 | network_system |

### 6.2 Creating errors

```cpp
#include <kcenon/common/patterns/result.h>

using namespace kcenon::common;

// Construct an error with code, message, and originating module
auto err = make_error<std::string>(
    error_codes::INVALID_ARGUMENT,   // code
    "Name must not be empty",        // human-readable message
    "user_service"                   // originating module
);
```

### 6.3 Result<T> patterns

```cpp
Result<int> compute(int x);

// Pattern 1: Check and access
auto r = compute(42);
if (r.is_ok()) {
    use(r.value());
} else {
    log(r.error().message);
}

// Pattern 2: Default value
int val = compute(42).unwrap_or(-1);

// Pattern 3: Transform the success value
auto doubled = compute(42).map([](int v) { return v * 2; });

// Pattern 4: Chain fallible operations
auto chained = compute(42).and_then([](int v) { return compute(v + 1); });

// Pattern 5: Recover from errors
auto recovered = compute(42).or_else([](const error_info&) { return ok(0); });

// Pattern 6: Wrap throwing code
auto safe = try_catch<int>([] {
    return might_throw();
}, "my_module");
```

### 6.4 VoidResult

For functions that succeed or fail but return no value:

```cpp
VoidResult save(const Data& data) {
    if (!valid(data)) {
        return make_error<std::monostate>(
            error_codes::INVALID_ARGUMENT,
            "Invalid data",
            "storage"
        );
    }
    // ... perform save ...
    return VoidResult(std::monostate{});
}
```

---

## 7. Next Steps

* **Examples** -- Browse the `examples/` directory for complete, runnable
  programs covering Result<T>, executors, unwrap diagnostics, and
  multi-system integration.
* **API Reference** -- See [docs/API_REFERENCE.md](API_REFERENCE.md) for
  the full public API surface.
* **Architecture** -- See [docs/ARCHITECTURE.md](ARCHITECTURE.md) for the
  module layout, dependency graph, and design decisions.
* **Error Code Registry** -- See
  [docs/ERROR_CODE_REGISTRY.md](ERROR_CODE_REGISTRY.md) for the complete
  list of error codes across all systems.
* **Integration Guide** -- See
  [docs/INTEGRATION_GUIDE.md](INTEGRATION_GUIDE.md) for combining
  common_system with thread_system, logger_system, database_system, and
  other downstream projects.
