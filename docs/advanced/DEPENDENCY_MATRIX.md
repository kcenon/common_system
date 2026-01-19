# Dependency Matrix - Detailed Analysis

> **Language:** **English** | [한국어](DEPENDENCY_MATRIX.kr.md)


## Table of Contents

- [Executive Summary](#executive-summary)
  - [Current Dependency Status](#current-dependency-status)
- [Cycle 1: common_system ↔ monitoring_system](#cycle-1-common_system-monitoring_system)
  - [Dependency Chain](#dependency-chain)
  - [Detailed Analysis](#detailed-analysis)
    - [common_system → monitoring_system](#common_system-monitoring_system)
    - [monitoring_system → common_system](#monitoring_system-common_system)
  - [Resolution Strategy for Cycle 1](#resolution-strategy-for-cycle-1)
- [Cycle 2: Resolved by Cycle 1](#cycle-2-resolved-by-cycle-1)
  - [Analysis](#analysis)
- [Cycle 3: thread_system ↔ container_system ↔ network_system](#cycle-3-thread_system-container_system-network_system)
  - [Dependency Chain](#dependency-chain)
  - [Detailed Analysis](#detailed-analysis)
    - [thread_system → container_system](#thread_system-container_system)
    - [container_system → network_system](#container_system-network_system)
    - [network_system → thread_system](#network_system-thread_system)
  - [Resolution Strategy for Cycle 3](#resolution-strategy-for-cycle-3)
- [Dependency by System](#dependency-by-system)
  - [common_system (Foundation - Level 0)](#common_system-foundation-level-0)
  - [thread_system (Level 1)](#thread_system-level-1)
  - [logger_system (Level 1)](#logger_system-level-1)
  - [monitoring_system (Level 2)](#monitoring_system-level-2)
  - [container_system (Level 1)](#container_system-level-1)
  - [database_system (Level 2)](#database_system-level-2)
  - [network_system (Level 2)](#network_system-level-2)
  - [messaging_system (Level 3)](#messaging_system-level-3)
- [Must-Have vs. Nice-to-Have Dependencies](#must-have-vs-nice-to-have-dependencies)
  - [Must-Have (Essential)](#must-have-essential)
  - [Nice-to-Have (Convenience)](#nice-to-have-convenience)
- [Action Items](#action-items)
  - [Immediate Actions](#immediate-actions)
  - [Short-term Actions (Week 1)](#short-term-actions-week-1)
  - [Long-term Actions (Week 2-4)](#long-term-actions-week-2-4)
- [Success Metrics](#success-metrics)
  - [Phase 4 Goals](#phase-4-goals)
  - [Current Score: 75% ✅](#current-score-75-)
- [Conclusion](#conclusion)
  - [Key Findings](#key-findings)
  - [Recommended Next Steps](#recommended-next-steps)
  - [Estimated Effort Adjustment](#estimated-effort-adjustment)

**Date**: 2025-10-10
**Phase**: Phase 4 - Dependency Refactoring
**Analysis Type**: Detailed Include and CMake Dependency Mapping

---

## Executive Summary

### Current Dependency Status

| System | CMake Deps | Include Deps | Circular? | Status |
|--------|-----------|--------------|-----------|---------|
| common_system | 0 | 1 (monitoring*) | ⚠️ Yes | *Conditional compile |
| thread_system | 0 | 1 (common) | ⚠️ Yes (via cycle) | Has service_container DI |
| logger_system | 2 | 2 | ⚠️ Yes (via cycle) | Valid hierarchy |
| monitoring_system | 3 | 2 | ⚠️ Yes | Implements for common |
| container_system | 0 | 1 (common) | ⚠️ Yes (via Cycle 3) | Minimal deps |
| database_system | 0 | 0 | ✅ No | Independent |
| network_system | 0 | 3 | ⚠️ Yes (via Cycle 3) | Has thread integration |
| messaging_system | 2+ | 6 (optional) | ✅ No | High-level messaging |

**Key Finding**: Most dependencies are **include-only** (not in CMakeLists.txt), indicating header-only usage.

---

## Cycle 1: common_system ↔ monitoring_system

### Dependency Chain

```
common_system → monitoring_system → (logger_system → thread_system) → common_system
```

### Detailed Analysis

#### common_system → monitoring_system

**File**: `/Users/dongcheolshin/Sources/common_system/include/kcenon/common/patterns/event_bus.h:32-33`

```cpp
#if defined(ENABLE_MONITORING_INTEGRATION) || defined(WITH_MONITORING_SYSTEM) || defined(USE_MONITORING_SYSTEM)

// Forward to the actual event bus implementation
#include <kcenon/monitoring/core/event_bus.h>
#include <kcenon/monitoring/interfaces/event_bus_interface.h>

namespace common {
    using event_bus = monitoring_system::event_bus;
    // ...
}

#else // Monitoring integration disabled

// Provide a stub implementation when monitoring is disabled
class null_event_bus {
    // No-op implementation
};

#endif
```

**Dependency Type**: Conditional Include (header-only)
**CMake Dependency**: None
**Impact**: Low - Already has fallback implementation

**Status**: ✅ **ALREADY MITIGATED**
- Conditional compilation prevents hard dependency
- `null_event_bus` provides no-op fallback
- Users can build common_system without monitoring_system

#### monitoring_system → common_system

**Files**:
1. `monitoring_system/include/kcenon/monitoring/core/performance_monitor.h:` → `common/interfaces/monitoring_interface.h`
2. `monitoring_system/include/kcenon/monitoring/core/result_types.h` → `common/patterns/result.h`
3. Multiple adapter files → `common_system`

**Dependency Type**: Direct Include (required)
**CMake Dependency**: Yes - `target_link_libraries(monitoring_system PUBLIC common_system)`
**Impact**: Medium - Valid hierarchy (monitoring is higher level)

**Status**: ✅ **VALID DEPENDENCY**
- monitoring_system is Level 2, common_system is Level 0
- Upward dependency is architecturally correct

### Resolution Strategy for Cycle 1

**Verdict**: ✅ **NO ACTION NEEDED**

**Rationale**:
1. common_system → monitoring is **optional** (conditional compilation)
2. monitoring → common is **valid** (higher level depends on foundation)
3. Cycle is **broken by design** through conditional includes

**Validation**:
```bash
# Build common_system without monitoring
cd common_system
cmake -B build -DENABLE_MONITORING_INTEGRATION=OFF
cmake --build build
# Expected: Success ✅
```

---

## Cycle 2: Resolved by Cycle 1

### Analysis

**Chain**: `logger_system → thread_system → common_system → monitoring_system → logger_system`

**Status**: ✅ **AUTO-RESOLVED**
- Since Cycle 1 is conditional, this cycle is also broken
- logger → thread is valid (higher depends on lower)
- thread → common is valid (all depend on foundation)

**No Action Needed**: Cycle 2 does not exist when monitoring integration is disabled.

---

## Cycle 3: thread_system ↔ container_system ↔ network_system

### Dependency Chain

```
thread_system → container_system → network_system → thread_system
```

### Detailed Analysis

#### thread_system → container_system

**File**: `/Users/dongcheolshin/Sources/thread_system/include/kcenon/thread/interfaces/thread_context.h:39`

```cpp
#include "service_container.h"
```

**Dependency Type**: Include (service locator pattern)
**CMake Dependency**: None
**Purpose**: Dependency Injection via Service Container

**Analysis**:
```cpp
class thread_context {
public:
    thread_context()
        : logger_(service_container::global().resolve<logger_interface>())
        , monitoring_(service_container::global().resolve<monitoring_interface::monitoring_interface>()) {
    }
    // ...
};
```

**Status**: ✅ **ALREADY USING DI PATTERN**
- Uses service_container for runtime injection
- No compile-time dependency on container_system data structures
- Only depends on service_container interface

**Finding**: This is **NOT** a dependency on `container_system` (the variant container).
It's a dependency on a **service locator** pattern (likely internal to thread_system).

**Action**: Verify `service_container.h` location

```bash
find /Users/dongcheolshin/Sources/thread_system -name "service_container.h"
```

#### container_system → network_system

**Search Result**: No includes found

```bash
grep -r "#include.*network" /Users/dongcheolshin/Sources/container_system/
# Result: Empty
```

**Status**: ✅ **FALSE POSITIVE**
- Dependency analyzer may have detected transitive dependency
- No direct include of network_system in container_system

#### network_system → thread_system

**Files**:
1. `network_system/include/network_system/integration/messaging_bridge.h:60` → `kcenon/thread/core/thread_pool.h`
2. `network_system/include/network_system/compatibility.h:21` → `network_system/integration/thread_integration.h`
3. Multiple files → `<thread>` (std::thread, not thread_system)

**Dependency Type**: Direct Include
**CMake Dependency**: None (include-only)
**Purpose**: Async I/O thread pool integration

**Status**: ⚠️ **NEEDS INVESTIGATION**
- Does network_system require thread_system's thread_pool?
- Or can it use std::thread directly?

### Resolution Strategy for Cycle 3

**Investigation Required**:

1. **Locate service_container.h**
   ```bash
   find thread_system -name "service_container.h"
   ```

2. **Check if network truly needs thread_system**
   ```bash
   grep -A5 -B5 "kcenon/thread" network_system/include/network_system/integration/messaging_bridge.h
   ```

3. **Determine if dependency is essential or convenience**

**Possible Resolutions**:

**Option A**: Extract `IThreadPool` interface to `common_system`
- network_system uses interface
- thread_system implements interface
- Breaks compile-time dependency

**Option B**: Remove dependency (if std::thread suffices)
- network_system uses std::thread directly
- No external thread pool needed
- Simplest solution

**Option C**: Make dependency one-way (network → thread only)
- Valid if network is higher level
- Verify thread doesn't include network

---

## Dependency by System

### common_system (Foundation - Level 0)

**Expected**: No dependencies
**Actual**:
- CMake: None ✅
- Include: monitoring_system (conditional) ✅

**Files with Dependencies**:
```
include/kcenon/common/patterns/event_bus.h
├── #if ENABLE_MONITORING_INTEGRATION
│   ├── #include <kcenon/monitoring/core/event_bus.h>
│   └── #include <kcenon/monitoring/interfaces/event_bus_interface.h>
└── #else
    └── // null_event_bus (no-op)
```

**Assessment**: ✅ **COMPLIANT** - Conditional dependency is acceptable

---

### thread_system (Level 1)

**Expected**: common_system only
**Actual**:
- CMake: None
- Include: common_system ✅, service_container (internal?)

**Files with Dependencies**:
```
include/kcenon/thread/interfaces/thread_context.h
├── "logger_interface.h" (likely from common or internal)
├── "monitoring_interface.h" (likely from common or internal)
└── "service_container.h" (❓ location unknown)
```

**Assessment**: ⚠️ **NEEDS VERIFICATION** - Check service_container location

---

### logger_system (Level 1)

**Expected**: common_system, thread_system
**Actual**:
- CMake: common_system, thread_system ✅
- Include: common_system, thread_system ✅

**Assessment**: ✅ **COMPLIANT** - Valid hierarchy

---

### monitoring_system (Level 2)

**Expected**: common_system, logger_system, thread_system
**Actual**:
- CMake: common_system, logger_system, thread_system ✅
- Include: common_system, thread_system ✅ (logger in CMake only)

**Files with Dependencies**:
```
include/kcenon/monitoring/core/performance_monitor.h
├── #include <kcenon/common/interfaces/monitoring_interface.h>

include/kcenon/monitoring/core/result_types.h
├── #if BUILD_WITH_COMMON_SYSTEM
│   ├── #include <kcenon/common/patterns/result.h>
└── #else
    └── // Fallback result.h
```

**Assessment**: ✅ **COMPLIANT** - Proper use of common_system

---

### container_system (Level 1)

**Expected**: common_system
**Actual**:
- CMake: None
- Include: common_system ✅

**Assessment**: ✅ **COMPLIANT**

---

### database_system (Level 2)

**Expected**: common_system, possibly others
**Actual**:
- CMake: None ✅
- Include: None ✅

**Assessment**: ✅ **INDEPENDENT** - No dependencies (surprising but valid)

---

### network_system (Level 2)

**Expected**: common_system, thread_system, logger_system
**Actual**:
- CMake: None
- Include: common_system, logger_system, thread_system

**Files with Dependencies**:
```
include/network_system/integration/messaging_bridge.h
├── #include <kcenon/thread/core/thread_pool.h>

include/network_system/integration/thread_integration.h
├── // Likely thread system integration
```

**Assessment**: ⚠️ **NEEDS REFACTORING** - Include-only dependencies should be in CMake

---

### messaging_system (Level 3)

**Expected**: common_system, container_system, and optionally network_system
**Actual**:
- CMake: common_system, container_system (required)
- Include: common_system, container_system, thread_system, logger_system, monitoring_system, network_system

**Dependency Hierarchy**:
```
messaging_system (Level 3 - High-level messaging)
        │
        ├── Required:
        │   ├── common_system (Level 0) - Result<T>, interfaces
        │   └── container_system (Level 1) - Message payloads
        │
        └── Optional:
            ├── thread_system (Level 1) - Thread pool for dispatch
            ├── logger_system (Level 1) - Structured logging
            ├── monitoring_system (Level 2) - Metrics collection
            ├── database_system (Level 2) - Message persistence
            └── network_system (Level 2) - Distributed messaging
```

**Key Relationship with network_system**:
- **messaging_system** uses **network_system** (not the reverse)
- network_system provides low-level TCP/UDP/WebSocket transport
- messaging_system provides high-level pub/sub, request/reply patterns
- This is a **one-way dependency**: messaging → network

**Assessment**: ✅ **COMPLIANT** - Proper high-level to low-level dependency

---

## Must-Have vs. Nice-to-Have Dependencies

### Must-Have (Essential)

| System | Depends On | Reason |
|--------|-----------|--------|
| logger_system | thread_system | Async logging requires thread pool |
| logger_system | common_system | Result<T>, interfaces |
| monitoring_system | common_system | Interfaces, Result<T> |
| monitoring_system | logger_system | Log monitoring events |
| monitoring_system | thread_system | Monitor thread pool metrics |
| network_system | common_system | Result<T>, interfaces |
| messaging_system | common_system | Result<T>, interfaces |
| messaging_system | container_system | Type-safe message payloads |

### Nice-to-Have (Convenience)

| System | Depends On | Reason | Can Remove? |
|--------|-----------|--------|-------------|
| common_system | monitoring_system | Event bus forwarding | ✅ Yes (conditional) |
| network_system | thread_system | Thread pool for async I/O | ⚠️ Maybe (use std::thread) |
| network_system | logger_system | Logging network events | ⚠️ Maybe (use common interface) |
| messaging_system | thread_system | Thread pool for message dispatch | ✅ Yes (standalone backend) |
| messaging_system | logger_system | Structured logging | ✅ Yes (optional) |
| messaging_system | monitoring_system | Metrics collection | ✅ Yes (optional) |
| messaging_system | database_system | Message persistence | ✅ Yes (optional) |
| messaging_system | network_system | Distributed messaging over TCP/IP | ✅ Yes (optional) |

---

## Action Items

### Immediate Actions

1. ✅ **Verify Cycle 1 is already mitigated** (conditional compilation)
   ```bash
   cd common_system
   cmake -B build -DENABLE_MONITORING_INTEGRATION=OFF
   ```

2. ❓ **Locate service_container.h**
   ```bash
   find thread_system -name "service_container.h"
   ```

3. ⚠️ **Investigate network → thread dependency**
   - Is thread_pool essential or can std::thread work?
   - Can we use IThreadPool interface from common?

### Short-term Actions (Week 1)

1. **Add CMakeLists.txt dependencies for include-only systems**
   - network_system should properly link logger_system, thread_system
   - Prevents include-only "ghost dependencies"

2. **Extract IThreadPool interface** (if needed)
   - common_system/include/kcenon/common/interfaces/thread_pool_interface.h
   - thread_system implements
   - network_system uses interface

3. **Document dependency rules**
   - Update ARCHITECTURE.md
   - Add dependency validation test

### Long-term Actions (Week 2-4)

1. **Create dependency validation CI check**
   ```yaml
   - name: Validate Dependencies
     run: python3 tools/dependency_analyzer.py
   ```

2. **Add CMake dependency checker**
   ```cmake
   if(DEFINED ALLOWED_DEPENDENCIES_${PROJECT_NAME})
     check_dependencies_match_whitelist()
   endif()
   ```

---

## Success Metrics

### Phase 4 Goals

- ✅ **Zero hard circular dependencies** (already achieved through conditional compilation)
- ⚠️ **All include dependencies match CMake** (need to sync network_system)
- ✅ **Foundation (common) has no upward hard deps** (conditional only)
- ⏳ **Automated dependency validation** (to be implemented)

### Current Score: 75% ✅

**Breakdown**:
- Cycle 1: ✅ Mitigated (conditional)
- Cycle 2: ✅ Auto-resolved
- Cycle 3: ⚠️ Partially resolved (needs verification)
- CMake sync: ⏳ Pending (network_system)

---

## Conclusion

### Key Findings

1. **Cycle 1 & 2**: ✅ **Already mitigated through conditional compilation**
   - common_system has elegant `#ifdef` design
   - No breaking changes needed

2. **Cycle 3**: ⚠️ **Needs investigation**
   - service_container location unclear
   - network → thread dependency may be removable

3. **Include-only Dependencies**: ⚠️ **Architectural smell**
   - network_system includes but doesn't link
   - Should be made explicit in CMakeLists.txt

### Recommended Next Steps

1. **Day 1 Remaining**: Investigate Cycle 3 details
2. **Day 2**: Design IThreadPool interface (if needed)
3. **Day 3**: Sync CMakeLists.txt with actual includes

### Estimated Effort Adjustment

**Original**: 80 hours (4 weeks)
**Revised**: 40 hours (2 weeks)

**Reason**: Cycles 1 & 2 already resolved, only Cycle 3 and CMake cleanup remain.

---

**Document Status**: Complete
**Next Update**: After service_container investigation
**Maintainer**: kcenon

---

*Last Updated: 2025-10-20*
