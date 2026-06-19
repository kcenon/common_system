---
doc_id: "COM-GUID-004-USAGE"
doc_title: "Feature Flags Framework - Usage Guide"
doc_version: "1.0.0"
doc_date: "2026-04-04"
doc_status: "Released"
project: "common_system"
category: "GUID"
---

# Feature Flags Framework - Usage Guide

> **SSOT**: This document is part of the [Feature Flags Framework Guide](FEATURE_FLAGS_GUIDE.md).

<!-- TODO: target file does not exist -->
> **Language:** **English** | [한국어](FEATURE_FLAGS_GUIDE.kr.md)

**Complete Guide**: Compile-Time Feature Detection, Runtime Feature Flags, and Cross-System Integration

**Status**: ✅ **Complete**

This guide provides usage documentation for the feature flags framework in common_system, including overview, examples, CMake integration, migration, and troubleshooting.

---

## Table of Contents

- [Overview](#overview)
- [Feature Flags System Overview](#1-feature-flags-system-overview)
  - [Compile-Time vs Runtime](#compile-time-vs-runtime)
  - [Flag Lifecycle](#flag-lifecycle)
  - [Architecture Diagram](#architecture-diagram)
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
- [Feature Flags Reference](FEATURE_FLAGS_REFERENCE.md) — Header-level API reference for all feature flag macros

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
