# common_system Integration Policy

## Overview

This document defines the official integration policy for common_system across all systems in the project. The policy ensures consistency, predictability, and proper documentation of system dependencies.

## Integration Tiers

### Tier 1: Core Interface Systems (Required)

These systems **REQUIRE** common_system and cannot function without it:

| System | Rationale |
|--------|-----------|
| **logger_system** | Implements `ILogger` interface and uses `Result<T>` for error handling |
| **monitoring_system** | Implements `IMonitor` interface and uses `Result<T>` pattern |

**CMake Configuration**:
```cmake
# Core systems - common_system is mandatory
find_package(common_system CONFIG QUIET)
if(NOT common_system_FOUND)
    # Fallback to path-based search
    # ... error if not found
endif()
```

### Tier 2: Infrastructure Systems (Optional, Default ON)

These systems **OPTIONALLY** integrate with common_system but benefit significantly from it:

| System | Default | Rationale |
|--------|---------|-----------|
| **thread_system** | ON | Uses `Result<T>` for error handling, benefits from standard interfaces |
| **container_system** | ON | Can use `Result<T>` for validation and error reporting |
| **database_system** | ON | Uses `Result<T>` for database operation results |
| **network_system** | ON | Can use common interfaces for logging, monitoring, and execution |

**CMake Configuration**:
```cmake
# Infrastructure systems - common_system integration is optional but recommended
option(BUILD_WITH_COMMON_SYSTEM "Enable common_system integration" ON)

if(BUILD_WITH_COMMON_SYSTEM)
    find_package(common_system CONFIG QUIET)
    if(NOT common_system_FOUND)
        message(WARNING "common_system not found, falling back to standalone mode")
        set(BUILD_WITH_COMMON_SYSTEM OFF)
    else()
        target_compile_definitions(${PROJECT_NAME} PUBLIC BUILD_WITH_COMMON_SYSTEM)
        target_link_libraries(${PROJECT_NAME} PUBLIC kcenon::common_system)
    endif()
endif()
```

### Tier 3: Standalone Systems (Independent)

| System | Integration Status |
|--------|-------------------|
| **common_system** | Foundation - provides interfaces for all other systems |

## Integration Features by Tier

### Tier 1 Benefits (Core Systems)

- **Mandatory interfaces**: `ILogger`, `IMonitor`, `IExecutor`
- **Type-safe error handling**: `Result<T>`, `VoidResult`
- **Standardized patterns**: Factory patterns, dependency injection
- **Cross-system compatibility**: Guaranteed compatibility with all systems

### Tier 2 Benefits (Infrastructure Systems)

When `BUILD_WITH_COMMON_SYSTEM=ON`:
- **Enhanced error handling**: `Result<T>` pattern for operations
- **Interface compatibility**: Can use standard `ILogger`, `IMonitor` interfaces
- **Better diagnostics**: Rich error information with error codes
- **Future-proof**: Ready for advanced integration features

When `BUILD_WITH_COMMON_SYSTEM=OFF`:
- **Standalone operation**: Traditional error handling (bool, exceptions)
- **Minimal dependencies**: Can be used independently
- **Legacy compatibility**: Maintains backward compatibility

## Default Configuration Rationale

### Why Default ON for Infrastructure Systems?

1. **Best Practice Alignment**: Modern C++ projects benefit from standardized error handling
2. **Integration Ready**: Most users want systems to work together seamlessly
3. **Enhanced Features**: `Result<T>` provides better error diagnostics than bool
4. **Future Compatibility**: Prepares codebase for advanced features

### Migration Path

Users can still opt-out if needed:
```bash
cmake -B build -S . -DBUILD_WITH_COMMON_SYSTEM=OFF
```

## Verification Checklist

- [ ] Core systems (Tier 1) always find common_system or fail gracefully
- [ ] Infrastructure systems (Tier 2) default to `BUILD_WITH_COMMON_SYSTEM=ON`
- [ ] All systems build successfully with both `ON` and `OFF` (except Tier 1)
- [ ] Documentation clearly states integration requirements
- [ ] CMake messages inform users of integration status

## Integration Status

| System | Tier | Default | Status | Last Verified |
|--------|------|---------|--------|---------------|
| common_system | 3 | N/A | Foundation | 2025-10-03 |
| logger_system | 1 | Required | ✅ Implemented | 2025-10-03 |
| monitoring_system | 1 | Required | ✅ Implemented | 2025-10-03 |
| thread_system | 2 | ON | ✅ Implemented | 2025-10-03 |
| container_system | 2 | ON | ✅ Implemented | 2025-10-03 |
| database_system | 2 | ON | ✅ Implemented | 2025-10-03 |
| network_system | 2 | ON | ✅ Implemented | 2025-10-03 |

## Version History

- **v1.0.0** (2025-10-03): Initial policy definition
  - Established 3-tier integration model
  - Standardized default to ON for infrastructure systems
  - Aligned network_system with other infrastructure systems

## References

- [INTEGRATION.md](./INTEGRATION.md) - Integration guide and examples
- [ARCHITECTURE.md](./ARCHITECTURE.md) - System architecture overview
- [NEED_TO_FIX.md](./NEED_TO_FIX.md) - Implementation tracking
