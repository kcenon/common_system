# Adapter Framework Guide

> **Language:** **English** | [한국어](ADAPTER_GUIDE.kr.md)

**Complete Guide**: Type-Safe Adapters, Interface Adaptation, and Cross-System Integration

**Status**: ✅ **Complete**

This guide provides comprehensive documentation for the adapter framework in common_system, covering both the legacy adapter system (typed_adapter, smart_adapter) and the new unified adapter system (interface_adapter, adapter_factory).

---

## Table of Contents

- [Overview](#overview)
- [Adapter Pattern in kcenon](#1-adapter-pattern-in-kcenon)
  - [Why Adapters](#why-adapters)
  - [Loose Coupling Benefits](#loose-coupling-benefits)
  - [Adapter Lifecycle](#adapter-lifecycle)
- [Base Adapter](#2-base-adapter-adapterh)
  - [adapter_base Interface](#adapter_base-interface)
  - [Adapter Traits](#adapter-traits)
  - [Type Safety Without RTTI](#type-safety-without-rtti)
- [Legacy Adapters (Deprecated)](#3-legacy-adapters-deprecated)
  - [typed_adapter](#typed_adapter)
  - [smart_adapter_factory](#smart_adapter_factory)
  - [Deprecation Status](#deprecation-status)
- [New Unified Adapter System](#4-new-unified-adapter-system)
  - [adapter<T>](#adaptert)
  - [interface_adapter](#interface_adapter)
  - [adapter_factory](#adapter_factory)
  - [Zero-Cost Abstraction](#zero-cost-abstraction)
- [Cross-System Integration](#5-cross-system-integration)
  - [Logger Integration](#logger-integration)
  - [Executor Integration](#executor-integration)
  - [DI Container Setup](#di-container-setup)
- [Creating Custom Adapters](#6-creating-custom-adapters)
  - [Step-by-Step Guide](#step-by-step-guide)
  - [Interface Design Best Practices](#interface-design-best-practices)
  - [Testing Adapters](#testing-adapters)
- [Migration Guide](#7-migration-guide)
  - [Migration Strategy](#migration-strategy)
  - [API Mapping](#api-mapping)
  - [Code Examples](#code-examples)
- [Troubleshooting](#8-troubleshooting)
- [Related Documentation](#related-documentation)

---

## Overview

The adapter framework is a cornerstone of the kcenon ecosystem, enabling type-safe, loosely-coupled integration between systems. It provides compile-time type safety without RTTI overhead.

### Key Features

| Feature | Description |
|---------|-------------|
| **Type Safety** | Compile-time type checking without RTTI |
| **Zero-Cost Abstraction** | Direct casts when possible, no wrapper overhead |
| **Wrapper Depth Tracking** | Prevents infinite adapter chains |
| **Smart Pointer Support** | Works with shared_ptr, unique_ptr, and values |
| **Interface Adaptation** | Adapts implementations to interfaces |

### Adapter Files

The adapter framework is located at `include/kcenon/common/adapters/`:

- `adapter.h` — **New** unified adapter system (recommended)
- `typed_adapter.h` — **Legacy** typed adapter (deprecated)
- `smart_adapter.h` — **Legacy** smart adapter factory (deprecated)
- `adapters.h` — Migration guide and unified utilities

**Migration Status**: Legacy adapters (typed_adapter, smart_adapter) are deprecated. Use the new unified adapter system from `adapter.h`.

---

## 1. Adapter Pattern in kcenon

### Why Adapters

The kcenon ecosystem uses adapters instead of direct dependencies to achieve:

#### 1. Loose Coupling

Systems depend on interfaces, not implementations:

```cpp
// WITHOUT adapters: Tight coupling
class DatabaseSystem {
    thread_pool pool_;  // Direct dependency on thread_pool
    logger logger_;     // Direct dependency on logger
};

// WITH adapters: Loose coupling
class DatabaseSystem {
    std::shared_ptr<IExecutor> executor_;  // Depends on interface
    std::shared_ptr<ILogger> logger_;       // Depends on interface
};
```

#### 2. Testability

Mock implementations can be easily substituted:

```cpp
// Production
auto db = DatabaseSystem(
    make_interface_adapter<IExecutor>(thread_pool),
    make_interface_adapter<ILogger>(production_logger)
);

// Testing
auto db_test = DatabaseSystem(
    make_interface_adapter<IExecutor>(mock_executor),
    make_interface_adapter<ILogger>(mock_logger)
);
```

#### 3. Modularity

Systems can be composed dynamically:

```cpp
// Compose systems based on configuration
if (config.async_mode) {
    executor = make_interface_adapter<IExecutor>(thread_pool);
} else {
    executor = make_interface_adapter<IExecutor>(sync_executor);
}
```

### Loose Coupling Benefits

| Aspect | Direct Dependency | With Adapters |
|--------|-------------------|---------------|
| **Compilation** | Requires full type definition | Interface-only headers |
| **Testing** | Difficult to mock | Easy mocking |
| **Flexibility** | Fixed at compile time | Configurable at runtime |
| **Maintenance** | Changes ripple through systems | Changes isolated to implementations |

### Adapter Lifecycle

Adapters follow a standard lifecycle:

```
┌──────────────────────────────────────────────────┐
│ 1. Create                                        │
│    auto adapter = make_interface_adapter<I>(impl);│
└────────────────┬─────────────────────────────────┘
                 ▼
┌──────────────────────────────────────────────────┐
│ 2. Register (Optional)                           │
│    container.register_adapter("logger", adapter);│
└────────────────┬─────────────────────────────────┘
                 ▼
┌──────────────────────────────────────────────────┐
│ 3. Resolve                                       │
│    auto logger = container.resolve<ILogger>();   │
└────────────────┬─────────────────────────────────┘
                 ▼
┌──────────────────────────────────────────────────┐
│ 4. Use                                           │
│    logger->info("Message");                      │
└────────────────┬─────────────────────────────────┘
                 ▼
┌──────────────────────────────────────────────────┐
│ 5. Destroy                                       │
│    (Automatic via shared_ptr reference counting) │
└──────────────────────────────────────────────────┘
```

---

## 2. Base Adapter (`adapter.h`)

### adapter_base Interface

The `adapter_base` class provides a common interface for all adapters without RTTI:

```cpp
class adapter_base {
public:
    virtual ~adapter_base() = default;

    // Get wrapper depth (number of adapter layers)
    virtual size_t get_adapter_depth() const = 0;

    // Check if this is an adapter
    virtual bool is_adapter() const { return true; }

    // Get unique type ID without RTTI
    virtual size_t get_type_id() const = 0;
};
```

**Key Methods**:

| Method | Purpose |
|--------|---------|
| `get_adapter_depth()` | Returns nesting depth (0 = direct implementation) |
| `is_adapter()` | Always returns `true` for adapters |
| `get_type_id()` | Unique type identifier without RTTI |

### Adapter Traits

`adapter_traits<T>` provides compile-time type information:

```cpp
template<typename T>
struct adapter_traits {
    using value_type = T;
    using pointer_type = T*;
    using reference_type = T&;
    using const_reference_type = const T&;
    static constexpr bool is_smart_pointer = false;
    static constexpr bool supports_weak = false;
};
```

**Specializations**:

#### For `std::shared_ptr<T>`

```cpp
template<typename T>
struct adapter_traits<std::shared_ptr<T>> {
    using value_type = T;
    using pointer_type = std::shared_ptr<T>;
    using weak_type = std::weak_ptr<T>;
    static constexpr bool is_smart_pointer = true;
    static constexpr bool supports_weak = true;  // ✅ Supports weak_ptr
};
```

#### For `std::unique_ptr<T>`

```cpp
template<typename T, typename Deleter>
struct adapter_traits<std::unique_ptr<T, Deleter>> {
    using value_type = T;
    using pointer_type = std::unique_ptr<T, Deleter>;
    static constexpr bool is_smart_pointer = true;
    static constexpr bool supports_weak = false;  // ❌ No weak_ptr
};
```

### Type Safety Without RTTI

The adapter framework achieves type safety without RTTI using:

1. **Static Type IDs**: Each adapter type gets a unique compile-time ID
2. **Template Specialization**: Compile-time dispatch based on type
3. **No `dynamic_cast`**: Uses static casts and type IDs

**Example**:

```cpp
// No RTTI required
size_t get_static_type_id() {
    static const size_t id = generate_type_id();
    return id;
}
```

**Benefits**:
- ✅ No RTTI overhead
- ✅ Smaller binary size
- ✅ Compatible with `-fno-rtti` builds
- ✅ Faster type checks

---

## 3. Legacy Adapters (Deprecated)

### typed_adapter

**Status**: ⚠️ **Deprecated** — Use `interface_adapter` from `adapter.h` instead

The `typed_adapter` wraps an implementation to match an interface:

```cpp
template<typename Interface, typename Implementation>
class typed_adapter : public Interface, public adapter_base {
public:
    explicit typed_adapter(std::shared_ptr<Implementation> impl);

    std::shared_ptr<Implementation> unwrap() const;
    size_t get_wrapper_depth() const;
};
```

**Example**:

```cpp
// Legacy API (deprecated)
auto adapter = std::make_shared<typed_adapter<ILogger, logger>>(logger_impl);
adapter->info("Message");  // Calls logger_impl::info

// Unwrap
auto impl = adapter->unwrap();  // Get original logger_impl
```

**Limitations**:
- ❌ Deprecated in favor of `interface_adapter`
- ❌ Will be removed in a future major version
- ⚠️ Use only for backward compatibility

### smart_adapter_factory

**Status**: ⚠️ **Deprecated** — Use `adapter_factory` from `adapter.h` instead

The `smart_adapter_factory` provides zero-cost adaptation when possible:

```cpp
class smart_adapter_factory {
public:
    // Create adapter (zero-cost if Impl implements Interface)
    template<typename Interface, typename Impl>
    static std::shared_ptr<Interface> make_adapter(std::shared_ptr<Impl> impl);

    // Try to unwrap adapter
    template<typename T, typename Interface>
    static std::shared_ptr<T> try_unwrap(std::shared_ptr<Interface> ptr);
};
```

**Example**:

```cpp
// Legacy API (deprecated)
auto executor = smart_adapter_factory::make_adapter<IExecutor>(thread_pool);

// If thread_pool implements IExecutor: zero-cost cast
// Otherwise: creates typed_adapter<IExecutor, thread_pool>
```

**Zero-Cost Optimization**:

```cpp
if constexpr (std::is_base_of_v<Interface, Impl>) {
    // Direct cast, no wrapper
    return std::static_pointer_cast<Interface>(impl);
} else {
    // Create adapter wrapper
    return std::make_shared<typed_adapter<Interface, Impl>>(impl);
}
```

### Deprecation Status

| Legacy API | New API | Removal Version |
|-----------|---------|-----------------|
| `typed_adapter<I, T>` | `interface_adapter<I, T>` | v2.0.0 |
| `smart_adapter_factory` | `adapter_factory` | v2.0.0 |
| `make_smart_adapter<I>(impl)` | `make_interface_adapter<I>(impl)` | v2.0.0 |
| `safe_unwrap<T>(ptr)` | `adapter_factory::try_unwrap<T>(ptr)` | v2.0.0 |

**Deprecation Timeline**:
- **v1.x**: Legacy APIs maintained, warnings in documentation
- **v2.0**: Legacy APIs removed, migration required

---

## 4. New Unified Adapter System

### adapter<T>

The unified `adapter<T>` template works with values, shared_ptr, and unique_ptr:

```cpp
template<typename T, typename Traits = adapter_traits<T>>
class adapter {
public:
    explicit adapter(T value);

    auto get() const noexcept;       // Get raw pointer
    auto operator*() const;           // Dereference
    auto operator->() const noexcept; // Arrow operator
    explicit operator bool() const noexcept; // Validity check
    const T& value() const& noexcept; // Get underlying storage
};
```

**Usage Examples**:

#### Value Type

```cpp
auto a = adapter<int>(42);
int value = *a;         // 42
int* ptr = a.get();     // Pointer to value
```

#### Shared Pointer

```cpp
auto a = adapter(std::make_shared<MyClass>());
auto ptr = a.get();     // MyClass*
ptr->method();          // Call MyClass::method

auto weak = a.weak();   // Get weak_ptr (only for shared_ptr)
```

#### Unique Pointer

```cpp
auto a = adapter(std::make_unique<MyClass>());
auto ptr = a.get();     // MyClass*
ptr->method();

// No weak() - unique_ptr doesn't support weak references
```

### interface_adapter

**Status**: ✅ **Recommended** — Replaces `typed_adapter`

The `interface_adapter` provides type-safe interface adaptation:

```cpp
template<typename Interface, typename Implementation>
class interface_adapter : public Interface, public adapter_base {
public:
    explicit interface_adapter(std::shared_ptr<Implementation> impl);

    std::shared_ptr<Implementation> unwrap() const;
    size_t get_wrapper_depth() const;
};
```

**Example**:

```cpp
// Create interface adapter
auto logger_adapter = make_interface_adapter<ILogger>(logger_impl);

// Use as ILogger
logger_adapter->info("Message");

// Unwrap to get original implementation
auto impl = logger_adapter->unwrap();
```

### adapter_factory

**Status**: ✅ **Recommended** — Replaces `smart_adapter_factory`

The `adapter_factory` creates zero-cost adapters when possible:

```cpp
class adapter_factory {
public:
    // Create adapter (zero-cost if possible)
    template<typename Interface, typename Impl>
    static std::shared_ptr<Interface> create(std::shared_ptr<Impl> impl);

    // Try to unwrap adapter
    template<typename T, typename Interface>
    static std::shared_ptr<T> try_unwrap(std::shared_ptr<Interface> ptr);

    // Check if zero-cost adaptation is possible
    template<typename Interface, typename Impl>
    static constexpr bool is_zero_cost();
};
```

**Example**:

```cpp
// Create adapter
auto executor = adapter_factory::create<IExecutor>(thread_pool);

// Try to unwrap
if (auto pool = adapter_factory::try_unwrap<thread_pool>(executor)) {
    // Successfully unwrapped
}

// Check if zero-cost
constexpr bool zero_cost = adapter_factory::is_zero_cost<IExecutor, thread_pool>();
```

### Zero-Cost Abstraction

The new adapter system provides zero-cost abstraction when the implementation already implements the interface:

```cpp
// Case 1: Implementation already implements interface
class MyExecutor : public IExecutor { /* ... */ };

auto executor = adapter_factory::create<IExecutor>(
    std::make_shared<MyExecutor>()
);
// Result: Direct cast, no wrapper (zero-cost)


// Case 2: Implementation does NOT implement interface
class ThreadPool { /* ... */ };  // No IExecutor inheritance

auto executor = adapter_factory::create<IExecutor>(
    std::make_shared<ThreadPool>()
);
// Result: Creates interface_adapter<IExecutor, ThreadPool>
```

**Performance Comparison**:

| Scenario | Overhead |
|----------|----------|
| Direct cast (zero-cost) | 0 bytes, 0 CPU cycles |
| Adapter wrapper | 1 vtable pointer, 1 indirect call |

---

## 5. Cross-System Integration

### Logger Integration

Connect logger_system to any system that needs logging:

```cpp
#include <kcenon/common/adapters/adapter.h>
#include <kcenon/logger/logger.h>

// Create logger implementation
auto logger_impl = std::make_shared<logger>("app");

// Adapt to ILogger interface
auto logger_adapter = make_interface_adapter<ILogger>(logger_impl);

// Use in database system
DatabaseSystem db(logger_adapter);

// Use in network system
NetworkSystem network(logger_adapter);
```

**Benefits**:
- Both systems use the same logger instance
- No direct dependency on `logger_system` headers
- Easy to swap implementations (e.g., for testing)

### Executor Integration

Connect thread_system to provide asynchronous execution:

```cpp
#include <kcenon/common/adapters/adapter.h>
#include <kcenon/thread/thread_pool.h>

// Create thread pool
auto pool = std::make_shared<thread_pool>(8);  // 8 threads

// Adapt to IExecutor interface
auto executor = make_interface_adapter<IExecutor>(pool);

// Use in database system
DatabaseSystem db(executor, logger);

// Use in network system
NetworkSystem network(executor, logger);
```

### DI Container Setup

Complete dependency injection container setup:

```cpp
#include <kcenon/common/adapters/adapter.h>

class ServiceContainer {
public:
    template<typename Interface>
    void register_adapter(const std::string& name,
                          std::shared_ptr<Interface> adapter) {
        adapters_[name] = adapter;
    }

    template<typename Interface>
    std::shared_ptr<Interface> resolve(const std::string& name) {
        auto it = adapters_.find(name);
        if (it == adapters_.end()) {
            throw std::runtime_error("Service not found: " + name);
        }
        return std::static_pointer_cast<Interface>(it->second);
    }

private:
    std::unordered_map<std::string, std::shared_ptr<void>> adapters_;
};

// Setup container
ServiceContainer container;

// Register logger
auto logger = make_interface_adapter<ILogger>(logger_impl);
container.register_adapter("logger", logger);

// Register executor
auto executor = make_interface_adapter<IExecutor>(pool);
container.register_adapter("executor", executor);

// Resolve dependencies
auto db = std::make_shared<DatabaseSystem>(
    container.resolve<IExecutor>("executor"),
    container.resolve<ILogger>("logger")
);
```

---

## 6. Creating Custom Adapters

### Step-by-Step Guide

#### Step 1: Define Interface

```cpp
// ICache.h
class ICache {
public:
    virtual ~ICache() = default;

    virtual void set(const std::string& key, const std::string& value) = 0;
    virtual std::optional<std::string> get(const std::string& key) = 0;
    virtual bool exists(const std::string& key) const = 0;
};
```

#### Step 2: Implement Interface

```cpp
// redis_cache.h
class redis_cache {
public:
    void set(const std::string& key, const std::string& value);
    std::optional<std::string> get(const std::string& key);
    bool exists(const std::string& key) const;

private:
    // Redis connection details
};
```

#### Step 3: Create Adapter

**Option A**: Use interface_adapter (if implementation doesn't inherit interface)

```cpp
// Create adapter
auto cache_adapter = make_interface_adapter<ICache>(
    std::make_shared<redis_cache>()
);
```

**Option B**: Implement interface directly (zero-cost)

```cpp
// redis_cache.h
class redis_cache : public ICache {  // Inherit interface
public:
    void set(const std::string& key, const std::string& value) override;
    std::optional<std::string> get(const std::string& key) override;
    bool exists(const std::string& key) const override;
};

// Create adapter (zero-cost)
auto cache_adapter = adapter_factory::create<ICache>(
    std::make_shared<redis_cache>()
);
// Result: Direct cast, no wrapper
```

#### Step 4: Use Adapter

```cpp
class Application {
public:
    Application(std::shared_ptr<ICache> cache) : cache_(cache) {}

    void run() {
        cache_->set("key", "value");
        auto value = cache_->get("key");
    }

private:
    std::shared_ptr<ICache> cache_;
};

// Production
auto app = Application(cache_adapter);

// Testing
auto mock_cache = make_interface_adapter<ICache>(mock_cache_impl);
auto test_app = Application(mock_cache);
```

### Interface Design Best Practices

#### 1. Single Responsibility

Each interface should have a single, well-defined purpose:

```cpp
// GOOD: Focused interface
class ILogger {
    virtual void log(LogLevel level, const std::string& message) = 0;
};

// BAD: Too many responsibilities
class IEverything {
    virtual void log(const std::string& message) = 0;
    virtual void execute(Task task) = 0;
    virtual void cache_set(const std::string& key, const std::string& value) = 0;
};
```

#### 2. Minimal Interface

Keep interfaces as small as possible:

```cpp
// GOOD: Minimal interface
class IExecutor {
    virtual void execute(Task task) = 0;
};

// BAD: Too many methods
class IExecutor {
    virtual void execute(Task task) = 0;
    virtual size_t thread_count() const = 0;
    virtual void set_thread_count(size_t count) = 0;
    virtual std::vector<TaskStats> get_stats() const = 0;
    // ... 20 more methods
};
```

#### 3. Value Semantics

Prefer value types for parameters and return values:

```cpp
// GOOD: Value semantics
class ICache {
    virtual void set(const std::string& key, const std::string& value) = 0;
    virtual std::optional<std::string> get(const std::string& key) = 0;
};

// AVOID: Raw pointers
class ICache {
    virtual void set(const char* key, const char* value) = 0;
    virtual const char* get(const char* key) = 0;  // Who owns this?
};
```

### Testing Adapters

#### Unit Testing

```cpp
#include <catch2/catch.hpp>

TEST_CASE("Cache adapter forwards calls correctly", "[adapter]") {
    // Create mock
    auto mock = std::make_shared<MockCache>();

    // Create adapter
    auto adapter = make_interface_adapter<ICache>(mock);

    // Test set
    EXPECT_CALL(*mock, set("key", "value")).Times(1);
    adapter->set("key", "value");

    // Test get
    EXPECT_CALL(*mock, get("key")).WillOnce(Return(std::optional("value")));
    auto result = adapter->get("key");
    REQUIRE(result.has_value());
    REQUIRE(result.value() == "value");
}
```

#### Integration Testing

```cpp
TEST_CASE("Cache adapter integrates with real Redis", "[integration]") {
    // Create real implementation
    auto redis = std::make_shared<redis_cache>("localhost:6379");

    // Create adapter
    auto cache = make_interface_adapter<ICache>(redis);

    // Test integration
    cache->set("test_key", "test_value");
    auto result = cache->get("test_key");
    REQUIRE(result.has_value());
    REQUIRE(result.value() == "test_value");
}
```

---

## 7. Migration Guide

### Migration Strategy

Migrate from legacy adapters to the new system in 3 phases:

#### Phase 1: Preparation
1. Identify all uses of `typed_adapter` and `smart_adapter_factory`
2. Review API mapping table
3. Create migration plan

#### Phase 2: Gradual Migration
1. Migrate one module at a time
2. Run tests after each migration
3. Keep legacy code until all modules are migrated

#### Phase 3: Cleanup
1. Remove all legacy adapter usage
2. Update documentation
3. Enable deprecation warnings

### API Mapping

| Legacy API | New API | Notes |
|-----------|---------|-------|
| `typed_adapter<I, T>` | `interface_adapter<I, T>` | Direct replacement |
| `smart_adapter_factory::make_adapter<I>(impl)` | `adapter_factory::create<I>(impl)` | Same behavior |
| `make_smart_adapter<I>(impl)` | `make_interface_adapter<I>(impl)` | Convenience function |
| `smart_adapter_factory::try_unwrap<T>(ptr)` | `adapter_factory::try_unwrap<T>(ptr)` | Direct replacement |
| `safe_unwrap<T>(ptr)` | `adapter_factory::try_unwrap<T>(ptr)` | Direct replacement |
| `smart_adapter_factory::is_zero_cost<I, T>()` | `adapter_factory::is_zero_cost<I, T>()` | Static check |

### Code Examples

#### Before (Legacy)

```cpp
#include <kcenon/common/adapters/typed_adapter.h>
#include <kcenon/common/adapters/smart_adapter.h>

// Create typed adapter
auto logger_adapter = std::make_shared<typed_adapter<ILogger, logger>>(logger_impl);

// Create smart adapter
auto executor = make_smart_adapter<IExecutor>(thread_pool);

// Unwrap
auto impl = safe_unwrap<thread_pool>(executor);
```

#### After (New)

```cpp
#include <kcenon/common/adapters/adapter.h>

// Create interface adapter
auto logger_adapter = make_interface_adapter<ILogger>(logger_impl);

// Create adapter (zero-cost if possible)
auto executor = adapter_factory::create<IExecutor>(thread_pool);

// Unwrap
auto impl = adapter_factory::try_unwrap<thread_pool>(executor);
```

#### Migration Steps

1. **Replace includes**:
   ```cpp
   // Before
   #include <kcenon/common/adapters/typed_adapter.h>
   #include <kcenon/common/adapters/smart_adapter.h>

   // After
   #include <kcenon/common/adapters/adapter.h>
   ```

2. **Replace typed_adapter**:
   ```cpp
   // Before
   auto adapter = std::make_shared<typed_adapter<ILogger, logger>>(logger_impl);

   // After
   auto adapter = make_interface_adapter<ILogger>(logger_impl);
   ```

3. **Replace smart_adapter_factory**:
   ```cpp
   // Before
   auto executor = smart_adapter_factory::make_adapter<IExecutor>(pool);

   // After
   auto executor = adapter_factory::create<IExecutor>(pool);
   ```

4. **Replace unwrap**:
   ```cpp
   // Before
   auto impl = safe_unwrap<T>(ptr);

   // After
   auto impl = adapter_factory::try_unwrap<T>(ptr);
   ```

5. **Run tests** and verify behavior is unchanged

---

## 8. Troubleshooting

### Adapter Chain Too Deep

**Symptom**:
```
runtime_error: Adapter chain too deep (3 levels, max: 2)
```

**Cause**: Wrapping an adapter with another adapter multiple times

**Solution**: Unwrap before re-adapting

```cpp
// WRONG: Wrapping an adapter
auto adapter1 = make_interface_adapter<ILogger>(logger);
auto adapter2 = make_interface_adapter<ILogger>(adapter1);  // ERROR

// CORRECT: Unwrap first
auto adapter1 = make_interface_adapter<ILogger>(logger);
auto impl = adapter_factory::try_unwrap<logger>(adapter1);
if (impl) {
    auto adapter2 = make_interface_adapter<ILogger>(impl);  // OK
}
```

### Unwrap Failure

**Symptom**:
```cpp
auto impl = adapter_factory::try_unwrap<T>(ptr);
// impl is nullptr
```

**Cause**: Pointer is not an adapter, or wrong underlying type

**Solution**: Check type before unwrapping

```cpp
// Check if it's an adapter
if (auto base = std::dynamic_pointer_cast<adapter_base>(ptr)) {
    // It's an adapter
    auto impl = adapter_factory::try_unwrap<T>(ptr);
    if (!impl) {
        // Wrong underlying type
    }
} else {
    // Not an adapter, might be direct implementation
    auto impl = std::static_pointer_cast<T>(ptr);
}
```

### Zero-Cost Check Failure

**Symptom**: Expected zero-cost adaptation, but adapter wrapper was created

**Cause**: Implementation doesn't inherit from interface

**Solution**: Make implementation inherit from interface

```cpp
// Before (creates wrapper)
class MyExecutor {
    void execute(Task task) { /* ... */ }
};

// After (zero-cost)
class MyExecutor : public IExecutor {
    void execute(Task task) override { /* ... */ }
};
```

---

## Related Documentation

- [Integration Guide](INTEGRATION_GUIDE.md) — Cross-system integration patterns
- [API Reference](API_REFERENCE.md) — Complete API documentation
- [Architecture](ARCHITECTURE.md) — System architecture and design principles

### Adapter Framework Headers

- `include/kcenon/common/adapters/adapter.h` — **New** unified adapter system
- `include/kcenon/common/adapters/typed_adapter.h` — **Legacy** typed adapter (deprecated)
- `include/kcenon/common/adapters/smart_adapter.h` — **Legacy** smart adapter (deprecated)
- `include/kcenon/common/adapters/adapters.h` — Migration guide

---

**Version**: 1.0.0
**Last Updated**: 2026-02-08
**License**: BSD 3-Clause (see LICENSE file)
