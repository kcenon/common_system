# Common System - Frequently Asked Questions

**Version:** 1.0 | **Last Updated:** November 2025

This FAQ covers the most common questions about the common_system library - a header-only C++17 foundation library providing universal interfaces and design patterns for modular system architectures.

---

## Table of Contents

1. [Installation & Setup](#installation--setup)
2. [Basic Usage](#basic-usage)
3. [Result<T> Pattern](#resultt-pattern)
4. [RAII & Resource Management](#raii--resource-management)
5. [Integration](#integration)
6. [Troubleshooting](#troubleshooting)

---

## Installation & Setup

### Q1: Is common_system a header-only library? Do I need to build it?

**A:** Yes, common_system is a **pure header-only C++17 library**. There is no compilation step required.

**Installation options:**

```bash
# Option 1: Clone and use directly (simplest)
git clone https://github.com/kcenon/common_system.git
# Then add to your include path: -I/path/to/common_system/include

# Option 2: Copy headers to system directory
cp -r common_system/include/kcenon /usr/local/include/

# Option 3: Use with CMake
# In your CMakeLists.txt:
include_directories(/path/to/common_system/include)
```

No linking or .lib/.a files needed. Just include headers and compile.

---

### Q2: What are the minimum compiler and C++ standard requirements?

**A:** common_system requires:

- **C++ Standard:** C++17 minimum (C++20 recommended for enhanced features)
- **Compiler Support:**
  - GCC 7.0+
  - Clang 5.0+
  - MSVC 2017+
  - AppleClang 10.0+

**Compiler flags for C++17:**

```bash
# GCC/Clang
g++ -std=c++17 myfile.cpp

# MSVC
cl /std:c++17 myfile.cpp

# CMake
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
```

---

### Q3: Does common_system have external dependencies?

**A:** No. common_system is **completely self-contained** with **zero external dependencies**.

It uses only C++ standard library features:
- `<variant>`, `<optional>`, `<string>`, `<type_traits>`
- `<system_error>`, `<stdexcept>`
- C++20 `<source_location>` when available (graceful fallback to C++17)

This makes it trivial to integrate into any project without dependency management complexity.

---

### Q4: How do I verify the installation is correct?

**A:** Create a simple test file to verify:

```cpp
#include <kcenon/common/common.h>
#include <iostream>

using namespace kcenon::common;

int main() {
    // Test Result<T>
    auto result = Result<int>::ok(42);
    std::cout << "Value: " << result.value() << std::endl;

    // Test VoidResult
    auto void_result = ok();
    std::cout << "Void result OK: " << void_result.is_ok() << std::endl;

    // Test version
    std::cout << "common_system v" << version_info::string << std::endl;

    return 0;
}
```

Compile with C++17:
```bash
g++ -std=c++17 -I/path/to/common_system/include test.cpp -o test && ./test
```

---

## Basic Usage

### Q5: What is Result<T> and when should I use it?

**A:** `Result<T>` is a **type-safe error handling pattern** similar to Rust's `Result` or C++23's `std::expected`. It can be in one of two states:

1. **Ok state:** Contains a value of type `T`
2. **Error state:** Contains error information

Use it instead of exceptions for:
- Library APIs where errors are expected and recoverable
- Performance-critical code where exception overhead matters
- Embedded systems or hard real-time systems
- Distributed systems where errors must be explicitly propagated

```cpp
#include <kcenon/common/patterns/result.h>
using namespace kcenon::common;

// Function that can fail
Result<int> divide(int a, int b) {
    if (b == 0) {
        return Result<int>::err(
            error_codes::INVALID_ARGUMENT,
            "Division by zero",
            "math_module"
        );
    }
    return Result<int>::ok(a / b);
}

int main() {
    auto result = divide(10, 2);

    // Check state
    if (result.is_ok()) {
        std::cout << "Result: " << result.value() << std::endl;
    } else {
        std::cout << "Error: " << result.error().message << std::endl;
    }

    return 0;
}
```

---

### Q6: How do I check if a Result succeeded or failed?

**A:** Result<T> provides multiple ways to check status:

```cpp
Result<int> result = some_operation();

// Method 1: Direct state checks
if (result.is_ok()) {
    int value = result.value();
}

if (result.is_err()) {
    error_info err = result.error();
}

// Method 2: Free functions (C-style)
if (is_ok(result)) {
    int value = get_value(result);
}

if (is_error(result)) {
    error_info err = get_error(result);
}

// Method 3: Pointer-safe checks
if (auto* value = get_if_ok(result)) {
    std::cout << *value << std::endl;
}

if (auto* err = get_if_error(result)) {
    std::cout << err->message << std::endl;
}

// Method 4: Get with default value
int safe_value = result.unwrap_or(-1);
// Returns actual value if ok, -1 if error
```

---

### Q7: What's the difference between Result<T> and VoidResult?

**A:** `VoidResult` is a specialized version of `Result<T>` for operations that don't return a value:

```cpp
// For functions that return a value
Result<std::string> read_file(const std::string& path);

// For functions that don't return a value
VoidResult write_file(const std::string& path, const std::string& content);

// VoidResult is equivalent to:
using VoidResult = Result<std::monostate>;

// Usage
VoidResult result = write_file("data.txt", "hello");

if (result.is_ok()) {
    std::cout << "Write successful" << std::endl;
} else {
    std::cout << "Write failed: " << result.error().message << std::endl;
}

// Creating VoidResult
VoidResult success() {
    return ok();  // Create successful void result
}

VoidResult failure() {
    return make_error<std::monostate>(
        error_codes::IO_ERROR,
        "Cannot write file",
        "file_module"
    );
}
```

---

### Q8: How do I handle errors from Result<T> in my functions?

**A:** The most idiomatic patterns are:

```cpp
#include <kcenon/common/patterns/result.h>
using namespace kcenon::common;

// Pattern 1: Propagate errors (early return)
Result<int> process_data(const std::string& input) {
    auto parsed = parse_integer(input);
    if (parsed.is_err()) {
        return parsed.error();  // Propagate error
    }

    auto validated = validate(parsed.value());
    if (validated.is_err()) {
        return validated.error();
    }

    return Result<int>::ok(processed);
}

// Pattern 2: Using helper macros (syntactic sugar)
Result<int> process_data_v2(const std::string& input) {
    COMMON_ASSIGN_OR_RETURN(auto parsed, parse_integer(input));
    COMMON_ASSIGN_OR_RETURN(auto validated, validate(parsed));
    return Result<int>::ok(validated);
}

// Pattern 3: Error recovery with or_else
Result<int> process_with_fallback(const std::string& input) {
    return or_else(
        parse_integer(input),
        [](const error_info&) {
            return Result<int>::ok(0);  // Default value
        }
    );
}

// Pattern 4: Exception wrapping
Result<int> call_legacy_api() {
    return try_catch<int>([]() {
        return legacy_function_that_throws();
    }, "legacy_module");
}
```

---

## Result<T> Pattern

### Q9: How do I chain multiple operations with Result<T>?

**A:** Use monadic operations (`map`, `and_then`, `or_else`):

```cpp
#include <kcenon/common/patterns/result.h>
using namespace kcenon::common;

Result<int> divide(int a, int b);
Result<std::string> format_number(int num);

// Chaining with map: transform value if success
Result<std::string> result = divide(10, 2)
    .map([](int val) { return val * 2; })      // 5 * 2 = 10
    .and_then([](int val) {
        return format_number(val);  // Returns Result<std::string>
    });

// Detailed example
int main() {
    // Start with a result
    auto step1 = divide(100, 5);  // Result<int> = 20

    // map: transform the value
    auto step2 = step1.map([](int x) { return x * 2; });  // 20 * 2 = 40

    // and_then: chain operations that return Result
    auto step3 = step2.and_then([](int x) {
        return divide(x, 2);  // 40 / 2 = 20
    });

    // or_else: provide fallback on error
    auto final = step3.or_else([](const error_info& err) {
        std::cerr << "Error: " << err.message << std::endl;
        return Result<int>::ok(0);
    });

    if (final.is_ok()) {
        std::cout << "Final result: " << final.value() << std::endl;
    }

    return 0;
}
```

**Key differences:**
- `map(f)`: `f` returns a regular value → wraps result
- `and_then(f)`: `f` returns a `Result<T>` → flattens nesting
- `or_else(f)`: Called only on error → allows recovery

---

### Q10: What are the error code conventions?

**A:** common_system defines standard error codes organized by range:

```cpp
#include <kcenon/common/error/error_codes.h>
using namespace kcenon::common;

// Common error codes (0 to -99)
error_codes::SUCCESS              // 0
error_codes::INVALID_ARGUMENT     // -1
error_codes::NOT_FOUND            // -2
error_codes::PERMISSION_DENIED    // -3
error_codes::TIMEOUT              // -4
error_codes::CANCELLED            // -5
error_codes::NOT_INITIALIZED      // -6
error_codes::ALREADY_EXISTS       // -7
error_codes::OUT_OF_MEMORY        // -8
error_codes::IO_ERROR             // -9
error_codes::NETWORK_ERROR        // -10
error_codes::INTERNAL_ERROR       // -99

// Module-specific ranges (for other systems)
error_codes::THREAD_ERROR_BASE    // -100
error_codes::LOGGER_ERROR_BASE    // -200
error_codes::MONITORING_ERROR_BASE// -300

// Custom module errors
constexpr int MY_MODULE_ERROR = -500;

int main() {
    return make_error<std::string>(
        error_codes::INVALID_ARGUMENT,
        "Invalid configuration parameter",
        "my_module"
    );
}
```

---

### Q11: How do I create error information with additional context?

**A:** The `error_info` structure stores code, message, module, and optional details:

```cpp
#include <kcenon/common/patterns/result.h>
using namespace kcenon::common;

// Basic error
error_info err1("Something went wrong");

// Error with code and message
error_info err2(
    error_codes::NOT_FOUND,
    "File not found"
);

// Error with code, message, and module
error_info err3(
    error_codes::IO_ERROR,
    "Cannot read file",
    "file_system"
);

// Error with full context
error_info err4(
    error_codes::PERMISSION_DENIED,
    "Access denied",
    "security_module",
    "User does not have 'write' permission"  // details
);

// Using in Result<T>
Result<std::string> read_config(const std::string& path) {
    if (path.empty()) {
        return Result<std::string>::err(error_info{
            error_codes::INVALID_ARGUMENT,
            "Configuration path cannot be empty",
            "config_loader",
            "Please provide a valid file path"
        });
    }

    // ... implementation
    return Result<std::string>::ok(content);
}
```

---

### Q12: Can I wrap exceptions and convert them to Result<T>?

**A:** Yes, use `try_catch` to automatically map exceptions to error codes:

```cpp
#include <kcenon/common/patterns/result.h>
using namespace kcenon::common;

// Legacy function that throws
int parse_json(const std::string& json);

int main() {
    // Wrap exception-throwing code
    auto result = try_catch<int>(
        [](){ return parse_json(invalid_json); },
        "json_parser"
    );

    if (result.is_err()) {
        auto& err = result.error();
        std::cout << "Caught: " << err.message << " (code: " << err.code << ")" << std::endl;
        // Error code automatically assigned based on exception type
    }

    // Exception type → Error code mapping:
    // std::invalid_argument    → INVALID_ARGUMENT
    // std::out_of_range        → INVALID_ARGUMENT
    // std::bad_alloc           → OUT_OF_MEMORY
    // std::system_error        → System error code
    // std::logic_error         → INTERNAL_ERROR
    // std::runtime_error       → INTERNAL_ERROR
    // std::exception           → INTERNAL_ERROR
    // Unknown exception        → INTERNAL_ERROR

    return 0;
}

// For void operations, use try_catch_void
VoidResult safe_cleanup() {
    return try_catch_void(
        []() {
            cleanup_resources();  // May throw
        },
        "cleanup_module"
    );
}
```

---

## RAII & Resource Management

### Q13: What RAII patterns does common_system provide?

**A:** common_system is designed with RAII principles but focuses on **interfaces and patterns** rather than specific implementations. Key RAII concepts:

```cpp
#include <kcenon/common/common.h>
using namespace kcenon::common;

// Pattern 1: RAII scope guard (using standard C++ features)
template<typename F>
class ScopeGuard {
    F cleanup_;
public:
    ScopeGuard(F cleanup) : cleanup_(cleanup) {}
    ~ScopeGuard() { cleanup_(); }
};

// Pattern 2: Automatic resource management with Result
Result<int> safe_file_operation(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        return make_error<int>(
            error_codes::NOT_FOUND,
            "Cannot open file: " + path,
            "file_module"
        );
    }

    // File is RAII-managed; automatically closed on scope exit
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());

    return Result<int>::ok(content.size());
}

// Pattern 3: Smart pointer patterns with Result
Result<std::shared_ptr<Resource>> create_resource() {
    auto resource = std::make_shared<Resource>();

    if (!resource->initialize()) {
        return make_error<std::shared_ptr<Resource>>(
            error_codes::INTERNAL_ERROR,
            "Failed to initialize resource",
            "resource_manager"
        );
    }

    return Result<std::shared_ptr<Resource>>::ok(resource);
    // Resource automatically cleaned up if error is returned
}
```

---

### Q14: How do I manage custom resources with Result<T>?

**A:** Combine Result<T> with standard C++ resource management:

```cpp
#include <kcenon/common/patterns/result.h>
#include <memory>
using namespace kcenon::common;

// Custom resource class with RAII
class Database {
public:
    Database() { std::cout << "Database connected" << std::endl; }
    ~Database() { std::cout << "Database disconnected" << std::endl; }

    bool query(const std::string& sql) { return true; }
};

// Factory function returning Result
Result<std::unique_ptr<Database>> connect(const std::string& connection_string) {
    if (connection_string.empty()) {
        return make_error<std::unique_ptr<Database>>(
            error_codes::INVALID_ARGUMENT,
            "Connection string cannot be empty",
            "database"
        );
    }

    try {
        auto db = std::make_unique<Database>();
        return Result<std::unique_ptr<Database>>::ok(std::move(db));
    } catch (const std::exception& e) {
        return try_catch<std::unique_ptr<Database>>(
            []() { throw; },
            "database"
        );
    }
}

int main() {
    auto db_result = connect("localhost:5432");

    if (db_result.is_ok()) {
        auto& db = db_result.value();
        db->query("SELECT * FROM users");
        // db automatically destroyed on scope exit
    } else {
        std::cout << "Connection failed: " << db_result.error().message << std::endl;
    }

    return 0;
}
```

---

### Q15: What about custom deleters for smart pointers?

**A:** common_system works seamlessly with custom deleters:

```cpp
#include <kcenon/common/patterns/result.h>
#include <memory>
using namespace kcenon::common;

// C-style resource requiring custom cleanup
struct CResource {
    int id;
};

extern "C" {
    CResource* create_resource();
    void destroy_resource(CResource* res);
}

// Custom deleter
auto c_resource_deleter = [](CResource* res) {
    if (res) destroy_resource(res);
};

// Result with custom-deleter unique_ptr
Result<std::unique_ptr<CResource, decltype(c_resource_deleter)>>
create_c_resource() {
    auto res = std::unique_ptr<CResource, decltype(c_resource_deleter)>(
        create_resource(),
        c_resource_deleter
    );

    if (!res) {
        return make_error<std::unique_ptr<CResource, decltype(c_resource_deleter)>>(
            error_codes::OUT_OF_MEMORY,
            "Failed to allocate resource",
            "c_interop"
        );
    }

    return Result<std::unique_ptr<CResource, decltype(c_resource_deleter)>>::ok(std::move(res));
}

int main() {
    if (auto result = create_c_resource(); result.is_ok()) {
        auto& res = result.value();
        std::cout << "Resource ID: " << res->id << std::endl;
        // Automatically cleaned up with custom deleter
    }
    return 0;
}
```

---

## Integration

### Q16: How do I integrate common_system with my project using CMake?

**A:** Add it as a header-only library dependency:

```cmake
# CMakeLists.txt
cmake_minimum_required(VERSION 3.16)
project(MyProject)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Option 1: Direct include (simplest)
include_directories(${CMAKE_SOURCE_DIR}/common_system/include)

# Option 2: Using find_package (if installed)
# find_package(common_system REQUIRED)

# Your executable
add_executable(myapp main.cpp)

# Option 1: Link directly
target_include_directories(myapp PRIVATE ${CMAKE_SOURCE_DIR}/common_system/include)

# Option 2: If using find_package
# target_link_libraries(myapp PRIVATE common_system::common_system)

# Enable compiler warnings
if(MSVC)
    target_compile_options(myapp PRIVATE /W4)
else()
    target_compile_options(myapp PRIVATE -Wall -Wextra -Wpedantic)
endif()
```

---

### Q17: How do I use common_system in conjunction with other KCENON systems?

**A:** common_system provides the foundation; other systems extend it:

```cpp
#include <kcenon/common/common.h>
#include <kcenon/thread/thread_pool.h>  // Implements IExecutor
#include <kcenon/logger/logger.h>       // Uses Result<T>

using namespace kcenon::common;

int main() {
    // 1. Create a thread pool (implements IExecutor interface)
    auto executor = kcenon::thread::ThreadPool(4);

    // 2. Use it with other systems that expect IExecutor
    auto logger = kcenon::logger::Logger::create(
        &executor,
        "myapp.log"
    );

    // 3. All results are handled uniformly with Result<T>
    auto write_result = logger->write(
        kcenon::logger::LogLevel::Info,
        "Application started"
    );

    if (write_result.is_err()) {
        std::cerr << "Logging failed: " << write_result.error().message << std::endl;
    }

    return 0;
}
```

**Key integration points:**
- **IExecutor:** Used by thread_system, network_system, database_system
- **Result<T>:** Used by all systems for error handling
- **error_codes:** Centralized error definitions
- **event_bus:** Central event routing for monitoring_system

---

### Q18: Can I use common_system standalone without other systems?

**A:** Absolutely. common_system is **completely standalone**:

```cpp
// Minimal example using only common_system
#include <kcenon/common/patterns/result.h>
#include <iostream>
#include <fstream>
#include <string>

using namespace kcenon::common;

Result<std::string> read_file(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return make_error<std::string>(
            error_codes::NOT_FOUND,
            "File not found: " + path,
            "file_reader"
        );
    }

    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    return Result<std::string>::ok(content);
}

int main() {
    auto result = read_file("data.txt");

    if (result.is_ok()) {
        std::cout << "File contents:\n" << result.value() << std::endl;
    } else {
        std::cout << "Error: " << result.error().message << std::endl;
    }

    return 0;
}
```

No dependencies on thread_system, logger_system, or any other component.

---

## Troubleshooting

### Q19: I'm getting "error: no matching function" with Result<T>

**A:** This usually means a type mismatch. Verify:

```cpp
#include <kcenon/common/patterns/result.h>
using namespace kcenon::common;

// Common mistake: Type mismatch
Result<int> get_value() {
    std::string result = "42";

    // WRONG: Trying to return string as int result
    // return Result<int>::ok(result);  // ← Type error!

    // CORRECT: Use consistent types
    return Result<int>::ok(std::stoi(result));
}

// Debugging type errors
Result<std::vector<int>> process() {
    std::vector<int> data;
    data.push_back(1);

    // Type must match Result<T>
    return Result<std::vector<int>>::ok(std::move(data));
}

int main() {
    // Use explicit template parameters if needed
    Result<double> x = Result<double>::ok(3.14);

    // For error returns, specify the same type
    if (x.is_ok()) {
        return Result<double>::err(error_codes::INTERNAL_ERROR, "Should not happen");
    }

    return 0;
}
```

**Solutions:**
1. Check that error and ok values have matching types
2. Use explicit template parameters: `Result<T>::ok(...)` not `Result::ok(...)`
3. For complex types, use `auto` with deduction

---

### Q20: How do I debug Result<T> errors?

**A:** Use the detailed error information:

```cpp
#include <kcenon/common/patterns/result.h>
#include <iostream>
using namespace kcenon::common;

Result<int> failing_operation();

int main() {
    auto result = failing_operation();

    if (result.is_err()) {
        const auto& err = result.error();

        // Print full error information
        std::cout << "Error Details:" << std::endl
                  << "  Code: " << err.code << std::endl
                  << "  Message: " << err.message << std::endl
                  << "  Module: " << err.module << std::endl;

        if (err.details.has_value()) {
            std::cout << "  Details: " << err.details.value() << std::endl;
        }
    }

    // Using try_catch with detailed error mapping
    auto wrapped = try_catch<int>(
        []() {
            throw std::invalid_argument("Invalid number format");
            return 42;
        },
        "parser"
    );

    if (wrapped.is_err()) {
        const auto& err = wrapped.error();
        std::cout << "Exception mapped to code: " << err.code << std::endl;
        std::cout << "Details field: " << err.details.value_or("N/A") << std::endl;
    }

    return 0;
}
```

---

### Q21: I'm getting template errors with complex types

**A:** Header-only libraries can have complex template error messages. Simplify:

```cpp
#include <kcenon/common/patterns/result.h>
#include <vector>
using namespace kcenon::common;

// Complex type - easier to debug with alias
using IntVector = std::vector<int>;
using IntVectorResult = Result<IntVector>;

// Now errors are more readable
IntVectorResult get_numbers() {
    IntVector data;
    data.push_back(1);
    data.push_back(2);
    data.push_back(3);

    return IntVectorResult::ok(std::move(data));
}

// For function pointers and other complex types
struct ComplexCallable {
    int (*fn)(int, int);
    std::string name;
};

using CallableResult = Result<ComplexCallable>;

int main() {
    auto result = get_numbers();

    // Use structured bindings for clarity
    if (result.is_ok()) {
        for (int num : result.value()) {
            std::cout << num << " ";
        }
        std::cout << std::endl;
    }

    return 0;
}
```

**Tips:**
1. Use type aliases for complex types
2. Avoid deeply nested Result types (use flattening with `and_then`)
3. Enable verbose compiler output: `-ftemplate-backtrace-limit=0`

---

### Q22: Does common_system work with C++20 and modern compiler features?

**A:** Yes, common_system is **forward-compatible** with C++20+:

```cpp
// C++20 features (automatic detection)
#include <kcenon/common/patterns/result.h>
#include <source_location>  // C++20
using namespace kcenon::common;

// If C++20 available, unwrap() captures source_location automatically
Result<int> operation() {
    return Result<int>::err(error_codes::INTERNAL_ERROR, "Failed", "module");
}

int main() {
    auto result = operation();

    try {
        result.unwrap();  // Source location info included in error message
    } catch (const std::exception& e) {
        std::cout << "Detailed error:\n" << e.what() << std::endl;
    }

    // C++20 aggregate initialization
    error_info err{
        .code = error_codes::INVALID_ARGUMENT,
        .message = "Invalid input",
        .module = "parser"
    };

    return 0;
}
```

**Compile with C++20:**
```bash
g++ -std=c++20 -I/path/to/common_system/include myfile.cpp -o myapp
```

---

### Q23: Why does my IDE show errors but code compiles fine?

**A:** This is common with header-only libraries. Solutions:

1. **Update IntelliSense database:**
   - VSCode: `C_Cpp.updateChannel: Insiders`
   - Visual Studio: `Rescan Solution`
   - CLion: `File > Invalidate Caches`

2. **Configure include paths in IDE:**
   ```json
   // .vscode/c_cpp_properties.json
   {
       "configurations": [
           {
               "name": "Linux",
               "includePath": [
                   "${workspaceFolder}/common_system/include"
               ],
               "intelliSenseMode": "gcc-x64"
           }
       ]
   }
   ```

3. **Force C++17 standard in IDE:**
   - VSCode: Set `cppStandard: "c++17"` in settings
   - Visual Studio: Project Properties → C/C++ → Language → C++ Standard

---

## Summary

This FAQ covers the essential knowledge needed to effectively use common_system. For more detailed information:

- **[API Documentation](https://doxygen.kcenon.dev):** Complete reference
- **[Architecture Guide](../ARCHITECTURE.md):** System design and patterns
- **[Examples Directory](../../examples):** Working code samples
- **[GitHub Repository](https://github.com/kcenon/common_system):** Source and issues

---

**Questions or feedback?** Please open an issue on [GitHub](https://github.com/kcenon/common_system/issues)
