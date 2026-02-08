# Feature Flags Framework Guide

> **Language:** **English** | [한국어](FEATURE_FLAGS_GUIDE.kr.md)

**Complete Guide**: Compile-Time Feature Detection, Runtime Feature Flags, and Cross-System Integration

**Status**: ✅ **Complete**

This guide provides comprehensive documentation for the feature flags framework in common_system, including compile-time feature detection (`feature_detection.h`), system module integration flags (`feature_system_deps.h`), and runtime configuration.

---

## Table of Contents

- [Overview](#overview)
- [Feature Flags System Overview](#1-feature-flags-system-overview)
  - [Compile-Time vs Runtime](#compile-time-vs-runtime)
  - [Flag Lifecycle](#flag-lifecycle)
  - [Architecture Diagram](#architecture-diagram)
- [Core Definitions](#2-core-definitions-feature_flags_coreh)
  - [Preprocessor Helpers](#preprocessor-helpers)
  - [Compiler Detection](#compiler-detection)
  - [C++ Standard Detection](#c-standard-detection)
  - [Platform Detection](#platform-detection)
  - [Legacy Aliases](#legacy-aliases)
- [Feature Detection](#3-feature-detection-feature_detectionh)
  - [C++20 Features](#c20-features)
  - [C++23 Features](#c23-features)
  - [Constexpr Enhancements](#constexpr-enhancements)
  - [Platform-Specific Features](#platform-specific-features)
- [Runtime Feature Flags](#4-runtime-feature-flags-feature_flagsh)
  - [Flag Registration](#flag-registration)
  - [Dynamic Evaluation](#dynamic-evaluation)
  - [Flag Change Notifications](#flag-change-notifications)
  - [Thread-Safe Access](#thread-safe-access)
- [System Dependencies](#5-system-dependencies-feature_system_depsh)
  - [System Module Integration](#system-module-integration)
  - [Dependency Resolution](#dependency-resolution)
  - [Optional vs Required](#optional-vs-required)
  - [Feature Compatibility Matrix](#feature-compatibility-matrix)
- [Usage Examples](#6-usage-examples)
  - [Compile-Time Examples](#compile-time-examples)
  - [Runtime Examples](#runtime-examples)
  - [Cross-System Integration](#cross-system-integration)
- [CMake Integration](#7-cmake-integration)
- [Migration Guide](#8-migration-guide)
- [Troubleshooting](#9-troubleshooting)
- [Related Documentation](#related-documentation)

---

## Overview

The feature flags framework provides a robust, production-ready solution for managing compile-time and runtime feature detection across the kcenon ecosystem.

### Key Features

| Feature | Description | Header File |
|---------|-------------|-------------|
| **Compile-Time Detection** | Detect compiler, platform, and C++ standard features | `feature_flags_core.h`, `feature_detection.h` |
| **System Integration** | Manage dependencies between ecosystem modules | `feature_system_deps.h` |
| **Legacy Compatibility** | Backward-compatible aliases for old macro names | `feature_flags.h` |
| **Runtime Flags** | Dynamic feature activation (via configuration) | `unified_config.h` integration |

### Feature Flag Files

The feature flags subsystem is located at `include/kcenon/common/config/`:

- `feature_flags_core.h` — Preprocessor helpers, compiler/platform detection
- `feature_detection.h` — C++ standard library feature detection
- `feature_system_deps.h` — Cross-system integration flags
- `feature_flags.h` — Main entry point (includes all above)

---

## 1. Feature Flags System Overview

### Compile-Time vs Runtime

The feature flags framework supports two types of feature flags:

#### Compile-Time Flags

Compile-time flags are resolved at build time using preprocessor macros:

```cpp
#include <kcenon/common/config/feature_flags.h>

#if KCENON_HAS_SOURCE_LOCATION
    #include <source_location>
    using location_type = std::source_location;
#else
    using location_type = kcenon::common::source_location;  // fallback
#endif
```

**Advantages**:
- Zero runtime overhead (code is eliminated at compile time)
- Type-safe conditional compilation
- Platform-specific optimizations

**Use Cases**:
- Platform-specific code paths (Windows vs Linux vs macOS)
- C++ standard feature detection (C++17 vs C++20 vs C++23)
- Compiler-specific optimizations (MSVC vs GCC vs Clang)
- Hardware capability detection (SIMD: AVX2, NEON)

#### Runtime Flags

Runtime flags are resolved at application startup or during execution:

```cpp
if (config.monitoring.enabled) {
    // Enable monitoring subsystem
    monitoring_system::start();
}
```

**Advantages**:
- No recompilation required for flag changes
- Dynamic feature toggling via configuration files
- A/B testing and gradual rollouts

**Use Cases**:
- Feature toggles (enable/disable features without redeploying)
- Experimental features with gradual rollout
- Environment-specific behavior (dev vs staging vs production)
- Hot-reload of feature flags via configuration

### Flag Lifecycle

Feature flags follow a standard lifecycle:

```
┌──────────────────────────────────────────────────┐
│ 1. Define                                        │
│    - Add KCENON_* macro in feature_*.h           │
│    - Document detection criteria                 │
└────────────────┬─────────────────────────────────┘
                 ▼
┌──────────────────────────────────────────────────┐
│ 2. Activate                                      │
│    - CMake sets compile definition               │
│    - Or: Auto-detected via compiler/platform     │
└────────────────┬─────────────────────────────────┘
                 ▼
┌──────────────────────────────────────────────────┐
│ 3. Check                                         │
│    - #if KCENON_* in application code            │
│    - Conditional compilation based on flag       │
└────────────────┬─────────────────────────────────┘
                 ▼
┌──────────────────────────────────────────────────┐
│ 4. Deprecate (Optional)                          │
│    - Mark as deprecated in documentation         │
│    - Provide migration path                      │
│    - Keep for backward compatibility             │
└────────────────┬─────────────────────────────────┘
                 ▼
┌──────────────────────────────────────────────────┐
│ 5. Remove (Major Version)                        │
│    - Remove in next major version                │
│    - Update documentation and migration guide    │
└──────────────────────────────────────────────────┘
```

### Architecture Diagram

The feature flags framework integrates with the broader kcenon ecosystem:

```
┌────────────────────────────────────────────────────────────┐
│                    Application Code                        │
│                                                            │
│  #if KCENON_HAS_JTHREAD                                   │
│      std::jthread worker(...);                            │
│  #endif                                                    │
│                                                            │
│  #if KCENON_WITH_LOGGER_SYSTEM                            │
│      logger.log("message");                               │
│  #endif                                                    │
└──────────────┬─────────────────────────────────────────────┘
               │ Includes
               ▼
┌────────────────────────────────────────────────────────────┐
│              feature_flags.h (Main Entry Point)            │
│  • Aggregates all feature detection headers                │
│  • Provides legacy aliases                                 │
│  • Static assertions for minimum requirements              │
└──────────────┬─────────────────────────────────────────────┘
               │ Includes
        ┌──────┴──────┬───────────────────┐
        ▼             ▼                   ▼
┌───────────────┐ ┌───────────────┐ ┌───────────────┐
│feature_flags_ │ │feature_       │ │feature_system_│
│core.h         │ │detection.h    │ │deps.h         │
│               │ │               │ │               │
│• Compiler     │ │• C++20/23     │ │• Thread       │
│  detection    │ │  features     │ │  system       │
│• Platform     │ │• source_      │ │• Logger       │
│  detection    │ │  location     │ │  system       │
│• C++ standard │ │• jthread      │ │• Monitoring   │
│• Preprocessor │ │• format       │ │• Database     │
│  helpers      │ │• span         │ │• Network      │
└───────────────┘ └───────────────┘ └───────────────┘
        │                 │                   │
        │ Detected at     │ Detected at       │ Set via
        │ compile time    │ compile time      │ CMake
        ▼                 ▼                   ▼
┌────────────────────────────────────────────────────────────┐
│              Compile Definitions                           │
│  -DKCENON_COMPILER_CLANG=1                                │
│  -DKCENON_HAS_CPP20=1                                     │
│  -DKCENON_WITH_LOGGER_SYSTEM=1                            │
└────────────────────────────────────────────────────────────┘
```

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

## 6. Usage Examples

### Compile-Time Examples

#### Example 1: Platform-Specific Code

```cpp
#include <kcenon/common/config/feature_flags.h>

class FileWatcher {
public:
    FileWatcher(const std::string& path) : path_(path) {
#if KCENON_PLATFORM_LINUX
        // Use inotify
        fd_ = inotify_init();
        watch_ = inotify_add_watch(fd_, path.c_str(), IN_MODIFY);
#elif KCENON_PLATFORM_MACOS
        // Use kqueue
        kq_ = kqueue();
        // Setup kqueue monitoring
#elif KCENON_PLATFORM_WINDOWS
        // Use ReadDirectoryChangesW
        handle_ = CreateFile(path.c_str(), ...);
#endif
    }

    ~FileWatcher() {
#if KCENON_PLATFORM_LINUX
        inotify_rm_watch(fd_, watch_);
        close(fd_);
#elif KCENON_PLATFORM_MACOS
        close(kq_);
#elif KCENON_PLATFORM_WINDOWS
        CloseHandle(handle_);
#endif
    }

private:
    std::string path_;
#if KCENON_PLATFORM_LINUX
    int fd_;
    int watch_;
#elif KCENON_PLATFORM_MACOS
    int kq_;
#elif KCENON_PLATFORM_WINDOWS
    HANDLE handle_;
#endif
};
```

#### Example 2: C++ Standard Feature Detection

```cpp
#include <kcenon/common/config/feature_flags.h>

template <typename T>
class Result {
public:
#if KCENON_HAS_EXPECTED
    // Use std::expected as underlying type
    using storage_type = std::expected<T, ErrorInfo>;
#else
    // Use custom Result implementation
    using storage_type = internal::result_storage<T>;
#endif

    static Result ok(T value) {
#if KCENON_HAS_EXPECTED
        return Result(std::expected<T, ErrorInfo>(std::move(value)));
#else
        return Result(internal::result_storage<T>::ok(std::move(value)));
#endif
    }

    static Result err(const ErrorInfo& error) {
#if KCENON_HAS_EXPECTED
        return Result(std::unexpected(error));
#else
        return Result(internal::result_storage<T>::err(error));
#endif
    }

private:
    storage_type storage_;
};
```

#### Example 3: Compiler-Specific Optimizations

```cpp
#include <kcenon/common/config/feature_flags.h>

class HotPath {
public:
#if KCENON_COMPILER_GCC || KCENON_COMPILER_CLANG
    __attribute__((hot))
#endif
    int fast_computation(int x) {
#if KCENON_HAS_BUILTIN(__builtin_expect)
        if (__builtin_expect(x > 0, 1)) {
            // Hot path (expected to be true)
            return x * 2;
        } else {
            // Cold path
            return 0;
        }
#else
        if (x > 0) {
            return x * 2;
        } else {
            return 0;
        }
#endif
    }
};
```

### Runtime Examples

#### Example 1: Feature Toggle

```cpp
#include <kcenon/common/config/config_loader.h>

class Application {
public:
    Application(const unified_config& config) : config_(config) {}

    void run() {
        // Feature toggle: Enable experimental feature via configuration
        if (config_.monitoring.tracing.enabled) {
            run_with_tracing();
        } else {
            run_without_tracing();
        }
    }

private:
    void run_with_tracing() {
        auto tracer = create_tracer(config_.monitoring.tracing);

        for (const auto& request : requests_) {
            auto span = tracer.start_span("process_request");
            process_request(request);
            span.end();
        }
    }

    void run_without_tracing() {
        for (const auto& request : requests_) {
            process_request(request);
        }
    }

    unified_config config_;
};
```

#### Example 2: Gradual Rollout

```cpp
class FeatureGate {
public:
    bool is_enabled(const std::string& feature_name, const std::string& user_id) {
        // Percentage-based rollout
        auto hash = std::hash<std::string>{}(user_id);
        double percentage = (hash % 100) / 100.0;

        if (feature_name == "new_algorithm") {
            // Gradually roll out to 20% of users
            return percentage < 0.20;
        }

        return false;
    }
};

void process_data(const Data& data, const std::string& user_id, FeatureGate& gate) {
    if (gate.is_enabled("new_algorithm", user_id)) {
        // New algorithm (20% of users)
        process_with_new_algorithm(data);
    } else {
        // Old algorithm (80% of users)
        process_with_old_algorithm(data);
    }
}
```

### Cross-System Integration

#### Example 1: Logger + Thread System

```cpp
#include <kcenon/common/config/feature_flags.h>

#if KCENON_WITH_LOGGER_SYSTEM && KCENON_WITH_THREAD_SYSTEM
    #include <kcenon/logger/async_logger.h>
    #include <kcenon/thread/thread_pool.h>

    class Application {
    public:
        Application(const unified_config& config)
            : pool_(config.thread.pool_size)
            , logger_(pool_, config.logger) {}

        void process_request(const Request& req) {
            // Submit to thread pool
            pool_.submit([this, req] {
                // Log asynchronously
                logger_.info("Processing request: {}", req.id);
                do_process(req);
            });
        }

    private:
        thread_pool pool_;
        async_logger logger_;
    };
#else
    #error "This application requires logger_system and thread_system"
#endif
```

#### Example 2: Database + Monitoring System

```cpp
#if KCENON_WITH_DATABASE_SYSTEM && KCENON_WITH_MONITORING_SYSTEM
    #include <kcenon/database/connection_pool.h>
    #include <kcenon/monitoring/metrics.h>

    class DataAccessLayer {
    public:
        DataAccessLayer(const unified_config& config)
            : pool_(config.database)
            , query_counter_("database.queries.total")
            , query_duration_("database.queries.duration") {}

        Result<User> get_user(int user_id) {
            query_counter_.increment();

            auto timer = query_duration_.start_timer();

            auto conn = pool_.acquire();
            if (conn.is_err()) {
                return make_error<User>(conn.error());
            }

            return conn.value()->execute_query<User>(
                "SELECT * FROM users WHERE id = ?", user_id);
        }

    private:
        connection_pool pool_;
        counter_metric query_counter_;
        histogram_metric query_duration_;
    };
#endif
```

---

## 7. CMake Integration

Feature flags are typically configured via CMake:

```cmake
# CMakeLists.txt

# Option to enable system integrations
option(ENABLE_LOGGER_INTEGRATION "Enable logger_system integration" ON)
option(ENABLE_THREAD_INTEGRATION "Enable thread_system integration" ON)
option(ENABLE_MONITORING_INTEGRATION "Enable monitoring_system integration" OFF)

# Configure feature flags
if(ENABLE_LOGGER_INTEGRATION)
    target_compile_definitions(common_system PUBLIC KCENON_WITH_LOGGER_SYSTEM=1)
    target_link_libraries(common_system PUBLIC logger_system)
endif()

if(ENABLE_THREAD_INTEGRATION)
    target_compile_definitions(common_system PUBLIC KCENON_WITH_THREAD_SYSTEM=1)
    target_link_libraries(common_system PUBLIC thread_system)
endif()

if(ENABLE_MONITORING_INTEGRATION)
    target_compile_definitions(common_system PUBLIC KCENON_WITH_MONITORING_SYSTEM=1)
    target_link_libraries(common_system PUBLIC monitoring_system)
endif()

# C++ standard selection
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Print feature summary (optional)
add_compile_definitions(KCENON_PRINT_FEATURE_SUMMARY)
```

**Build Configuration**:

```bash
# Enable specific systems
cmake -B build -DENABLE_LOGGER_INTEGRATION=ON \
               -DENABLE_THREAD_INTEGRATION=ON \
               -DENABLE_MONITORING_INTEGRATION=OFF

# Build with C++20
cmake -B build -DCMAKE_CXX_STANDARD=20

# Build with C++23 (if compiler supports)
cmake -B build -DCMAKE_CXX_STANDARD=23
```

---

## 8. Migration Guide

### Migrating from Legacy Macros

#### Step 1: Identify Legacy Macros

Search for legacy macro usage in your codebase:

```bash
# Find all legacy macro usage
grep -r "COMMON_HAS_SOURCE_LOCATION" src/
grep -r "USE_THREAD_SYSTEM" src/
grep -r "BUILD_WITH_LOGGER" src/
```

#### Step 2: Replace with New Macros

| Legacy Macro | New Macro |
|-------------|-----------|
| `COMMON_HAS_SOURCE_LOCATION` | `KCENON_HAS_SOURCE_LOCATION` |
| `USE_THREAD_SYSTEM` | `KCENON_WITH_THREAD_SYSTEM` |
| `BUILD_WITH_LOGGER` | `KCENON_WITH_LOGGER_SYSTEM` |
| `BUILD_WITH_MONITORING` | `KCENON_WITH_MONITORING_SYSTEM` |
| `WITH_THREAD_SYSTEM` | `KCENON_WITH_THREAD_SYSTEM` |

**Before**:

```cpp
#if USE_THREAD_SYSTEM
    #include <kcenon/thread/thread_pool.h>
#endif

#if COMMON_HAS_SOURCE_LOCATION
    #include <source_location>
#endif
```

**After**:

```cpp
#if KCENON_WITH_THREAD_SYSTEM
    #include <kcenon/thread/thread_pool.h>
#endif

#if KCENON_HAS_SOURCE_LOCATION
    #include <source_location>
#endif
```

#### Step 3: Update CMake Configuration

Update `CMakeLists.txt` to use new macro names:

**Before**:

```cmake
option(USE_THREAD_SYSTEM "Enable thread_system" ON)
if(USE_THREAD_SYSTEM)
    target_compile_definitions(app PUBLIC USE_THREAD_SYSTEM=1)
endif()
```

**After**:

```cmake
option(ENABLE_THREAD_INTEGRATION "Enable thread_system" ON)
if(ENABLE_THREAD_INTEGRATION)
    target_compile_definitions(app PUBLIC KCENON_WITH_THREAD_SYSTEM=1)
endif()
```

#### Step 4: Test Migration

1. Build with legacy aliases enabled (default):
   ```bash
   cmake -B build
   make -C build
   ```

2. Run tests to ensure no regression:
   ```bash
   ctest --test-dir build --output-on-failure
   ```

3. Disable legacy aliases to verify migration:
   ```cmake
   target_compile_definitions(app PUBLIC KCENON_ENABLE_LEGACY_ALIASES=0)
   ```

4. Fix any remaining legacy macro usage

### Gradual Migration Strategy

For large codebases, use a gradual migration approach:

1. **Phase 1**: Keep legacy aliases enabled, add new macro usage
   - Deadline: v0.3.x releases
   - Action: New code uses `KCENON_*` macros only

2. **Phase 2**: Migrate existing code module by module
   - Deadline: Before v0.4.0
   - Action: Replace legacy macros with `KCENON_*` macros

3. **Phase 3**: Disable legacy aliases by default
   - Deadline: v0.4.0
   - Action: `KCENON_ENABLE_LEGACY_ALIASES=0` by default

4. **Phase 4**: Remove legacy alias support
   - Deadline: v1.0.0
   - Action: Delete legacy alias definitions

---

## 9. Troubleshooting

### Feature Not Detected

**Symptom**:

```cpp
#if KCENON_HAS_FORMAT
    // This block is never compiled
#endif
```

**Solution**:

1. Check compiler support:
   ```bash
   # GCC
   g++ --version
   # GCC 13+ required for std::format

   # Clang
   clang++ --version
   # Clang 14+ required for std::format
   ```

2. Verify C++ standard:
   ```bash
   # Check CMake configuration
   grep CMAKE_CXX_STANDARD CMakeLists.txt

   # Should be 20 or higher for std::format
   set(CMAKE_CXX_STANDARD 20)
   ```

3. Enable feature summary to debug:
   ```cmake
   add_compile_definitions(KCENON_PRINT_FEATURE_SUMMARY)
   ```

   **Output**:
   ```
   === KCENON Feature Detection Summary ===
   Compiler: GCC
   C++ Standard: C++20
   source_location: Available
   jthread: Available
   std::format: Unavailable (GCC 12 doesn't support std::format)
   Concepts: Available
   Ranges: Available
   =========================================
   ```

### System Integration Not Working

**Symptom**:

```cpp
#if KCENON_WITH_LOGGER_SYSTEM
    // This block is never compiled, even though logger_system is linked
#endif
```

**Solution**:

1. Check CMake compile definitions:
   ```cmake
   # Should be present
   target_compile_definitions(app PUBLIC KCENON_WITH_LOGGER_SYSTEM=1)
   ```

2. Verify macro is defined:
   ```cpp
   #include <kcenon/common/config/feature_flags.h>

   #ifndef KCENON_WITH_LOGGER_SYSTEM
       #error "KCENON_WITH_LOGGER_SYSTEM is not defined!"
   #endif

   #if KCENON_WITH_LOGGER_SYSTEM == 0
       #error "KCENON_WITH_LOGGER_SYSTEM is 0!"
   #endif
   ```

3. Check header include order:
   ```cpp
   // WRONG: Including logger header before feature_flags.h
   #include <kcenon/logger/logger.h>
   #include <kcenon/common/config/feature_flags.h>

   // CORRECT: Include feature_flags.h first
   #include <kcenon/common/config/feature_flags.h>
   #include <kcenon/logger/logger.h>
   ```

### Legacy Macro Conflicts

**Symptom**:

```
warning: macro redefinition: USE_THREAD_SYSTEM
```

**Solution**:

1. Check for duplicate definitions:
   ```bash
   grep -r "define USE_THREAD_SYSTEM" .
   ```

2. Ensure legacy aliases are enabled:
   ```cpp
   // Check if legacy aliases are enabled
   #if KCENON_ENABLE_LEGACY_ALIASES
       #pragma message("Legacy aliases enabled")
   #else
       #pragma message("Legacy aliases disabled")
   #endif
   ```

3. Remove explicit legacy macro definitions:
   ```cmake
   # WRONG: Defining both old and new macros
   target_compile_definitions(app PUBLIC
       USE_THREAD_SYSTEM=1
       KCENON_WITH_THREAD_SYSTEM=1)

   # CORRECT: Define only new macro (legacy alias is automatic)
   target_compile_definitions(app PUBLIC
       KCENON_WITH_THREAD_SYSTEM=1)
   ```

### C++17 Static Assertion Failure

**Symptom**:

```
error: static assertion failed: common_system requires C++17 or later.
```

**Solution**:

1. Update CMake C++ standard:
   ```cmake
   set(CMAKE_CXX_STANDARD 17)
   set(CMAKE_CXX_STANDARD_REQUIRED ON)
   ```

2. Or specify via command line:
   ```bash
   cmake -B build -DCMAKE_CXX_STANDARD=17
   ```

3. For older compilers, use explicit flag:
   ```cmake
   target_compile_options(app PRIVATE -std=c++17)
   ```

---

## Related Documentation

- [API Reference](API_REFERENCE.md) — Complete API documentation for all systems
- [Configuration Guide](CONFIG_GUIDE.md) — Configuration subsystem documentation
- [Integration Guide](INTEGRATION_GUIDE.md) — Cross-system integration patterns
- [Architecture](ARCHITECTURE.md) — System architecture and design principles

### Feature Flag Headers

- `include/kcenon/common/config/feature_flags_core.h` — Preprocessor helpers, compiler/platform detection
- `include/kcenon/common/config/feature_detection.h` — C++ standard library feature detection
- `include/kcenon/common/config/feature_system_deps.h` — System module integration flags
- `include/kcenon/common/config/feature_flags.h` — Main entry point

### External Resources

- [C++ Feature Test Macros](https://en.cppreference.com/w/cpp/feature_test) — Official C++ feature test macro reference
- [Compiler Support](https://en.cppreference.com/w/cpp/compiler_support) — C++20/23 compiler support matrix

---

**Version**: 1.0.0
**Last Updated**: 2026-02-08
**License**: BSD 3-Clause (see LICENSE file)
