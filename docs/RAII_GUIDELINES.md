# RAII Pattern Guidelines for Common System

> **Language:** **English** | [한국어](RAII_GUIDELINES_KO.md)


**Document Version**: 1.0
**Created**: 2025-10-08
**Target**: All systems using common_system
**Phase**: Phase 2 - Resource Management Standardization

---

## Table of Contents

1. [Overview](#overview)
2. [RAII Principles](#raii-principles)
3. [Resource Categories](#resource-categories)
4. [Smart Pointer Guidelines](#smart-pointer-guidelines)
5. [Implementation Patterns](#implementation-patterns)
6. [Common Pitfalls](#common-pitfalls)
7. [Integration with Result<T>](#integration-with-resultt)
8. [Examples](#examples)
9. [Checklist](#checklist)

---

## Overview

Resource Acquisition Is Initialization (RAII) is the fundamental C++ idiom for managing resource lifetimes. This document establishes guidelines for consistent RAII usage across all systems.

### Core RAII Principle

> **Resources are acquired in constructors and released in destructors automatically.**

**Benefits**:
- ✅ Exception safety guaranteed
- ✅ No manual cleanup required
- ✅ Clear ownership semantics
- ✅ Deterministic resource release
- ✅ Thread-safe resource management

---

## RAII Principles

### Principle 1: Acquire Resources in Constructors

```cpp
class file_writer {
    FILE* handle_;
public:
    explicit file_writer(const std::string& path) {
        handle_ = fopen(path.c_str(), "w");
        if (!handle_) {
            throw std::runtime_error("Failed to open file");
        }
    }

    ~file_writer() {
        if (handle_) {
            fclose(handle_);
        }
    }

    // Delete copy, enable move
    file_writer(const file_writer&) = delete;
    file_writer& operator=(const file_writer&) = delete;
    file_writer(file_writer&& other) noexcept
        : handle_(std::exchange(other.handle_, nullptr)) {}
    file_writer& operator=(file_writer&& other) noexcept {
        if (this != &other) {
            if (handle_) fclose(handle_);
            handle_ = std::exchange(other.handle_, nullptr);
        }
        return *this;
    }
};
```

### Principle 2: Release Resources in Destructors

**Destructors must**:
- Never throw exceptions (`noexcept` by default in C++11+)
- Be idempotent (safe to call multiple times)
- Handle all cleanup paths

```cpp
~resource_guard() noexcept {
    // Safe cleanup - never throws
    if (resource_) {
        try {
            cleanup(resource_);
        } catch (...) {
            // Log error but don't propagate
            // Destructors must not throw
        }
    }
}
```

### Principle 3: Delete Copy, Enable Move

**Rule of Five/Zero**:
- Define **all five** special member functions, or
- Define **none** (use `= default` or `= delete`)

```cpp
class resource_wrapper {
public:
    // Explicitly delete copy operations
    resource_wrapper(const resource_wrapper&) = delete;
    resource_wrapper& operator=(const resource_wrapper&) = delete;

    // Explicitly define move operations
    resource_wrapper(resource_wrapper&&) noexcept = default;
    resource_wrapper& operator=(resource_wrapper&&) noexcept = default;

    // Destructor
    ~resource_wrapper() = default;
};
```

---

## Resource Categories

### Category 1: System Resources

**Examples**: Files, sockets, mutexes, threads, timers

**Guidelines**:
- Always use RAII wrappers
- Never use naked handles
- Provide timeout for acquisition

```cpp
// File handles
class file_handle {
    FILE* handle_ = nullptr;
public:
    explicit file_handle(const char* path, const char* mode);
    ~file_handle() { if (handle_) fclose(handle_); }
    FILE* get() const { return handle_; }
};

// Socket handles
class socket_handle {
    int fd_ = -1;
public:
    explicit socket_handle(int domain, int type, int protocol);
    ~socket_handle() { if (fd_ >= 0) close(fd_); }
    int get() const { return fd_; }
};

// Mutex locks
// Use std::lock_guard, std::unique_lock, std::scoped_lock
```

### Category 2: Memory Resources

**Examples**: Heap allocations, memory pools, buffers

**Guidelines**:
- Prefer `std::unique_ptr` and `std::shared_ptr`
- Use custom deleters when needed
- Avoid naked `new`/`delete`

```cpp
// Unique ownership
auto buffer = std::make_unique<char[]>(size);

// Shared ownership
auto shared_buffer = std::make_shared<buffer_type>(args);

// Custom deleter
auto resource = std::unique_ptr<Resource, void(*)(Resource*)>(
    acquire_resource(),
    [](Resource* r) { release_resource(r); }
);
```

### Category 3: Logical Resources

**Examples**: Locks, transactions, scopes

**Guidelines**:
- Use scope guards for cleanup actions
- Implement RAII wrappers for logical resources

```cpp
// Transaction guard
class transaction_guard {
    database& db_;
    bool committed_ = false;
public:
    explicit transaction_guard(database& db) : db_(db) {
        db_.begin_transaction();
    }

    ~transaction_guard() {
        if (!committed_) {
            db_.rollback();
        }
    }

    void commit() {
        db_.commit();
        committed_ = true;
    }
};
```

---

## Smart Pointer Guidelines

### std::unique_ptr<T>

**Use when**: Exclusive ownership

```cpp
class logger {
    std::unique_ptr<writer> writer_;
public:
    explicit logger(std::unique_ptr<writer> w)
        : writer_(std::move(w)) {}
};

// Creation
auto logger = std::make_unique<file_logger>("app.log");
```

**Rules**:
- ✅ Use `std::make_unique` for construction
- ✅ Pass by `std::unique_ptr<T>` to transfer ownership
- ✅ Pass by `T*` or `T&` for non-owning access
- ❌ Never use `new` directly

### std::shared_ptr<T>

**Use when**: Shared ownership needed

```cpp
class session : public std::enable_shared_from_this<session> {
    void start_async_operation() {
        auto self = shared_from_this();
        async_op([self, this]() {
            // 'self' keeps session alive during callback
            process();
        });
    }
};
```

**Rules**:
- ✅ Use `std::make_shared` for construction
- ✅ Use `std::weak_ptr` to break cycles
- ✅ Inherit from `std::enable_shared_from_this` when needed
- ⚠️ Be cautious of circular references

### std::weak_ptr<T>

**Use when**: Non-owning reference to shared resource

```cpp
class connection_manager {
    std::vector<std::weak_ptr<connection>> connections_;

    void cleanup_dead_connections() {
        connections_.erase(
            std::remove_if(connections_.begin(), connections_.end(),
                [](const auto& weak) { return weak.expired(); }),
            connections_.end()
        );
    }
};
```

### Raw Pointers

**Use when**: Non-owning, guaranteed valid references

```cpp
// ✅ Non-owning parameter
void process(const logger* logger) {
    // logger guaranteed valid during function scope
}

// ✅ Optional non-owning parameter
void log_if_available(logger* logger) {
    if (logger) {
        logger->info("message");
    }
}

// ❌ Never for ownership
void bad_example(Thing* thing) {
    delete thing;  // Who owns this? Unclear!
}
```

---

## Implementation Patterns

### Pattern 1: Basic RAII Wrapper

```cpp
template<typename Resource, typename Deleter>
class resource_guard {
    Resource resource_;
    Deleter deleter_;
    bool valid_ = true;

public:
    explicit resource_guard(Resource r, Deleter d)
        : resource_(r), deleter_(d) {}

    ~resource_guard() {
        if (valid_) {
            deleter_(resource_);
        }
    }

    // Delete copy
    resource_guard(const resource_guard&) = delete;
    resource_guard& operator=(const resource_guard&) = delete;

    // Enable move
    resource_guard(resource_guard&& other) noexcept
        : resource_(std::exchange(other.resource_, Resource{}))
        , deleter_(std::move(other.deleter_))
        , valid_(std::exchange(other.valid_, false)) {}

    Resource get() const { return resource_; }
    void release() { valid_ = false; }
};
```

### Pattern 2: Scope Guard

```cpp
template<typename Func>
class scope_exit {
    Func cleanup_;
    bool dismissed_ = false;

public:
    explicit scope_exit(Func f) : cleanup_(std::move(f)) {}

    ~scope_exit() {
        if (!dismissed_) {
            cleanup_();
        }
    }

    void dismiss() { dismissed_ = true; }

    scope_exit(const scope_exit&) = delete;
    scope_exit& operator=(const scope_exit&) = delete;
};

// Helper
template<typename F>
auto make_scope_exit(F&& f) {
    return scope_exit<std::decay_t<F>>(std::forward<F>(f));
}

// Usage
void example() {
    auto guard = make_scope_exit([&] {
        cleanup_resources();
    });

    // Do work
    // Cleanup happens automatically
}
```

### Pattern 3: Connection Guard (from NEED_TO_FIX.md)

```cpp
class connection_guard {
    connection_pool* pool_;
    connection* conn_;

public:
    explicit connection_guard(connection_pool* pool)
        : pool_(pool), conn_(pool->acquire()) {
        if (!conn_) {
            throw std::runtime_error("Failed to acquire connection");
        }
    }

    ~connection_guard() {
        if (conn_) {
            pool_->release(conn_);
        }
    }

    connection* operator->() { return conn_; }
    const connection* operator->() const { return conn_; }

    connection_guard(const connection_guard&) = delete;
    connection_guard& operator=(const connection_guard&) = delete;
};
```

---

## Common Pitfalls

### Pitfall 1: Forgetting to Delete Copy Constructors

```cpp
// ❌ Bad - allows copying which could double-delete
class resource {
    void* handle_;
public:
    ~resource() { free(handle_); }
};

// ✅ Good - explicitly delete copy
class resource {
    void* handle_;
public:
    ~resource() { free(handle_); }
    resource(const resource&) = delete;
    resource& operator=(const resource&) = delete;
};
```

### Pitfall 2: Throwing in Destructors

```cpp
// ❌ Bad - destructor can throw
~bad_resource() {
    cleanup();  // May throw!
}

// ✅ Good - destructor never throws
~good_resource() noexcept {
    try {
        cleanup();
    } catch (...) {
        // Log but don't propagate
    }
}
```

### Pitfall 3: Circular shared_ptr References

```cpp
// ❌ Bad - circular reference causes leak
class Node {
    std::shared_ptr<Node> parent_;  // Circular!
    std::shared_ptr<Node> child_;
};

// ✅ Good - use weak_ptr to break cycle
class Node {
    std::weak_ptr<Node> parent_;     // Non-owning
    std::shared_ptr<Node> child_;    // Owning
};
```

### Pitfall 4: Naked new/delete

```cpp
// ❌ Bad - manual memory management
Widget* w = new Widget();
// ... use w ...
delete w;  // Easy to forget!

// ✅ Good - automatic cleanup
auto w = std::make_unique<Widget>();
// ... use w ...
// Automatic cleanup
```

---

## Integration with Result<T>

RAII works seamlessly with `common::Result<T>` for exception-free error handling.

### Pattern: RAII Factory Functions

```cpp
Result<std::unique_ptr<file_writer>> create_file_writer(const std::string& path) {
    FILE* handle = fopen(path.c_str(), "w");
    if (!handle) {
        return error_info{errno, std::strerror(errno), "file_writer"};
    }

    return std::make_unique<file_writer>(handle);  // RAII wrapper
}

// Usage
auto result = create_file_writer("output.txt");
if (is_error(result)) {
    // Handle error
    return;
}

auto writer = std::move(std::get<std::unique_ptr<file_writer>>(result));
// Use writer - automatic cleanup on scope exit
```

### Pattern: RAII with Error Propagation

```cpp
Result<void> process_file(const std::string& path) {
    // Acquire resource with RAII
    auto file_result = create_file_writer(path);
    if (is_error(file_result)) {
        return std::get<error_info>(file_result);
    }

    auto writer = std::move(std::get<std::unique_ptr<file_writer>>(file_result));

    // Use resource
    auto write_result = writer->write("data");
    if (is_error(write_result)) {
        return std::get<error_info>(write_result);
    }

    return std::monostate{};
    // writer automatically cleaned up here
}
```

---

## Examples

### Example 1: File Writer with RAII

```cpp
class file_writer {
    FILE* handle_ = nullptr;

public:
    static Result<std::unique_ptr<file_writer>> create(const std::string& path) {
        FILE* handle = fopen(path.c_str(), "w");
        if (!handle) {
            return error_info{errno, std::strerror(errno), "file_writer::create"};
        }

        return std::unique_ptr<file_writer>(new file_writer(handle));
    }

    ~file_writer() {
        if (handle_) {
            fclose(handle_);
        }
    }

    Result<void> write(const std::string& data) {
        size_t written = fwrite(data.data(), 1, data.size(), handle_);
        if (written != data.size()) {
            return error_info{EIO, "Write failed", "file_writer::write"};
        }
        return std::monostate{};
    }

private:
    explicit file_writer(FILE* handle) : handle_(handle) {}

    file_writer(const file_writer&) = delete;
    file_writer& operator=(const file_writer&) = delete;
};
```

### Example 2: Connection Pool with RAII

```cpp
class connection_pool {
    std::vector<std::unique_ptr<connection>> connections_;
    std::queue<connection*> available_;
    std::mutex mutex_;

public:
    class connection_guard {
        connection_pool* pool_;
        connection* conn_;

    public:
        connection_guard(connection_pool* pool, connection* conn)
            : pool_(pool), conn_(conn) {}

        ~connection_guard() {
            if (conn_) {
                pool_->release(conn_);
            }
        }

        connection* operator->() { return conn_; }

        connection_guard(const connection_guard&) = delete;
        connection_guard& operator=(const connection_guard&) = delete;

        connection_guard(connection_guard&& other) noexcept
            : pool_(other.pool_)
            , conn_(std::exchange(other.conn_, nullptr)) {}
    };

    Result<connection_guard> acquire() {
        std::lock_guard<std::mutex> lock(mutex_);

        if (available_.empty()) {
            return error_info{EAGAIN, "Pool exhausted", "connection_pool::acquire"};
        }

        auto* conn = available_.front();
        available_.pop();

        return connection_guard{this, conn};
    }

private:
    void release(connection* conn) {
        std::lock_guard<std::mutex> lock(mutex_);
        available_.push(conn);
    }
};
```

---

## Checklist

Use this checklist when implementing RAII:

### Design Phase
- [ ] Identify all resources that need management
- [ ] Determine ownership model (unique vs shared)
- [ ] Design exception-safe constructors
- [ ] Plan error handling strategy

### Implementation Phase
- [ ] Resources acquired in constructor
- [ ] Resources released in destructor
- [ ] Destructor is `noexcept`
- [ ] Copy constructor deleted (if not copyable)
- [ ] Copy assignment deleted (if not copyable)
- [ ] Move constructor implemented (if movable)
- [ ] Move assignment implemented (if movable)
- [ ] Smart pointers used for heap allocations
- [ ] No naked `new` or `delete`

### Integration Phase
- [ ] Factory functions return `Result<std::unique_ptr<T>>`
- [ ] Error paths properly handled
- [ ] Resource ownership documented
- [ ] Thread safety documented

### Testing Phase
- [ ] Exception safety tested
- [ ] Resource leak tested (AddressSanitizer)
- [ ] Double-delete prevented
- [ ] Move semantics tested
- [ ] Concurrent access tested (if applicable)

---

## References

- [C++ Core Guidelines: Resource Management](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#r-resource-management)
- [NEED_TO_FIX.md Phase 2: Resource Management](../../NEED_TO_FIX.md)
- [common_system Result<T> Documentation](./ERRORS.md)

---

**Document Status**: Phase 2 Baseline
**Next Review**: After Phase 2 completion
**Maintainer**: Architecture Team

---

*Last Updated: 2025-10-20*
