---
doc_id: "COM-GUID-028"
doc_title: "C++20 Modules: Dual-Build Strategy"
doc_version: "1.0.0"
doc_date: "2026-09-12"
doc_status: "Released"
project: "common_system"
category: "GUID"
---

# C++20 Modules: Dual-Build Strategy

> **Language:** **English** | See also: [Module Migration Guide](MODULE_MIGRATION.md)

## Overview

common_system provides **header-only** mode by default and experimental
**C++20 modules** as an opt-in source build. Modules require a supported compiler
and its dependency scanner (for example, `clang-scan-deps` beside `clang++`).
The table describes current source targets; the published v0.2.0 FetchContent
example uses `kcenon::common`, as shown in the [README](../../README.md#getting-started).

| Mode | CMake Target | Import Style | Default |
|------|-------------|-------------|---------|
| **Header-only** | `kcenon::common` / `kcenon::common_system` | `#include <kcenon/common/...>` | Yes |
| **C++20 Modules** | `kcenon::common_modules` | `import kcenon.common;` | No |

## When to Use Each Mode

**Use header-only (default)** when:

- Your toolchain does not meet the module requirements below
- You target macOS with Apple Clang
- You need sanitizer builds (ASan, TSan, UBSan) — module support under sanitizers
  is not yet validated in CI
- You are distributing a library consumed by projects with varying toolchains
- You want to use headers with your existing build generator

**Use C++20 modules** when:

- You have a supported compiler and CMake 3.28+
- You use the Ninja or Visual Studio generator
- You want to evaluate compilation behavior with separately compiled module units
- You want stronger encapsulation (non-exported symbols are not visible)

## Compiler and CMake Matrix

### Minimum Requirements

| Compiler | Header-Only | C++20 Modules | Notes |
|----------|-------------|---------------|-------|
| GCC | 11+ | **14+** | Modules require `-std=c++20` and `CXX_SCAN_FOR_MODULES` |
| Clang | 14+ | **16+** | Recommended for modules; best scanning support |
| MSVC | 2022 (19.30+) | **2022 17.4 (19.34+)** | Requires Visual Studio generator |
| Apple Clang | 14+ | **Not supported** | No module dependency scanning |

### CMake and Generator Requirements

| Requirement | Header-Only | C++20 Modules |
|-------------|-------------|---------------|
| CMake version | 3.20+ | **3.28+** |
| Generator | Any | **Ninja** or **Visual Studio** |
| `CXX_SCAN_FOR_MODULES` | N/A | Set automatically |

The header CMake minimum above applies to current source. The published v0.2.0
release requires CMake 3.28+ even in header mode. Downstream compiler requirements
are listed in the [setup guide](QUICK_START.md#compiler-requirements).

## Enabling Module Builds

```bash
# Configure with module support
cmake -G Ninja -B build \
    -DCOMMON_BUILD_MODULES=ON \
    -DCMAKE_CXX_COMPILER=clang++

# Build
cmake --build build
```

### CMake Option

```cmake
option(COMMON_BUILD_MODULES "Build C++20 module version of common_system" OFF)
```

When enabled, CMake performs these checks at configure time:

1. **CMake version** >= 3.28 — if not, disables with a warning
2. **Compiler** is not Apple Clang — if so, disables with a warning
3. **Generator** is Ninja or Visual Studio — if not, disables with a warning
4. **Compiler version** meets module minimums — checked via `kcenon_check_compiler_requirements(MODULES)`; a version below the minimum is a configuration error

The first three checks disable module creation with a warning. Compiler-version
failure stops configuration. Missing scanners can also prevent generation or
building. Inspect [common-modules.cmake](../../cmake/common-modules.cmake) for the checks.

### Consuming the Module Target

This pins an existing source commit with module support, rather than the staged,
unpublished v1.0.0. Save the following files as `CMakeLists.txt` and `main.cpp`.

```cmake
cmake_minimum_required(VERSION 3.28)
project(my_project CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
set(COMMON_BUILD_MODULES ON CACHE BOOL "" FORCE)
set(COMMON_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(COMMON_BUILD_INTEGRATION_TESTS OFF CACHE BOOL "" FORCE)
set(COMMON_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(COMMON_BUILD_BENCHMARKS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    common_system
    GIT_REPOSITORY https://github.com/kcenon/common_system.git
    GIT_TAG 8558128033457ffb4635f564b0d495dddd6f8807
)
FetchContent_MakeAvailable(common_system)

if(NOT TARGET kcenon::common_modules)
    message(FATAL_ERROR "This example requires the module target; check the toolchain")
endif()
add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE kcenon::common_modules)
```

```cpp
// main.cpp — module usage
import kcenon.common;

int main() {
    auto result = kcenon::common::ok(42);
    return result.is_ok() && result.value() == 42 ? 0 : 1;
}
```

Configure with Ninja and your upstream Clang installation, whose `clang-scan-deps`
must be available, then build and run:

```bash
cmake -S . -B build -G Ninja -DCMAKE_CXX_COMPILER=/path/to/llvm/bin/clang++
cmake --build build --target my_app
./build/my_app
```

Replace `/path/to/llvm` with your compiler installation. The consumer and module
both use C++20 with extensions disabled. A header fallback cannot satisfy this
example: it explicitly requires the module target and compiles an `import`.

## Fallback Behavior

The dual-build strategy is designed so that **header-only is always available**:

```
cmake configure
    │
    ├─ COMMON_BUILD_MODULES=OFF (default)
    │   └─ Header-only target only
    │
    └─ COMMON_BUILD_MODULES=ON
        │
        ├─ All checks pass
        │   └─ Both targets available:
        │       kcenon::common (headers)
        │       kcenon::common_modules (modules)
        │
        └─ CMake / Apple Clang / generator check fails
            └─ Warning printed, COMMON_BUILD_MODULES set to OFF
                └─ Header-only target only (graceful fallback)
```

Key points:

- The header-only target (`kcenon::common`) is **always** built regardless of module settings
- Enabling modules adds `kcenon::common_modules` as an **additional** target
- CMake / Apple Clang / generator rejection produces a warning and leaves only headers;
  an insufficient compiler version is fatal, and an absent scanner can fail the build
- You can mix headers and modules in different parts of the same project

## Known Limitations

### Apple Clang

Apple Clang (shipped with Xcode) does not implement C++20 module dependency scanning.
On macOS, use header-only mode or install upstream Clang via Homebrew:

```bash
brew install llvm
cmake -G Ninja -B build \
    -DCOMMON_BUILD_MODULES=ON \
    -DCMAKE_CXX_COMPILER=$(brew --prefix llvm)/bin/clang++
```

### Sanitizers (ASan, TSan, UBSan)

C++20 modules under sanitizer instrumentation have known issues across compilers:

- **GCC 14**: Module BMI (Binary Module Interface) files may not carry sanitizer
  metadata correctly, causing false positives or missed detections
- **Clang 16-17**: Sanitizer passes may conflict with module precompilation;
  Clang 18+ improves this but full support is still evolving
- **MSVC**: Sanitizer support with modules is experimental

**Recommendation**: Use header-only mode for sanitizer builds. The project's sanitizer
CMake presets (`asan`, `tsan`, `ubsan`) use header-only mode by default.

### Generator Constraint

Only the **Ninja** and **Visual Studio** generators support `CXX_SCAN_FOR_MODULES`.
Unix Makefiles and other generators will trigger a fallback to header-only mode.

### Mixing Headers and Modules

While you can use headers in some translation units and modules in others within the
same project, avoid importing the module and including headers for the same symbols
in a single translation unit. This can cause ODR (One Definition Rule) violations.

## Related Documentation

- [Module Migration Guide](MODULE_MIGRATION.md) — Step-by-step migration from headers to modules
- [Architecture Overview](../ARCHITECTURE.md) — System design and module structure
- [Quick Start Guide](QUICK_START.md) — Getting started with common_system
