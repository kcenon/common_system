# Phase 4: Dependency Refactoring - Detailed Implementation Plan

**Date**: 2025-10-10
**Phase**: Phase 4 - Dependency Refactoring
**Status**: Planning Complete - Ready for Implementation
**Priority**: HIGH - Circular dependencies must be resolved

---

## Executive Summary

### Current State ⚠️

**Circular Dependencies Detected**: 3 major cycles found

1. `common_system → monitoring_system → logger_system → thread_system → common_system`
2. `common_system → monitoring_system → logger_system → thread_system → container_system → common_system`
3. `thread_system → container_system → network_system → thread_system`

### Impact

- **Build Issues**: May cause linker errors or initialization order problems
- **Maintenance**: Difficult to modify any system without affecting others
- **Testing**: Cannot test systems in isolation
- **Layer Violations**: Foundation (common_system) depends on higher-level systems

### Solution Strategy

**Dependency Injection via Interfaces** - Move coupling points to abstract interfaces in `common_system`, allowing runtime dependency injection instead of compile-time linking.

---

## Detailed Analysis

### Cycle 1: common_system ↔ monitoring_system

#### Problem

**File**: `/Users/dongcheolshin/Sources/common_system/include/kcenon/common/patterns/event_bus.h`

```cpp
// PROBLEM: common_system includes monitoring_system
#include <kcenon/monitoring/core/event_bus.h>
#include <kcenon/monitoring/interfaces/event_bus_interface.h>
```

**Reverse Dependency**: `monitoring_system` includes `common_system`:
- `performance_monitor.h` → `common/interfaces/monitoring_interface.h`
- `result_types.h` → `common/patterns/result.h`
- Multiple adapter files → `common_system`

#### Root Cause

`common_system` is supposed to be the foundation layer (Level 0) but directly depends on `monitoring_system` (Level 2).

#### Solution: Extract Event Bus to Interface

**Step 1**: Create abstract event bus interface in `common_system`

```cpp
// common_system/include/kcenon/common/interfaces/event_bus_interface.h
namespace common::interfaces {
    template<typename EventType>
    class IEventBus {
    public:
        virtual ~IEventBus() = default;

        virtual void publish(const EventType& event) = 0;
        virtual void subscribe(std::function<void(const EventType&)> handler) = 0;
        virtual void unsubscribe(size_t subscription_id) = 0;
    };
}
```

**Step 2**: Update `common_system/event_bus.h` to use interface pointer

```cpp
// common_system/include/kcenon/common/patterns/event_bus.h
#pragma once

#include <kcenon/common/interfaces/event_bus_interface.h>
#include <memory>

namespace common::patterns {
    template<typename EventType>
    class EventBus {
    private:
        std::shared_ptr<interfaces::IEventBus<EventType>> impl_;

    public:
        // Dependency injection constructor
        explicit EventBus(std::shared_ptr<interfaces::IEventBus<EventType>> impl)
            : impl_(std::move(impl)) {}

        void publish(const EventType& event) {
            if (impl_) impl_->publish(event);
        }

        void subscribe(std::function<void(const EventType&)> handler) {
            if (impl_) impl_->subscribe(std::move(handler));
        }
    };
}
```

**Step 3**: `monitoring_system` implements the interface

```cpp
// monitoring_system/include/kcenon/monitoring/core/event_bus_impl.h
#pragma once

#include <kcenon/common/interfaces/event_bus_interface.h>
#include <vector>
#include <mutex>

namespace monitoring {
    template<typename EventType>
    class EventBusImpl : public common::interfaces::IEventBus<EventType> {
    private:
        std::vector<std::function<void(const EventType&)>> handlers_;
        mutable std::mutex mutex_;

    public:
        void publish(const EventType& event) override {
            std::lock_guard lock(mutex_);
            for (const auto& handler : handlers_) {
                handler(event);
            }
        }

        void subscribe(std::function<void(const EventType&)> handler) override {
            std::lock_guard lock(mutex_);
            handlers_.push_back(std::move(handler));
        }

        void unsubscribe(size_t subscription_id) override {
            std::lock_guard lock(mutex_);
            if (subscription_id < handlers_.size()) {
                handlers_.erase(handlers_.begin() + subscription_id);
            }
        }
    };
}
```

**Impact**: Breaks `common_system → monitoring_system` dependency

---

### Cycle 2: logger_system ↔ thread_system

#### Problem

**Current Dependency**:
- `logger_system` → `thread_system` (for async logging worker threads)
- `thread_system` → `common_system` → `monitoring_system` → `logger_system` (cycle via monitoring)

#### Analysis

The direct dependency `logger_system → thread_system` is valid (higher-level system can depend on lower-level).

The problem is the reverse path through monitoring/common.

#### Solution

Once Cycle 1 is fixed (common → monitoring broken), this cycle is automatically resolved.

**Verification Required**:
- Check if `thread_system` directly includes `logger_system` (should not)
- Ensure `thread_system` only depends on `common_system`

---

### Cycle 3: thread_system ↔ container_system ↔ network_system

#### Problem

```
thread_system → container_system → network_system → thread_system
```

#### Investigation Needed

Check actual include dependencies:

```bash
grep -r "#include.*thread" /Users/dongcheolshin/Sources/container_system/
grep -r "#include.*container" /Users/dongcheolshin/Sources/network_system/
grep -r "#include.*network" /Users/dongcheolshin/Sources/thread_system/
```

#### Likely Scenario

- `thread_system` provides threading primitives
- `container_system` uses threads for concurrent containers
- `network_system` uses threads for async I/O
- Some systems may be using each other's utilities

#### Solution Strategy

1. **Interface Segregation**: Extract thread pool interface to `common_system`
2. **Remove Unnecessary Dependencies**: Check if dependencies are truly needed or can be removed

```cpp
// common_system/include/kcenon/common/interfaces/thread_pool_interface.h
namespace common::interfaces {
    class IThreadPool {
    public:
        virtual ~IThreadPool() = default;

        virtual void enqueue(std::function<void()> task) = 0;
        virtual size_t thread_count() const = 0;
        virtual void wait_all() = 0;
    };
}
```

---

## Implementation Plan

### Week 1: Analysis and Preparation

#### Day 1: Detailed Code Investigation

**Tasks**:
1. Map all include dependencies manually
2. Identify which dependencies are essential vs. convenience
3. Create dependency matrix spreadsheet

**Deliverables**:
- `docs/DEPENDENCY_MATRIX.md` - Complete mapping
- List of "nice-to-have" vs. "must-have" dependencies

#### Day 2: Interface Design

**Tasks**:
1. Design `IEventBus` interface
2. Design `IThreadPool` interface
3. Design other needed interfaces
4. Review with team

**Deliverables**:
- `common_system/include/kcenon/common/interfaces/` (all interface headers)
- Architecture decision record (ADR)

#### Day 3: Create Feature Branch

**Tasks**:
```bash
cd /Users/dongcheolshin/Sources/common_system
git checkout -b feat/phase4-dependency-refactoring

cd /Users/dongcheolshin/Sources/monitoring_system
git checkout -b feat/phase4-dependency-refactoring

cd /Users/dongcheolshin/Sources/thread_system
git checkout -b feat/phase4-dependency-refactoring
```

**Deliverables**:
- Clean feature branches for all affected systems

---

### Week 2-3: Implementation

#### Task 1: Fix Cycle 1 (common ↔ monitoring)

**Duration**: 3 days

**Steps**:

1. **Create Interfaces** (4 hours)
   - Add `common_system/include/kcenon/common/interfaces/event_bus_interface.h`
   - Add unit tests for interface

2. **Refactor common_system** (4 hours)
   - Update `event_bus.h` to use dependency injection
   - Remove direct includes of `monitoring_system`
   - Update all usages in `common_system`

3. **Implement in monitoring_system** (4 hours)
   - Create `EventBusImpl` class
   - Implement interface methods
   - Add factory function

4. **Update Integration Points** (4 hours)
   - Find all places that create `EventBus`
   - Inject `EventBusImpl` instance
   - Update tests

5. **Testing** (4 hours)
   - Unit tests for new interface
   - Integration tests for event bus
   - Verify no circular dependency in build

**Success Criteria**:
- `common_system` CMakeLists.txt has no dependency on `monitoring_system`
- `common_system` includes have no `#include <kcenon/monitoring/...>`
- All tests pass
- Build succeeds

#### Task 2: Verify Cycle 2 Resolution

**Duration**: 1 day

**Steps**:

1. Re-run dependency analyzer
2. Verify no `common → monitoring → logger → thread → common` cycle
3. Check that `logger → thread` is unidirectional and valid

**Success Criteria**:
- Dependency analyzer shows no Cycle 2
- Layer diagram shows proper hierarchy

#### Task 3: Fix Cycle 3 (thread ↔ container ↔ network)

**Duration**: 3 days

**Steps**:

1. **Investigate** (4 hours)
   - Map actual includes
   - Determine root cause
   - Design solution (likely interface extraction)

2. **Extract Thread Pool Interface** (4 hours)
   - Create `IThreadPool` in `common_system`
   - Update `thread_system` to implement interface

3. **Update Dependent Systems** (8 hours)
   - `container_system` uses `IThreadPool*` instead of concrete class
   - `network_system` uses `IThreadPool*` instead of concrete class
   - Inject thread pool instances at runtime

4. **Testing** (4 hours)
   - Unit tests for interface
   - Integration tests for thread pool usage
   - Concurrent stress tests

**Success Criteria**:
- No circular dependency in build
- All three systems build independently
- Tests pass

#### Task 4: CMakeLists.txt Cleanup

**Duration**: 2 days

**Steps**:

1. Remove unnecessary `target_link_libraries` entries
2. Update dependency finder modules (e.g., `ThreadSystemDependencies.cmake`)
3. Verify dependency graph is acyclic

**Success Criteria**:
- Dependency analyzer reports 0 circular dependencies
- CMake configuration succeeds for all systems

---

### Week 4: Validation and Documentation

#### Task 5: Comprehensive Testing

**Duration**: 3 days

**Test Matrix**:

| System | Builds Standalone | Tests Pass | No Circular Deps |
|--------|-------------------|------------|------------------|
| common_system | ✅ | ✅ | ✅ |
| thread_system | ✅ | ✅ | ✅ |
| logger_system | ✅ | ✅ | ✅ |
| monitoring_system | ✅ | ✅ | ✅ |
| container_system | ✅ | ✅ | ✅ |
| database_system | ✅ | ✅ | ✅ |
| network_system | ✅ | ✅ | ✅ |

**Integration Tests**:
- Full system integration with dependency injection
- Performance benchmarks (ensure no regression)
- Memory leak tests (ASAN/LSAN)

#### Task 6: Documentation

**Duration**: 2 days

**Documents to Create/Update**:

1. **PHASE4_COMPLETION.md** (similar to PHASE2_COMPLETION.md)
   - Summary of refactoring
   - Before/after dependency graphs
   - Performance impact analysis

2. **DEPENDENCY_INJECTION_GUIDE.md**
   - How to use new interfaces
   - Best practices for dependency injection
   - Migration guide for existing code

3. **ARCHITECTURE.md** Update
   - New layer diagram (acyclic)
   - Interface-based coupling explanation
   - Dependency rules

4. **README.md** Updates for each system
   - Dependency section updates
   - Build instructions updates

---

## Risk Mitigation

### Risk 1: Breaking Changes

**Probability**: High
**Impact**: High

**Mitigation**:
1. **Feature Flags**: Use `#ifdef` to maintain old behavior during transition
2. **Adapter Pattern**: Create adapters for backward compatibility
3. **Gradual Migration**: Migrate one system at a time
4. **Rollback Plan**: Keep feature branches until fully validated

### Risk 2: Performance Regression

**Probability**: Medium
**Impact**: Medium

**Mitigation**:
1. **Benchmark Before/After**: Run BASELINE.md benchmarks
2. **Profile Critical Paths**: Ensure virtual function overhead is acceptable
3. **Optimization**: Use templates instead of virtual functions where needed

Example optimization:

```cpp
// Instead of always virtual:
template<typename EventBusImpl>
class EventBus {
    EventBusImpl impl_;  // No virtual call overhead
public:
    void publish(const auto& event) { impl_.publish(event); }
};
```

### Risk 3: Incomplete Migration

**Probability**: Medium
**Impact**: High

**Mitigation**:
1. **Automated Validation**: Dependency analyzer in CI/CD
2. **Comprehensive Testing**: 100% system interaction coverage
3. **Code Review**: Team review of all changes
4. **Regression Suite**: Automated tests prevent backsliding

---

## Success Criteria

### Phase 4 Complete When:

- ✅ **Zero Circular Dependencies** - Dependency analyzer reports 0 cycles
- ✅ **Clean Layer Architecture** - Foundation (L0) has no upward dependencies
- ✅ **All Systems Build Independently** - Each system builds without others
- ✅ **All Tests Pass** - 100% test suite passing
- ✅ **No Performance Regression** - BASELINE.md metrics maintained (±5%)
- ✅ **Documentation Complete** - PHASE4_COMPLETION.md, guides, updated READMEs
- ✅ **CI/CD Validation** - Automated dependency checking in place

---

## Timeline Summary

| Week | Tasks | Deliverables |
|------|-------|--------------|
| 1 | Analysis, Design, Branch Setup | Interfaces designed, branches ready |
| 2 | Fix Cycle 1, 2 | common ↔ monitoring broken, verified |
| 3 | Fix Cycle 3, CMake cleanup | All cycles resolved |
| 4 | Testing, Documentation | PHASE4_COMPLETION.md, all tests pass |

**Total Duration**: 4 weeks
**Estimated Effort**: ~80 engineering hours
**Resources Required**: 1-2 engineers

---

## Validation Commands

### Check for Circular Dependencies

```bash
cd /Users/dongcheolshin/Sources/tools
python3 dependency_analyzer.py

# Expected output:
# ✅ No circular dependencies found!
```

### Verify Each System Builds Standalone

```bash
#!/bin/bash
SYSTEMS="common_system thread_system logger_system monitoring_system container_system database_system network_system"

for sys in $SYSTEMS; do
    echo "Building $sys..."
    cd /Users/dongcheolshin/Sources/$sys
    mkdir -p build && cd build
    cmake .. && cmake --build .

    if [ $? -eq 0 ]; then
        echo "✅ $sys built successfully"
    else
        echo "❌ $sys build failed"
        exit 1
    fi
done

echo "✅ All systems build independently"
```

### Run Dependency Validation Test

```cpp
// tests/dependency_validation_test.cpp
#include <gtest/gtest.h>
#include "dependency_analyzer.h"

TEST(DependencyValidation, NoCircularDependencies) {
    auto analyzer = DependencyAnalyzer("/Users/dongcheolshin/Sources");
    analyzer.analyze_all_systems();

    auto cycles = analyzer.detect_cycles();

    EXPECT_TRUE(cycles.empty())
        << "Found circular dependencies: "
        << format_cycles(cycles);
}

TEST(DependencyValidation, ProperLayering) {
    // common_system (L0) should have no dependencies
    auto deps = analyzer.get_dependencies("common_system");
    EXPECT_TRUE(deps.empty())
        << "Foundation layer should have no dependencies";
}
```

---

## Next Steps

1. **Review this plan** with team (1 day)
2. **Approve interfaces** design (1 day)
3. **Begin Week 1** implementation

---

**Plan Status**: ✅ APPROVED FOR EXECUTION
**Created**: 2025-10-10
**Maintainer**: kcenon
**Phase**: 4 - Dependency Refactoring
