# Phase 5: Integration Testing - Implementation Plan

**Date**: 2025-10-10
**Phase**: Phase 5 - Integration Testing
**Status**: Planning Complete - Ready for Implementation
**Duration**: 2 weeks (estimated)

---

## Executive Summary

### Objective

Create comprehensive integration test suite to validate cross-system interactions, ensuring:
- Systems work correctly together
- Interfaces are properly implemented
- Error handling works across boundaries
- Performance meets requirements under integration scenarios

### Current State

**Unit Test Coverage**: 60-70% (per-system tests exist)
**Integration Tests**: None (no cross-system tests)
**Target Coverage**: 80%+ (including integration)

### Scope

1. **Integration Test Framework** - Shared testing infrastructure
2. **Cross-System Tests** - 20+ integration scenarios
3. **Failure Scenario Tests** - 15+ error/failure cases
4. **Performance Integration Tests** - 10+ performance scenarios
5. **CI/CD Integration** - Automated test execution

---

## Phase 5 Goals

### Must-Have ✅

- [ ] Integration test framework operational
- [ ] 20+ cross-system integration tests
- [ ] Test coverage increase to 75%+
- [ ] All tests pass in CI/CD
- [ ] No performance regression

### Nice-to-Have 🎯

- [ ] 30+ cross-system tests
- [ ] 80%+ test coverage
- [ ] Performance benchmarks in CI
- [ ] Test report generation

---

## Architecture

### Integration Test Structure

```
common_system/
└── integration_tests/
    ├── CMakeLists.txt
    ├── framework/
    │   ├── system_fixture.h           # Base test fixture
    │   ├── test_helpers.h              # Common utilities
    │   └── mock_factories.h            # Mock object creation
    ├── scenarios/
    │   ├── logger_thread_integration_test.cpp
    │   ├── monitoring_logger_integration_test.cpp
    │   ├── network_thread_integration_test.cpp
    │   └── full_system_integration_test.cpp
    ├── failures/
    │   ├── network_failure_test.cpp
    │   ├── thread_exhaustion_test.cpp
    │   └── memory_pressure_test.cpp
    └── performance/
        ├── cross_system_latency_test.cpp
        └── throughput_under_load_test.cpp
```

### Test Categories

#### 1. Cross-System Integration (20+ tests)

**Purpose**: Verify systems work together correctly

**Examples**:
- Logger + Thread: Async logging with thread pool
- Monitoring + Logger: Event logging with monitoring
- Network + Thread: Async network I/O
- Database + Thread: Connection pooling

#### 2. Failure Scenarios (15+ tests)

**Purpose**: Verify error handling across boundaries

**Examples**:
- Network connection failures
- Thread pool exhaustion
- Database connection timeout
- Memory allocation failures
- Resource cleanup on errors

#### 3. Performance Integration (10+ tests)

**Purpose**: Ensure performance requirements under integration

**Examples**:
- Cross-system call latency
- Throughput with multiple systems
- Memory usage with all systems active
- CPU usage under load

---

## Implementation Plan

### Week 1: Framework and Core Tests

#### Day 1-2: Test Framework Setup

**Task 1.1**: Create integration test infrastructure

```cmake
# common_system/integration_tests/CMakeLists.txt
cmake_minimum_required(VERSION 3.16)

# Find GTest
find_package(GTest REQUIRED)

# Integration test library (shared utilities)
add_library(integration_test_framework INTERFACE)
target_include_directories(integration_test_framework INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/framework
)
target_link_libraries(integration_test_framework INTERFACE
    GTest::gtest
    GTest::gtest_main
)

# Add integration test executable
add_executable(integration_tests
    scenarios/logger_thread_integration_test.cpp
    scenarios/monitoring_logger_integration_test.cpp
    # ... more tests
)

target_link_libraries(integration_tests PRIVATE
    integration_test_framework
    kcenon::common
    # Link all systems being tested
)

# Register with CTest
include(GoogleTest)
gtest_discover_tests(integration_tests)
```

**Task 1.2**: Create base test fixture

```cpp
// framework/system_fixture.h
#pragma once

#include <gtest/gtest.h>
#include <memory>

namespace integration_tests {

/**
 * @brief Base fixture for integration tests
 *
 * Provides common setup/teardown and helper methods
 * for cross-system integration testing.
 */
class SystemFixture : public ::testing::Test {
protected:
    void SetUp() override {
        // Common setup for all integration tests
        // Initialize test environment
    }

    void TearDown() override {
        // Common cleanup
        // Ensure no resource leaks
    }

    // Helper methods for test creation
    template<typename T>
    std::unique_ptr<T> create_test_instance() {
        return std::make_unique<T>();
    }
};

/**
 * @brief Fixture for multi-system integration tests
 *
 * Initializes multiple systems in dependency order
 * and provides lifecycle management.
 */
class MultiSystemFixture : public SystemFixture {
protected:
    void SetUp() override {
        SystemFixture::SetUp();

        // Initialize systems in dependency order
        // Level 0: common (already available)
        // Level 1: thread, logger, container
        // Level 2: monitoring, database, network
    }

    void TearDown() override {
        // Shutdown in reverse order
        // Ensure graceful cleanup

        SystemFixture::TearDown();
    }
};

} // namespace integration_tests
```

#### Day 3-4: Core Integration Tests

**Test 1**: Logger + Thread Integration

```cpp
// scenarios/logger_thread_integration_test.cpp
#include "framework/system_fixture.h"
#include <kcenon/logger/core/logger.h>
#include <kcenon/thread/core/thread_pool.h>

using namespace integration_tests;

class LoggerThreadIntegrationTest : public MultiSystemFixture {
protected:
    // Test-specific setup
};

TEST_F(LoggerThreadIntegrationTest, AsyncLoggingWithThreadPool) {
    // Create logger with thread pool for async logging
    auto thread_pool = std::make_shared<kcenon::thread::thread_pool>(4);
    auto logger = create_logger_with_thread_pool(thread_pool);

    // Log messages asynchronously
    const int num_messages = 1000;
    for (int i = 0; i < num_messages; ++i) {
        logger->info("Test message {}", i);
    }

    // Wait for all messages to be processed
    thread_pool->wait_all();

    // Verify all messages were logged
    auto log_count = count_log_messages();
    EXPECT_EQ(log_count, num_messages);
}

TEST_F(LoggerThreadIntegrationTest, LoggerHandlesThreadPoolShutdown) {
    auto thread_pool = std::make_shared<kcenon::thread::thread_pool>(4);
    auto logger = create_logger_with_thread_pool(thread_pool);

    logger->info("Message before shutdown");

    // Shutdown thread pool
    thread_pool->shutdown();

    // Logger should handle gracefully (either queue or sync log)
    EXPECT_NO_THROW(logger->info("Message after shutdown"));
}
```

**Test 2**: Monitoring + Logger Integration

```cpp
// scenarios/monitoring_logger_integration_test.cpp
TEST_F(MonitoringLoggerIntegrationTest, MonitoringEventsAreLogged) {
    auto logger = create_file_logger("test_monitoring.log");
    auto monitor = create_monitor_with_logger(logger);

    // Record metrics
    monitor->record_counter("test_counter", 10);
    monitor->record_gauge("test_gauge", 42.5);

    // Verify events were logged
    auto logs = read_log_file("test_monitoring.log");
    EXPECT_THAT(logs, Contains("test_counter"));
    EXPECT_THAT(logs, Contains("test_gauge"));
}
```

#### Day 5: Failure Scenario Tests

**Test 3**: Network Failure Handling

```cpp
// failures/network_failure_test.cpp
TEST_F(NetworkFailureTest, HandlesConnectionFailureGracefully) {
    auto network_client = create_network_client();

    // Attempt to connect to non-existent server
    auto result = network_client->connect("localhost", 99999);

    // Should return error, not throw
    EXPECT_FALSE(result.is_ok());
    EXPECT_EQ(result.error().code, error_codes::network::connection_failed);
}

TEST_F(NetworkFailureTest, RetriesOnTransientFailures) {
    auto network_client = create_network_client_with_retry(3);

    // Mock transient failure (succeeds on 2nd attempt)
    mock_transient_network_failure(2);

    auto result = network_client->connect_with_retry("localhost", 8080);

    // Should eventually succeed
    EXPECT_TRUE(result.is_ok());
}
```

---

### Week 2: Advanced Tests and CI Integration

#### Day 1-2: Performance Integration Tests

**Test 4**: Cross-System Latency

```cpp
// performance/cross_system_latency_test.cpp
TEST_F(PerformanceIntegrationTest, LoggerThreadLatency) {
    auto thread_pool = create_thread_pool(4);
    auto logger = create_async_logger(thread_pool);

    // Measure end-to-end latency
    const int iterations = 10000;
    std::vector<std::chrono::nanoseconds> latencies;

    for (int i = 0; i < iterations; ++i) {
        auto start = std::chrono::steady_clock::now();

        logger->info("Test message");
        thread_pool->wait_all(); // Wait for completion

        auto end = std::chrono::steady_clock::now();
        latencies.push_back(end - start);
    }

    // Calculate percentiles
    auto p50 = calculate_percentile(latencies, 50);
    auto p95 = calculate_percentile(latencies, 95);
    auto p99 = calculate_percentile(latencies, 99);

    // Verify latency requirements
    EXPECT_LT(p50.count(), 1000) << "P50 latency > 1μs";
    EXPECT_LT(p95.count(), 5000) << "P95 latency > 5μs";
    EXPECT_LT(p99.count(), 10000) << "P99 latency > 10μs";
}
```

#### Day 3: Full System Integration Test

**Test 5**: All Systems Working Together

```cpp
// scenarios/full_system_integration_test.cpp
TEST_F(FullSystemIntegrationTest, CompleteSystemInitialization) {
    // Initialize all systems
    auto thread_pool = create_thread_pool(8);
    auto logger = create_logger(thread_pool);
    auto monitor = create_monitor(logger, thread_pool);
    auto network = create_network_system(thread_pool, logger);
    auto database = create_database_system(thread_pool, logger);

    // Verify all systems are operational
    EXPECT_TRUE(thread_pool->is_running());
    EXPECT_TRUE(logger->is_initialized());
    EXPECT_TRUE(monitor->is_running());
    EXPECT_TRUE(network->is_initialized());
    EXPECT_TRUE(database->is_connected());
}

TEST_F(FullSystemIntegrationTest, GracefulShutdown) {
    // Initialize all systems
    auto systems = initialize_all_systems();

    // Shutdown in reverse dependency order
    EXPECT_NO_THROW(systems.shutdown_all());

    // Verify clean shutdown
    EXPECT_FALSE(systems.thread_pool->is_running());
    EXPECT_FALSE(systems.monitor->is_running());
}
```

#### Day 4: CI/CD Integration

**Task 2.1**: Add integration tests to CI

```yaml
# .github/workflows/integration-tests.yml
name: Integration Tests

on: [push, pull_request]

jobs:
  integration-tests:
    runs-on: ${{ matrix.os }}
    strategy:
      matrix:
        os: [ubuntu-latest, macos-latest]
        build_type: [Debug, Release]

    steps:
      - uses: actions/checkout@v3

      - name: Install Dependencies
        run: |
          # Install GTest, system dependencies

      - name: Build Integration Tests
        run: |
          cmake -B build -DCMAKE_BUILD_TYPE=${{ matrix.build_type }}
          cmake --build build --target integration_tests

      - name: Run Integration Tests
        run: |
          cd build
          ctest --output-on-failure --tests-regex "integration_"

      - name: Upload Test Results
        if: always()
        uses: actions/upload-artifact@v3
        with:
          name: test-results-${{ matrix.os }}-${{ matrix.build_type }}
          path: build/Testing/Temporary/
```

#### Day 5: Documentation and Coverage

**Task 2.2**: Measure test coverage

```bash
# tools/measure_coverage.sh
#!/bin/bash

# Build with coverage flags
cmake -B build_coverage \
    -DCMAKE_BUILD_TYPE=Debug \
    -DENABLE_COVERAGE=ON

cmake --build build_coverage

# Run all tests
cd build_coverage
ctest --output-on-failure

# Generate coverage report
lcov --capture --directory . --output-file coverage.info
lcov --remove coverage.info '/usr/*' '*/vcpkg/*' '*/test/*' --output-file coverage_filtered.info

# Generate HTML report
genhtml coverage_filtered.info --output-directory coverage_html

# Display summary
lcov --list coverage_filtered.info
```

---

## Test Scenarios

### Priority 1: Core Integration (Must-Have)

1. ✅ Logger + Thread: Async logging
2. ✅ Monitoring + Logger: Event logging
3. ✅ Network + Thread: Async I/O
4. ✅ Database + Thread: Connection pooling
5. ✅ Logger + Monitoring + Thread: Full observability

### Priority 2: Failure Scenarios (Must-Have)

6. ✅ Network connection failure
7. ✅ Thread pool exhaustion
8. ✅ Database connection timeout
9. ✅ Memory allocation failure
10. ✅ Graceful degradation

### Priority 3: Performance (Nice-to-Have)

11. ✅ Cross-system latency
12. ✅ Throughput under load
13. ✅ Memory usage integration
14. ✅ CPU usage under stress

---

## Success Criteria

### Phase 5 Complete When:

- ✅ Integration test framework operational
- ✅ 20+ integration tests implemented
- ✅ All tests pass in CI/CD
- ✅ Test coverage ≥75%
- ✅ No performance regression
- ✅ Documentation complete
- ✅ PHASE5_COMPLETION.md created

### Metrics

| Metric | Current | Target | Status |
|--------|---------|--------|--------|
| Unit Test Coverage | 60-70% | - | Baseline |
| Integration Tests | 0 | 20+ | TODO |
| Total Coverage | 60-70% | 75%+ | TODO |
| CI Pass Rate | N/A | 100% | TODO |
| Performance | Baseline | No regression | TODO |

---

## Deliverables

### Week 1

1. ✅ Integration test framework (CMakeLists.txt, fixtures)
2. ✅ Core integration tests (logger+thread, monitoring+logger)
3. ✅ Failure scenario tests (network, thread pool)
4. ✅ Test documentation

### Week 2

1. ✅ Performance integration tests
2. ✅ Full system integration test
3. ✅ CI/CD integration (GitHub Actions)
4. ✅ Coverage measurement and reporting
5. ✅ PHASE5_COMPLETION.md

---

## Risk Mitigation

### Risk 1: Test Environment Setup Complexity

**Probability**: Medium
**Impact**: Medium

**Mitigation**:
- Use Docker for consistent environments
- Mock external dependencies (databases, networks)
- Provide clear setup documentation

### Risk 2: Flaky Tests

**Probability**: High (integration tests often flaky)
**Impact**: High

**Mitigation**:
- Use deterministic test data
- Proper cleanup in TearDown
- Retry logic for transient failures
- Test isolation (no shared state)

### Risk 3: CI/CD Performance

**Probability**: Low
**Impact**: Medium

**Mitigation**:
- Run integration tests in parallel
- Cache dependencies
- Use matrix builds efficiently
- Timeout protection

---

## Timeline

| Week | Day | Task | Deliverable |
|------|-----|------|-------------|
| 1 | 1-2 | Framework setup | CMakeLists.txt, fixtures |
| 1 | 3-4 | Core integration tests | 10+ tests |
| 1 | 5 | Failure scenario tests | 5+ tests |
| 2 | 1-2 | Performance tests | 5+ tests |
| 2 | 3 | Full system test | 1 comprehensive test |
| 2 | 4 | CI/CD integration | GitHub Actions |
| 2 | 5 | Documentation | PHASE5_COMPLETION.md |

**Total Duration**: 2 weeks
**Estimated Effort**: 40 hours

---

## Next Steps

1. **Day 1 Morning**: Create integration_tests/ directory structure
2. **Day 1 Afternoon**: Implement test framework (fixtures, helpers)
3. **Day 2**: Write first 5 integration tests
4. **Day 3**: Add failure scenario tests
5. **Week 2**: Performance tests, CI integration, documentation

---

**Plan Status**: ✅ READY FOR EXECUTION
**Created**: 2025-10-10
**Maintainer**: kcenon
**Phase**: 5 - Integration Testing
