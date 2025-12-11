# Troubleshooting Guide - common_system

> **Language:** **English** | [한국어](TROUBLESHOOTING_KO.md)

A comprehensive troubleshooting guide for the common_system header-only library. This document covers the most common issues encountered when using, integrating, and debugging header-only C++ libraries.

**Version:** 0.2.0 | **Last Updated:** November 2025

---

## Table of Contents

1. [Compilation Errors](#compilation-errors)
2. [Linker Issues](#linker-issues)
3. [IDE and IntelliSense Problems](#ide-and-intellisense-problems)
4. [Integration Issues](#integration-issues)
5. [Result<T> Usage Errors](#resultt-usage-errors)
6. [Template Instantiation Problems](#template-instantiation-problems)
7. [Platform-Specific Issues](#platform-specific-issues)
8. [Debugging Techniques](#debugging-techniques)

---

## Compilation Errors

### Error: "unknown type name 'Result'" or "no member named 'is_ok'"

**Symptoms:**
```
error: unknown type name 'Result'
error: no member named 'is_ok' in 'Result<int>'
error: no matching function for call to 'Result<int>::ok'
```

**Root Causes:**
- Missing or incorrect include directive
- Namespace qualification issue
- Incorrect C++ standard setting

**Solution:**

Ensure you include the correct header and use proper namespace qualification:

```cpp
// CORRECT
#include <kcenon/common/patterns/result.h>
using namespace kcenon::common;

Result<int> value = Result<int>::ok(42);
if (value.is_ok()) {
    std::cout << value.value() << std::endl;
}

// OR without 'using namespace'
#include <kcenon/common/patterns/result.h>

kcenon::common::Result<int> value = kcenon::common::Result<int>::ok(42);
if (value.is_ok()) {
    std::cout << value.value() << std::endl;
}
```

For all common_system features, use the aggregate header:

```cpp
// Include everything
#include <kcenon/common/common.h>
using namespace kcenon::common;
```

**Verification:**
```bash
# Check compilation
g++ -std=c++17 -I/path/to/common_system/include -c test.cpp

# Verify C++17 standard is enabled
g++ --version
# Output should support C++17
```

---

### Error: "C++ standard must be C++17 or later"

**Symptoms:**
```
error: C++17 standard required for 'std::optional'
error: 'std::variant' not declared in this scope
error: use of undeclared identifier 'constexpr'
```

**Root Causes:**
- C++ standard not set to C++17 or higher
- Compiler flags not properly configured
- CMake configuration incomplete

**Solution:**

**For command-line compilation:**
```bash
# GCC/Clang
g++ -std=c++17 -I/path/to/common_system/include main.cpp -o myapp
clang++ -std=c++17 -I/path/to/common_system/include main.cpp -o myapp

# MSVC
cl /std:c++17 /I"C:\path\to\common_system\include" main.cpp

# Check compiler version
g++ --version
clang++ --version
cl /?  # MSVC
```

**For CMake projects:**
```cmake
cmake_minimum_required(VERSION 3.16)
project(MyProject)

# Set C++17 as required
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Add common_system
target_include_directories(myapp PRIVATE /path/to/common_system/include)

# Explicitly set for target
target_compile_features(myapp PUBLIC cxx_std_17)
```

**Minimum compiler versions:**
- GCC: 7.0+
- Clang: 5.0+
- MSVC: 2017 (v141)+
- AppleClang: 10.0+

---

### Error: "'std::monostate' is not a member of std"

**Symptoms:**
```
error: 'std::monostate' is not a member of std
error: no member named 'monostate' in namespace 'std'
```

**Root Cause:**
C++17 feature not available in your compiler.

**Solution:**

Use C++17 explicitly:

```cmake
# CMakeLists.txt
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
```

Or check what `VoidResult` is defined as:

```cpp
#include <kcenon/common/patterns/result.h>

// VoidResult is Result<std::monostate> when C++17+
using namespace kcenon::common;

// VoidResult requires C++17
VoidResult success = ok();
VoidResult failure = make_error<std::monostate>(
    error_codes::INVALID_ARGUMENT,
    "Failed operation",
    "my_module"
);
```

---

### Error: "undefined reference to non-header definitions" in result.h

**Symptoms:**
```
undefined reference to `kcenon::common::get_value'
undefined reference to `error_codes::INVALID_ARGUMENT'
```

**Root Cause:**
Attempting to use function from a header that is header-only. All implementations are in headers - no .cpp files are required.

**Solution:**

All common_system code is in headers. Verify your includes use only headers:

```cpp
// CORRECT - all in headers
#include <kcenon/common/patterns/result.h>
#include <kcenon/common/error/error_codes.h>
#include <kcenon/common/patterns/result_helpers.h>

// Not required (and doesn't exist)
// #include <kcenon/common/src/result.cpp>  // ← WRONG
```

If you get undefined reference errors, check:
1. Include paths are correct
2. You're not looking for .cpp or .lib files
3. The header file exists in `include/kcenon/common/`

---

### Error: "template instantiation depth" or "instantiation recursion"

**Symptoms:**
```
fatal error: template instantiation depth exceeded
error: recursive template instantiation exceeded maximum depth
error: too many nested type instantiations
```

**Root Cause:**
Complex nested Result types or circular template dependencies.

**Solution:**

Avoid deeply nested Result types:

```cpp
// WRONG - deeply nested
Result<Result<Result<int>>> deeply_nested;

// CORRECT - flatten with and_then
Result<int> operation1();
Result<int> operation2(int);

auto result = operation1()
    .and_then([](int v1) {
        return operation2(v1);
    });
```

Use type aliases for complex templates:

```cpp
// Define a clear type alias first
using IntVector = std::vector<int>;
using IntVectorResult = Result<IntVector>;

// Now use it
IntVectorResult get_numbers() {
    IntVector data;
    data.push_back(1);
    return IntVectorResult::ok(std::move(data));
}
```

For debugging, increase verbosity:

```bash
# GCC/Clang - show full instantiation path
g++ -std=c++17 -ftemplate-backtrace-limit=0 main.cpp -o myapp

# MSVC - show template details
cl /std:c++17 /diagnostics:caret main.cpp
```

---

## Linker Issues

### Understanding: "Why am I getting linker errors in a header-only library?"

**Important Note:** Header-only libraries should NOT produce linker errors. If you encounter linker errors, it indicates a misconfiguration.

**Typical Causes:**
1. Missing include guard or pragma once
2. Attempting to link against a library that doesn't exist
3. Incorrect CMake configuration
4. Symbol name mangling issues with different compilation units

---

### Error: "undefined reference to 'kcenon::common::...'" in header-only code

**Symptoms:**
```
/usr/bin/ld: undefined reference to `kcenon::common::Result<int>::ok'
collect2: error: ld returned 1 exit status
```

**Root Cause:**
Common_system provides header-only implementations. If you get linker errors, verify:

1. All headers are truly header-only (contain only inline/template code)
2. You're not accidentally compiling a .cpp file that calls external functions

**Solution:**

Verify that all code is in headers:

```cpp
// Check what you're including
#include <kcenon/common/patterns/result.h>
// This should ONLY contain class definitions and inline functions

// If linker error occurs, verify no external linking is needed:
int main() {
    // This should compile and link with ONLY headers
    auto result = kcenon::common::Result<int>::ok(42);
    return result.is_ok() ? 0 : 1;
}
```

Compile with verbose output to see what's happening:

```bash
# Show all symbols being linked
g++ -std=c++17 -v -I/path/to/common_system/include main.cpp -o myapp

# Check for undefined symbols
nm -u ./myapp | grep kcenon
```

---

### Error: "multiple definition of..." in header files

**Symptoms:**
```
error: multiple definition of 'kcenon::common::version_info::string'
```

**Root Cause:**
Header file is missing `inline` keyword or pragma once.

**Solution:**

All common_system headers use `#pragma once` to prevent multiple inclusions:

```cpp
// At the top of every header
#pragma once

// Rest of header content...
```

If this error occurs in YOUR code using common_system, ensure your headers also use it:

```cpp
// myheader.h
#pragma once

#include <kcenon/common/patterns/result.h>

// Your definitions here (use inline if needed)
inline kcenon::common::Result<int> my_function() {
    return kcenon::common::Result<int>::ok(42);
}
```

---

## IDE and IntelliSense Problems

### Issue: "Squiggly lines" in IDE despite code compiling correctly

**Symptoms:**
- Red underlines under class names and functions
- "Cannot find include file" errors in IntelliSense
- Auto-completion not working for Result<T>
- Syntax highlighting incorrect

**Root Cause:**
IDE's IntelliSense engine is out of sync with compiler configuration.

**Solution:**

**For Visual Studio Code:**

1. Install the C++ extension (ms-vscode.cpptools)
2. Create `.vscode/c_cpp_properties.json`:

```json
{
    "configurations": [
        {
            "name": "Linux",
            "includePath": [
                "${workspaceFolder}",
                "/path/to/common_system/include",
                "/usr/include"
            ],
            "defines": [],
            "compilerPath": "/usr/bin/g++",
            "cStandard": "c17",
            "cppStandard": "c++17",
            "intelliSenseMode": "gcc-x64",
            "compileCommands": "${workspaceFolder}/build/compile_commands.json"
        },
        {
            "name": "macOS",
            "includePath": [
                "${workspaceFolder}",
                "/path/to/common_system/include"
            ],
            "defines": [],
            "macFrameworkPath": [
                "/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/System/Library/Frameworks"
            ],
            "compilerPath": "/usr/bin/clang++",
            "cppStandard": "c++17",
            "intelliSenseMode": "clang-x64"
        },
        {
            "name": "Windows",
            "includePath": [
                "${workspaceFolder}",
                "C:/path/to/common_system/include"
            ],
            "defines": [
                "_DEBUG",
                "UNICODE",
                "_UNICODE"
            ],
            "compilerPath": "cl.exe",
            "cppStandard": "c++17",
            "intelliSenseMode": "msvc-x64"
        }
    ],
    "version": 4
}
```

3. Reload IntelliSense:
   - Command palette: `C++: Rescan Solution`
   - Or restart VSCode

**For Visual Studio:**

1. Right-click project → Properties
2. VC++ Directories → Include Directories
3. Add: `/path/to/common_system/include`
4. C/C++ → Language → C++ Standard: `ISO C++17 (/std:c++17)`
5. Edit → IntelliSense → Rescan Solution

**For CLion/JetBrains:**

1. File → Settings → Languages & Frameworks → C/C++
2. Add include path: `/path/to/common_system/include`
3. Set C++ standard: `C++17`
4. File → Invalidate Caches → Invalidate and Restart

**For GCC/Clang command-line:**

Ensure compile_commands.json is generated:

```cmake
# In CMakeLists.txt
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
```

---

### Issue: "Template resolution failure" in editor but code compiles

**Symptoms:**
- Editor can't find Result<T> methods
- Auto-completion shows incomplete suggestions
- Hover-over information is missing or incorrect

**Solution:**

This is common with header-only libraries due to template complexity. Solutions:

1. **Force reparse of includes:**
   - VSCode: `Ctrl+Shift+P` → `C++: Rescan Solution`
   - CLion: File → Invalidate Caches
   - Visual Studio: Edit → IntelliSense → Rescan Solution

2. **Update IntelliSense engine:**
   - VSCode: Set `"C_Cpp.intelliSenseEngine": "Disabled"` then re-enable
   - Visual Studio: Update to latest version

3. **Simplify your includes:**
   ```cpp
   // Use aggregate header instead of individual headers
   #include <kcenon/common/common.h>  // Includes all components

   // Instead of
   // #include <kcenon/common/patterns/result.h>
   // #include <kcenon/common/error/error_codes.h>
   // #include <kcenon/common/patterns/result_helpers.h>
   ```

---

### Issue: Auto-completion not working for Result<T>

**Symptoms:**
- Typing `result.` doesn't show methods
- Constructor suggestions missing
- Error propagation helper suggestions absent

**Solution:**

1. Verify Result<T> is correctly instantiated:
   ```cpp
   // Make type explicit for better IntelliSense
   kcenon::common::Result<int> my_result;  // Not just 'auto'
   ```

2. Use type aliases for complex types:
   ```cpp
   using ResultInt = kcenon::common::Result<int>;
   using ResultStr = kcenon::common::Result<std::string>;

   // Now IntelliSense works better
   ResultInt get_number() { return ResultInt::ok(42); }
   ```

3. Check that template member visibility is enabled in IDE settings

---

## Integration Issues

### Error: "CMake cannot find common_system"

**Symptoms:**
```
CMake Error: No such file or directory when looking for 'common_system'
CMake Error: Could not find a package configuration file
```

**Solution:**

**Option 1: Direct include (simplest for header-only):**

```cmake
cmake_minimum_required(VERSION 3.16)
project(MyProject)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Directly add include path
include_directories(/path/to/common_system/include)

add_executable(myapp main.cpp)
target_include_directories(myapp PRIVATE /path/to/common_system/include)
```

**Option 2: Using find_package (if installed):**

```cmake
find_package(common_system REQUIRED)

add_executable(myapp main.cpp)
target_link_libraries(myapp PRIVATE kcenon::common)
```

First, install common_system:

```bash
cd /path/to/common_system
mkdir build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --install .
```

**Option 3: As Git submodule:**

```bash
git submodule add https://github.com/kcenon/common_system.git third_party/common_system
```

Then in CMakeLists.txt:

```cmake
add_subdirectory(third_party/common_system)

add_executable(myapp main.cpp)
target_link_libraries(myapp PRIVATE kcenon::common)
```

---

### Error: Include path issues when using relative paths

**Symptoms:**
```
fatal error: kcenon/common/patterns/result.h: No such file or directory
error: cannot find include file
```

**Solution:**

Always use absolute paths or CMAKE_SOURCE_DIR:

```cmake
# WRONG
include_directories(../../common_system/include)

# CORRECT
include_directories(${CMAKE_SOURCE_DIR}/../common_system/include)

# OR better
include_directories(/path/to/common_system/include)

# OR with relative path variable
set(COMMON_SYSTEM_DIR "${CMAKE_SOURCE_DIR}/../common_system")
include_directories(${COMMON_SYSTEM_DIR}/include)
```

Verify the path exists:

```bash
# Check that headers exist
ls /path/to/common_system/include/kcenon/common/
# Should show: adapters/, error/, interfaces/, patterns/, utils/, common.h
```

---

### Error: "target_link_libraries(...) header-only target cannot be linked"

**Symptoms:**
```
CMake Warning: Cannot add compile dependencies to INTERFACE library
```

**Root Cause:**
Trying to link a header-only library with target_link_libraries for compilation.

**Solution:**

Header-only libraries should use only include directories:

```cmake
# WRONG (for header-only libraries)
target_link_libraries(myapp PRIVATE common_system)

# CORRECT
target_include_directories(myapp PRIVATE /path/to/common_system/include)

# OR if using alias
target_include_directories(myapp PRIVATE ${COMMON_SYSTEM_INCLUDE_DIR})
```

---

## Result<T> Usage Errors

### Error: Type mismatch - "no matching function for call to Result<T>::ok"

**Symptoms:**
```
error: no matching function for call to 'Result<int>::ok'
error: invalid initialization of reference of type 'const int&' from an rvalue of type 'std::string'
```

**Root Cause:**
Trying to create Result<T> with incompatible type.

**Solution:**

Ensure types match exactly:

```cpp
using namespace kcenon::common;

// WRONG
Result<int> get_value() {
    std::string result = "42";
    return Result<int>::ok(result);  // ← Type error! string ≠ int
}

// CORRECT
Result<int> get_value() {
    std::string result = "42";
    return Result<int>::ok(std::stoi(result));  // Convert to int
}

// Or return different type
Result<std::string> get_value_as_string() {
    std::string result = "42";
    return Result<std::string>::ok(result);  // ← Correct type
}
```

**For complex types, use type aliases:**

```cpp
using IntVector = std::vector<int>;
using IntVectorResult = Result<IntVector>;

IntVectorResult get_numbers() {
    IntVector data;
    data.push_back(1);
    data.push_back(2);
    return IntVectorResult::ok(std::move(data));
}
```

---

### Error: "error: Result<T> declaration requires a type argument"

**Symptoms:**
```
error: template argument missing for template parameter 'T'
error: Result<T> instantiation requires a type
```

**Solution:**

Always provide explicit type parameter:

```cpp
// WRONG
auto result = Result::ok(42);  // ← Missing type!

// CORRECT
auto result = Result<int>::ok(42);  // ← Type specified

// OR with explicit type
Result<int> result = Result<int>::ok(42);

// Type deduction works with structured bindings
if (auto r = Result<int>::ok(42); r.is_ok()) {
    int value = r.value();
}
```

---

### Error: "Attempting to call value() on error result"

**Symptoms:**
Runtime crash or exception when accessing error Result's value:

```cpp
Result<int> result = Result<int>::err(
    error_codes::NOT_FOUND,
    "Item not found",
    "database"
);

// This will crash or throw!
int value = result.value();  // ← Undefined behavior!
```

**Solution:**

Always check state before accessing value:

```cpp
using namespace kcenon::common;

Result<int> operation();

// Pattern 1: Check before access
auto result = operation();
if (result.is_ok()) {
    std::cout << "Value: " << result.value() << std::endl;
} else {
    std::cout << "Error: " << result.error().message << std::endl;
}

// Pattern 2: Use get_if_ok
if (auto* value = get_if_ok(result)) {
    std::cout << "Value: " << *value << std::endl;
}

// Pattern 3: Use unwrap_or with default
int safe_value = result.unwrap_or(-1);  // Returns -1 if error

// Pattern 4: Use map to transform safely
auto mapped = result.map([](int v) { return v * 2; });
```

---

### Error: Error code type mismatch

**Symptoms:**
```
error: no matching function for call to 'error_info' with arguments '(double, const string&, const string&)'
```

**Solution:**

Error codes must be `int`. Use constants from error_codes.h:

```cpp
#include <kcenon/common/error/error_codes.h>
using namespace kcenon::common;

// CORRECT - use int error codes
auto err = error_info(
    error_codes::INVALID_ARGUMENT,  // This is int
    "Invalid input",
    "parser"
);

// WRONG - don't use enum values
// error_info(-1.5, "msg", "module");  // ← Wrong type!

// Available error codes
// error_codes::SUCCESS              // 0
// error_codes::INVALID_ARGUMENT     // -1
// error_codes::NOT_FOUND            // -2
// error_codes::PERMISSION_DENIED    // -3
// error_codes::TIMEOUT              // -4
// error_codes::CANCELLED            // -5
// error_codes::NOT_INITIALIZED      // -6
// error_codes::ALREADY_EXISTS       // -7
// error_codes::OUT_OF_MEMORY        // -8
// error_codes::IO_ERROR             // -9
// error_codes::NETWORK_ERROR        // -10
// error_codes::INTERNAL_ERROR       // -99
```

---

### Error: "COMMON_ASSIGN_OR_RETURN macro not found"

**Symptoms:**
```
error: COMMON_ASSIGN_OR_RETURN was not declared in this scope
```

**Solution:**

The macro is available in result_helpers.h:

```cpp
#include <kcenon/common/patterns/result_helpers.h>
using namespace kcenon::common;

Result<int> process(const std::string& input) {
    // Using the macro
    COMMON_ASSIGN_OR_RETURN(auto parsed, parse_integer(input));
    COMMON_ASSIGN_OR_RETURN(auto validated, validate(parsed));

    return Result<int>::ok(validated);
}
```

Or implement manually without macro:

```cpp
Result<int> process(const std::string& input) {
    auto parsed = parse_integer(input);
    if (parsed.is_err()) {
        return parsed.error();
    }

    auto validated = validate(parsed.value());
    if (validated.is_err()) {
        return validated.error();
    }

    return Result<int>::ok(validated.value());
}
```

---

## Template Instantiation Problems

### Issue: Explicit template instantiation vs implicit

**Symptoms:**
Complex error messages with multiple template instantiation paths.

**Solution:**

Use implicit instantiation (most common for header-only):

```cpp
// IMPLICIT - let compiler deduce types
kcenon::common::Result<int> get_int() {
    return kcenon::common::Result<int>::ok(42);
}

// For complex types, help the compiler
template<typename T>
kcenon::common::Result<std::vector<T>> get_vector() {
    std::vector<T> data;
    data.push_back(T{});
    return kcenon::common::Result<std::vector<T>>::ok(std::move(data));
}

// Usage
auto int_result = get_int();           // Works
auto vec_result = get_vector<int>();  // Must specify type explicitly
```

---

### Error: "undefined symbol in template class"

**Symptoms:**
```
error: undefined reference to 'void kcenon::common::Result<MyClass>::value()'
```

**Root Cause:**
MyClass is incomplete or forward-declared.

**Solution:**

Ensure the type is fully defined before using in Result<T>:

```cpp
// WRONG
class MyClass;  // Forward declaration
Result<MyClass> get_instance();  // ← Error! Type incomplete

// CORRECT
class MyClass {  // Full definition
public:
    int value;
};

Result<MyClass> get_instance() {
    MyClass obj;
    obj.value = 42;
    return Result<MyClass>::ok(obj);
}
```

For template types, provide instantiation:

```cpp
template<typename T>
Result<std::vector<T>> process_data(const std::vector<T>& input) {
    std::vector<T> result;
    // Process...
    return Result<std::vector<T>>::ok(result);
}

// Instantiate explicitly
template Result<std::vector<int>> process_data(const std::vector<int>&);
template Result<std::vector<std::string>> process_data(const std::vector<std::string>&);
```

---

## Platform-Specific Issues

### macOS: Clang version incompatibility

**Symptoms:**
```
error: C++17 standard not supported by this version of clang
```

**Solution:**

Update Xcode:

```bash
# Check current version
clang++ --version

# Update Xcode
xcode-select --install

# Or via App Store for latest
# App Store → Xcode → Update
```

Specify C++ standard explicitly:

```bash
clang++ -std=c++17 -stdlib=libc++ -I/path/to/common_system/include main.cpp -o myapp
```

---

### Windows: MSVC incompatible standard flag

**Symptoms:**
```
error: unknown option -std=c++17
error: /std:c++17 not recognized
```

**Solution:**

Use MSVC-specific flags:

```batch
REM MSVC uses /std:c++17 not -std=c++17
cl /std:c++17 /I"C:\path\to\common_system\include" main.cpp

REM Or in CMake
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
```

For older Visual Studio, ensure you're using 2017 or later:

```batch
REM Check MSVC version
cl /?
REM Should show version 191 or higher (2017+)
```

---

### Linux: GCC version too old

**Symptoms:**
```
error: 'std::optional' is not a member of 'std'
error: 'std::variant' is not a member of 'std'
```

**Solution:**

Update GCC to version 7.0 or later:

```bash
# Check current version
g++ --version

# Ubuntu/Debian
sudo apt-get install g++-7 g++-8 g++-9 g++-10

# Use specific version
g++-7 -std=c++17 main.cpp -o myapp

# Or set as default
sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-7 100
```

---

### Cross-platform: Include path separators

**Symptoms:**
```
error: kcenon\common\patterns\result.h: No such file or directory (Windows)
error: kcenon/common/patterns/result.h: No such file or directory (Linux/macOS)
```

**Solution:**

Use forward slashes in includes (portable across all platforms):

```cpp
// CORRECT - forward slashes work everywhere
#include <kcenon/common/patterns/result.h>

// WRONG for portability (but works on Windows)
// #include <kcenon\common\patterns\result.h>
```

For CMake paths, use forward slashes:

```cmake
# CORRECT
set(COMMON_SYSTEM_DIR "${CMAKE_SOURCE_DIR}/../common_system")

# NOT
set(COMMON_SYSTEM_DIR "${CMAKE_SOURCE_DIR}\..\common_system")
```

---

## Debugging Techniques

### Technique 1: Enable verbose compiler output

**GCC/Clang:**
```bash
g++ -std=c++17 -v -I/path/to/common_system/include main.cpp -o myapp
g++ -std=c++17 -ftemplate-backtrace-limit=0 main.cpp -o myapp  # Show full template path
```

**MSVC:**
```batch
cl /std:c++17 /diagnostics:caret main.cpp  # Show detailed diagnostics
cl /std:c++17 /showIncludes main.cpp       # Show all included files
```

---

### Technique 2: Create minimal reproduction

Isolate the issue to minimum code:

```cpp
#include <kcenon/common/patterns/result.h>
using namespace kcenon::common;

int main() {
    // Minimal test of the issue
    Result<int> r = Result<int>::ok(42);
    return r.is_ok() ? 0 : 1;
}
```

Compile this alone to verify basic setup works.

---

### Technique 3: Use static_assert for compile-time debugging

```cpp
#include <kcenon/common/patterns/result.h>
#include <type_traits>

using namespace kcenon::common;

// Verify type relationships
static_assert(std::is_default_constructible_v<Result<int>>, "Result<int> should be default constructible");
static_assert(std::is_copy_constructible_v<Result<int>>, "Result<int> should be copy constructible");
static_assert(std::is_move_constructible_v<Result<int>>, "Result<int> should be move constructible");

// Verify error_info
static_assert(std::is_constructible_v<error_info, int, std::string, std::string>,
              "error_info should be constructible from int, string, string");
```

---

### Technique 4: Use string representation for debugging

```cpp
#include <kcenon/common/patterns/result.h>
#include <iostream>

using namespace kcenon::common;

// Print error details
void debug_error(const Result<int>& result) {
    if (result.is_err()) {
        const auto& err = result.error();
        std::cout << "Error:\n"
                  << "  Code: " << err.code << "\n"
                  << "  Message: " << err.message << "\n"
                  << "  Module: " << err.module << "\n";
        if (err.details.has_value()) {
            std::cout << "  Details: " << err.details.value() << "\n";
        }
    } else {
        std::cout << "Success: " << result.value() << "\n";
    }
}
```

---

### Technique 5: Enable compiler warnings

```bash
# GCC/Clang - maximum warnings
g++ -std=c++17 -Wall -Wextra -Wpedantic -Wconversion -Wshadow \
    -I/path/to/common_system/include main.cpp -o myapp

# MSVC - level 4 warnings
cl /std:c++17 /W4 main.cpp
```

---

## Common Error Codes Reference

Quick reference for standard error codes:

```cpp
#include <kcenon/common/error/error_codes.h>
using namespace kcenon::common;

// Common errors (0 to -99)
error_codes::SUCCESS              // 0 - Success
error_codes::INVALID_ARGUMENT     // -1 - Invalid argument
error_codes::NOT_FOUND            // -2 - Not found
error_codes::PERMISSION_DENIED    // -3 - Permission denied
error_codes::TIMEOUT              // -4 - Timeout
error_codes::CANCELLED            // -5 - Cancelled
error_codes::NOT_INITIALIZED      // -6 - Not initialized
error_codes::ALREADY_EXISTS       // -7 - Already exists
error_codes::OUT_OF_MEMORY        // -8 - Out of memory
error_codes::IO_ERROR             // -9 - I/O error
error_codes::NETWORK_ERROR        // -10 - Network error
error_codes::INTERNAL_ERROR       // -99 - Internal error

// Thread system errors (-100 to -199)
error_codes::THREAD_ERROR_BASE    // -100 - Base for thread errors

// Logger system errors (-200 to -299)
error_codes::LOGGER_ERROR_BASE    // -200 - Base for logger errors

// Monitoring system errors (-300 to -399)
error_codes::MONITORING_ERROR_BASE // -300 - Base for monitoring errors
```

---

## Getting Help

If you encounter issues not covered here:

1. **Check the FAQ:** [FAQ.md](FAQ.md) - Common questions and answers
2. **Review examples:** See `/examples` directory for working code
3. **Check architecture:** [ARCHITECTURE.md](../ARCHITECTURE.md) - System design details
4. **GitHub Issues:** https://github.com/kcenon/common_system/issues
5. **Error messages:** Search for exact error text in documentation

---

**Questions or issues?** Please open a GitHub issue with:
- Minimal code reproduction
- Compiler version output
- Full error message
- Your CMakeLists.txt or build command

