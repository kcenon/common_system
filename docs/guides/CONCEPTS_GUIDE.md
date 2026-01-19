# C++20 Concepts Guide

**Language:** **English** | [한국어](CONCEPTS_GUIDE.kr.md)

This guide explains how to use C++20 concepts in the Common System library for compile-time type validation.

---

## Table of Contents

- [Overview](#overview)
- [Benefits](#benefits)
- [Quick Start](#quick-start)
- [Available Concepts](#available-concepts)
- [Usage Examples](#usage-examples)
- [Migration from SFINAE](#migration-from-sfinae)
- [Error Messages](#error-messages)

---

## Overview

C++20 concepts provide a way to constrain template parameters at compile time. The Common System library provides comprehensive concept definitions that:

- Validate types at compile time with clear error messages
- Express type requirements explicitly as documentation
- Replace verbose SFINAE-based constraints
- Improve IDE support for auto-completion

### Requirements

- C++20 compiler with concepts support
- GCC 10+, Clang 10+, MSVC 2022+

---

## Benefits

### Before (SFINAE)

```cpp
template<typename F,
         typename = std::enable_if_t<
             std::is_invocable_v<F> &&
             std::is_void_v<std::invoke_result_t<F>>>>
void execute_async(F&& func);
```

**Error message:**
```
error: no matching function for call to 'execute_async'
note: candidate template ignored: substitution failure [with F = int]:
      no type named 'type' in 'std::enable_if<false>'
```

### After (Concepts)

```cpp
template<concepts::VoidCallable F>
void execute_async(F&& func);
```

**Error message:**
```
error: constraints not satisfied for 'execute_async' [with F = int]
note: because 'int' does not satisfy 'VoidCallable'
note: because 'std::invocable<int>' evaluated to false
```

---

## Quick Start

Include the unified concepts header:

```cpp
#include <kcenon/common/concepts/concepts.h>

using namespace kcenon::common::concepts;

// Use concepts in template constraints
template<Resultable R>
void process(const R& result) {
    if (result.is_ok()) {
        // Handle success
    }
}

template<EventHandler<MyEvent> H>
uint64_t subscribe(H&& handler) {
    return bus.subscribe<MyEvent>(std::forward<H>(handler));
}
```

---

## Available Concepts

### Core Concepts (core.h)

| Concept | Description |
|---------|-------------|
| `Resultable` | Types with `is_ok()` and `is_err()` methods |
| `Unwrappable` | Types supporting value extraction (`unwrap()`, `unwrap_or()`) |
| `Mappable` | Types supporting `map()` transformation |
| `Chainable` | Types supporting `and_then()` chaining |
| `MonadicResult` | Complete Result-like types with all monadic operations |
| `OptionalLike` | Optional value containers (`has_value()`, `is_some()`, `is_none()`) |
| `ErrorInfo` | Error information types with `code`, `message`, `module` |
| `ValueOrError` | Types holding either value or error |

### Callable Concepts (callable.h)

| Concept | Description |
|---------|-------------|
| `Invocable<F, Args...>` | Callable types |
| `VoidCallable<F, Args...>` | Callables returning void |
| `ReturnsResult<F, R, Args...>` | Callables returning specific type |
| `NoexceptCallable<F, Args...>` | Noexcept callables |
| `Predicate<F, Args...>` | Boolean-returning callables |
| `UnaryFunction<F, Arg>` | Single-argument callables |
| `BinaryFunction<F, Arg1, Arg2>` | Two-argument callables |
| `JobLike` | Types satisfying IJob interface |
| `ExecutorLike` | Types satisfying IExecutor interface |
| `TaskFactory<F, T>` | Callables creating tasks |
| `DelayedCallable<F>` | Callables for delayed execution |

### Event Concepts (event.h)

| Concept | Description |
|---------|-------------|
| `EventType` | Valid event types (class, copy-constructible) |
| `EventHandler<H, E>` | Event handler callables |
| `EventFilter<F, E>` | Event filter predicates |
| `TimestampedEvent` | Events with timestamp |
| `NamedEvent` | Events with module name |
| `ErrorEvent` | Error events with message and code |
| `MetricEvent` | Metric events with name, value, unit |
| `ModuleLifecycleEvent` | Module lifecycle events |
| `FullErrorEvent` | Complete error events |
| `FullMetricEvent` | Complete metric events |
| `EventBusLike` | Event bus interface types |

### Service Concepts (service.h)

| Concept | Description |
|---------|-------------|
| `ServiceInterface` | Valid service interfaces (polymorphic, virtual destructor) |
| `ServiceImplementation<TImpl, TIface>` | Service implementations |
| `ServiceFactory<F, T>` | Service factory callables (with container) |
| `SimpleServiceFactory<F, T>` | Simple factory callables (no container) |
| `ServiceContainerLike` | Service container types |
| `ServiceScopeLike` | Service scope types |
| `InjectableService` | Auto-injectable services |
| `SharedService` | Types shareable via shared_ptr |
| `ConfigSection` | Configuration section types |
| `Validatable` | Self-validating types |
| `InitializableService` | Services requiring initialization |
| `DisposableService` | Services requiring cleanup |

### Container Concepts (container.h)

| Concept | Description |
|---------|-------------|
| `Container` | Basic container requirements |
| `SequenceContainer` | Sequential containers (push_back, front, back) |
| `AssociativeContainer` | Key-based containers (find, count) |
| `MappingContainer` | Key-value containers |
| `ResizableContainer` | Resizable containers (resize, reserve, capacity) |
| `ClearableContainer` | Clearable containers |
| `InsertableContainer` | Containers supporting insert |
| `ErasableContainer` | Containers supporting erase |
| `RandomAccessContainer` | Random access containers (operator[]) |
| `BoundedContainer` | Fixed capacity containers |
| `ThreadSafeContainer` | Thread-safe containers |
| `PoolableContainer` | Object pool containers |
| `CircularBuffer` | Circular buffer types |

---

## Usage Examples

### Result Processing

```cpp
#include <kcenon/common/concepts/concepts.h>

template<concepts::MonadicResult R>
auto process_chain(const R& result) {
    return result
        .map([](auto& v) { return v * 2; })
        .and_then([](auto v) { return R::ok(v + 1); });
}
```

### Event Bus Subscription

```cpp
#include <kcenon/common/patterns/event_bus.h>

struct MyEvent {
    std::string data;
};

// Subscribe with concept-constrained handler
auto& bus = kcenon::common::get_event_bus();
auto id = bus.subscribe<MyEvent>([](const MyEvent& evt) {
    std::cout << "Received: " << evt.data << std::endl;
});
```

### Service Registration

```cpp
#include <kcenon/common/di/service_container.h>

class IMyService {
public:
    virtual ~IMyService() = default;
    virtual void do_work() = 0;
};

class MyServiceImpl : public IMyService {
public:
    void do_work() override { /* ... */ }
};

// Register with concept-constrained templates
auto& container = kcenon::common::di::service_container::global();
container.register_type<IMyService, MyServiceImpl>();
```

### Custom Concept Usage

```cpp
#include <kcenon/common/concepts/concepts.h>

// Create a function that only accepts Validatable types
template<concepts::Validatable T>
bool is_valid(const T& obj) {
    auto result = obj.validate();
    return result.is_ok();
}

// Create a function for EventHandler types
template<concepts::EventType E, concepts::EventHandler<E> H>
void subscribe_and_log(H&& handler) {
    std::cout << "Subscribing handler..." << std::endl;
    bus.subscribe<E>(std::forward<H>(handler));
}
```

---

## Migration from SFINAE

### Step 1: Include concepts header

```cpp
#include <kcenon/common/concepts/concepts.h>
```

### Step 2: Replace enable_if with concepts

**Before:**
```cpp
template<typename F,
         typename = std::enable_if_t<std::is_invocable_v<F>>>
void execute(F&& func);
```

**After:**
```cpp
template<concepts::Invocable F>
void execute(F&& func);
```

### Step 3: Replace static_assert with concepts

**Before:**
```cpp
template<typename T>
void process(T& container) {
    static_assert(has_begin_v<T>, "T must have begin()");
    static_assert(has_end_v<T>, "T must have end()");
    // ...
}
```

**After:**
```cpp
template<concepts::Container T>
void process(T& container) {
    // ...
}
```

---

## Error Messages

### Concept Violation Examples

**Invalid Event Type:**
```cpp
bus.subscribe<int>([](int) {});  // Error!
// error: constraints not satisfied for 'subscribe'
// note: because 'int' does not satisfy 'EventType'
// note: because 'std::is_class_v<int>' evaluated to false
```

**Invalid Service Interface:**
```cpp
class NonPolymorphic {};  // No virtual methods
container.register_type<NonPolymorphic, Impl>();  // Error!
// error: constraints not satisfied for 'register_type'
// note: because 'NonPolymorphic' does not satisfy 'ServiceInterface'
// note: because 'std::is_polymorphic_v<NonPolymorphic>' evaluated to false
```

**Invalid Event Handler:**
```cpp
bus.subscribe<MyEvent>([](MyEvent&) { return 42; });  // Error!
// error: constraints not satisfied for 'subscribe'
// note: because lambda does not satisfy 'EventHandler'
// note: because return type is not void
```

---

## Best Practices

1. **Include the unified header** - Use `concepts/concepts.h` for convenience
2. **Use namespace alias** - `namespace concepts = kcenon::common::concepts;`
3. **Prefer concepts over SFINAE** - Concepts provide clearer error messages
4. **Document custom concepts** - Add `@concept` Doxygen tags
5. **Test concept violations** - Ensure invalid types produce helpful errors

---

## Related Documentation

- [API Reference](../API_REFERENCE.md)
- [Result Pattern Guide](result_pattern.md)
- [Event Bus Guide](event_bus.md)
- [Dependency Injection Guide](dependency_injection.md)
