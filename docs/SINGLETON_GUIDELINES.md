# Singleton Pattern Guidelines

This guide establishes standardized singleton pattern guidelines for the kcenon ecosystem to prevent Static Destruction Order Fiasco (SDOF) issues across all systems.

## Background

Multiple systems in the kcenon ecosystem use singleton patterns for various managers and loggers. The interaction between these singletons during static destruction has caused recurring issues:

- network_system#301: CI failures due to SDOF
- network_system#302, #304: io_context_thread_manager SDOF
- thread_system#293, #295: thread_logger and thread_pool SDOF

## Problem Statement

When multiple systems are integrated (e.g., network_system with thread_system), their singletons can be destroyed in unpredictable order, leading to:

- `free(): invalid pointer` crashes
- Accessing destroyed objects
- Undefined behavior during process termination

## Singleton Patterns

### 1. Meyer's Singleton (Default for Pure Data)

Use when the singleton has no dependencies and won't be accessed during destruction of other objects.

```cpp
class PureDataSingleton {
public:
    static PureDataSingleton& instance() {
        static PureDataSingleton instance;
        return instance;
    }

private:
    PureDataSingleton() = default;
    ~PureDataSingleton() = default;
};
```

**When to Use:**
- Pure data singletons with no dependencies
- Configuration managers that are only read during initialization
- Singletons that are guaranteed to be destroyed last

### 2. Intentional Leak Pattern (Required for Infrastructure)

Use for any singleton that might be accessed during other objects' destruction.

```cpp
class InfrastructureSingleton {
public:
    static InfrastructureSingleton& instance() {
        // Intentionally leak to avoid SDOF
        static InfrastructureSingleton* instance = new InfrastructureSingleton();
        return *instance;
    }

    // Optional: explicit shutdown for resource cleanup
    static void shutdown() {
        // ... cleanup logic (logging, flushing, etc.)
        // Note: instance is never deleted
    }

private:
    InfrastructureSingleton() = default;
    ~InfrastructureSingleton() = default;  // Never called
};
```

**When to Use:**
- Loggers and logging infrastructure
- Thread pools and async managers
- Any singleton that might be accessed during other objects' destruction
- Global managers that coordinate cross-system resources

## common_system Audit Results

| Singleton | Location | Current Pattern | SDOF Risk | Recommendation |
|-----------|----------|-----------------|-----------|----------------|
| `simple_event_bus::instance()` | event_bus.h:424 | Meyer's Singleton | ⚠️ Medium | Consider Intentional Leak if used during destruction |
| `GlobalLoggerRegistry::instance()` | global_logger_registry.h:335 | Meyer's Singleton | ⚠️ Medium | Consider Intentional Leak for cross-system logging |
| `GlobalLoggerRegistry::null_logger()` | global_logger_registry.h:340 | Meyer's Singleton | ✅ Low | Acceptable (pure data, no dependencies) |
| `patterns::Singleton<T>` | forward.h:21 | Forward declaration only | N/A | No implementation found |

### Detailed Analysis

#### simple_event_bus

```cpp
// Current implementation (event_bus.h:424-427)
static simple_event_bus& instance() {
    static simple_event_bus instance;
    return instance;
}
```

**Risk Analysis:**
- Event bus may be used for cleanup notifications during shutdown
- If subscribers log events in their destructors, SDOF may occur
- Consider migration to Intentional Leak if cross-system events are needed

#### GlobalLoggerRegistry

```cpp
// Current implementation (global_logger_registry.h:335-338)
static GlobalLoggerRegistry& instance() {
    static GlobalLoggerRegistry instance;
    return instance;
}
```

**Risk Analysis:**
- Logger registry is likely accessed during shutdown for final logging
- Other system destructors may call logging functions
- Strong candidate for Intentional Leak pattern

## Static Destruction Safety Checklist

For new singletons, verify:

- [ ] No logging calls in destructor
- [ ] No access to other singletons in destructor
- [ ] No thread operations in destructor
- [ ] Consider Intentional Leak if any of the above are needed

## Documentation Requirements

Each singleton should document:

1. **Pattern Choice**: Why this pattern was chosen
2. **Memory Implications**: Leak size (for Intentional Leak pattern)
3. **Shutdown Procedure**: How to cleanly shutdown (if applicable)

### Example Documentation

```cpp
/**
 * @brief Get the singleton instance.
 *
 * Pattern: Intentional Leak
 * Reason: This logger may be accessed during static destruction
 *         by other system components for final logging.
 * Memory: ~200 bytes leaked (acceptable for process lifetime singleton)
 * Shutdown: Call shutdown() before main() returns for graceful cleanup.
 *
 * @return Reference to the singleton instance
 */
static MyLogger& instance();
```

## Migration Guide

### From Meyer's Singleton to Intentional Leak

```cpp
// Before (Meyer's Singleton)
class MySingleton {
public:
    static MySingleton& instance() {
        static MySingleton instance;
        return instance;
    }
private:
    MySingleton() = default;
    ~MySingleton() { /* cleanup */ }
};

// After (Intentional Leak)
class MySingleton {
public:
    static MySingleton& instance() {
        static MySingleton* instance = new MySingleton();
        return *instance;
    }

    static void shutdown() {
        // Move cleanup logic here
        // Called explicitly before main() returns
    }

private:
    MySingleton() = default;
    ~MySingleton() = default;  // Never called
};
```

### Adding Shutdown Support

```cpp
// In main.cpp or application entry point
int main() {
    // Application code...

    // Before returning, call shutdown in reverse dependency order
    MyDependentSingleton::shutdown();
    MySingleton::shutdown();

    return 0;
}
```

## Code Review Checklist

When reviewing singleton implementations:

1. **Pattern Selection**
   - [ ] Is the correct pattern used for the use case?
   - [ ] Is the pattern choice documented?

2. **Thread Safety**
   - [ ] Is the singleton thread-safe for construction?
   - [ ] Is the singleton thread-safe for access?

3. **Destruction Safety**
   - [ ] Does the destructor access other singletons?
   - [ ] Does the destructor perform logging?
   - [ ] Does the destructor involve thread operations?

4. **Documentation**
   - [ ] Is the pattern documented in the code?
   - [ ] Is the shutdown procedure documented (if applicable)?

## Related Issues

### thread_system
- #293: thread_logger Intentional Leak (CLOSED)
- #295: thread_pool SDOF prevention

### network_system
- #301: CI standardization (blocked by SDOF)
- #302: io_context_thread_manager safe destruction
- #304: io_context_thread_manager Intentional Leak

## References

- [C++ Core Guidelines I.3: Avoid singletons](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#i3-avoid-singletons)
- [Static Initialization Order Fiasco](https://isocpp.org/wiki/faq/ctors#static-init-order)
- [Intentional Leak Pattern](https://google.github.io/styleguide/cppguide.html#Static_and_Global_Variables) (Google C++ Style Guide)

## Summary Table

| Pattern | Thread-Safe | Memory | SDOF Safe | Use Case |
|---------|-------------|--------|-----------|----------|
| Meyer's Singleton | Yes (C++11) | Auto-freed | No | Pure data, no dependencies |
| Intentional Leak | Yes | Leaked | Yes | Infrastructure, cross-system |

## Version History

- **1.0.0** (2025-12-15): Initial release with common_system audit results
