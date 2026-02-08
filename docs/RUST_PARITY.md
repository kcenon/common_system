# Rust/C++ Feature Parity Matrix

> **Language:** **English** | [한국어](RUST_PARITY.kr.md)

**Part 1 of 3**: Overall Parity Status and API Mapping

**Status**: ✅ **Part 1 Complete** | Part 2 Planned | Part 3 Planned

This document provides a comprehensive comparison between the C++ implementations and Rust ports of the kcenon ecosystem systems. It helps developers choose the right language for their projects and understand migration paths.

---

## Table of Contents

- [Overview](#overview)
- [Overall Parity Status](#1-overall-parity-status)
  - [Methodology](#methodology)
  - [System Comparison](#system-comparison)
  - [Interpretation Guide](#interpretation-guide)
- [API Mapping Guide](#3-api-mapping-guide)
  - [Naming Conventions](#naming-conventions)
  - [Type Mapping](#type-mapping)
  - [Error Handling Differences](#error-handling-differences)
  - [Memory Management](#memory-management)
  - [Async/Await Patterns](#asyncawait-patterns)
- [Next Steps](#next-steps)

---

## Overview

The kcenon ecosystem consists of **7 core C++ systems** with parallel **Rust ports** for several of them. This parity matrix documents:
- Which features are available in each language
- Implementation status and stability
- API differences and migration patterns
- Interoperability considerations

### Why Rust Ports?

Rust ports provide:
- **Memory safety**: No data races, no null pointer dereferences
- **Performance**: Zero-cost abstractions with RAII-like semantics
- **Modern tooling**: Cargo for dependency management and testing
- **Async-first**: Native async/await with tokio ecosystem
- **Cross-platform**: Excellent support for Windows, Linux, macOS, WASM

### When to Use C++ vs Rust

| Factor | Prefer C++ | Prefer Rust |
|--------|-----------|-------------|
| **Existing codebase** | Large C++ codebase | Greenfield project |
| **Team expertise** | C++ developers | Rust developers |
| **Feature completeness** | Need advanced features | Core features sufficient |
| **Interop with C/C++** | Tight integration needed | Standalone service |
| **Compile time** | Critical (header-only) | Acceptable (longer builds) |
| **Memory safety** | Manual management OK | Critical requirement |

---

## 1. Overall Parity Status

### Methodology

**File Count Limitations**:
- **Not a direct feature measure**: C++ uses header files (66-133 per system), Rust uses modules (6-25 .rs files)
- **Architectural differences**: C++ header-only vs Rust compiled libraries
- **Feature density**: One Rust file may contain multiple C++ header equivalents

**Parity Calculation**:
Parity percentages are estimated based on:
1. **Core feature coverage**: Essential functionality availability
2. **API completeness**: Number of public interfaces ported
3. **Testing coverage**: Test suite completeness
4. **Documentation**: API docs and examples

**Status Definitions**:
- **Stable**: Production-ready, well-tested, documented
- **In Progress**: Functional but incomplete, ongoing development
- **Experimental**: Proof-of-concept, unstable API
- **Planned**: Roadmap item, not yet started
- **N/A**: Not applicable (Rust-only or C++-only)

### System Comparison

| System | C++ Headers | Rust Files | Est. Parity | Status | Notes |
|--------|------------|------------|-------------|---------|-------|
| **common_system** | 66 | N/A | N/A | C++ Only | Foundation layer, interfaces |
| **thread_system** | 133 | 25 | ~60% | In Progress | Core pool ✅, advanced features partial |
| **logger_system** | 93 | 24 | ~70% | In Progress | Async logging ✅, some sinks missing |
| **container_system** | 1 | 20 | >100% | Stable | Rust has more features (serde integration) |
| **database_system** | 4 | 13 | ~40% | Experimental | Rust: SQLite only; C++: minimal |
| **network_system** | 73 | 23 | ~50% | In Progress | TCP/HTTP ✅, advanced protocols partial |
| **monitoring_system** | 98 | 16 | ~50% | In Progress | Metrics ✅, some integrations missing |
| **integrated_thread_system** | N/A | 6 | N/A | Rust Only | Combines thread + logger |

**Last Updated**: 2026-02-08
**Source**: File counts from repository analysis

### Interpretation Guide

#### thread_system (~60% parity)
**Rust has**: Basic thread pool, job scheduling, autoscaling
**Rust missing**: NUMA support, DAG scheduler, advanced diagnostics
**Recommendation**: Use C++ for HPC workloads, Rust for web services

#### logger_system (~70% parity)
**Rust has**: Async logging, console/file sinks, structured logging
**Rust missing**: Some specialized sinks, log rotation (partial)
**Recommendation**: Rust is production-ready for most use cases

#### container_system (>100% parity)
**Rust has**: JSON/binary serialization with serde, type-safe containers
**C++ has**: Minimal container utilities
**Recommendation**: **Prefer Rust** for serialization-heavy applications

#### database_system (~40% parity)
**Rust has**: SQLite support with async queries
**C++ has**: Minimal database abstraction (only 4 headers)
**Rust missing**: PostgreSQL, MySQL support
**Recommendation**: Use Rust for SQLite, external libs for other DBs

#### network_system (~50% parity)
**Rust has**: TCP server/client, HTTP basics with tokio
**Rust missing**: WebSocket, advanced protocols, TLS
**Recommendation**: Use C++ for production networking (more mature)

#### monitoring_system (~50% parity)
**Rust has**: Metrics collection, basic health checks
**Rust missing**: Full event bus integration, circuit breakers
**Recommendation**: Evaluate based on specific monitoring needs

#### integrated_thread_system (Rust-only)
**Purpose**: Combines thread_system + logger_system for simplified initialization
**Status**: Experimental, not feature-complete
**Recommendation**: Use for quick prototypes, avoid for production

---

## 3. API Mapping Guide

This section provides patterns for translating between C++ and Rust APIs, useful for:
- Porting C++ code to Rust
- Interoperating between C++ and Rust services
- Understanding design differences

### Naming Conventions

Both languages use **snake_case** for functions and variables, but differ in type naming.

#### C++ Conventions
```cpp
// Namespace: lowercase
namespace kcenon::common::patterns {

// Class names: PascalCase (sometimes snake_case for STL-like)
class Result;
class thread_pool;

// Functions: snake_case
auto create_thread_pool(size_t size) -> std::shared_ptr<thread_pool>;

// Constants: UPPER_SNAKE_CASE
constexpr size_t MAX_THREADS = 128;

// Type aliases: snake_case or PascalCase
using error_code = int;
using ErrorInfo = /* ... */;
}
```

#### Rust Conventions
```rust
// Module: lowercase
pub mod kcenon::common::patterns {

// Struct/Enum names: PascalCase
pub struct Result<T, E>;
pub struct ThreadPool;

// Functions: snake_case
pub fn create_thread_pool(size: usize) -> Arc<ThreadPool>;

// Constants: UPPER_SNAKE_CASE
pub const MAX_THREADS: usize = 128;

// Type aliases: PascalCase
pub type ErrorCode = i32;
pub type ErrorInfo = /* ... */;
}
```

**Key Difference**: Rust enforces PascalCase for all types (structs, enums, traits), while C++ allows snake_case for class names.

### Type Mapping

Common type equivalences between C++ STL and Rust standard library:

| C++ Type | Rust Type | Notes |
|----------|-----------|-------|
| `std::string` | `String` | Rust strings are always UTF-8 |
| `std::string_view` | `&str` | Rust string slice (borrowed) |
| `const char*` | `*const c_char` | Use `CStr` for safety |
| `std::vector<T>` | `Vec<T>` | Both heap-allocated, growable |
| `std::array<T, N>` | `[T; N]` | Fixed-size stack arrays |
| `std::map<K, V>` | `HashMap<K, V>` | Rust uses hash map by default |
| `std::unordered_map<K, V>` | `HashMap<K, V>` | Direct equivalent |
| `std::set<T>` | `HashSet<T>` | Unordered in Rust |
| `std::optional<T>` | `Option<T>` | Rust's Option is pervasive |
| `std::unique_ptr<T>` | `Box<T>` | Heap allocation, single owner |
| `std::shared_ptr<T>` | `Arc<T>` | Thread-safe reference counting |
| `std::shared_ptr<T>` (non-threaded) | `Rc<T>` | Single-threaded ref counting |
| `std::mutex<T>` | `Mutex<T>` | Rust's Mutex owns the data |
| `std::lock_guard<M>` | `MutexGuard<'a, T>` | RAII lock guard |

#### Custom Types

**kcenon Result<T>** (C++):
```cpp
namespace kcenon::common {
template<typename T>
class Result {
public:
    bool is_ok() const;
    bool is_err() const;
    T value() const;          // Throws if error
    ErrorInfo error() const;  // Throws if ok

    // Monadic operations
    template<typename F>
    auto and_then(F&& func) -> Result<U>;

    template<typename F>
    auto map(F&& func) -> Result<U>;
};

using VoidResult = Result<void>;
}
```

**Rust std::result::Result<T, E>**:
```rust
pub enum Result<T, E> {
    Ok(T),
    Err(E),
}

impl<T, E> Result<T, E> {
    pub fn is_ok(&self) -> bool;
    pub fn is_err(&self) -> bool;
    pub fn unwrap(self) -> T;           // Panics if Err
    pub fn expect(self, msg: &str) -> T; // Panics with message

    // Monadic operations
    pub fn and_then<U, F>(self, f: F) -> Result<U, E>
        where F: FnOnce(T) -> Result<U, E>;

    pub fn map<U, F>(self, f: F) -> Result<U, E>
        where F: FnOnce(T) -> U;
}

// Common alias for kcenon errors
pub type VoidResult = Result<(), ErrorInfo>;
```

**Key Differences**:
1. **Error type**: C++ uses `ErrorInfo`, Rust uses generic `E` (commonly `ErrorInfo` or `Box<dyn Error>`)
2. **Panic vs Exception**: Rust `unwrap()` panics (non-recoverable), C++ `value()` may throw
3. **Pattern matching**: Rust encourages `match` over `is_ok()`/`is_err()`

### Error Handling Differences

#### C++ Approach

**kcenon Result<T> Pattern**:
```cpp
#include <kcenon/common/patterns/result.h>

using namespace kcenon::common;

Result<Config> load_config(const std::string& path) {
    if (!std::filesystem::exists(path)) {
        return make_error<Config>(
            error_codes::NOT_FOUND,
            "Config file not found",
            "config_loader"
        );
    }

    auto config = parse_file(path);
    return ok(config);
}

// Usage with monadic operations
auto result = load_config("app.yaml")
    .and_then(validate_config)
    .map(apply_defaults);

if (result.is_err()) {
    std::cerr << "Error: " << result.error().message << "\n";
    return 1;
}

auto config = result.value();
```

**Exception-Based** (legacy code):
```cpp
try {
    auto config = load_config_throwing("app.yaml");
    // Use config
} catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << "\n";
}
```

#### Rust Approach

**Result<T, E> Pattern**:
```rust
use std::fs;
use std::result::Result;

#[derive(Debug)]
struct ErrorInfo {
    code: i32,
    message: String,
    source: String,
}

fn load_config(path: &str) -> Result<Config, ErrorInfo> {
    if !fs::metadata(path).is_ok() {
        return Err(ErrorInfo {
            code: 404,
            message: "Config file not found".to_string(),
            source: "config_loader".to_string(),
        });
    }

    let config = parse_file(path)?; // ? operator for early return
    Ok(config)
}

// Usage with monadic operations
let result = load_config("app.yaml")
    .and_then(validate_config)
    .map(apply_defaults);

match result {
    Ok(config) => {
        // Use config
    }
    Err(e) => {
        eprintln!("Error: {}", e.message);
        return Err(e);
    }
}
```

**? Operator** (idiomatic Rust):
```rust
fn load_and_validate_config(path: &str) -> Result<Config, ErrorInfo> {
    let config = load_config(path)?;          // Early return if Err
    let validated = validate_config(config)?; // Chain with ?
    Ok(apply_defaults(validated))
}
```

**Comparison**:

| Aspect | C++ Result<T> | Rust Result<T, E> |
|--------|--------------|-------------------|
| **Error propagation** | `.and_then()` chains | `?` operator (syntactic sugar) |
| **Early return** | Manual `if (result.is_err()) return result;` | Automatic with `?` |
| **Pattern matching** | Not idiomatic (use `.is_ok()`) | Idiomatic with `match` |
| **Panic vs Throw** | `.value()` may throw | `.unwrap()` panics (unrecoverable) |
| **Error composition** | Manual wrapping | `From` trait for automatic conversion |

### Memory Management

#### C++ RAII with Smart Pointers

```cpp
#include <memory>
#include <kcenon/thread/core/thread_pool.h>

class Application {
    std::shared_ptr<thread_pool> pool_;
    std::unique_ptr<Config> config_;

public:
    Application() {
        // Shared ownership for thread pool (used by multiple components)
        pool_ = create_thread_pool(4);

        // Unique ownership for config (owned by Application only)
        config_ = std::make_unique<Config>(load_config("app.yaml"));
    }

    // Automatic cleanup when Application is destroyed
    ~Application() = default;

    void run() {
        pool_->submit([cfg = config_.get()]() {
            // Capture raw pointer (safe because Application owns it)
            process_with_config(cfg);
        });
    }
};

// Shared across threads
auto pool = std::make_shared<thread_pool>(4);
auto task = [pool]() {
    // pool reference count increased, safe to use
    pool->submit(/* ... */);
};
std::thread(task).detach(); // Safe: pool kept alive by shared_ptr
```

**Key Points**:
- `unique_ptr`: Exclusive ownership, moveable, zero overhead
- `shared_ptr`: Reference counting, thread-safe, slight overhead
- Manual lifetime management needed for raw pointers

#### Rust Ownership and Borrowing

```rust
use std::sync::Arc;
use crate::thread_pool::ThreadPool;

struct Application {
    pool: Arc<ThreadPool>,  // Shared ownership (like shared_ptr)
    config: Box<Config>,    // Owned (like unique_ptr)
}

impl Application {
    pub fn new() -> Self {
        Application {
            pool: Arc::new(ThreadPool::new(4)),
            config: Box::new(load_config("app.yaml")),
        }
    }

    // Automatic cleanup when Application is dropped
    // No explicit destructor needed

    pub fn run(&self) {
        let pool = Arc::clone(&self.pool); // Increment ref count
        let config_ref = &*self.config;    // Borrow (compile-time checked)

        pool.submit(move || {
            // pool moved into closure (ref count ensures safety)
            // Cannot move config_ref (borrow checker prevents dangling)
        });
    }
}

// Shared across threads
let pool = Arc::new(ThreadPool::new(4));
let pool_clone = Arc::clone(&pool);
std::thread::spawn(move || {
    // pool_clone moved into thread, original pool still valid
    pool_clone.submit(/* ... */);
});
```

**Key Points**:
- `Box<T>`: Exclusive ownership (like `unique_ptr`), no runtime overhead
- `Arc<T>`: Atomic reference counting (like `shared_ptr`), thread-safe
- `Rc<T>`: Non-atomic ref counting (faster, single-threaded)
- **Borrow checker**: Compile-time verification of lifetime safety (no dangling pointers possible)

**Lifetime Annotations**:
```rust
// Lifetime 'a ensures reference validity
fn get_config_value<'a>(config: &'a Config, key: &str) -> &'a str {
    config.get(key) // Returned reference tied to config's lifetime
}

// Compiler error: cannot return reference to local variable
fn invalid() -> &str {
    let s = String::from("hello");
    &s // ERROR: s dropped at end of function
}
```

**Comparison**:

| Aspect | C++ Smart Pointers | Rust Ownership |
|--------|-------------------|----------------|
| **Safety** | Runtime errors if misused | Compile-time verification |
| **Overhead** | Ref counting for `shared_ptr` | Same for `Arc`, zero for `Box` |
| **Dangling pointers** | Possible with raw pointers | Impossible (borrow checker) |
| **Learning curve** | Moderate (RAII patterns) | Steep (ownership rules) |
| **Flexibility** | Escape hatches (raw pointers) | Unsafe blocks for FFI |

### Async/Await Patterns

#### C++ Async (std::async, std::future)

```cpp
#include <future>
#include <thread>

// Basic async task
auto future = std::async(std::launch::async, []() {
    return expensive_computation();
});

// Wait for result
auto result = future.get(); // Blocks until ready

// With thread pool (kcenon pattern)
#include <kcenon/thread/core/thread_pool.h>

auto pool = create_thread_pool(4);

pool->submit([](auto result) {
    // Callback-based async
    return expensive_computation();
});
```

**Limitations**:
- No native async/await syntax (requires C++20 coroutines, complex)
- `std::async` creates threads (expensive for many tasks)
- Callback-based patterns can lead to "callback hell"

#### Rust Async/Await (tokio runtime)

```rust
use tokio;

// Async function with await
async fn fetch_data(url: &str) -> Result<String, reqwest::Error> {
    let response = reqwest::get(url).await?; // Suspend until ready
    let text = response.text().await?;
    Ok(text)
}

// Runtime execution
#[tokio::main]
async fn main() {
    let result = fetch_data("https://api.example.com").await;
    match result {
        Ok(data) => println!("Data: {}", data),
        Err(e) => eprintln!("Error: {}", e),
    }
}

// Concurrent tasks
async fn process_multiple() {
    let task1 = fetch_data("url1");
    let task2 = fetch_data("url2");

    // Run concurrently
    let (result1, result2) = tokio::join!(task1, task2);
}

// With thread pool (kcenon Rust port)
use crate::thread_pool::ThreadPool;

let pool = Arc::new(ThreadPool::new(4));
pool.submit(async move {
    let data = fetch_data("url").await;
    process(data);
});
```

**Advantages**:
- Native `async`/`await` syntax (readable, composable)
- Lightweight tasks (green threads, not OS threads)
- Rich ecosystem (tokio, async-std, futures)

**Comparison**:

| Aspect | C++ std::async | Rust async/await |
|--------|---------------|------------------|
| **Syntax** | Callbacks, `.then()` chains | `async fn`, `await` keyword |
| **Performance** | Thread-per-task (expensive) | Task scheduler (efficient) |
| **Ecosystem** | Limited (Boost.Asio external) | Rich (tokio, async-std) |
| **Learning curve** | Moderate | Moderate (borrowing + async) |
| **Cancellation** | Manual (std::stop_token in C++20) | Built-in (tokio::select!, drop) |

---

## Next Steps

This completes **Part 1** of the Rust/C++ parity matrix. The following parts will cover:

### Part 2: Feature Comparison for Core Systems (Planned)
- Detailed feature tables for:
  - thread_system (thread pool, policies, NUMA, autoscaler, DAG scheduler)
  - logger_system (sinks, async logging, log rotation, structured logging)
  - container_system (JSON/binary serialization, typed containers)
  - monitoring_system (event bus, metrics, health checks, circuit breakers)
- API comparison examples for each feature
- Build verification for Rust ports

### Part 3: Remaining Systems, Interop, and Roadmap (Planned)
- Feature tables for database_system, network_system, integrated_thread_system
- FFI interoperability patterns (C ABI, unsafe Rust)
- Data format and protocol compatibility
- Shared configuration strategies
- Roadmap for future ports and feature completeness

---

## Related Documentation

- [Architecture Overview](ARCHITECTURE.md) - System design philosophy
- [Integration Guide](INTEGRATION_GUIDE.md) - Multi-system integration patterns
- [API Reference](API_REFERENCE.md) - C++ interface specifications
- [Quick Start](guides/QUICK_START.md) - Getting started with C++ systems

---

**Document Version**: 1.0.0 (Part 1)
**Last Updated**: 2026-02-08
**Authors**: kcenon team
**Related Issues**: kcenon/common_system#330, #338
**Living Document**: This matrix is updated as Rust ports evolve. Check commit history for latest changes.
