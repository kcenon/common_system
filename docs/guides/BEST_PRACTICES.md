# Best Practices for common_system

**Version**: 0.1.1.0
**Last Updated**: 2026-01-03

A comprehensive guide to building reliable, maintainable code using common_system patterns and idioms.

---

## Table of Contents

1. [Result<T> Pattern](#1-resultt-pattern)
2. [Error Handling](#2-error-handling)
3. [RAII and Resource Management](#3-raii-and-resource-management)
4. [Smart Pointer Usage](#4-smart-pointer-usage)
5. [Interface Design](#5-interface-design)
6. [Template Guidelines](#6-template-guidelines)
7. [Header-Only Libraries](#7-header-only-libraries)
8. [Thread Safety](#8-thread-safety)
9. [Performance Optimization](#9-performance-optimization)
10. [Testing Patterns](#10-testing-patterns)

---

## 1. Result<T> Pattern

The `Result<T>` type provides exception-free, explicit error handling throughout the codebase.

### Recommended API Style

> **Important:** Always prefer member methods over free functions when working with `Result<T>`.

The `Result<T>` API provides two styles for the same operations:

| Operation | Member Method (✅ Recommended) | Free Function (⚠ Legacy) |
|-----------|-------------------------------|--------------------------|
| Check success | `result.is_ok()` | `is_ok(result)` |
| Check error | `result.is_err()` | `is_error(result)` |
| Get value | `result.value()` | `get_value(result)` |
| Get error | `result.error()` | `get_error(result)` |
| Get with default | `result.unwrap_or(def)` | `value_or(result, def)` |
| Transform | `result.map(func)` | `map(result, func)` |
| Chain | `result.and_then(func)` | `and_then(result, func)` |

**Why member methods are preferred:**
- **Consistency**: 84.9% of codebase usage already uses member methods
- **Readability**: `result.is_ok()` reads more naturally than `is_ok(result)`
- **IDE support**: Better autocomplete and navigation
- **Method chaining**: Fluent API style: `result.map(...).and_then(...)`

**When free functions are appropriate:**
- **ADL contexts**: Generic templates that require argument-dependent lookup
- **Legacy compatibility**: When interfacing with code that expects free functions

> **Note**: Free functions are now marked as `[[deprecated]]` and will be removed in v2.0.0.
> The `COMMON_ASSIGN_OR_RETURN` and `COMMON_RETURN_IF_ERROR` macros have been updated
> to use member methods internally.

**Example:**
```cpp
// ✅ RECOMMENDED - Member method style
auto result = parse_config(filename);
if (result.is_ok()) {
    const auto& config = result.value();
    process(config);
} else {
    log_error("Parse failed: {}", result.error().message);
}

// ⚠ LEGACY - Free function style (avoid for new code)
auto result = parse_config(filename);
if (is_ok(result)) {
    const auto& config = get_value(result);
    process(config);
} else {
    log_error("Parse failed: {}", get_error(result).message);
}
```

### Creation

**DO:**
```cpp
#include <kcenon/common/patterns/result.h>
#include <kcenon/common/error/error_codes.h>

using namespace common;

// Return success
Result<int> parse_number(const std::string& str) {
    try {
        return ok(std::stoi(str));
    } catch (...) {
        return error<int>(
            error::codes::common::invalid_argument,
            "Invalid integer format",
            "parser"
        );
    }
}

// Return void success
Result<void> validate_config(const Config& cfg) {
    if (cfg.port < 1024) {
        return error<std::monostate>(
            error::codes::common::invalid_argument,
            "Port must be >= 1024",
            "config"
        );
    }
    return ok();
}
```

**DON'T:**
```cpp
// ❌ Returning only error code without context
return error<int>(42, "Error", "");

// ❌ Generic error messages
return error<int>(
    error::codes::common::io_error,
    "Error",
    "module"
);

// ❌ Mixing exceptions with Result
try {
    // ... operation ...
} catch (...) {
    // Exceptions should be converted at boundaries
    throw;
}
```

### Propagation

**DO: Use Monadic Operations for Composition**
```cpp
Result<User> create_user(const std::string& name, int age) {
    return validate_name(name)
        .and_then([age](auto) { return validate_age(age); })
        .and_then([name, age](auto) {
            return User::create(name, age);
        })
        .map([](auto user) {
            log_user_created(user);
            return user;
        });
}
```

**DO: Use ASSIGN_OR_RETURN Macro for Multi-Step Operations**
```cpp
Result<ProcessedData> process_pipeline(const RawData& raw) {
    ASSIGN_OR_RETURN(auto step1, validate_input(raw));
    ASSIGN_OR_RETURN(auto step2, transform(step1));
    ASSIGN_OR_RETURN(auto step3, finalize(step2));

    return ok(step3);
}
```

**DO: Provide Context in Error Messages**
```cpp
Result<Connection> connect(const std::string& host, int port) {
    Connection conn;
    auto result = conn.connect(host, port);

    if (result.is_err()) {
        return error<Connection>(
            error::codes::network_system::connection_failed,
            "Failed to connect to database",
            "connection_pool",
            fmt::format("host: {}:{}, timeout: 30s", host, port)
        );
    }
    return ok(std::move(conn));
}
```

**DON'T: Ignore Error Details**
```cpp
// ❌ Bad - loses error information
auto result = some_operation();
if (result.is_err()) {
    return error<int>(error::codes::common::unknown, "", "");
}

// ❌ Bad - unhandled Result
auto result = database.query("SELECT ...");
// Result ignored - error never handled
```

### Handling

**DO: Check and Handle Explicitly**
```cpp
auto result = risky_operation();

if (result.is_ok()) {
    int value = result.value();
    process(value);
} else {
    const auto& err = result.error();
    log_error("Operation failed: {} (code: {})", err.message, err.code);

    if (err.code == error::codes::common::out_of_memory) {
        // Critical - signal shutdown
        system_shutdown();
    }
}
```

**DO: Chain with or_else for Fallbacks**
```cpp
Result<Config> load_config() {
    return load_from_file("config.json")
        .or_else([](const error_info&) {
            return load_from_file("config.default.json");
        })
        .or_else([](const error_info&) {
            return ok(Config::builtin_defaults());
        });
}
```

**DON'T: Unwrap Without Checking**
```cpp
// ❌ Dangerous - throws std::runtime_error if error
int value = result.unwrap();

// ✅ Safe - unwrap with default
int value = result.unwrap_or(0);

// ✅ Safe - check first
if (result.is_ok()) {
    int value = result.value();
}
```

---

## 2. Error Handling

### Error Code Usage

**DO: Use Centralized Error Codes**
```cpp
#include <kcenon/common/error/error_codes.h>

namespace common::error::codes {

namespace my_system {
    constexpr int base = -700;
    constexpr int initialization_failed = base + 0;
    constexpr int resource_exhausted = base + 1;
    constexpr int invalid_state = base + 2;
}

}
```

**DO: Document Error Conditions**
```cpp
/**
 * @brief Initialize connection pool
 * @param size Maximum number of connections
 * @return Result<void>
 *
 * Error conditions:
 * - common::invalid_argument: size <= 0
 * - database_system::connection_failed: Cannot connect to database
 * - common::out_of_memory: Insufficient memory
 */
Result<void> initialize(int size);
```

**DON'T: Use Arbitrary Error Codes**
```cpp
// ❌ Random numbers - hard to debug
return error<int>(12345, "Failed", "module");

// ❌ Reusing error codes across systems
return error<int>(
    error::codes::logger_system::file_open_failed,  // Wrong category!
    "Database connection failed",
    "database"
);
```

### Exception Conversion

**DO: Convert at Module Boundaries**
```cpp
#include <kcenon/common/patterns/result.h>

// Internal function may throw
void internal_parse() {
    // May throw std::invalid_argument, std::out_of_range
}

// Public API wraps it
Result<void> public_parse() {
    try {
        internal_parse();
        return ok();
    } catch (const std::invalid_argument& e) {
        return error<std::monostate>(
            error::codes::common::invalid_argument,
            e.what(),
            "parser"
        );
    } catch (const std::exception& e) {
        return error<std::monostate>(
            error::codes::common::unknown,
            e.what(),
            "parser"
        );
    }
}
```

**DO: Log Before Returning Error**
```cpp
Result<Data> fetch_data(const std::string& source) {
    auto result = data_provider.get(source);

    if (result.is_err()) {
        logger->warn("Data fetch failed from '{}': {}",
                    source, result.error().message);
        return error<Data>(result.error());
    }

    return ok(result.value());
}
```

**DON'T: Silently Drop Errors**
```cpp
// ❌ Error lost - no one knows this failed
database.query("SELECT ...");

// ❌ Error ignored in branching
if (auto result = operation(); result.is_err()) {
    // Missing handler - error silently ignored
}
```

---

## 3. RAII and Resource Management

The principle: **Resources are acquired in constructors and released in destructors automatically**.

### Basic Pattern

**DO: Implement Rule of Five Completely**
```cpp
class FileWriter {
    FILE* handle_ = nullptr;

public:
    explicit FileWriter(const std::string& path) {
        handle_ = fopen(path.c_str(), "w");
        if (!handle_) {
            throw std::runtime_error("Cannot open file: " + path);
        }
    }

    // Destructor
    ~FileWriter() noexcept {
        if (handle_) {
            fclose(handle_);
        }
    }

    // Delete copy
    FileWriter(const FileWriter&) = delete;
    FileWriter& operator=(const FileWriter&) = delete;

    // Enable move
    FileWriter(FileWriter&& other) noexcept
        : handle_(std::exchange(other.handle_, nullptr)) {}

    FileWriter& operator=(FileWriter&& other) noexcept {
        if (this != &other) {
            if (handle_) fclose(handle_);
            handle_ = std::exchange(other.handle_, nullptr);
        }
        return *this;
    }

    // Accessor
    FILE* get() const { return handle_; }
};
```

**DO: Use Scope Guards for Cleanup**
```cpp
template<typename Func>
class ScopeExit {
    Func cleanup_;
    bool dismissed_ = false;

public:
    explicit ScopeExit(Func f) : cleanup_(std::move(f)) {}
    ~ScopeExit() { if (!dismissed_) cleanup_(); }
    void dismiss() { dismissed_ = true; }

    ScopeExit(const ScopeExit&) = delete;
    ScopeExit& operator=(const ScopeExit&) = delete;
};

// Usage
{
    auto guard = ScopeExit([&]() {
        cleanup_resources();
    });

    // Do work - cleanup happens automatically
}
```

**DON'T: Use Manual Resource Management**
```cpp
// ❌ Error-prone - easy to forget cleanup
FILE* file = fopen("data.txt", "r");
// ... use file ...
fclose(file);  // What if exception thrown above?

// ✅ Automatic cleanup
auto file = FileWriter("data.txt");
// cleanup guaranteed on scope exit
```

### Integration with Result<T>

**DO: Return RAII Resources from Factory Functions**
```cpp
Result<std::unique_ptr<FileWriter>> create_file_writer(const std::string& path) {
    FILE* handle = fopen(path.c_str(), "w");
    if (!handle) {
        return error<std::unique_ptr<FileWriter>>(
            error::codes::logger_system::file_open_failed,
            std::strerror(errno),
            "FileWriter::create"
        );
    }
    return ok(std::make_unique<FileWriter>(handle));
}

// Usage
auto result = create_file_writer("output.txt");
if (result.is_err()) {
    return result.error();
}

auto writer = std::move(result.value());
// writer automatically cleaned up on scope exit
```

**DON'T: Mix Manual Cleanup with Result<T>**
```cpp
// ❌ Confusing ownership
Result<FILE*> bad_open(const std::string& path) {
    FILE* f = fopen(path.c_str(), "r");
    return ok(f);  // Who deletes this? Caller?
}

// Later
auto file = bad_open("data.txt");
if (file.is_ok()) {
    // Is caller responsible for fclose()?
    // Or does the system?
}
```

---

## 4. Smart Pointer Usage

### Decision Tree

```
Do you manage a resource?
├─ No → Use references (&) or raw pointers (*)
└─ Yes
   ├─ Single owner? → std::unique_ptr<T>
   ├─ Multiple owners? → std::shared_ptr<T>
   └─ Non-owning reference to shared? → std::weak_ptr<T>
```

### unique_ptr Usage

**DO: Use for Exclusive Ownership**
```cpp
class Logger {
    std::unique_ptr<FileHandle> file_;
    std::vector<std::unique_ptr<Formatter>> formatters_;

public:
    explicit Logger(std::unique_ptr<FileHandle> file)
        : file_(std::move(file)) {}

    void add_formatter(std::unique_ptr<Formatter> fmt) {
        formatters_.push_back(std::move(fmt));
    }
};

// Usage
auto logger = std::make_unique<Logger>(
    std::make_unique<FileHandle>("app.log")
);
```

**DO: Transfer Ownership with Move Semantics**
```cpp
class Manager {
    std::unique_ptr<Resource> resource_;

public:
    void set_resource(std::unique_ptr<Resource> res) {
        resource_ = std::move(res);
    }

    std::unique_ptr<Resource> release_resource() {
        return std::move(resource_);
    }
};
```

**DO: Use Non-Owning Access Patterns**
```cpp
class Client {
public:
    // Non-owning pointer (may be null)
    void set_logger(Logger* logger) {
        logger_ = logger;
    }

    // Non-owning reference (guaranteed valid)
    void process(const Resource& resource) {
        handle(resource);
    }

private:
    Logger* logger_ = nullptr;  // Non-owning
};

// Usage
auto logger = std::make_unique<Logger>();
client.set_logger(logger.get());
```

**DON'T: Use new/delete Directly**
```cpp
// ❌ Manual memory management
Widget* w = new Widget();
// ... use w ...
delete w;  // Easy to forget!

// ✅ Automatic cleanup
auto w = std::make_unique<Widget>();
```

### shared_ptr Usage

**DO: Use for Shared Ownership**
```cpp
class Session : public std::enable_shared_from_this<Session> {
    std::shared_ptr<Socket> socket_;

public:
    void start_async_read() {
        auto self = shared_from_this();
        socket_->async_read([self, this](const auto& data) {
            // 'self' keeps 'this' alive during async operation
            handle_data(data);
        });
    }
};
```

**DO: Use make_shared for Efficiency**
```cpp
// Single allocation - better cache locality
auto widget = std::make_shared<Widget>(arg1, arg2);

// Multiple allocations - less efficient
auto widget = std::shared_ptr<Widget>(new Widget(arg1, arg2));
```

**DO: Break Circular References with weak_ptr**
```cpp
class Node {
    std::weak_ptr<Node> parent_;      // Non-owning
    std::shared_ptr<Node> child_;     // Owning
    std::string value_;

public:
    void set_parent(std::shared_ptr<Node> parent) {
        parent_ = parent;  // Store weak reference
    }

    std::shared_ptr<Node> get_parent() {
        return parent_.lock();  // Convert to shared_ptr
    }
};
```

**DON'T: Create Circular References**
```cpp
// ❌ Circular reference causes memory leak
class Node {
    std::shared_ptr<Node> parent_;
    std::shared_ptr<Node> child_;

    void link(std::shared_ptr<Node> p) {
        parent_ = p;      // Cycle!
        p->child_ = shared_from_this();  // Deadlock
    }
};
```

---

## 5. Interface Design

Good interfaces are the foundation of maintainable systems.

### IExecutor Pattern

**DO: Depend on Abstract Interfaces**
```cpp
#include <kcenon/common/interfaces/executor_interface.h>

class TaskProcessor {
    std::shared_ptr<IExecutor> executor_;

public:
    explicit TaskProcessor(std::shared_ptr<IExecutor> executor)
        : executor_(executor) {}

    Result<std::future<void>> process() {
        auto job = std::make_unique<ProcessingJob>();
        return executor_->execute(std::move(job));
    }
};

// Different implementations can be injected
TaskProcessor processor(thread_pool_executor);
TaskProcessor mock_processor(mock_executor);
```

**DO: Implement Job-Based Execution**
```cpp
class MyJob : public IJob {
public:
    VoidResult execute() override {
        // Task logic here
        return ok();
    }

    std::string get_name() const override {
        return "MyJob";
    }

    int get_priority() const override {
        return 5;  // Higher priority
    }
};

// Usage
auto result = executor->execute(std::make_unique<MyJob>());
if (result.is_err()) {
    return result.error();
}
auto future = result.value();
```

**DON'T: Use Legacy Function-Based API**
```cpp
// ❌ Deprecated - will be removed
executor->submit([]() {
    // Task logic
});

// ✅ Job-based API
executor->execute(std::make_unique<LambdaJob>(
    []() { /* task logic */ },
    "task_name"
));
```

### Dependency Injection

**DO: Use Constructor Injection**
```cpp
class DataProcessor {
    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<IDatabase> database_;
    std::shared_ptr<IExecutor> executor_;

public:
    DataProcessor(
        std::shared_ptr<ILogger> logger,
        std::shared_ptr<IDatabase> database,
        std::shared_ptr<IExecutor> executor)
        : logger_(logger),
          database_(database),
          executor_(executor) {}

private:
    Result<void> process() {
        logger_->info("Starting processing");
        // Use injected dependencies
    }
};
```

**DON'T: Use Global Singletons**
```cpp
// ❌ Hard to test, tight coupling
class DataProcessor {
    void process() {
        auto logger = Logger::instance();
        auto db = Database::instance();
        // Tightly coupled to global state
    }
};
```

---

## 6. Template Guidelines

### When to Use Templates

**DO: Use Templates for Generic Algorithms**
```cpp
// Generic container adapter
template<typename Container, typename Predicate>
auto find_if(Container& c, Predicate p) {
    return std::find_if(c.begin(), c.end(), p);
}

// Generic result transformer
template<typename T, typename F>
Result<std::invoke_result_t<F, T>> transform_result(
    const Result<T>& result, F transformer) {
    if (result.is_err()) {
        return error(result.error());
    }
    return ok(transformer(result.value()));
}
```

**DO: Constrain Templates**
```cpp
template<typename T>
requires std::is_arithmetic_v<T>
Result<T> parse_number(const std::string& str) {
    // Compile error if T is not arithmetic
}

// SFINAE alternative (C++17)
template<typename T>
std::enable_if_t<std::is_arithmetic_v<T>, Result<T>>
parse_number(const std::string& str) {
    // Similar constraint
}
```

**DO: Document Template Parameters**
```cpp
/**
 * @brief Cache with automatic expiration
 * @tparam Key Type of cache keys
 * @tparam Value Type of cached values
 * @tparam Eviction Policy for removing expired entries (default: LRU)
 */
template<typename Key, typename Value,
         typename Eviction = LRUEviction>
class ExpiringCache {
    // Implementation
};
```

**DON'T: Over-Template**
```cpp
// ❌ Unnecessary template complexity
template<typename T>
class SimpleWrapper {
    T value_;
public:
    T get() { return value_; }
};

// ✅ Use concrete types when possible
class ConfigWrapper {
    Config value_;
public:
    Config get() { return value_; }
};
```

### Specialization and Instantiation

**DO: Provide Template Specializations**
```cpp
// Primary template
template<typename T>
class Logger {
    void log(const T& value) {
        std::cerr << value << "\n";
    }
};

// Specialization for std::vector
template<typename T>
class Logger<std::vector<T>> {
    void log(const std::vector<T>& vec) {
        for (const auto& item : vec) {
            std::cerr << item << " ";
        }
        std::cerr << "\n";
    }
};
```

**DON'T: Instantiate Unnecessarily**
```cpp
// ❌ Creates multiple template instances
std::vector<std::string> strings;
std::vector<std::wstring> wide_strings;
// Both are separate template instantiations

// ✅ Type-erase when possible
std::vector<std::shared_ptr<IProcessor>> processors;
```

---

## 7. Header-Only Libraries

### Organization

**DO: Use Inline Functions Carefully**
```cpp
// file.h - Header-only

// This is fine - simple one-liner
inline int get_default_timeout() {
    return 30000;
}

// Define complex logic in inline functions
template<typename T>
inline Result<T> safe_convert(const std::string& str) {
    try {
        return ok(T(str));
    } catch (...) {
        return error<T>(
            error::codes::common::invalid_argument,
            "Conversion failed",
            "safe_convert"
        );
    }
}
```

**DO: Minimize Compilation Time**
```cpp
// my_header.h
#pragma once

// Only include what's necessary
#include <string>
#include <vector>
#include <kcenon/common/patterns/result.h>

// Forward declarations when possible
class Logger;
class Database;

// Inline simple functions
inline std::string get_version() { return "1.0.0"; }

// Template definitions (required to be in header)
template<typename T>
Result<T> parse(const std::string& str);
```

**DO: Use #pragma once**
```cpp
#pragma once

// ... header content ...
```

**DON'T: Cause Header Bloat**
```cpp
// ❌ Including heavy headers unnecessarily
#include <iostream>       // For debugging
#include <fstream>        // Not used
#include <boost/asio.hpp> // Massive header

// ✅ Include only what's needed
#include <string>
#include <memory>
```

---

## 8. Thread Safety

### Data Sharing

**DO: Document Thread Safety**
```cpp
/**
 * @brief Thread-safe logger with internal synchronization
 *
 * Thread Safety:
 * - Multiple threads may call log() simultaneously
 * - Reference counting is atomic
 * - Shared state is protected by internal mutex
 */
class ThreadSafeLogger {
    std::mutex mutex_;
    std::vector<std::string> messages_;

public:
    void log(const std::string& msg) {
        std::lock_guard<std::mutex> lock(mutex_);
        messages_.push_back(msg);
    }
};
```

**DO: Use Atomic Operations for Counters**
```cpp
class ConnectionPool {
    std::atomic<int> active_count_{0};

public:
    void record_connection() {
        ++active_count_;  // Thread-safe
    }

    int get_active_count() const {
        return active_count_.load();  // Thread-safe
    }
};
```

**DO: Use Correct Lock Types**
```cpp
// std::lock_guard - Simple RAII lock
{
    std::lock_guard<std::mutex> lock(mutex_);
    // Guaranteed to be unlocked on exit
}

// std::unique_lock - For more control
std::unique_lock<std::mutex> lock(mutex_);
lock.unlock();  // Manual unlock possible
// Auto-unlock on destruction

// std::scoped_lock - For multiple mutexes
std::scoped_lock lock(mutex1_, mutex2_);
// Deadlock-safe locking of multiple mutexes
```

**DON'T: Share Mutable State Without Synchronization**
```cpp
// ❌ Unsafe - race condition
class Counter {
    int count_ = 0;  // NOT protected
public:
    void increment() {
        ++count_;  // Race!
    }
};

// ✅ Protected
class SafeCounter {
    std::atomic<int> count_{0};
public:
    void increment() {
        ++count_;  // Atomic
    }
};
```

### Result<T> Thread Safety

**DO: Understand Result<T> Limitations**
```cpp
// Result<T> is NOT thread-safe for concurrent modification
std::shared_ptr<std::mutex> mutex;
Result<Data> shared_result;

// Thread 1
{
    std::lock_guard lock(*mutex);
    shared_result = ok(data);
}

// Thread 2
{
    std::lock_guard lock(*mutex);
    if (shared_result.is_ok()) {
        auto value = shared_result.value();
    }
}
```

**DON'T: Share Result<T> Across Threads**
```cpp
// ❌ Unsafe - no synchronization
Result<Data> result;

std::thread t1([&]() {
    result = ok(data1);  // Race!
});

std::thread t2([&]() {
    auto value = result.value();  // Race!
});
```

---

## 9. Performance Optimization

### Move Semantics

**DO: Use Move for Large Objects**
```cpp
class DataCache {
    std::vector<CacheEntry> entries_;

public:
    void add_entry(CacheEntry entry) {
        entries_.push_back(std::move(entry));  // Avoid copy
    }

    Result<std::vector<CacheEntry>> get_snapshot() {
        return ok(std::move(entries_));  // Return without copy
    }
};

// Function accepting large result
Result<std::vector<int>> compute_all() {
    auto data = std::vector<int>();
    // ... fill data ...
    return ok(std::move(data));  // Move, not copy
}
```

**DO: Use RVO (Return Value Optimization)**
```cpp
Result<ComplexObject> create_object() {
    ComplexObject obj;
    // ... configure obj ...
    return ok(obj);  // RVO - no copy needed
}
```

**DO: Pass Large Objects by Const Reference**
```cpp
// Avoid unnecessary copies
Result<void> process(const std::vector<Data>& data) {
    // Use data as-is, no copy
}

// Only move when transferring ownership
Result<void> consume(std::unique_ptr<Resource> res) {
    // res is moved in
}
```

### Smart Pointer Optimization

**DO: Use make_unique/make_shared**
```cpp
// Single allocation - better
auto obj = std::make_unique<Widget>(args);
auto shared = std::make_shared<Widget>(args);

// Avoid separate allocations
auto obj = std::unique_ptr<Widget>(new Widget(args));  // Two allocations
```

**DO: Pass By const Reference When Not Transferring**
```cpp
// No ref count change
void process(const std::shared_ptr<Widget>& widget) {
    widget->do_something();
}

// vs unnecessary copy
void process_bad(std::shared_ptr<Widget> widget) {
    widget->do_something();  // Ref count increment/decrement
}
```

### Result<T> Overhead

**DO: Be Mindful of Size**
```cpp
// sizeof(Result<T>) ≈ sizeof(T) + sizeof(error_info) + padding

// Check for large Result types
static_assert(sizeof(Result<Data>) <= 256);  // Reasonable limit

// Pass large Results by const reference
void process(const Result<LargeData>& result) {
    if (result.is_ok()) {
        const auto& data = result.value();  // Reference - no copy
    }
}
```

### Caching Patterns

**DO: Use Weak Pointers for Caches**
```cpp
class ResourceCache {
    std::unordered_map<std::string, std::weak_ptr<Resource>> cache_;
    std::mutex mutex_;

public:
    std::shared_ptr<Resource> get_or_create(const std::string& key) {
        std::lock_guard lock(mutex_);

        // Check if still cached
        auto it = cache_.find(key);
        if (it != cache_.end()) {
            if (auto resource = it->second.lock()) {
                return resource;  // Cache hit
            }
            cache_.erase(it);  // Expired
        }

        // Create new
        auto resource = std::make_shared<Resource>(key);
        cache_[key] = resource;
        return resource;
    }
};
```

---

## 10. Testing Patterns

### Unit Testing Result<T>

**DO: Test Both Success and Error Cases**
```cpp
#include <gtest/gtest.h>

class ParserTest : public ::testing::Test {};

TEST_F(ParserTest, ParseValidInteger) {
    auto result = parse_int("42");
    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(42, result.value());
}

TEST_F(ParserTest, ParseInvalidInteger) {
    auto result = parse_int("not_a_number");
    ASSERT_TRUE(result.is_err());
    EXPECT_EQ(error::codes::common::invalid_argument, result.error().code);
}
```

**DO: Test Error Propagation**
```cpp
TEST_F(PipelineTest, ErrorPropagatesCorrectly) {
    auto result = validate_and_process(invalid_input);

    ASSERT_TRUE(result.is_err());
    EXPECT_EQ(error::codes::common::invalid_argument, result.error().code);
    EXPECT_THAT(result.error().message,
                HasSubstr("must be positive"));
}
```

**DO: Mock Interfaces**
```cpp
class MockLogger : public ILogger {
public:
    MOCK_METHOD(void, info, (const std::string&), (override));
    MOCK_METHOD(void, error, (const std::string&), (override));
};

TEST(DataProcessorTest, LogsOnSuccess) {
    auto mock_logger = std::make_shared<MockLogger>();
    DataProcessor processor(mock_logger);

    EXPECT_CALL(*mock_logger, info)
        .WillOnce(::testing::Return());

    auto result = processor.process(valid_data);
    ASSERT_TRUE(result.is_ok());
}
```

### RAII Testing

**DO: Test Exception Safety**
```cpp
TEST(FileWriterTest, CleanupOnException) {
    {
        FileWriter writer("test.txt");
        EXPECT_THROW(writer.write(nullptr), std::invalid_argument);
        // File should be properly closed despite exception
    }

    // Verify file was closed
    std::ifstream file("test.txt");
    EXPECT_FALSE(file.fail());
}
```

**DO: Use AddressSanitizer**
```bash
# Compile with ASan
cmake . -DCMAKE_CXX_FLAGS="-fsanitize=address"
make && make test

# Should report any memory leaks or use-after-free
```

### Smart Pointer Testing

**DO: Test Ownership Transfer**
```cpp
TEST(SmartPointerTest, OwnershipTransfer) {
    auto original = std::make_unique<Resource>();
    auto* ptr = original.get();

    // Verify original still owns
    EXPECT_NE(nullptr, original);

    // Transfer ownership
    auto moved = std::move(original);
    EXPECT_EQ(nullptr, original);  // Original is now nullptr
    EXPECT_EQ(ptr, moved.get());   // Moved has the resource
}
```

**DO: Test Circular Reference Detection**
```cpp
TEST(SharedPtrTest, NoCircularReference) {
    std::shared_ptr<Node> parent = std::make_shared<Node>();
    std::shared_ptr<Node> child = std::make_shared<Node>();

    // Using weak_ptr to avoid circular reference
    parent->set_child(child);
    child->set_parent(parent);  // weak_ptr - no cycle

    parent.reset();
    // Parent should be deleted (no circular reference)
    EXPECT_EQ(1, child->parent().use_count());
}
```

---

## Quick Reference Table

| Category | Pattern | Usage |
|----------|---------|-------|
| Error Handling | `Result<T>` | All module boundaries |
| Error Codes | Centralized enums | `error::codes::system::error` |
| Ownership | `unique_ptr` | Single owner |
| Ownership | `shared_ptr` | Multiple owners |
| Non-Owning | `weak_ptr` | Break cycles |
| Non-Owning | Raw pointer/ref | Optional/required access |
| Interfaces | `IExecutor`, `ILogger` | Dependency injection |
| Resources | RAII wrappers | System resources |
| Cleanup | `ScopeExit` | Scope-based actions |
| Execution | `IJob` | Job-based tasks |
| Testing | Mock interfaces | Testability |

---

## Related Documentation

- [Error Handling Guidelines](../ERROR_HANDLING.md)
- [RAII Guidelines](../RAII_GUIDELINES.md)
- [Smart Pointer Guidelines](../SMART_POINTER_GUIDELINES.md)
- [IExecutor Migration Guide](../IEXECUTOR_MIGRATION_GUIDE.md)
- [Error Code Guidelines](../ERROR_CODE_GUIDELINES.md)

---

## Checklist for Code Review

When reviewing code using common_system patterns:

- [ ] All functions at module boundaries use `Result<T>`
- [ ] Error codes are from centralized `error::codes` namespace
- [ ] No naked `new`/`delete`
- [ ] Smart pointers match ownership semantics
- [ ] RAII used for resource management
- [ ] Destructors are `noexcept`
- [ ] Move semantics implemented where needed
- [ ] Circular references broken with `weak_ptr`
- [ ] Interfaces injected via constructor
- [ ] Tests cover both success and error paths
- [ ] No exceptions crossing module boundaries (converted to Result)
- [ ] Thread-unsafe shared state is documented or protected

---

*Last Updated: 2026-01-03*
*Version: 1.1*
