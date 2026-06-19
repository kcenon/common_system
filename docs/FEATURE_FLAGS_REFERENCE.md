---
doc_id: "COM-GUID-004-REF"
doc_title: "Feature Flags Framework - API Reference"
doc_version: "1.0.0"
doc_date: "2026-04-04"
doc_status: "Released"
project: "common_system"
category: "GUID"
---

# Feature Flags Framework - API Reference

> **SSOT**: This document is part of the [Feature Flags Framework Guide](FEATURE_FLAGS_GUIDE.md).

<!-- TODO: target file does not exist -->
> **Language:** **English** | [한국어](FEATURE_FLAGS_GUIDE.kr.md)

Header-level API reference for all feature flag macros, detection logic, and system dependency flags.

---

## Table of Contents

- [Core Definitions (`feature_flags_core.h`)](#2-core-definitions-feature_flags_coreh)
  - [Preprocessor Helpers](#preprocessor-helpers)
  - [Compiler Detection](#compiler-detection)
  - [C++ Standard Detection](#c-standard-detection)
  - [Platform Detection](#platform-detection)
  - [Legacy Aliases](#legacy-aliases)
- [Feature Detection (`feature_detection.h`)](#3-feature-detection-feature_detectionh)
  - [C++20 Features](#c20-features)
  - [C++23 Features](#c23-features)
  - [Constexpr Enhancements](#constexpr-enhancements)
  - [Platform-Specific Features](#platform-specific-features)
- [Runtime Feature Flags (`feature_flags.h`)](#4-runtime-feature-flags-feature_flagsh)
  - [Flag Registration](#flag-registration)
  - [Dynamic Evaluation](#dynamic-evaluation)
  - [Flag Change Notifications](#flag-change-notifications)
  - [Thread-Safe Access](#thread-safe-access)
- [System Dependencies (`feature_system_deps.h`)](#5-system-dependencies-feature_system_depsh)
  - [System Module Integration](#system-module-integration)
  - [Dependency Resolution](#dependency-resolution)
  - [Optional vs Required](#optional-vs-required)
  - [Feature Compatibility Matrix](#feature-compatibility-matrix)

---

## 2. Core Definitions (`feature_flags_core.h`)

### Preprocessor Helpers

The core header provides helper macros for safe feature detection:

#### `KCENON_HAS_INCLUDE(x)`

Check if a header file is available:

```cpp
#if KCENON_HAS_INCLUDE(<source_location>)
    #include <source_location>
    // Use std::source_location
#else
    // Use fallback implementation
#endif
```

**Fallback**: Evaluates to `0` if `__has_include` is not supported by the compiler.

#### `KCENON_HAS_CPP_ATTRIBUTE(x)`

Check if a C++ attribute is available:

```cpp
#if KCENON_HAS_CPP_ATTRIBUTE(nodiscard)
    [[nodiscard]] int compute_value();
#else
    int compute_value();
#endif
```

**Fallback**: Evaluates to `0` if `__has_cpp_attribute` is not supported.

#### `KCENON_HAS_FEATURE(x)`

Check if a Clang-specific feature is available:

```cpp
#if KCENON_HAS_FEATURE(cxx_decltype_auto)
    // Use decltype(auto)
#endif
```

**Fallback**: Evaluates to `0` if `__has_feature` is not supported (non-Clang compilers).

#### `KCENON_HAS_BUILTIN(x)`

Check if a compiler builtin is available:

```cpp
#if KCENON_HAS_BUILTIN(__builtin_expect)
    #define KCENON_LIKELY(x) __builtin_expect(!!(x), 1)
#else
    #define KCENON_LIKELY(x) (x)
#endif
```

**Fallback**: Evaluates to `0` if `__has_builtin` is not supported.

### Compiler Detection

The framework detects the compiler being used:

| Macro | Description | Example Value |
|-------|-------------|---------------|
| `KCENON_COMPILER_MSVC` | Set to `1` if MSVC is detected | `1` or `0` |
| `KCENON_COMPILER_CLANG` | Set to `1` if Clang is detected | `1` or `0` |
| `KCENON_COMPILER_GCC` | Set to `1` if GCC is detected | `1` or `0` |
| `KCENON_COMPILER_VERSION` | MSVC version number | `1930` (MSVC 2022) |
| `KCENON_CLANG_VERSION` | Clang version number | `140001` (Clang 14.0.1) |
| `KCENON_GCC_VERSION` | GCC version number | `110201` (GCC 11.2.1) |

**Example**:

```cpp
#if KCENON_COMPILER_MSVC
    #pragma warning(disable: 4996)  // Disable specific MSVC warning
#elif KCENON_COMPILER_GCC || KCENON_COMPILER_CLANG
    #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
```

### C++ Standard Detection

The framework detects the C++ standard being used:

| Macro | Description | Condition |
|-------|-------------|-----------|
| `KCENON_CPLUSPLUS` | C++ standard version | `__cplusplus` or `_MSVC_LANG` (for MSVC) |
| `KCENON_HAS_CPP17` | Set to `1` if C++17 or later | `KCENON_CPLUSPLUS >= 201703L` |
| `KCENON_HAS_CPP20` | Set to `1` if C++20 or later | `KCENON_CPLUSPLUS >= 202002L` |
| `KCENON_HAS_CPP23` | Set to `1` if C++23 or later | `KCENON_CPLUSPLUS >= 202302L` |

**Example**:

```cpp
#if KCENON_HAS_CPP20
    #include <concepts>
    template <std::integral T>
    T add(T a, T b) { return a + b; }
#else
    template <typename T>
    T add(T a, T b) { return a + b; }
#endif
```

**Minimum Requirement**: The framework requires **C++17** as the minimum standard. A static assertion enforces this:

```cpp
static_assert(KCENON_HAS_CPP17,
    "common_system requires C++17 or later. "
    "Please compile with -std=c++17 or higher.");
```

### Platform Detection

The framework detects the target platform:

| Macro | Description | Platforms |
|-------|-------------|-----------|
| `KCENON_PLATFORM_WINDOWS` | Set to `1` on Windows | Windows (32-bit and 64-bit) |
| `KCENON_PLATFORM_LINUX` | Set to `1` on Linux | Linux distributions |
| `KCENON_PLATFORM_MACOS` | Set to `1` on macOS | macOS and iOS |
| `KCENON_PLATFORM_UNIX` | Set to `1` on Unix-like systems | Linux or macOS |

**Example**:

```cpp
#if KCENON_PLATFORM_WINDOWS
    #include <windows.h>
    void platform_init() {
        // Windows-specific initialization
        SetConsoleCP(CP_UTF8);
    }
#elif KCENON_PLATFORM_UNIX
    #include <unistd.h>
    void platform_init() {
        // Unix-specific initialization
    }
#endif
```

### Legacy Aliases

For backward compatibility, legacy macro names are supported:

| Legacy Macro | New Macro | Status |
|-------------|-----------|---------|
| `COMMON_HAS_SOURCE_LOCATION` | `KCENON_HAS_SOURCE_LOCATION` | ⚠️ Deprecated |
| `USE_THREAD_SYSTEM` | `KCENON_WITH_THREAD_SYSTEM` | ⚠️ Deprecated |
| `BUILD_WITH_LOGGER` | `KCENON_WITH_LOGGER_SYSTEM` | ⚠️ Deprecated |
| `WITH_MONITORING_SYSTEM` | `KCENON_WITH_MONITORING_SYSTEM` | ⚠️ Deprecated |

**Migration**: Legacy aliases can be disabled by setting `KCENON_ENABLE_LEGACY_ALIASES=0`:

```cmake
target_compile_definitions(my_target PRIVATE KCENON_ENABLE_LEGACY_ALIASES=0)
```

**Deprecation Timeline**:
- **v0.3.x**: Legacy aliases enabled by default, warnings in documentation
- **v0.4.0**: Legacy aliases disabled by default, opt-in required
- **v1.0.0**: Legacy aliases removed completely

---

## 3. Feature Detection (`feature_detection.h`)

The feature detection header provides macros for detecting C++20 and C++23 standard library features.

### C++20 Features

#### `KCENON_HAS_SOURCE_LOCATION`

Detect `std::source_location` availability:

```cpp
#if KCENON_HAS_SOURCE_LOCATION
    #include <source_location>

    void log_message(std::string_view message,
                     std::source_location loc = std::source_location::current()) {
        std::cout << loc.file_name() << ":" << loc.line() << " - " << message << "\n";
    }
#else
    void log_message(std::string_view message) {
        std::cout << message << "\n";
    }
#endif
```

**Detection Criteria**:
1. C++20 or later (`KCENON_HAS_CPP20`)
2. `<source_location>` header available
3. `__cpp_lib_source_location >= 201907L`

#### `KCENON_HAS_JTHREAD`

Detect `std::jthread` and `std::stop_token` availability:

```cpp
#if KCENON_HAS_JTHREAD
    #include <thread>

    std::jthread worker([](std::stop_token st) {
        while (!st.stop_requested()) {
            // Do work
        }
    });

    // Automatic joining and stop request on destruction
#else
    std::thread worker([] {
        while (running.load()) {
            // Do work
        }
    });

    worker.join();  // Manual joining required
#endif
```

**Detection Criteria**:
1. C++20 or later
2. `__cpp_lib_jthread >= 201911L` OR `__cpp_lib_stop_token >= 201907L`

#### `KCENON_HAS_FORMAT`

Detect `std::format` availability:

```cpp
#if KCENON_HAS_FORMAT
    #include <format>

    std::string message = std::format("User {} logged in at {}", username, timestamp);
#else
    #include <sstream>

    std::ostringstream oss;
    oss << "User " << username << " logged in at " << timestamp;
    std::string message = oss.str();
#endif
```

**Detection Criteria**:
1. C++20 or later
2. `<format>` header available
3. `__cpp_lib_format >= 201907L`

**Note**: `std::format` support varies by compiler:
- **GCC**: Supported from GCC 13+
- **Clang**: Supported from Clang 14+ (with libc++)
- **MSVC**: Supported from Visual Studio 2019 16.10+

#### `KCENON_HAS_SPAN`

Detect `std::span` availability:

```cpp
#if KCENON_HAS_SPAN
    #include <span>

    void process_data(std::span<const int> data) {
        for (int value : data) {
            // Process value
        }
    }

    std::vector<int> vec = {1, 2, 3, 4, 5};
    process_data(vec);  // Implicit conversion to span
#else
    void process_data(const int* data, size_t size) {
        for (size_t i = 0; i < size; ++i) {
            // Process data[i]
        }
    }

    std::vector<int> vec = {1, 2, 3, 4, 5};
    process_data(vec.data(), vec.size());
#endif
```

**Detection Criteria**:
1. C++20 or later
2. `<span>` header available
3. `__cpp_lib_span >= 202002L`

#### `KCENON_HAS_RANGES`

Detect C++20 Ranges library availability:

```cpp
#if KCENON_HAS_RANGES
    #include <ranges>
    #include <algorithm>

    std::vector<int> vec = {1, 2, 3, 4, 5};
    auto even = vec | std::views::filter([](int x) { return x % 2 == 0; })
                    | std::views::transform([](int x) { return x * 2; });

    for (int value : even) {
        std::cout << value << "\n";  // Prints: 4, 8
    }
#else
    std::vector<int> vec = {1, 2, 3, 4, 5};
    std::vector<int> result;

    std::copy_if(vec.begin(), vec.end(), std::back_inserter(result),
                 [](int x) { return x % 2 == 0; });

    std::transform(result.begin(), result.end(), result.begin(),
                   [](int x) { return x * 2; });

    for (int value : result) {
        std::cout << value << "\n";
    }
#endif
```

**Detection Criteria**:
1. C++20 or later
2. `<ranges>` header available
3. `__cpp_lib_ranges >= 201911L`

#### `KCENON_HAS_CONCEPTS`

Detect C++20 concepts availability:

```cpp
#if KCENON_HAS_CONCEPTS
    #include <concepts>

    template <std::integral T>
    T multiply(T a, T b) {
        return a * b;
    }
#else
    template <typename T>
    T multiply(T a, T b) {
        static_assert(std::is_integral_v<T>, "T must be an integral type");
        return a * b;
    }
#endif
```

**Detection Criteria**:
1. C++20 or later
2. `<concepts>` header available
3. `__cpp_concepts >= 201907L`

#### `KCENON_HAS_COROUTINES`

Detect C++20 coroutines availability:

```cpp
#if KCENON_HAS_COROUTINES
    #include <coroutine>

    struct Generator {
        struct promise_type { /* ... */ };
        // Coroutine implementation
    };

    Generator counter() {
        for (int i = 0; i < 10; ++i) {
            co_yield i;
        }
    }
#else
    // Manual state machine or callback-based approach
#endif
```

**Detection Criteria**:
1. C++20 or later
2. `<coroutine>` header available
3. `__cpp_impl_coroutine >= 201902L`

#### `KCENON_HAS_THREE_WAY_COMPARISON`

Detect C++20 three-way comparison (spaceship operator):

```cpp
#if KCENON_HAS_THREE_WAY_COMPARISON
    #include <compare>

    struct Point {
        int x, y;

        auto operator<=>(const Point&) const = default;
        // Generates all six comparison operators
    };
#else
    struct Point {
        int x, y;

        bool operator==(const Point& other) const {
            return x == other.x && y == other.y;
        }
        bool operator!=(const Point& other) const { return !(*this == other); }
        bool operator<(const Point& other) const {
            if (x != other.x) return x < other.x;
            return y < other.y;
        }
        bool operator<=(const Point& other) const { return !(other < *this); }
        bool operator>(const Point& other) const { return other < *this; }
        bool operator>=(const Point& other) const { return !(*this < other); }
    };
#endif
```

**Detection Criteria**:
1. C++20 or later
2. `<compare>` header available
3. `__cpp_impl_three_way_comparison >= 201907L`

### C++23 Features

#### `KCENON_HAS_EXPECTED`

Detect `std::expected` availability:

```cpp
#if KCENON_HAS_EXPECTED
    #include <expected>

    std::expected<int, std::string> divide(int a, int b) {
        if (b == 0) {
            return std::unexpected("Division by zero");
        }
        return a / b;
    }

    auto result = divide(10, 2);
    if (result.has_value()) {
        std::cout << "Result: " << result.value() << "\n";
    } else {
        std::cerr << "Error: " << result.error() << "\n";
    }
#else
    #include <kcenon/common/patterns/result.h>

    Result<int> divide(int a, int b) {
        if (b == 0) {
            return make_error<int>(1, "Division by zero", "divide");
        }
        return Result<int>::ok(a / b);
    }
#endif
```

**Detection Criteria**:
1. C++23 or later
2. `<expected>` header available
3. `__cpp_lib_expected >= 202202L`

#### `KCENON_HAS_STACKTRACE`

Detect `std::stacktrace` availability:

```cpp
#if KCENON_HAS_STACKTRACE
    #include <stacktrace>

    void log_error(const std::string& message) {
        std::cerr << "Error: " << message << "\n";
        std::cerr << "Stack trace:\n" << std::stacktrace::current() << "\n";
    }
#else
    void log_error(const std::string& message) {
        std::cerr << "Error: " << message << "\n";
        // Manual stack trace capture (platform-specific)
    }
#endif
```

**Detection Criteria**:
1. C++23 or later
2. `<stacktrace>` header available
3. `__cpp_lib_stacktrace >= 202011L`

#### `KCENON_HAS_OPTIONAL_MONADIC`

Detect `std::optional` monadic operations (C++23):

```cpp
#if KCENON_HAS_OPTIONAL_MONADIC
    #include <optional>

    std::optional<int> get_user_id();
    std::optional<std::string> get_username(int user_id);

    auto username = get_user_id()
        .and_then([](int id) { return get_username(id); })
        .or_else([] { return std::optional<std::string>("Guest"); });
#else
    auto user_id = get_user_id();
    std::optional<std::string> username;

    if (user_id.has_value()) {
        username = get_username(*user_id);
    }

    if (!username.has_value()) {
        username = "Guest";
    }
#endif
```

**Detection Criteria**:
1. C++23 or later
2. `__cpp_lib_optional >= 202110L`

### Constexpr Enhancements

#### `KCENON_HAS_CONSTEXPR_VECTOR`

Detect `constexpr std::vector` (C++20):

```cpp
#if KCENON_HAS_CONSTEXPR_VECTOR
    constexpr std::vector<int> create_vector() {
        std::vector<int> vec;
        vec.push_back(1);
        vec.push_back(2);
        vec.push_back(3);
        return vec;
    }

    constexpr auto vec = create_vector();
    static_assert(vec.size() == 3);
#else
    // Non-constexpr implementation
    std::vector<int> create_vector() {
        return {1, 2, 3};
    }
#endif
```

**Detection Criteria**:
1. C++20 or later
2. `__cpp_lib_constexpr_vector >= 201907L`

#### `KCENON_HAS_CONSTEXPR_STRING`

Detect `constexpr std::string` (C++20):

```cpp
#if KCENON_HAS_CONSTEXPR_STRING
    constexpr std::string create_string() {
        std::string str = "Hello, ";
        str += "World!";
        return str;
    }

    constexpr auto greeting = create_string();
#else
    std::string create_string() {
        return "Hello, World!";
    }
#endif
```

**Detection Criteria**:
1. C++20 or later
2. `__cpp_lib_constexpr_string >= 201907L`

### Platform-Specific Features

While `feature_detection.h` focuses on standard library features, platform-specific features can be detected using the macros from `feature_flags_core.h`:

```cpp
// SIMD instruction set detection
#if defined(__AVX2__)
    #define KCENON_HAS_AVX2 1
#else
    #define KCENON_HAS_AVX2 0
#endif

#if defined(__ARM_NEON)
    #define KCENON_HAS_NEON 1
#else
    #define KCENON_HAS_NEON 0
#endif

// Usage
#if KCENON_HAS_AVX2
    #include <immintrin.h>
    void vectorized_add(float* a, const float* b, size_t n) {
        // AVX2-optimized implementation
    }
#elif KCENON_HAS_NEON
    #include <arm_neon.h>
    void vectorized_add(float* a, const float* b, size_t n) {
        // NEON-optimized implementation
    }
#else
    void vectorized_add(float* a, const float* b, size_t n) {
        // Scalar fallback
        for (size_t i = 0; i < n; ++i) {
            a[i] += b[i];
        }
    }
#endif
```

---

## 4. Runtime Feature Flags (`feature_flags.h`)

While compile-time flags are defined in the headers, runtime feature flags are managed through the configuration system.

### Flag Registration

Runtime flags are typically registered during application initialization:

```cpp
#include <kcenon/common/config/unified_config.h>
#include <kcenon/common/config/config_loader.h>

int main(int argc, char** argv) {
    // Load configuration (includes runtime feature flags)
    auto config_result = config_loader::load("config.yaml");

    if (config_result.is_err()) {
        std::cerr << "Configuration error: " << config_result.error().message << "\n";
        return 1;
    }

    auto config = config_result.value();

    // Use runtime flags
    if (config.monitoring.enabled) {
        monitoring_system::start();
    }

    if (config.logger.async) {
        logger_system::init_async(config.logger);
    }

    return 0;
}
```

### Dynamic Evaluation

Runtime flags can be dynamically evaluated throughout the application:

```cpp
void process_data(const unified_config& config, const Data& data) {
    // Check runtime flag before expensive operation
    if (config.monitoring.tracing.enabled) {
        auto span = tracer::start_span("process_data");
        // Traced execution
        do_process(data);
        span.end();
    } else {
        // Non-traced execution
        do_process(data);
    }
}
```

### Flag Change Notifications

With hot-reload support via `config_watcher`, runtime flags can be changed without application restart:

```cpp
config_watcher watcher("config.yaml");

watcher.on_change([](const unified_config& old_cfg,
                     const unified_config& new_cfg) {
    // React to flag changes
    if (old_cfg.monitoring.enabled != new_cfg.monitoring.enabled) {
        if (new_cfg.monitoring.enabled) {
            monitoring_system::start();
        } else {
            monitoring_system::stop();
        }
    }

    if (old_cfg.logger.level != new_cfg.logger.level) {
        logger_system::set_level(new_cfg.logger.level);
    }
});

watcher.start();
```

### Thread-Safe Access

Runtime flag access is thread-safe when using `config_watcher`:

```cpp
// Thread-safe read access (shared lock)
const unified_config& current = watcher.current();

// Multiple threads can read simultaneously
std::thread t1([&watcher] {
    auto config = watcher.current();
    // Use config
});

std::thread t2([&watcher] {
    auto config = watcher.current();
    // Use config
});
```

---

## 5. System Dependencies (`feature_system_deps.h`)

### System Module Integration

The framework supports integration flags for 7 core kcenon systems:

| Macro | System | Description |
|-------|--------|-------------|
| `KCENON_WITH_THREAD_SYSTEM` | thread_system | Thread pool, cooperative cancellation |
| `KCENON_WITH_LOGGER_SYSTEM` | logger_system | Structured logging, log levels |
| `KCENON_WITH_MONITORING_SYSTEM` | monitoring_system | Metrics, health checks, tracing |
| `KCENON_WITH_CONTAINER_SYSTEM` | container_system | Specialized containers (queues, etc.) |
| `KCENON_WITH_NETWORK_SYSTEM` | network_system | Network communication, TLS |
| `KCENON_WITH_DATABASE_SYSTEM` | database_system | Database connections, pooling |
| `KCENON_WITH_MESSAGING_SYSTEM` | messaging_system | Message queuing, pub/sub |

**Example**:

```cpp
#include <kcenon/common/config/feature_flags.h>

#if KCENON_WITH_LOGGER_SYSTEM
    #include <kcenon/logger/logger.h>
    using logger_type = kcenon::logger::logger;
#else
    using logger_type = std::ostream;  // fallback to std::cout
#endif

void log_message(logger_type& logger, const std::string& message) {
#if KCENON_WITH_LOGGER_SYSTEM
    logger.info(message);
#else
    logger << message << "\n";
#endif
}
```

### Dependency Resolution

System integration flags are resolved at build time:

```
┌──────────────────────────────────────────────────┐
│ 1. CMake Configuration                           │
│    - User specifies which systems to enable      │
│    - option(ENABLE_LOGGER_INTEGRATION ON)        │
└────────────────┬─────────────────────────────────┘
                 ▼
┌──────────────────────────────────────────────────┐
│ 2. CMake Generates Compile Definitions           │
│    - -DKCENON_WITH_LOGGER_SYSTEM=1              │
│    - -DKCENON_WITH_THREAD_SYSTEM=1              │
└────────────────┬─────────────────────────────────┘
                 ▼
┌──────────────────────────────────────────────────┐
│ 3. Preprocessor Evaluation                       │
│    - #if KCENON_WITH_LOGGER_SYSTEM               │
│    - Conditional compilation                     │
└────────────────┬─────────────────────────────────┘
                 ▼
┌──────────────────────────────────────────────────┐
│ 4. Linker Resolution                             │
│    - Link with enabled system libraries          │
│    - target_link_libraries(app logger_system)    │
└──────────────────────────────────────────────────┘
```

### Optional vs Required

System dependencies can be optional or required:

**Optional Dependencies** (default):

```cpp
#if KCENON_WITH_LOGGER_SYSTEM
    // Enhanced logging available
    logger.debug("Detailed message");
#else
    // Fallback to basic logging
    std::cout << "Message\n";
#endif
```

**Required Dependencies** (enforced via static assertion):

```cpp
#if !KCENON_WITH_THREAD_SYSTEM
    #error "thread_system is required for this application"
#endif

int main() {
    thread_pool pool;
    // Use thread_system
}
```

### Feature Compatibility Matrix

Some features require specific system combinations:

| Feature | Requires | Notes |
|---------|----------|-------|
| **Async logging** | `KCENON_WITH_LOGGER_SYSTEM` + `KCENON_WITH_THREAD_SYSTEM` | Needs thread pool for async writes |
| **Distributed tracing** | `KCENON_WITH_MONITORING_SYSTEM` + `KCENON_WITH_NETWORK_SYSTEM` | Needs network for trace export |
| **Database connection pooling** | `KCENON_WITH_DATABASE_SYSTEM` + `KCENON_WITH_THREAD_SYSTEM` | Needs thread pool for connection management |

**Example**:

```cpp
#if KCENON_WITH_LOGGER_SYSTEM && KCENON_WITH_THREAD_SYSTEM
    // Async logging available
    logger_config config;
    config.async = true;
    logger_system::init(config);
#elif KCENON_WITH_LOGGER_SYSTEM
    // Synchronous logging only
    logger_config config;
    config.async = false;
    logger_system::init(config);
#else
    #error "logger_system is required"
#endif
```

---

**Version**: 1.0.0
**Last Updated**: 2026-02-08
