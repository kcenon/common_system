# Quick Start Guide - common_system

**5-minute guide** to get started with the header-only common_system library for C++20.

## Prerequisites

- **Compiler**: C++20 compatible (GCC 11+, Clang 14+, MSVC 2022+, Apple Clang 14+)
- **Build System**: CMake 3.16+ (optional for header-only usage)

## Installation

### Option 1: Copy Headers (Fastest)

```bash
# Clone the repository
git clone https://github.com/kcenon/common_system.git
cd common_system

# Copy include directory to your project
cp -r include/kcenon /path/to/your/project/include/
```

Add to your compiler flags:
```bash
-I/path/to/your/project/include
```

### Option 2: CMake FetchContent (Recommended)

```cmake
# In your CMakeLists.txt
include(FetchContent)

FetchContent_Declare(
    common_system
    GIT_REPOSITORY https://github.com/kcenon/common_system.git
    GIT_TAG main
)
FetchContent_MakeAvailable(common_system)

# Link to your target
target_link_libraries(your_target PRIVATE kcenon::common)
```

### Option 3: vcpkg Integration

```bash
# Add to your vcpkg manifest
vcpkg install common_system
```

## First Program: Using Result<T>

Create `main.cpp`:

```cpp
#include <kcenon/common/patterns/result.h>
#include <iostream>
#include <string>

using namespace kcenon::common;

// Function that returns Result<T>
Result<int> divide(int a, int b) {
    if (b == 0) {
        return make_error<int>(
            error_codes::INVALID_ARGUMENT,
            "Cannot divide by zero",
            "math_module"
        );
    }
    return ok(a / b);
}

int main() {
    // Case 1: Success
    auto result = divide(10, 2);
    if (is_ok(result)) {
        std::cout << "10 / 2 = " << get_value(result) << "\n";
    }

    // Case 2: Failure
    auto error_result = divide(10, 0);
    if (is_error(error_result)) {
        auto& err = get_error(error_result);
        std::cout << "Error: " << err.message
                  << " (code: " << err.code << ")\n";
    }

    return 0;
}
```

Compile:
```bash
g++ -std=c++17 -I./include main.cpp -o main
./main
# Output:
# 10 / 2 = 5
# Error: Cannot divide by zero (code: -1)
```

## Error Handling Example

Type-safe error handling without exceptions:

```cpp
#include <kcenon/common/patterns/result.h>
#include <iostream>
#include <fstream>

using namespace kcenon::common;

Result<std::string> read_config(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return make_error<std::string>(
            error_codes::NOT_FOUND,
            "Config file not found",
            "config_loader"
        );
    }

    std::string content((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());
    return ok(content);
}

int main() {
    // Basic check and handle
    auto result = read_config("app.conf");
    if (is_ok(result)) {
        std::cout << "Config size: " << get_value(result).length()
                  << " bytes\n";
    } else {
        auto& err = get_error(result);
        std::cerr << "Failed: " << err.message << "\n";
    }

    return 0;
}
```

### Result API Checklist

Before copying Result snippets into other repositories, verify the following:

1. **Factory usage** – Prefer `Result<T>::ok(...)` / `Result<T>::err(...)` (or helpers `ok(...)`, `make_error(...)`) instead of invoking constructors directly.
2. **Error access** – Use `result.error()` or `common::get_error(result)`; avoid obsolete `.get_error()` member calls.
3. **Void specialization** – For `Result<void>` return paths, call `Result<void>::err(error_info)` rather than `result_void::error(...)`.
4. **Migration reference** – If an integrating module wraps the Result API (e.g., `thread::result`), consult that module’s header to confirm supported helpers.

Including this quick check in your workflow keeps downstream documentation synchronized with the latest Result semantics.

## RAII Example

Using IExecutor interface with automatic resource management:

```cpp
#include <kcenon/common/interfaces/executor_interface.h>
#include <iostream>
#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>

using namespace kcenon::common::interfaces;

// Simple thread pool (RAII-compliant)
class ThreadPool : public IExecutor {
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex mtx_;
    std::condition_variable cv_;
    std::atomic<bool> running_{true};

public:
    ThreadPool(size_t num_threads) {
        for (size_t i = 0; i < num_threads; ++i) {
            workers_.emplace_back([this] { work(); });
        }
    }

    ~ThreadPool() { shutdown(true); }

    std::future<void> submit(std::function<void()> task) override {
        auto promise = std::make_shared<std::promise<void>>();
        auto future = promise->get_future();

        {
            std::lock_guard<std::mutex> lock(mtx_);
            tasks_.emplace([t = std::move(task), p = promise]() {
                try { t(); p->set_value(); }
                catch(...) { p->set_exception(std::current_exception()); }
            });
        }
        cv_.notify_one();
        return future;
    }

    // Other required interface methods...
    std::future<void> submit_delayed(std::function<void()>,
                                     std::chrono::milliseconds) override
    { return submit([] {}); }

    Result<std::future<void>> execute(std::unique_ptr<IJob>&&) override
    { return error<std::future<void>>(0, "Not implemented", ""); }

    Result<std::future<void>> execute_delayed(std::unique_ptr<IJob>&&,
                                              std::chrono::milliseconds) override
    { return error<std::future<void>>(0, "Not implemented", ""); }

    size_t worker_count() const override { return workers_.size(); }
    bool is_running() const override { return running_; }
    size_t pending_tasks() const override { return tasks_.size(); }

    void shutdown(bool wait) override {
        running_ = false;
        cv_.notify_all();
        for (auto& w : workers_) {
            if (w.joinable()) w.join();
        }
    }

private:
    void work() {
        while (running_) {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lock(mtx_);
                cv_.wait(lock, [this] {
                    return !tasks_.empty() || !running_;
                });
                if (!tasks_.empty()) {
                    task = std::move(tasks_.front());
                    tasks_.pop();
                }
            }
            if (task) task();
        }
    }
};

int main() {
    // RAII: ThreadPool destroyed and cleaned up automatically
    ThreadPool pool(4);

    auto future1 = pool.submit([] {
        std::cout << "Task 1\n";
    });

    auto future2 = pool.submit([] {
        std::cout << "Task 2\n";
    });

    future1.wait();
    future2.wait();

    // Automatic cleanup when pool goes out of scope
    return 0;
}
```

## Key Components at a Glance

| Component | Header | Purpose |
|-----------|--------|---------|
| **Result<T>** | `patterns/result.h` | Type-safe error handling (no exceptions) |
| **IExecutor** | `interfaces/executor_interface.h` | Async task execution abstraction |
| **error_codes** | `error/error_codes.h` | Centralized error code registry |
| **Event Bus** | `patterns/event_bus.h` | Event-driven communication |

## Next Steps

1. **[Error Handling Guide](../ERROR_HANDLING.md)** - Deep dive into Result<T> patterns and best practices
2. **[RAII Guidelines](../RAII_GUIDELINES.md)** - Resource management patterns
3. **[Integration Guide](../INTEGRATION.md)** - Using common_system with other libraries
4. **[Architecture Guide](../ARCHITECTURE.md)** - System design and module interaction
5. **[FAQ](./FAQ.md)** - Common questions and troubleshooting

## Common Tasks

### Check if Result succeeded
```cpp
if (is_ok(result)) { }              // Check for success
if (is_error(result)) { }           // Check for error
auto value = get_value(result);     // Get value (safe)
auto ref = result.value();          // Get reference directly
```

### Get Error Details
```cpp
auto& error = get_error(result);
std::cout << error.code << "\n";      // Error code (-1 to -699)
std::cout << error.message << "\n";   // Error message
std::cout << error.module << "\n";    // Source module
if (error.details.has_value()) {
    std::cout << error.details.value() << "\n";  // Optional details
}
```

### Handle with Defaults
```cpp
// Get value with fallback
int value = result.unwrap_or(0);     // Default value if error
int value = result.value_or(-1);     // Alternative name (C++23 compatible)
```

## Troubleshooting

**Q: Compiler can't find headers**
- Ensure include path is correct: `-I/path/to/common_system/include`
- Check namespace: use `kcenon::common`, not `common` (deprecated alias exists)
- Verify header file exists: `ls include/kcenon/common/patterns/result.h`

**Q: Result<T> API errors**
- Use `make_error<T>()` to create errors, not `error<T>()`
- Use `is_ok()` and `is_error()` helper functions (not `.is_ok()` method)
- Use `get_value()` and `get_error()` to access contents
- `.unwrap()` throws exceptions on error - use `.unwrap_or()` for safe defaults

**Q: How do I use IExecutor?**
- Implement the interface or use a third-party executor adapter
- Call `submit()` with a `std::function<void()>` to queue tasks
- Use the returned `std::future<void>` to wait for completion
- Check thread_system for production implementations

**Q: Compilation with warnings**
- Ignore deprecation warnings from internal Result implementation
- Use C++17 or higher: `-std=c++17`
- If using C++20, additional features like `source_location` are automatically enabled

See [Full Documentation](../) for complete reference.
