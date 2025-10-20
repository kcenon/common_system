# Smart Pointer Usage Guidelines

> **Language:** **English** | [한국어](SMART_POINTER_GUIDELINES_KO.md)


## Table of Contents

- [Decision Tree: Which Smart Pointer to Use?](#decision-tree-which-smart-pointer-to-use)
- [std::unique_ptr<T> - Exclusive Ownership](#stdunique_ptrt-exclusive-ownership)
  - [When to Use](#when-to-use)
  - [Basic Usage](#basic-usage)
  - [Function Parameters](#function-parameters)
  - [Return Values](#return-values)
  - [Custom Deleters](#custom-deleters)
  - [Common Patterns](#common-patterns)
- [std::shared_ptr<T> - Shared Ownership](#stdshared_ptrt-shared-ownership)
  - [When to Use](#when-to-use)
  - [Basic Usage](#basic-usage)
  - [std::enable_shared_from_this](#stdenable_shared_from_this)
  - [Reference Counting](#reference-counting)
  - [Circular References (Danger!)](#circular-references-danger)
  - [Thread Safety](#thread-safety)
- [std::weak_ptr<T> - Non-Owning Reference](#stdweak_ptrt-non-owning-reference)
  - [When to Use](#when-to-use)
  - [Basic Usage](#basic-usage)
  - [Observer Pattern](#observer-pattern)
  - [Cache Pattern](#cache-pattern)
- [Raw Pointers - Non-Owning Access](#raw-pointers-non-owning-access)
  - [When to Use Raw Pointers](#when-to-use-raw-pointers)
  - [Examples](#examples)
- [Ownership Documentation](#ownership-documentation)
- [Performance Considerations](#performance-considerations)
  - [unique_ptr Performance](#unique_ptr-performance)
  - [shared_ptr Performance](#shared_ptr-performance)
  - [Optimization Tips](#optimization-tips)
- [Migration Checklist](#migration-checklist)
  - [Phase 1: Identify](#phase-1-identify)
  - [Phase 2: Convert](#phase-2-convert)
  - [Phase 3: Verify](#phase-3-verify)
  - [Phase 4: Document](#phase-4-document)
- [Examples by System](#examples-by-system)
  - [logger_system](#logger_system)
  - [network_system](#network_system)
  - [database_system](#database_system)
- [Quick Reference](#quick-reference)
- [References](#references)

**Document Version**: 1.0
**Created**: 2025-10-08
**Related**: [RAII_GUIDELINES.md](./RAII_GUIDELINES.md)
**Phase**: Phase 2 - Resource Management Standardization

---

## Decision Tree: Which Smart Pointer to Use?

```
┌─────────────────────────────────┐
│ Do you need to manage a resource?│
└────────────┬────────────────────┘
             │
             ├─ No → Use reference (&) or pointer (*) for non-owning access
             │
             └─ Yes
                │
                ├─ Is ownership exclusive?
                │  │
                │  └─ Yes → std::unique_ptr<T>
                │
                ├─ Is ownership shared?
                │  │
                │  └─ Yes → std::shared_ptr<T>
                │
                └─ Need non-owning reference to shared resource?
                   │
                   └─ Yes → std::weak_ptr<T>
```

---

## std::unique_ptr<T> - Exclusive Ownership

### When to Use

✅ **Use std::unique_ptr when**:
- Single owner of a resource
- Ownership transfer is needed
- Resource must be explicitly managed
- You would otherwise use `new`/`delete`

❌ **Don't use when**:
- Multiple owners needed
- Resource is stack-allocated
- Non-owning access is sufficient

### Basic Usage

```cpp
// ✅ Creation with make_unique (C++14+)
auto widget = std::make_unique<Widget>(arg1, arg2);

// ✅ Explicit creation (when custom deleter needed)
auto file = std::unique_ptr<FILE, decltype(&fclose)>(
    fopen("file.txt", "r"),
    &fclose
);

// ✅ Ownership transfer
std::unique_ptr<Widget> transfer_ownership() {
    auto widget = std::make_unique<Widget>();
    return widget;  // Automatic move
}

// ✅ Array allocation
auto array = std::make_unique<int[]>(size);
array[0] = 42;

// ❌ Never use new directly
Widget* bad = new Widget();  // Don't do this!
```

### Function Parameters

```cpp
// ✅ Taking ownership
void take_ownership(std::unique_ptr<Widget> widget) {
    // Function owns widget now
}

// ✅ Transferring ownership (caller loses ownership)
auto widget = std::make_unique<Widget>();
take_ownership(std::move(widget));
// widget is now nullptr

// ✅ Non-owning access
void use_widget(Widget* widget) {
    if (widget) {
        widget->do_something();
    }
}

// ✅ Non-owning access (guaranteed non-null)
void use_widget_ref(Widget& widget) {
    widget.do_something();
}

// Usage
auto widget = std::make_unique<Widget>();
use_widget(widget.get());           // Non-owning pointer
use_widget_ref(*widget);            // Non-owning reference
```

### Return Values

```cpp
// ✅ Factory function
std::unique_ptr<Connection> create_connection(const std::string& host) {
    auto conn = std::make_unique<Connection>();
    if (!conn->connect(host)) {
        return nullptr;  // Indicate failure
    }
    return conn;
}

// ✅ With Result<T> for better error handling
Result<std::unique_ptr<Connection>> create_connection_safe(const std::string& host) {
    auto conn = std::make_unique<Connection>();
    auto result = conn->connect(host);
    if (is_error(result)) {
        return std::get<error_info>(result);
    }
    return std::move(conn);
}
```

### Custom Deleters

```cpp
// Function pointer deleter
auto file = std::unique_ptr<FILE, decltype(&fclose)>(
    fopen("data.txt", "r"),
    &fclose
);

// Lambda deleter
auto resource = std::unique_ptr<Resource, void(*)(Resource*)>(
    acquire_resource(),
    [](Resource* r) {
        cleanup_resource(r);
        delete r;
    }
);

// Stateful deleter
struct socket_deleter {
    int timeout_ms;

    void operator()(Socket* sock) const {
        sock->shutdown(timeout_ms);
        delete sock;
    }
};

auto socket = std::unique_ptr<Socket, socket_deleter>(
    new Socket(),
    socket_deleter{5000}  // 5 second timeout
);
```

### Common Patterns

```cpp
// Pattern 1: Pimpl idiom
class Widget {
    class Impl;
    std::unique_ptr<Impl> pimpl_;

public:
    Widget();
    ~Widget();  // Must be defined in .cpp where Impl is complete
};

// Pattern 2: Polymorphic factory
std::unique_ptr<ILogger> create_logger(LoggerType type) {
    switch (type) {
        case LoggerType::File:
            return std::make_unique<FileLogger>();
        case LoggerType::Network:
            return std::make_unique<NetworkLogger>();
        default:
            return nullptr;
    }
}

// Pattern 3: Resource guard
template<typename Resource>
class ResourceGuard {
    std::unique_ptr<Resource> resource_;

public:
    explicit ResourceGuard(std::unique_ptr<Resource> r)
        : resource_(std::move(r)) {}

    Resource* get() { return resource_.get(); }
};
```

---

## std::shared_ptr<T> - Shared Ownership

### When to Use

✅ **Use std::shared_ptr when**:
- Multiple owners of a resource
- Ownership lifetime is complex
- Resource must outlive any single owner
- Async operations need to keep resource alive

❌ **Don't use when**:
- Single owner is sufficient (use `unique_ptr`)
- Ownership is clear and simple
- Performance is critical (shared_ptr has overhead)

### Basic Usage

```cpp
// ✅ Creation with make_shared (preferred)
auto widget = std::make_shared<Widget>(arg1, arg2);

// ✅ Explicit creation (when custom deleter needed)
auto file = std::shared_ptr<FILE>(
    fopen("file.txt", "r"),
    &fclose
);

// ✅ Copying shares ownership
auto widget1 = std::make_shared<Widget>();
auto widget2 = widget1;  // Both own the widget
// Widget deleted when both widget1 and widget2 are destroyed

// ✅ Convert from unique_ptr
std::unique_ptr<Widget> unique_widget = std::make_unique<Widget>();
std::shared_ptr<Widget> shared_widget = std::move(unique_widget);
```

### std::enable_shared_from_this

```cpp
class Session : public std::enable_shared_from_this<Session> {
public:
    void start_async_read() {
        // Capture shared_ptr to self to keep session alive
        auto self = shared_from_this();

        socket_.async_read([self, this](const auto& data) {
            // 'self' keeps 'this' valid during callback
            handle_data(data);
        });
    }

    void handle_data(const std::vector<char>& data) {
        // Process data
    }

private:
    Socket socket_;
};

// ❌ Wrong - creates dangling pointer
class BadSession {
    void start_async_read() {
        socket_.async_read([this](const auto& data) {
            // 'this' might be deleted before callback!
            handle_data(data);
        });
    }
};
```

### Reference Counting

```cpp
auto widget = std::make_shared<Widget>();
std::cout << widget.use_count() << "\n";  // 1

{
    auto widget2 = widget;  // Increment ref count
    std::cout << widget.use_count() << "\n";  // 2
}  // widget2 destroyed, ref count decremented

std::cout << widget.use_count() << "\n";  // 1
// Widget destroyed when widget goes out of scope
```

### Circular References (Danger!)

```cpp
// ❌ Bad - Circular reference causes memory leak
class Node {
    std::shared_ptr<Node> parent_;  // Circular!
    std::shared_ptr<Node> child_;

public:
    void set_parent(std::shared_ptr<Node> parent) {
        parent_ = parent;
    }

    void set_child(std::shared_ptr<Node> child) {
        child_ = child;
    }
};

// Creates circular reference - memory leak!
auto parent = std::make_shared<Node>();
auto child = std::make_shared<Node>();
parent->set_child(child);
child->set_parent(parent);  // Circular reference!
// Neither parent nor child will ever be deleted

// ✅ Good - Use weak_ptr to break cycle
class Node {
    std::weak_ptr<Node> parent_;    // Non-owning
    std::shared_ptr<Node> child_;   // Owning

public:
    void set_parent(std::shared_ptr<Node> parent) {
        parent_ = parent;  // Stores weak reference
    }

    void set_child(std::shared_ptr<Node> child) {
        child_ = child;
    }

    std::shared_ptr<Node> get_parent() {
        return parent_.lock();  // Convert weak to shared
    }
};
```

### Thread Safety

```cpp
// ✅ Reference counting is thread-safe
std::shared_ptr<Widget> global_widget;

// Thread 1
global_widget = std::make_shared<Widget>();  // Thread-safe

// Thread 2
auto local_copy = global_widget;  // Thread-safe copy

// ⚠️ But the pointed-to object is NOT automatically thread-safe
class Counter {
    int count_ = 0;  // NOT thread-safe!

public:
    void increment() {
        ++count_;  // Race condition if called from multiple threads
    }
};

auto counter = std::make_shared<Counter>();
// Multiple threads calling counter->increment() is unsafe!

// ✅ Need to protect the object itself
class ThreadSafeCounter {
    int count_ = 0;
    std::mutex mutex_;

public:
    void increment() {
        std::lock_guard<std::mutex> lock(mutex_);
        ++count_;
    }
};
```

---

## std::weak_ptr<T> - Non-Owning Reference

### When to Use

✅ **Use std::weak_ptr when**:
- Breaking circular references
- Caching (where cache entries can expire)
- Observer pattern
- Need to check if shared resource still exists

### Basic Usage

```cpp
std::shared_ptr<Widget> shared = std::make_shared<Widget>();
std::weak_ptr<Widget> weak = shared;  // Non-owning reference

// Check if resource still exists
if (auto locked = weak.lock()) {
    // locked is a shared_ptr<Widget>
    locked->do_something();
}  // locked goes out of scope, ref count decremented

// Check if expired
if (weak.expired()) {
    std::cout << "Resource was deleted\n";
}
```

### Observer Pattern

```cpp
class Subject {
    std::vector<std::weak_ptr<Observer>> observers_;

public:
    void attach(std::shared_ptr<Observer> observer) {
        observers_.push_back(observer);
    }

    void notify() {
        // Remove expired observers
        observers_.erase(
            std::remove_if(observers_.begin(), observers_.end(),
                [](const auto& weak) { return weak.expired(); }),
            observers_.end()
        );

        // Notify remaining observers
        for (const auto& weak : observers_) {
            if (auto observer = weak.lock()) {
                observer->on_notify();
            }
        }
    }
};
```

### Cache Pattern

```cpp
class ResourceCache {
    std::unordered_map<std::string, std::weak_ptr<Resource>> cache_;
    std::mutex mutex_;

public:
    std::shared_ptr<Resource> get_or_create(const std::string& key) {
        std::lock_guard<std::mutex> lock(mutex_);

        // Try to get from cache
        auto it = cache_.find(key);
        if (it != cache_.end()) {
            if (auto resource = it->second.lock()) {
                return resource;  // Cache hit
            }
        }

        // Cache miss or expired - create new
        auto resource = std::make_shared<Resource>(key);
        cache_[key] = resource;
        return resource;
    }

    void cleanup_expired() {
        std::lock_guard<std::mutex> lock(mutex_);

        for (auto it = cache_.begin(); it != cache_.end();) {
            if (it->second.expired()) {
                it = cache_.erase(it);
            } else {
                ++it;
            }
        }
    }
};
```

---

## Raw Pointers - Non-Owning Access

### When to Use Raw Pointers

✅ **Use raw pointers (*) when**:
- Non-owning, optional access
- Null is a valid value
- C API interoperability

✅ **Use references (&) when**:
- Non-owning, guaranteed valid
- Parameter must not be null
- More semantic than pointers

❌ **Never use raw pointers for**:
- Ownership
- Dynamic memory management
- Transferring ownership

### Examples

```cpp
// ✅ Non-owning optional parameter
void log_if_available(Logger* logger, const std::string& message) {
    if (logger) {
        logger->log(message);
    }
}

// ✅ Non-owning required parameter
void log_always(Logger& logger, const std::string& message) {
    logger.log(message);  // Logger guaranteed valid
}

// ✅ Accessing owned resource
auto widget = std::make_unique<Widget>();
Widget* raw = widget.get();  // Non-owning access
use_widget(raw);  // Safe within widget's lifetime

// ❌ Never transfer ownership with raw pointers
Widget* create_widget() {
    return new Widget();  // Who deletes this? Unclear!
}

// ✅ Use smart pointers for ownership transfer
std::unique_ptr<Widget> create_widget_safe() {
    return std::make_unique<Widget>();
}
```

---

## Ownership Documentation

Document ownership in function signatures and comments:

```cpp
/**
 * @brief Create a new connection
 * @return Owned connection object. Caller is responsible for lifetime.
 */
std::unique_ptr<Connection> create_connection();

/**
 * @brief Process data using logger
 * @param logger Non-owning pointer to logger. May be null.
 */
void process_data(Logger* logger);

/**
 * @brief Register observer
 * @param observer Shared ownership. Object kept alive while registered.
 */
void register_observer(std::shared_ptr<Observer> observer);
```

---

## Performance Considerations

### unique_ptr Performance

- **Size**: Same as raw pointer (zero overhead)
- **Move**: O(1), cheap
- **Copy**: Deleted (compile-time error)
- **Overhead**: Destructor call only

### shared_ptr Performance

- **Size**: 2x pointer (ptr + control block)
- **Move**: O(1), cheap
- **Copy**: O(1), atomic increment
- **Overhead**: Atomic operations for ref count

```cpp
// Benchmarking results (approximate)
// Creation:
//   make_unique: ~10ns
//   make_shared: ~50ns
// Copy:
//   unique_ptr: N/A (deleted)
//   shared_ptr: ~5ns (atomic increment)
// Destruction:
//   unique_ptr: ~5ns
//   shared_ptr: ~10ns (atomic decrement + check)
```

### Optimization Tips

```cpp
// ✅ Pass by const reference when not transferring ownership
void process(const std::shared_ptr<Widget>& widget) {
    widget->do_something();  // No ref count change
}

// ❌ Pass by value unnecessarily
void process_bad(std::shared_ptr<Widget> widget) {
    widget->do_something();  // Unnecessary ref count increment/decrement
}

// ✅ Use make_unique/make_shared
auto widget = std::make_shared<Widget>();  // Single allocation

// ❌ Avoid separate allocation
auto widget = std::shared_ptr<Widget>(new Widget());  // Two allocations
```

---

## Migration Checklist

When converting code to use smart pointers:

### Phase 1: Identify
- [ ] Find all uses of `new` and `delete`
- [ ] Identify ownership semantics
- [ ] Document current lifetime assumptions

### Phase 2: Convert
- [ ] Replace `new` with `make_unique` or `make_shared`
- [ ] Remove manual `delete` calls
- [ ] Update function signatures
- [ ] Add move semantics where needed

### Phase 3: Verify
- [ ] Run with AddressSanitizer
- [ ] Run with ThreadSanitizer
- [ ] Verify no double-deletes
- [ ] Check for circular references
- [ ] Performance test (if critical path)

### Phase 4: Document
- [ ] Add ownership comments
- [ ] Update API documentation
- [ ] Note any shared ownership

---

## Examples by System

### logger_system

```cpp
class file_logger {
    std::unique_ptr<file_handle> file_;  // Exclusive ownership

public:
    static Result<std::unique_ptr<file_logger>> create(const std::string& path) {
        auto file = file_handle::open(path);
        if (is_error(file)) {
            return std::get<error_info>(file);
        }

        return std::unique_ptr<file_logger>(
            new file_logger(std::move(std::get<std::unique_ptr<file_handle>>(file)))
        );
    }
};
```

### network_system

```cpp
class session : public std::enable_shared_from_this<session> {
    void start() {
        auto self = shared_from_this();
        socket_.async_read([self, this](auto data) {
            handle_data(data);
        });
    }
};
```

### database_system

```cpp
class connection_pool {
    std::vector<std::unique_ptr<connection>> connections_;  // Own connections

    class connection_guard {
        connection_pool* pool_;
        connection* conn_;  // Non-owning

    public:
        ~connection_guard() {
            pool_->release(conn_);
        }
    };
};
```

---

## Quick Reference

| Scenario | Smart Pointer |
|----------|---------------|
| Single owner | `std::unique_ptr<T>` |
| Multiple owners | `std::shared_ptr<T>` |
| Break circular ref | `std::weak_ptr<T>` |
| Optional access | `T*` (raw pointer) |
| Required access | `T&` (reference) |
| Transfer ownership | `std::unique_ptr<T>` (move) |
| Share ownership | `std::shared_ptr<T>` (copy) |
| Factory function | `std::unique_ptr<T>` |
| Async callback | `std::shared_ptr<T>` |
| Cache/observer | `std::weak_ptr<T>` |

---

## References

- [C++ Core Guidelines: Smart Pointers](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Rr-smartpointer)
- [RAII Guidelines](./RAII_GUIDELINES.md)
- [NEED_TO_FIX.md Phase 2](../../NEED_TO_FIX.md)

---

**Document Status**: Phase 2 Baseline
**Next Review**: After Phase 2 completion
**Maintainer**: Architecture Team

---

*Last Updated: 2025-10-20*
