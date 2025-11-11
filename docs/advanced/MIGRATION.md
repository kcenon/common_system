> **Language:** **English** | [한국어](MIGRATION_KO.md)

# Migration Guide

## Table of Contents

- [Overview](#overview)
- [Migrating to common_system Integration](#migrating-to-common_system-integration)
- [Migrating to Result<T> Pattern](#migrating-to-resultt-pattern)
- [Migrating to Standard Interfaces](#migrating-to-standard-interfaces)
- [Version Migration Guides](#version-migration-guides)
- [Troubleshooting](#troubleshooting)

## Overview

This guide helps you migrate existing code to use the integrated system suite with common_system. Each section provides step-by-step instructions with before/after examples.

## Migrating to common_system Integration

### Step 1: Update CMakeLists.txt

Add common_system dependency to your project:

**Before** (standalone):
```cmake
cmake_minimum_required(VERSION 3.20)
project(MyProject)

add_executable(MyApp main.cpp)
```

**After** (integrated):
```cmake
cmake_minimum_required(VERSION 3.20)
project(MyProject)

# Add option for integration
option(BUILD_WITH_COMMON_SYSTEM "Enable common_system integration" ON)

if(BUILD_WITH_COMMON_SYSTEM)
    find_package(common_system CONFIG REQUIRED)
    target_link_libraries(MyApp PRIVATE kcenon::common_system)
    target_compile_definitions(MyApp PRIVATE BUILD_WITH_COMMON_SYSTEM)
endif()

add_executable(MyApp main.cpp)
```

### Step 2: Update Build Process

**Before**:
```bash
cmake -B build -S .
cmake --build build
```

**After**:
```bash
# With common_system integration (default)
cmake -B build -S .
cmake --build build

# Or explicitly disable if needed
cmake -B build -S . -DBUILD_WITH_COMMON_SYSTEM=OFF
cmake --build build
```

### Step 3: Update Source Code

Use conditional compilation for backward compatibility:

```cpp
#ifdef BUILD_WITH_COMMON_SYSTEM
#include <kcenon/common/patterns/result.h>
#include <kcenon/common/interfaces/logger_interface.h>
using namespace common;
#endif

class MyClass {
public:
    #ifdef BUILD_WITH_COMMON_SYSTEM
    VoidResult initialize() {
        if (!validate()) {
            return make_error(error_code::invalid_state, "Invalid state");
        }
        return ok();
    }
    #else
    bool initialize() {
        return validate();
    }
    #endif

private:
    bool validate();
};
```

## Migrating to Result<T> Pattern

### From bool Return Values

**Before**:
```cpp
bool process_data(const std::string& data) {
    if (data.empty()) {
        return false;  // Why did it fail?
    }
    
    if (!validate(data)) {
        return false;  // What's wrong with the data?
    }
    
    return do_processing(data);
}

// Usage
if (!process_data(input)) {
    std::cerr << "Processing failed" << std::endl;  // No context!
}
```

**After**:
```cpp
#include <kcenon/common/patterns/result.h>
using namespace common;

VoidResult process_data(const std::string& data) {
    if (data.empty()) {
        return make_error(error_code::invalid_argument, 
                         "Data cannot be empty");
    }
    
    if (!validate(data)) {
        return make_error(error_code::validation_failed,
                         std::format("Invalid data format: {}", data));
    }
    
    return do_processing(data);
}

// Usage
auto result = process_data(input);
if (is_error(result)) {
    auto err = get_error(result);
    std::cerr << "Processing failed: " << err.message 
              << " (code: " << static_cast<int>(err.code) << ")" << std::endl;
    
    // Can handle different error types
    if (err.code == error_code::invalid_argument) {
        // Handle invalid input
    }
}
```

### From Exceptions

**Before**:
```cpp
User load_user(const std::string& id) {
    auto conn = database->connect();
    if (!conn) {
        throw std::runtime_error("Database connection failed");
    }
    
    auto user = conn->query("SELECT * FROM users WHERE id = ?", id);
    if (!user) {
        throw std::runtime_error("User not found");
    }
    
    return *user;
}

// Usage
try {
    auto user = load_user("123");
    process(user);
} catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
}
```

**After**:
```cpp
#include <kcenon/common/patterns/result.h>
using namespace common;

Result<User> load_user(const std::string& id) {
    auto conn_result = database->connect();
    if (is_error(conn_result)) {
        return get_error(conn_result);  // Propagate error
    }
    
    auto conn = get_value(conn_result);
    auto user_result = conn->query("SELECT * FROM users WHERE id = ?", id);
    
    if (is_error(user_result)) {
        return make_error(error_code::not_found,
                         std::format("User {} not found", id));
    }
    
    return get_value(user_result);
}

// Usage (no exceptions)
auto result = load_user("123");
if (is_ok(result)) {
    auto user = get_value(result);
    process(user);
} else {
    auto err = get_error(result);
    std::cerr << "Error: " << err.message << std::endl;
    
    // Type-safe error handling
    switch (err.code) {
        case error_code::connection_failed:
            retry_connection();
            break;
        case error_code::not_found:
            create_default_user();
            break;
        default:
            log_error(err);
    }
}
```

### From Optional<T>

**Before**:
```cpp
std::optional<Config> load_config(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        return std::nullopt;  // Why did it fail?
    }
    
    Config config;
    if (!parse(file, config)) {
        return std::nullopt;  // Parse error? File error?
    }
    
    return config;
}

// Usage
auto config = load_config("config.json");
if (!config) {
    std::cerr << "Failed to load config" << std::endl;  // No details!
}
```

**After**:
```cpp
Result<Config> load_config(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        return make_error(error_code::file_not_found,
                         std::format("Config file not found: {}", path));
    }
    
    Config config;
    if (!parse(file, config)) {
        return make_error(error_code::parse_error,
                         std::format("Failed to parse config file: {}", path));
    }
    
    return ok(std::move(config));
}

// Usage
auto result = load_config("config.json");
if (is_ok(result)) {
    auto config = get_value(result);
    apply(config);
} else {
    auto err = get_error(result);
    std::cerr << "Config error: " << err.message << std::endl;
    
    // Can provide fallback based on error type
    if (err.code == error_code::file_not_found) {
        use_default_config();
    } else if (err.code == error_code::parse_error) {
        log_parse_error(err);
        use_default_config();
    }
}
```

## Migrating to Standard Interfaces

### Migrating to ILogger Interface

**Before** (direct dependency):
```cpp
#include <spdlog/spdlog.h>

class MyService {
private:
    std::shared_ptr<spdlog::logger> logger_;
    
public:
    MyService() {
        logger_ = spdlog::stdout_color_mt("service");
    }
    
    void process() {
        logger_->info("Processing started");
        // ... processing
        logger_->info("Processing completed");
    }
};
```

**After** (interface abstraction):
```cpp
#include <kcenon/common/interfaces/logger_interface.h>

class MyService {
private:
    std::shared_ptr<common::interfaces::ILogger> logger_;
    
public:
    MyService(std::shared_ptr<common::interfaces::ILogger> logger)
        : logger_(std::move(logger)) {}
    
    void process() {
        logger_->log(log_level::info, "Processing started");
        // ... processing
        logger_->log(log_level::info, "Processing completed");
    }
};

// Usage - can use ANY logger implementation
#include <kcenon/logger/core/logger.h>

auto logger = kcenon::logger::create_console_logger();
MyService service(logger);
service.process();
```

**Benefits**:
- No direct dependency on specific logger implementation
- Easy to swap logger implementations
- Easier testing (can use mock logger)
- Consistent logging interface across all systems

### Migrating to IExecutor Interface

**Before** (direct thread management):
```cpp
#include <thread>
#include <queue>

class TaskProcessor {
private:
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex mutex_;
    
public:
    TaskProcessor(size_t num_threads) {
        for (size_t i = 0; i < num_threads; ++i) {
            workers_.emplace_back([this]() {
                worker_loop();
            });
        }
    }
    
    void submit(std::function<void()> task) {
        std::lock_guard lock(mutex_);
        tasks_.push(std::move(task));
    }
    
private:
    void worker_loop() {
        // Complex worker implementation
    }
};
```

**After** (interface abstraction):
```cpp
#include <kcenon/common/interfaces/executor_interface.h>

class TaskProcessor {
private:
    std::shared_ptr<common::interfaces::IExecutor> executor_;
    
public:
    TaskProcessor(std::shared_ptr<common::interfaces::IExecutor> executor)
        : executor_(std::move(executor)) {}
    
    void submit(std::function<void()> task) {
        executor_->submit(std::move(task));
    }
};

// Usage - thread_system handles the complexity
#include <kcenon/thread/core/thread_pool.h>

auto executor = kcenon::thread::create_thread_pool(4);
TaskProcessor processor(executor);
processor.submit([]() {
    std::cout << "Task executed" << std::endl;
});
```

### Migrating to IMonitor Interface

**Before** (custom metrics):
```cpp
class ServiceMetrics {
private:
    std::atomic<int64_t> request_count_{0};
    std::atomic<int64_t> error_count_{0};
    std::atomic<double> avg_latency_{0.0};
    
public:
    void record_request() { ++request_count_; }
    void record_error() { ++error_count_; }
    void record_latency(double ms) { avg_latency_ = ms; }
    
    void print_stats() {
        std::cout << "Requests: " << request_count_ << std::endl;
        std::cout << "Errors: " << error_count_ << std::endl;
        std::cout << "Avg latency: " << avg_latency_ << "ms" << std::endl;
    }
};
```

**After** (standard interface):
```cpp
#include <kcenon/common/interfaces/monitoring_interface.h>

class Service {
private:
    std::shared_ptr<common::interfaces::IMonitor> monitor_;
    
public:
    Service(std::shared_ptr<common::interfaces::IMonitor> monitor)
        : monitor_(std::move(monitor)) {}
    
    void handle_request() {
        auto start = std::chrono::steady_clock::now();
        
        monitor_->record_metric("requests_total", 1);
        
        try {
            // Process request
        } catch (...) {
            monitor_->record_metric("errors_total", 1);
            throw;
        }
        
        auto duration = std::chrono::steady_clock::now() - start;
        auto ms = std::chrono::duration<double, std::milli>(duration).count();
        monitor_->record_metric("latency_ms", ms);
    }
    
    void print_stats() {
        auto metrics = monitor_->collect_metrics();
        for (const auto& [name, metric] : metrics) {
            std::cout << name << ": " << metric.value << std::endl;
        }
    }
};

// Usage
#include <kcenon/monitoring/core/performance_monitor.h>

auto monitor = kcenon::monitoring::create_performance_monitor();
Service service(monitor);
```

## Version Migration Guides

### Migrating from 0.x to 1.0

#### Breaking Changes

1. **Result<T> API Changes**
   ```cpp
   // Old (0.x)
   if (result.is_ok()) {
       auto value = result.value();
   }
   
   // New (1.0)
   if (is_ok(result)) {
       auto value = get_value(result);
   }
   ```

2. **Logger Interface Changes**
   ```cpp
   // Old (0.x)
   logger->log(LogLevel::INFO, "message");
   
   // New (1.0)
   logger->log(log_level::info, "message");
   ```

3. **Namespace Changes**
   ```cpp
   // Old (0.x)
   using namespace kcenon::common;
   
   // New (1.0)
   using namespace common;
   using namespace common::interfaces;
   ```

#### Migration Steps

1. **Update Dependencies**
   ```cmake
   # Update version requirement
   find_package(common_system 1.0 REQUIRED)
   ```

2. **Update Code**
   - Replace `result.is_ok()` with `is_ok(result)`
   - Replace `result.value()` with `get_value(result)`
   - Replace `result.error()` with `get_error(result)`
   - Update enum names to lowercase (e.g., `LogLevel::INFO` → `log_level::info`)

3. **Test Thoroughly**
   ```bash
   # Run all tests
   ctest --output-on-failure
   ```

### Migrating Between Minor Versions

Minor version updates are backward compatible. No code changes required, but review:

- New features in release notes
- Deprecated APIs (warnings during compilation)
- Performance improvements

## Troubleshooting

### Common Migration Issues

#### Issue 1: Linker Errors After Migration

**Problem**:
```
undefined reference to `common::make_error(...)`
```

**Solution**:
```cmake
# Ensure common_system is linked
target_link_libraries(MyApp PRIVATE kcenon::common_system)
```

#### Issue 2: Ambiguous Function Calls

**Problem**:
```cpp
error: call to 'ok' is ambiguous
```

**Solution**:
```cpp
// Explicitly use namespace
auto result = common::ok(value);

// Or use namespace directive
using namespace common;
auto result = ok(value);
```

#### Issue 3: Performance Regression

**Problem**: Code runs slower after migration to Result<T>

**Solution**:
```cpp
// Enable optimizations
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release

// Use move semantics
return ok(std::move(large_object));

// Avoid unnecessary copies
auto result = process();  // Don't: auto result = Result(process());
```

#### Issue 4: Cannot Find common_system

**Problem**:
```
CMake Error: Could not find a package configuration file provided by "common_system"
```

**Solution**:
```bash
# Option 1: Install common_system
cd common_system
cmake -B build -S . -DCMAKE_INSTALL_PREFIX=/usr/local
sudo cmake --install build

# Option 2: Use CMAKE_PREFIX_PATH
cmake -B build -S . -DCMAKE_PREFIX_PATH=/path/to/systems

# Option 3: Use sibling directory structure
/path/to/systems/
├── common_system/
├── thread_system/
└── my_project/
```

### Migration Checklist

Before starting migration:

- [ ] Backup existing code
- [ ] Review ARCHITECTURE.md
- [ ] Read INTEGRATION_POLICY.md
- [ ] Check system compatibility (C++20 required)
- [ ] Plan migration in phases (don't migrate everything at once)

During migration:

- [ ] Update CMakeLists.txt
- [ ] Add conditional compilation for backward compatibility
- [ ] Migrate one module at a time
- [ ] Test after each module migration
- [ ] Update documentation

After migration:

- [ ] Run full test suite
- [ ] Performance testing
- [ ] Code review
- [ ] Update project documentation
- [ ] Update CI/CD pipelines

### Getting Help

If you encounter issues:

1. Check [INTEGRATION.md](./INTEGRATION.md) for examples
2. Review [ARCHITECTURE.md](./ARCHITECTURE.md) for system design
3. Check existing Issues on GitHub
4. Create a new Issue with:
   - System versions
   - Error messages
   - Minimal reproduction example

## Best Practices

### Gradual Migration

Don't migrate everything at once:

```cpp
// Phase 1: Add integration support
#ifdef BUILD_WITH_COMMON_SYSTEM
    Result<Data> new_load_data() { /* ... */ }
#endif
    
    bool legacy_load_data() { /* keep existing */ }

// Phase 2: Migrate callers gradually
void process() {
    #ifdef BUILD_WITH_COMMON_SYSTEM
    auto result = new_load_data();
    if (is_error(result)) { /* ... */ }
    #else
    if (!legacy_load_data()) { /* ... */ }
    #endif
}

// Phase 3: Remove legacy code (in next release)
Result<Data> load_data() { /* ... */ }
```

### Maintain Backward Compatibility

Use feature flags:

```cpp
namespace my_app {
    #ifdef BUILD_WITH_COMMON_SYSTEM
    using error_type = common::Error;
    using result_type = common::VoidResult;
    #else
    struct error_type { std::string message; };
    using result_type = bool;
    #endif
}
```

### Document Changes

Update your README:

```markdown
## Dependencies

### Required
- CMake 3.20+
- C++20 compiler

### Optional
- common_system 1.0+ (recommended, enables Result<T> pattern)
- thread_system 1.0+ (for concurrent execution)

## Building

### With Integration (Recommended)
\`\`\`bash
cmake -B build -S . -DBUILD_WITH_COMMON_SYSTEM=ON
cmake --build build
\`\`\`

### Standalone
\`\`\`bash
cmake -B build -S . -DBUILD_WITH_COMMON_SYSTEM=OFF
cmake --build build
\`\`\`
```

## References

- [INTEGRATION.md](./INTEGRATION.md) - Integration guide with examples
- [ARCHITECTURE.md](./ARCHITECTURE.md) - System architecture
- [INTEGRATION_POLICY.md](./INTEGRATION_POLICY.md) - Integration tiers and policy
- [NEED_TO_FIX.md](./NEED_TO_FIX.md) - Project improvement tracking
