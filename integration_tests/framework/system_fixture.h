#pragma once

#include <gtest/gtest.h>
#include <memory>
#include <chrono>
#include <vector>
#include <algorithm>
#include <numeric>

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

    /**
     * @brief Create a test instance of a given type
     * @tparam T Type to instantiate
     * @return Unique pointer to the instance
     */
    template<typename T>
    std::unique_ptr<T> create_test_instance() {
        return std::make_unique<T>();
    }

    /**
     * @brief Calculate percentile from a sorted vector of durations
     * @param latencies Sorted vector of durations
     * @param percentile Percentile to calculate (0-100)
     * @return Duration at the specified percentile
     */
    template<typename Duration>
    Duration calculate_percentile(std::vector<Duration> latencies, int percentile) {
        if (latencies.empty()) {
            return Duration::zero();
        }

        std::sort(latencies.begin(), latencies.end());
        size_t index = (latencies.size() * percentile) / 100;
        if (index >= latencies.size()) {
            index = latencies.size() - 1;
        }
        return latencies[index];
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

/**
 * @brief Fixture for performance integration tests
 *
 * Provides utilities for measuring latency, throughput,
 * and resource usage across system boundaries.
 */
class PerformanceIntegrationTest : public MultiSystemFixture {
protected:
    /**
     * @brief Measure execution time of a callable
     * @tparam Func Callable type
     * @param func Function to measure
     * @return Execution duration in nanoseconds
     */
    template<typename Func>
    std::chrono::nanoseconds measure_execution_time(Func&& func) {
        auto start = std::chrono::steady_clock::now();
        func();
        auto end = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    }

    /**
     * @brief Run a benchmark multiple times and collect statistics
     * @tparam Func Callable type
     * @param func Function to benchmark
     * @param iterations Number of iterations
     * @return Vector of execution times
     */
    template<typename Func>
    std::vector<std::chrono::nanoseconds> benchmark(Func&& func, size_t iterations) {
        std::vector<std::chrono::nanoseconds> results;
        results.reserve(iterations);

        for (size_t i = 0; i < iterations; ++i) {
            results.push_back(measure_execution_time(func));
        }

        return results;
    }
};

} // namespace integration_tests
