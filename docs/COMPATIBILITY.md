# KCENON Ecosystem Compatibility Matrix

This document defines version compatibility and dependency requirements across all KCENON ecosystem components.

> **Language:** **English** | [한국어](COMPATIBILITY.kr.md)

---

## Overview

The KCENON ecosystem consists of interconnected C++20 libraries that share common patterns and interfaces. Ensuring compatibility between these systems is critical for stable integration.

### Versioning Standard

All KCENON systems follow [Semantic Versioning 2.0.0](https://semver.org/spec/v2.0.0.html):

| Version Component | Meaning |
|-------------------|---------|
| **MAJOR** (X.0.0) | Breaking API changes - requires migration |
| **MINOR** (0.X.0) | Backward-compatible new features |
| **PATCH** (0.0.X) | Backward-compatible bug fixes |
| **BUILD** (0.0.0.X) | Build metadata (optional) |

---

## Current Version Matrix

| System | Current Version | Minimum common_system | Status |
|--------|-----------------|----------------------|--------|
| **common_system** | 0.2.0 | - | Stable |
| **thread_system** | 0.1.0 | 0.2.0 | Stable |
| **logger_system** | 0.1.0 | 0.2.0 | Stable |
| **monitoring_system** | 0.1.0 | 0.2.0 | Stable |
| **container_system** | 0.1.0 | 0.2.0 | Stable |
| **network_system** | 0.1.0 | 0.2.0 | Stable |
| **database_system** | 0.1.0 | 0.2.0 | Stable |

---

## Dependency Graph

```
                    ┌─────────────────┐
                    │  common_system  │
                    │     v0.2.0      │
                    └────────┬────────┘
                             │
         ┌───────────────────┼───────────────────┐
         │                   │                   │
         ▼                   ▼                   ▼
┌─────────────────┐ ┌─────────────────┐ ┌─────────────────┐
│  thread_system  │ │  logger_system  │ │container_system │
│     v0.1.0      │ │     v0.1.0      │ │     v0.1.0      │
└────────┬────────┘ └────────┬────────┘ └─────────────────┘
         │                   │
         ▼                   ▼
┌─────────────────┐ ┌─────────────────┐
│monitoring_system│ │ network_system  │
│     v0.1.0      │ │     v0.1.0      │
└─────────────────┘ └────────┬────────┘
                             │
                             ▼
                    ┌─────────────────┐
                    │ database_system │
                    │     v0.1.0      │
                    └─────────────────┘
```

---

## Detailed Dependencies

### common_system (Foundation Layer)

The foundation library with no external KCENON dependencies. All other systems depend on this.

**Provides:**
- `Result<T>` error handling pattern
- Error code registry and definitions
- Core interfaces: `IExecutor`, `ILogger`, `IMonitor`, `IDatabase`
- Event bus pattern for decoupled communication
- Smart adapters and factory patterns
- C++20 concepts for type validation

**External Dependencies:**
- C++20 compiler (GCC 10+, Clang 12+, MSVC 19.29+)
- CMake 3.16+
- (Optional) yaml-cpp for YAML configuration support
- (Optional) GTest for testing

---

### thread_system

**Required:**
| Dependency | Minimum Version | Purpose |
|------------|-----------------|---------|
| common_system | 0.2.0 | Core interfaces (IExecutor, Result<T>) |

**Optional:**
| Dependency | Minimum Version | Purpose |
|------------|-----------------|---------|
| logger_system | 0.1.0 | Logging integration |
| monitoring_system | 0.1.0 | Performance monitoring |

---

### logger_system

**Required:**
| Dependency | Minimum Version | Purpose |
|------------|-----------------|---------|
| common_system | 0.2.0 | ILogger interface, GlobalLoggerRegistry |

**External:**
| Dependency | Version | Purpose |
|------------|---------|---------|
| spdlog | 1.12.0+ | High-performance logging backend |
| fmt | 10.0.0+ | String formatting |

---

### monitoring_system

**Required:**
| Dependency | Minimum Version | Purpose |
|------------|-----------------|---------|
| common_system | 0.2.0 | IMonitor interface, Result<T> |
| thread_system | 0.1.0 | Background task execution |
| logger_system | 0.1.0 | Metric and event logging |

---

### container_system

**Required:**
| Dependency | Minimum Version | Purpose |
|------------|-----------------|---------|
| common_system | 0.2.0 | Result<T>, value types |

**Optional:**
| Dependency | Minimum Version | Purpose |
|------------|-----------------|---------|
| thread_system | 0.1.0 | Thread-safe containers |

---

### network_system

**Required:**
| Dependency | Minimum Version | Purpose |
|------------|-----------------|---------|
| common_system | 0.2.0 | Core interfaces, Result<T> |
| thread_system | 0.1.0 | Async I/O, connection pools |
| logger_system | 0.1.0 | Network event logging |
| container_system | 0.1.0 | Message containers |

**External:**
| Dependency | Version | Purpose |
|------------|---------|---------|
| Boost.Asio | 1.80.0+ | Async networking |

---

### database_system

**Required:**
| Dependency | Minimum Version | Purpose |
|------------|-----------------|---------|
| common_system | 0.2.0 | IDatabase interface, Result<T> |
| thread_system | 0.1.0 | Query execution |
| logger_system | 0.1.0 | Query logging |
| network_system | 0.1.0 | Remote database connections |
| container_system | 0.1.0 | Record containers |

**External:**
| Dependency | Version | Purpose |
|------------|---------|---------|
| SQLite | 3.35.0+ | Embedded database |
| (Optional) PostgreSQL client | 14.0+ | PostgreSQL support |

---

## Known Incompatibilities

### Breaking Changes History

| Version Combination | Issue | Resolution |
|---------------------|-------|------------|
| common_system < 2.0.0 | Legacy `Result<T>` factory functions | Upgrade to common_system 2.0.0+ |
| thread_system with common_system < 0.2.0 | Missing GlobalLoggerRegistry | Upgrade common_system to 0.2.0+ |

---

## CMake Version Validation

To ensure compatibility at build time, add version checks to your CMakeLists.txt:

```cmake
# Find common_system with minimum version requirement
find_package(common_system 0.2.0 REQUIRED)

# Optional: Verify version programmatically
if(common_system_VERSION VERSION_LESS "0.2.0")
    message(FATAL_ERROR "common_system >= 0.2.0 is required. Found: ${common_system_VERSION}")
endif()

# Multiple dependencies with version requirements
find_package(thread_system 0.1.0 REQUIRED)
find_package(logger_system 0.1.0 REQUIRED)

# Version compatibility check example
if(thread_system_VERSION VERSION_LESS "0.1.0")
    message(FATAL_ERROR "thread_system >= 0.1.0 is required for monitoring features")
endif()
```

---

## Upgrade Guidelines

### Safe Upgrade Order

When upgrading multiple systems, follow this order to avoid compatibility issues:

1. **common_system** (always first - foundation layer)
2. **thread_system** (execution primitives)
3. **logger_system** (logging infrastructure)
4. **container_system** (data structures)
5. **monitoring_system** (depends on thread + logger)
6. **network_system** (depends on thread + logger + container)
7. **database_system** (depends on all above)

### Pre-Upgrade Checklist

- [ ] Review CHANGELOG.md for breaking changes
- [ ] Check this compatibility matrix for version requirements
- [ ] Run existing tests with new version in isolated environment
- [ ] Update CMakeLists.txt version requirements
- [ ] Test integration points between systems

---

## Runtime Compatibility Verification

Systems can verify compatibility at runtime using the version API:

```cpp
#include <kcenon/common/config/abi_version.h>

void verify_compatibility() {
    auto version = kcenon::common::get_version();

    // Semantic version check
    if (version.major < 2) {
        throw std::runtime_error("Requires common_system 2.0.0 or higher");
    }

    // ABI version check for binary compatibility
    if (kcenon::common::get_abi_version() != expected_abi) {
        throw std::runtime_error("ABI mismatch - recompilation required");
    }
}
```

---

## Reporting Compatibility Issues

If you encounter compatibility issues:

1. **Check this matrix** for known incompatibilities
2. **Review CHANGELOG.md** in affected systems
3. **Create an issue** at [GitHub Issues](https://github.com/kcenon/common_system/issues) with:
   - Versions of all involved systems
   - CMake configuration output
   - Error messages or unexpected behavior
   - Minimal reproduction case

---

## Version History

| Date | Change |
|------|--------|
| 2025-12-16 | Initial compatibility matrix created |

---

## License

This document is part of the KCENON ecosystem and is licensed under the BSD 3-Clause License.
