<!-- BSD 3-Clause License
     Copyright (c) 2025, kcenon
     See the LICENSE file in the project root for full license information. -->

# Contributing to common_system

Thank you for your interest in contributing to the common_system library! This document provides guidelines and instructions for contributing code, documentation, and improvements.

## Table of Contents

1. [Prerequisites](#prerequisites)
2. [Development Setup](#development-setup)
3. [Code Style Guidelines](#code-style-guidelines)
4. [Header-Only Best Practices](#header-only-best-practices)
5. [Testing Guidelines](#testing-guidelines)
6. [Documentation Standards](#documentation-standards)
7. [Contribution Areas](#contribution-areas)
8. [Submitting Changes](#submitting-changes)
9. [Code Review Process](#code-review-process)

## Prerequisites

Before you can develop for common_system, ensure you have the following installed:

### Required

- **C++17 Compiler**: Modern compiler with C++17 support
  - GCC 7.0 or later
  - Clang 5.0 or later
  - MSVC 2017 or later
  - Apple Clang 10.0 or later

- **CMake**: Version 3.16 or later
  ```bash
  cmake --version  # Should be >= 3.16
  ```

### Optional (for full feature support)

- **GoogleTest**: For running and writing tests
  - Installed via vcpkg (recommended)
  - Or system package manager

- **Doxygen**: For generating API documentation
  - Version 1.8.15 or later

- **clang-tidy**: For static analysis
  - Part of LLVM toolchain

### System Setup

The project uses vcpkg for dependency management. Install vcpkg if you plan to run tests:

```bash
# Clone vcpkg (if not already installed)
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.sh  # On macOS/Linux
# or .\bootstrap-vcpkg.bat  # On Windows
```

## Development Setup

### Clone and Build

```bash
# Clone the repository
git clone https://github.com/kcenon/common_system.git
cd common_system

# Create build directory
mkdir build
cd build

# Configure with CMake (header-only, no library build required)
cmake .. \
  -DCMAKE_CXX_STANDARD=17 \
  -DCMAKE_BUILD_TYPE=Release \
  -DCOMMON_BUILD_TESTS=ON \
  -DCOMMON_BUILD_EXAMPLES=ON

# Build tests and examples
cmake --build .

# Run tests
ctest --verbose
```

### Using the Build Script

Alternatively, use the provided build script:

```bash
# Linux/macOS
./build.sh --debug  # Debug build
./build.sh --release  # Release build
./build.sh --tests  # Run tests

# Windows
build.bat --debug
build.bat --release
```

### Verification

After setup, verify the installation:

```bash
# Run all tests
ctest --verbose

# Build examples
cmake --build . --target examples

# Run clang-tidy (if available)
clang-tidy --version
```

## Code Style Guidelines

The common_system library follows modern C++ conventions optimized for readability, maintainability, and compile-time efficiency.

### Naming Conventions

Enforce by `.clang-tidy` configuration:

```cpp
// Namespaces: lowercase with underscores
namespace kcenon::common {
    namespace patterns {
        // ...
    }
}

// Classes/Structs: lowercase with underscores
class smart_adapter_factory {
    // ...
};

struct error_info {
    int code;
};

// Functions: lowercase with underscores
template<typename T>
Result<T> try_catch(std::function<T()> func, const std::string& module);

// Variables: lowercase with underscores
int error_count = 0;
std::string module_name = "core";

// Member variables: trailing underscore for private/protected
class MyClass {
private:
    int value_;
    std::string name_;
};
```

### Code Formatting

- **Indentation**: 4 spaces (no tabs)
- **Line Length**: 100 characters maximum (soft limit, hard limit at 120)
- **Braces**: Allman style (opening brace on new line)

```cpp
// Good
class Result {
public:
    bool is_ok() const {
        return state_ == result_state::ok;
    }

    Result<T> and_then(std::function<Result<T>(const T&)> f) const {
        if (is_ok()) {
            return f(value_.value());
        }
        return error();
    }
};

// Good: Long template parameters split across lines
template<
    typename T,
    typename = std::enable_if_t<std::is_move_constructible_v<T>>
>
class Optional {
    // ...
};
```

### Modern C++ Practices

- Prefer `std::optional<T>` over raw pointers for optional values
- Use `std::variant<T, Error>` for discriminated unions
- Leverage `constexpr` where possible
- Prefer `std::string_view` for non-owning string parameters
- Use RAII principles consistently

```cpp
// Good: RAII with std::optional
template<typename T>
class Result {
private:
    result_state state_;
    std::optional<T> value_;
    std::optional<error_info> error_;
};

// Good: std::string_view for parameters
void log_message(std::string_view message) {
    // Non-owning reference to string data
}

// Good: constexpr where applicable
template<typename T>
constexpr bool is_reference_v = std::is_reference_v<T>;
```

### Comment Style

Use Doxygen-compatible comments for public APIs:

```cpp
/**
 * @brief Short description of the function.
 *
 * Longer description explaining the purpose, behavior, and any important
 * considerations. This helps users understand when and how to use this API.
 *
 * Thread Safety:
 * - Not thread-safe for concurrent modification
 * - Multiple threads may safely read if no thread modifies
 *
 * @tparam T The value type stored in the Result
 * @param func Callable returning value of type T
 * @param module Name of the module for error reporting
 * @return Result<T> containing value on success, error_info on failure
 *
 * @note This function captures exceptions and converts them to Result<T>.
 * @note Thread safety depends on the callable provided.
 *
 * @see error_info
 * @see result_state
 */
template<typename T>
Result<T> try_catch(
    std::function<T()> func,
    const std::string& module
);
```

### Include Guard Convention

Use `#pragma once` followed by comments:

```cpp
// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file result.h
 * @brief Result<T> type for exception-free error handling.
 */

#pragma once

#include <variant>
#include <optional>
// ... rest of includes
```

## Header-Only Best Practices

As a header-only library, common_system has specific constraints and opportunities:

### Organization

```
include/
├── kcenon/
│   └── common/
│       ├── common.h              # Main aggregate header
│       ├── adapters/
│       │   ├── adapters.h        # Aggregate
│       │   ├── typed_adapter.h
│       │   └── smart_adapter.h
│       ├── patterns/
│       │   ├── result.h          # Large templates
│       │   ├── event_bus.h
│       │   └── result_helpers.h
│       ├── interfaces/
│       │   ├── executor_interface.h
│       │   ├── logger_interface.h
│       │   └── database_interface.h
│       ├── error/
│       │   └── error_codes.h
│       ├── utils/
│       │   └── source_location.h
│       └── config/
│           └── abi_version.h
```

### Template Implementation Guidelines

1. **Keep Template Code in Headers**: All template code must remain in header files

```cpp
// WRONG: Function defined in .cpp (won't link)
template<typename T>
T get_value(const Result<T>& result);
// In cpp file:
// template<typename T>
// T get_value(const Result<T>& result) { ... }

// CORRECT: Implementation in header
template<typename T>
T get_value(const Result<T>& result) {
    if (result.is_ok()) {
        return result.value();
    }
    throw std::runtime_error("Result contains error");
}
```

2. **Explicit Template Instantiation**: Provide common instantiations where helpful

```cpp
// In header:
template<typename T>
class Result {
    // ... full definition
};

// Common instantiations are implicitly generated
// No explicit instantiation needed for header-only

// If creating non-header version later, explicitly instantiate:
// template class Result<int>;
// template class Result<std::string>;
```

3. **Forward Declarations**: Minimize dependencies through forward declarations

```cpp
// result.h
namespace kcenon::common {
    struct error_info;  // Forward declare instead of including

    template<typename T>
    class Result {
        // Uses error_info
    };
}

// Include only where needed
#include <kcenon/common/error/error_codes.h>  // Only when defining error_info
```

4. **Compilation Time**: Watch out for expensive template instantiations

```cpp
// AVOID: Creating new template instantiations in headers
// This will trigger recompilation

// PREFER: Use existing types or provide aliases
using IntResult = Result<int>;
using StringResult = Result<std::string>;
```

### Inline Functions

Mark simple getters and utility functions as `inline`:

```cpp
// Good: Inline simple accessor
template<typename T>
class Result {
public:
    inline bool is_ok() const {
        return state_ == result_state::ok;
    }

    // Longer functions don't need inline keyword
    // (compiler inlines automatically)
    Result<T> and_then(std::function<Result<T>(const T&)> f) const {
        // ... implementation
    }
};
```

### No Implementation Files

The library is pure header-only for core functionality:

```cpp
// common_system is header-only
// There are NO .cpp files for template code
// Only generated files exist: abi_version.cpp (optional, for versioning)
```

## Testing Guidelines

### Test Structure

Tests are organized in `/tests/unit/` and `/integration_tests/`:

```
tests/
├── unit/
│   ├── CMakeLists.txt
│   ├── main.cpp           # GTest runner
│   ├── result_test.cpp    # Result<T> tests
│   ├── executor_test.cpp  # IExecutor tests
│   └── thread_safety_tests.cpp
└── integration_tests/
    ├── framework/
    │   ├── test_helpers.h
    │   └── system_fixture.h
    └── scenarios/
```

### Writing Tests for Header-Only Code

```cpp
// tests/unit/result_test.cpp
#include <kcenon/common/patterns/result.h>
#include <gtest/gtest.h>
#include <stdexcept>

using namespace kcenon::common;

/**
 * @brief Test suite for Result<T> pattern
 */
class ResultTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup common test fixtures
    }
};

// Test template instantiation
TEST_F(ResultTest, ConstructsWithIntValue) {
    Result<int> result(42);
    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value(), 42);
}

// Test with std::string
TEST_F(ResultTest, ConstructsWithStringValue) {
    Result<std::string> result("hello");
    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value(), "hello");
}

// Test error handling
TEST_F(ResultTest, ConstructsWithError) {
    error_info err(100, "Test error");
    Result<int> result(err);
    ASSERT_TRUE(result.is_err());
    EXPECT_EQ(result.error().code, 100);
}

// Test monadic operations on templates
TEST_F(ResultTest, ChainOperationsWithAndThen) {
    Result<int> result = Result<int>(5)
        .and_then([](const int& val) {
            return Result<int>(val * 2);
        })
        .and_then([](const int& val) {
            return Result<int>(val + 3);
        });

    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value(), 13);  // (5 * 2) + 3
}
```

### Template Specialization Tests

When testing templates with different types:

```cpp
// Create template test fixtures
template<typename T>
class TemplateTest : public ::testing::Test {
public:
    using value_type = T;
};

// List types to test
using TestedTypes = ::testing::Types<
    int,
    double,
    std::string,
    std::vector<int>,
    std::pair<int, std::string>
>;

TYPED_TEST_SUITE(TemplateTest, TestedTypes);

TYPED_TEST(TemplateTest, ResultHandlesTypeCorrectly) {
    using T = typename TestFixture::value_type;
    // Test generic behavior for all types
}
```

### Thread Safety Tests

For header-only code with concurrency guarantees:

```cpp
#include <kcenon/common/patterns/result.h>
#include <gtest/gtest.h>
#include <thread>
#include <vector>

TEST(ThreadSafetyTest, MultipleReadersOK) {
    const Result<int> result(42);

    std::vector<std::thread> threads;
    std::vector<int> values;

    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([&result, &values, i]() {
            if (result.is_ok()) {
                values[i] = result.value();
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    // All threads should have read same value
    for (int val : values) {
        EXPECT_EQ(val, 42);
    }
}
```

### Running Tests

```bash
# Build and run all tests
cmake --build build
ctest --verbose

# Run specific test
ctest -R "ResultTest" --verbose

# Run with coverage
cmake --build build -DENABLE_COVERAGE=ON
cmake --build build
ctest --verbose
lcov --capture --directory . --output-file coverage.info
```

### Test CMakeLists.txt Pattern

```cmake
# tests/CMakeLists.txt
cmake_minimum_required(VERSION 3.16)

find_package(GTest REQUIRED)
find_package(Threads REQUIRED)

add_executable(common_system_tests
    result_test.cpp
    executor_test.cpp
    main.cpp
)

target_link_libraries(common_system_tests PRIVATE
    kcenon::common        # Header-only library
    GTest::gtest
    GTest::gtest_main
    Threads::Threads
)

target_compile_features(common_system_tests PRIVATE cxx_std_17)

add_test(NAME common_system_tests COMMAND common_system_tests)

set_tests_properties(common_system_tests PROPERTIES
    TIMEOUT 60
    LABELS "unit"
)
```

## Documentation Standards

All public APIs must be documented using Doxygen-compatible comments.

### Header Documentation

```cpp
/**
 * @file result.h
 * @brief Result<T> type for exception-free error handling.
 *
 * Provides a `Result<T>` type similar to Rust's Result or C++23's
 * `std::expected`, enabling explicit error propagation at module boundaries
 * without using exceptions.
 *
 * Thread Safety:
 * - Result<T> objects are NOT thread-safe for concurrent modification.
 * - Multiple threads may safely read the same Result<T> if no thread modifies it.
 * - Best practice: Use Result<T> as return values; avoid shared mutable access.
 *
 * @see error_info
 * @see result_state
 */
```

### Function Documentation

```cpp
/**
 * @brief Create a Result from a callable that may throw.
 *
 * This function executes the provided callable and captures any thrown
 * exceptions, converting them to Result<T> error state. This enables
 * exception-free error handling at function boundaries.
 *
 * Example usage:
 * @code
 * auto result = kcenon::common::try_catch<int>(
 *     []() { return 42; },
 *     "my_module"
 * );
 *
 * if (result.is_ok()) {
 *     std::cout << result.value() << std::endl;
 * } else {
 *     std::cerr << result.error().message << std::endl;
 * }
 * @endcode
 *
 * Exception Mapping:
 * - std::bad_alloc → OUT_OF_MEMORY
 * - std::invalid_argument → INVALID_ARGUMENT
 * - std::out_of_range → INVALID_ARGUMENT
 * - std::logic_error → INTERNAL_ERROR
 * - std::runtime_error → INTERNAL_ERROR
 * - Other exceptions → UNKNOWN_ERROR
 *
 * Thread Safety:
 * - The function is thread-safe if the callable is thread-safe.
 * - Exception safety: Strong guarantee.
 *
 * @tparam T The return type of the callable
 * @param func Callable that returns value of type T
 * @param module Module name for error reporting (appears in error_info)
 * @return Result<T> containing the value on success,
 *         or error_info on exception
 *
 * @note The callable must be exception-safe and not modify global state.
 * @note Thread safety depends on the callable provided.
 *
 * @see error_info for error structure
 * @see error_codes for error code definitions
 */
template<typename T>
Result<T> try_catch(
    std::function<T()> func,
    const std::string& module
);
```

### Example Code in Documentation

Include practical examples with expected output:

```cpp
/**
 * @brief Example: Chaining operations with and_then
 *
 * @code
 * Result<int> parse_int(const std::string& s) {
 *     try {
 *         int val = std::stoi(s);
 *         return Result<int>(val);
 *     } catch (...) {
 *         return Result<int>(error_info(1, "Parse failed"));
 *     }
 * }
 *
 * auto result = parse_int("42")
 *     .and_then([](const int& val) {
 *         return Result<int>(val * 2);  // Multiply by 2
 *     })
 *     .and_then([](const int& val) {
 *         if (val > 100) {
 *             return Result<int>(error_info(2, "Too large"));
 *         }
 *         return Result<int>(val);
 *     });
 *
 * if (result.is_ok()) {
 *     std::cout << "Result: " << result.value() << std::endl;  // "Result: 84"
 * } else {
 *     std::cout << "Error: " << result.error().message << std::endl;
 * }
 * @endcode
 */
```

### Generating Documentation

```bash
# Generate API documentation with Doxygen
cd common_system
./generate_docs.sh

# Or manually:
doxygen Doxyfile
# Output will be in docs/html/
```

## Contribution Areas

The common_system library welcomes contributions in several key areas:

### 1. Error Handling Patterns

Enhance and extend error handling beyond the current Result<T> implementation:

- **Error Mapping**: Add support for mapping external error types to Result<T>
  ```cpp
  // Example: Map Windows HRESULT to error_info
  error_info from_hresult(HRESULT hr);
  ```

- **Error Context**: Implement error context/stack traces
  ```cpp
  struct error_context {
      std::vector<error_info> stack;
      std::string full_trace() const;
  };
  ```

- **Custom Error Types**: Support user-defined error types in Result<T>
  ```cpp
  template<typename T, typename E = error_info>
  class Result { /* ... */ };
  ```

### 2. RAII Utilities

Add robust Resource Acquisition Is Initialization helpers:

- **Scope Guards**: RAII-based cleanup actions
  ```cpp
  template<typename Func>
  class scope_guard {
  public:
      scope_guard(Func cleanup) : cleanup_(cleanup) {}
      ~scope_guard() { cleanup_(); }
  };
  ```

- **Resource Wrappers**: RAII wrappers for system resources
  ```cpp
  class file_handle;  // RAII wrapper for FILE*
  class memory_buffer;  // RAII wrapper for allocated memory
  ```

- **Defer Pattern**: Rust-style defer for cleanup
  ```cpp
  void process() {
      auto* resource = acquire_resource();
      DEFER { release_resource(resource); };
      // use resource
  }  // resource automatically released
  ```

### 3. Smart Pointer Patterns

Enhance smart pointer utilities and patterns:

- **Intrusive Pointers**: For custom reference counting
  ```cpp
  template<typename T>
  class intrusive_ptr;  // Reference counting via object
  ```

- **Copy-on-Write Semantics**: Efficient value semantics
  ```cpp
  template<typename T>
  class cow_ptr;  // Copy-on-write smart pointer
  ```

- **Polymorphic Deleters**: Custom deletion for interface types
  ```cpp
  template<typename Interface>
  std::unique_ptr<Interface, polymorphic_deleter> make_managed(/* ... */);
  ```

### 4. Interface Improvements

Expand interface definitions for ecosystem modules:

- **New Interfaces**: Add interfaces for new system domains
  ```cpp
  // Examples: cache_interface, scheduler_interface, registry_interface
  ```

- **Interface Extensions**: Add methods to existing interfaces
  ```cpp
  class IExecutor {
      // Existing methods
      virtual void execute(std::function<void()> task) = 0;

      // Proposed: Add batch execution
      virtual void execute_batch(
          std::vector<std::function<void()>> tasks
      ) = 0;
  };
  ```

- **Adapter Improvements**: Enhance adapter pattern for complex conversions
  ```cpp
  // Improve smart_adapter_factory for variance
  ```

### 5. Code Quality & Tooling

Help improve code quality and developer experience:

- **Clang-Tidy**: Extend static analysis configuration
- **Code Coverage**: Improve test coverage for all components
- **Documentation**: Enhance API docs and examples
- **CI/CD**: Improve GitHub Actions workflows
- **Performance**: Profile and optimize template instantiation

### 6. Performance Optimizations

For header-only code:

- **Compile-Time Optimization**: Use constexpr more effectively
  ```cpp
  // Improve error code constants
  constexpr int error_codes::INVALID_ARGUMENT = 1;
  ```

- **Template Optimization**: Reduce template instantiation bloat
  ```cpp
  // Use extern templates where appropriate
  extern template class Result<int>;
  extern template class Result<std::string>;
  ```

- **Minimal Includes**: Reduce header dependencies
  ```cpp
  // Move includes to implementation as needed
  ```

### 7. Examples & Tutorials

Create examples demonstrating library usage:

```cpp
// examples/result_pattern_demo.cpp
// - Basic Result<T> usage
// - Error handling patterns
// - Integration with exceptions

// examples/adapter_pattern_demo.cpp
// - Smart adapter factory usage
// - Custom interface implementation

// examples/event_bus_demo.cpp
// - Event publishing and subscription
// - Event filtering and routing
```

## Submitting Changes

### Pre-Submission Checklist

Before submitting a pull request:

1. **Format Code**: Run clang-format
   ```bash
   find include tests -name "*.h" -o -name "*.cpp" | xargs clang-format -i
   ```

2. **Static Analysis**: Run clang-tidy
   ```bash
   clang-tidy -p build include/**/*.h tests/**/*.cpp
   ```

3. **Compile and Test**:
   ```bash
   cmake --build build
   ctest --verbose
   ```

4. **Code Coverage**: Check coverage doesn't decrease
   ```bash
   cmake --build build -DENABLE_COVERAGE=ON
   ctest --verbose
   ```

5. **Documentation**: Update relevant documentation files
   - Add/update Doxygen comments
   - Update README.md if applicable
   - Add examples in docs/guides/

6. **License Header**: Ensure files have correct license header
   ```cpp
   // BSD 3-Clause License
   // Copyright (c) 2025, kcenon
   // See the LICENSE file in the project root for full license information.
   ```

### Creating a Pull Request

1. Fork the repository
2. Create a feature branch: `git checkout -b feature/your-feature-name`
3. Commit changes with descriptive messages
4. Push to your fork: `git push origin feature/your-feature-name`
5. Open a pull request on the main repository

### Commit Message Format

Follow conventional commits:

```
type(scope): brief description

Longer explanation of the changes, why they're needed, and any important
implementation details or considerations.

Closes #123
```

Examples:
- `feat(result): add map_error method for error transformation`
- `fix(adapter): prevent unnecessary wrapper creation`
- `docs(error-handling): improve exception mapping documentation`
- `test(result): add template specialization tests`
- `refactor(patterns): simplify error code hierarchy`

## Code Review Process

### Review Criteria

All submissions are reviewed based on:

1. **Correctness**: Code works as intended, no undefined behavior
2. **Header-Only Compliance**: All template code in headers, no .cpp implementations
3. **C++17 Compatibility**: Works with C++17, C++20 support is optional
4. **Documentation**: Complete Doxygen comments with examples
5. **Testing**: Comprehensive tests with good coverage
6. **Performance**: No unnecessary runtime overhead
7. **Style**: Follows project conventions and clang-tidy rules
8. **Thread Safety**: Clear documentation of thread safety guarantees

### Review Feedback

Reviewers may request:

- Additional tests or test cases
- Documentation improvements
- Refactoring for clarity
- Performance optimizations
- Breaking change justifications

### Timeline

- Initial feedback: within 2-5 business days
- Revised submissions: reviewed within 2-3 business days
- Merge after approval: immediate

## Getting Help

- **Issues**: File bugs or feature requests on GitHub
- **Discussions**: Use GitHub Discussions for design questions
- **Documentation**: See docs/ folder for comprehensive guides
  - [Architecture Guide](../ARCHITECTURE.md)
  - [Error Handling](../ERROR_HANDLING.md)
  - [RAII Guidelines](../RAII_GUIDELINES.md)
  - [Smart Pointer Guidelines](../SMART_POINTER_GUIDELINES.md)

## License

By submitting code to common_system, you agree that your contributions will be licensed under the BSD 3-Clause License. See LICENSE file for details.

---

Thank you for contributing to common_system! Your effort helps make this foundational library better for everyone.
