---
doc_id: "COM-MIGR-006"
doc_title: "fmt → std::format Migration Guide"
doc_version: "1.0.0"
doc_date: "2026-04-04"
doc_status: "Released"
project: "common_system"
category: "MIGR"
---

# fmt → std::format Migration Guide

> **SSOT**: This document is the single source of truth for **fmt → std::format Migration Guide**.

> **Tracking**: [common_system#406](https://github.com/kcenon/common_system/issues/406)
>
> **Status**: Migration complete — all ecosystem projects now use C++20 `std::format`

This guide documents the standard approach for removing the `fmt` library
from kcenon ecosystem projects in favour of C++20 `std::format`.

## Prerequisites

| Requirement | Minimum |
|-------------|---------|
| C++ Standard | C++20 |
| GCC | 13+ (full `<format>` support) |
| Clang | 17+ (libc++ with `<format>`) |
| MSVC | 2022 17.4+ (19.34+) |
| Apple Clang | 15+ (Xcode 15, with libc++) |

## Migration Steps

### 1. Find all fmt usages

```bash
# Headers
grep -rn '#include <fmt/' src/ include/
grep -rn '#include "fmt/' src/ include/

# Function calls
grep -rn 'fmt::format\b' src/ include/
grep -rn 'fmt::print\b' src/ include/
grep -rn 'fmt::format_to\b' src/ include/
```

### 2. Replace includes

```cpp
// Before
#include <fmt/core.h>
#include <fmt/format.h>
#include <fmt/chrono.h>

// After
#include <format>
```

### 3. Replace function calls

| fmt | std::format | Notes |
|-----|-------------|-------|
| `fmt::format(...)` | `std::format(...)` | Direct replacement |
| `fmt::format_to(...)` | `std::format_to(...)` | Direct replacement |
| `fmt::print(...)` | `std::print(...)` | C++23; use `std::format` + stream for C++20 |
| `fmt::to_string(...)` | `std::format("{}", ...)` | No direct equivalent |
| `fmt::join(...)` | Manual loop or ranges | No std equivalent yet |

### 4. Handle custom formatters

```cpp
// Before (fmt)
template <>
struct fmt::formatter<MyType> {
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    auto format(const MyType& val, format_context& ctx) const {
        return fmt::format_to(ctx.out(), "MyType({})", val.value());
    }
};

// After (std::format)
template <>
struct std::formatter<MyType> {
    constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }
    auto format(const MyType& val, std::format_context& ctx) const {
        return std::format_to(ctx.out(), "MyType({})", val.value());
    }
};
```

### 5. Remove fmt from build configuration

```diff
# vcpkg.json
  "dependencies": [
-   "fmt",
    ...
  ],
  "overrides": [
-   { "name": "fmt", "version": "10.2.1" },
    ...
  ]
```

```diff
# CMakeLists.txt
- find_package(fmt CONFIG REQUIRED)
- target_link_libraries(${PROJECT_NAME} PRIVATE fmt::fmt)
```

### 6. Remove fmt feature flags

If the project has an optional `fmt-support` feature in `vcpkg.json`, remove it:

```diff
  "features": {
-   "fmt-support": {
-     "description": "Enable fmt formatting support",
-     "dependencies": ["fmt"]
-   },
    ...
  }
```

### 7. Update documentation

- Remove fmt from `LICENSE-THIRD-PARTY`
- Remove fmt from SOUP documentation
- Update DEPENDENCY_MATRIX.md (handled centrally in common_system)

## Known Differences

| Feature | fmt | std::format | Workaround |
|---------|-----|-------------|------------|
| Named arguments | `fmt::arg("name", val)` | Not in C++20 | Use positional |
| Color/style | `fmt::emphasis::bold` | Not available | Use ANSI codes |
| `fmt::join` | Joins ranges | Not in C++20 | Manual loop |
| Compile-time format | `FMT_COMPILE(...)` | `std::format` is compile-time checked | Direct replacement |
| Wide string | `fmt::format(L"...")` | `std::format(L"...")` | Direct replacement |

## Reference Migration: thread_system

thread_system completed this migration successfully. Key commit patterns:

1. Replace `#include <fmt/...>` → `#include <format>`
2. Replace `fmt::format(` → `std::format(`
3. Remove `fmt` from `vcpkg.json` deps and overrides
4. Remove `fmt::fmt` from CMake `target_link_libraries`
5. Update LICENSE-THIRD-PARTY

## Migration Status

| Project | Tracking Issue | Status |
|---------|---------------|--------|
| thread_system | — | Completed |
| logger_system | [logger_system#457](https://github.com/kcenon/logger_system/issues/457) | Completed |
| database_system | [database_system#399](https://github.com/kcenon/database_system/issues/399) | Completed |
| network_system | [network_system#791](https://github.com/kcenon/network_system/issues/791) | Completed |
| container_system | — | Completed (fmt-support feature removed) |
