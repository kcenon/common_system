# C++20 Module Migration Guide

**Guide for using C++20 modules** with common_system instead of traditional header includes.

> **Language:** **English** | [한국어](MODULE_MIGRATION_KO.md)

## Overview

C++20 modules provide an alternative to header-only usage with faster compilation times and better encapsulation. This guide explains how to migrate from headers to modules.

## Prerequisites

- **CMake**: 3.28 or later
- **Generator**: Ninja (required for module support)
- **Compiler**: One of the following:
  - Clang 16+ (recommended)
  - GCC 14+
  - MSVC 2022 17.4+

> **Note**: AppleClang does not fully support C++20 modules yet. Use header-only mode on macOS.

## Quick Migration

### Before (Headers)

```cpp
#include <kcenon/common/patterns/result.h>
#include <kcenon/common/interfaces/executor_interface.h>
#include <kcenon/common/patterns/event_bus.h>

using namespace kcenon::common;

int main() {
    auto result = ok(42);
    // ...
}
```

### After (Modules)

```cpp
import kcenon.common;

using namespace kcenon::common;

int main() {
    auto result = ok(42);
    // ...
}
```

## Building with Modules

### CMake Configuration

```bash
# Configure with module support enabled
cmake -G Ninja -B build \
    -DCOMMON_BUILD_MODULES=ON \
    -DCMAKE_CXX_COMPILER=clang++

# Build
cmake --build build
```

### Linking to Module Target

```cmake
# In your CMakeLists.txt
cmake_minimum_required(VERSION 3.28)
project(my_project CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Fetch common_system
include(FetchContent)
FetchContent_Declare(
    common_system
    GIT_REPOSITORY https://github.com/kcenon/common_system.git
    GIT_TAG main
)
FetchContent_MakeAvailable(common_system)

# Link to module target (when building with modules)
add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE kcenon::common_modules)
```

## Module Structure

The `kcenon.common` module is organized in tiers:

```
kcenon.common
├── :utils       (Tier 1) - CircularBuffer, ObjectPool, source_location
├── :error       (Tier 1) - Error codes and categories
├── :result      (Tier 2) - Result<T> pattern implementation
│   ├── :result.core      - Core Result<T>, VoidResult, error_info types
│   └── :result.utilities - Helper functions and macros
├── :concepts    (Tier 2) - C++20 concepts for type validation
├── :interfaces  (Tier 3) - Core interfaces (aggregator)
│   ├── :interfaces.logger   - ILogger, log_level, log_entry
│   ├── :interfaces.executor - IJob, IExecutor, IThreadPool
│   └── :interfaces.core     - Re-exports both logger and executor
├── :config      (Tier 3) - Configuration utilities
├── :di          (Tier 3) - Dependency injection
├── :patterns    (Tier 4) - EventBus implementation
└── :logging     (Tier 4) - Logging utilities
```

### Importing Specific Partitions

You can import the entire module or specific partitions:

```cpp
// Import everything
import kcenon.common;

// Or import specific partitions (if needed for faster compilation)
import kcenon.common:result;
import kcenon.common:interfaces;
```

## Header vs Module Comparison

| Aspect | Headers | Modules |
|--------|---------|---------|
| Compilation | Each TU processes headers | Compiled once, imported |
| Build time | Slower (repetitive parsing) | Faster (pre-compiled) |
| Macros | Can leak between TUs | Isolated |
| Symbol visibility | All symbols visible | Export-controlled |
| CMake version | 3.16+ | 3.28+ |
| Compiler support | All modern compilers | Clang 16+, GCC 14+, MSVC 2022 |

## Backward Compatibility

The header-only interface remains fully supported and is the default. You can:

1. Continue using headers without any changes
2. Mix headers and modules in different parts of your project
3. Fall back to headers if module compilation fails

```cmake
# Default: header-only (no module compilation)
cmake -B build

# Explicit: enable modules
cmake -B build -DCOMMON_BUILD_MODULES=ON
```

## Troubleshooting

### "Module not found" Error

Ensure you're using the correct CMake generator and compiler:

```bash
cmake -G Ninja -DCMAKE_CXX_COMPILER=clang++ -B build
```

### "Unsupported compiler" Warning

Module support requires specific compiler versions:

| Compiler | Minimum Version | Status |
|----------|-----------------|--------|
| Clang | 16.0 | Supported |
| GCC | 14.0 | Supported |
| MSVC | 17.4 (2022) | Supported |
| AppleClang | - | Not supported |

### Build Order Issues

Modules must be built before consuming targets. CMake 3.28+ handles this automatically with `CXX_SCAN_FOR_MODULES`.

## Migration Checklist

- [ ] Verify compiler version (Clang 16+, GCC 14+, MSVC 2022)
- [ ] Update CMake to 3.28+
- [ ] Use Ninja generator
- [ ] Enable `-DCOMMON_BUILD_MODULES=ON`
- [ ] Replace `#include` with `import` statements
- [ ] Link to `kcenon::common_modules` instead of `kcenon::common`
- [ ] Test build and runtime behavior

## Related Documentation

- [Architecture Overview](../ARCHITECTURE.md)
- [Project Structure](../PROJECT_STRUCTURE.md)
- [Quick Start Guide](QUICK_START.md)
- [Integration Guide](INTEGRATION.md)
