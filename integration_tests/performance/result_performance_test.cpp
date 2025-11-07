#include "system_fixture.h"
#include "test_helpers.h"
#include <kcenon/common/patterns/result.h>
#include <vector>
#include <chrono>
#include <numeric>

using namespace integration_tests;
using namespace kcenon::common;

/**
 * Performance tests for Result<T> pattern
 */
class ResultPerformanceTest : public PerformanceIntegrationTest {};

TEST_F(ResultPerformanceTest, ResultCreationLatency) {
    const int iterations = 10000;
    std::vector<std::chrono::nanoseconds> latencies;
    latencies.reserve(iterations);

    for (int i = 0; i < iterations; ++i) {
        auto latency = measure_execution_time([&]() {
            auto result = Result<int>::ok(42);
            (void)result;
        });
        latencies.push_back(latency);
    }

    auto p50 = calculate_percentile(latencies, 50);
    auto p95 = calculate_percentile(latencies, 95);
    auto p99 = calculate_percentile(latencies, 99);

    // Log results
    std::cout << "Result creation latency (ns):\n"
              << "  P50: " << p50.count() << "\n"
              << "  P95: " << p95.count() << "\n"
              << "  P99: " << p99.count() << "\n";

    // Verify reasonable performance (adjust thresholds based on baseline)
    EXPECT_LT(p50.count(), 1000) << "P50 latency too high";
    EXPECT_LT(p99.count(), 10000) << "P99 latency too high";
}

TEST_F(ResultPerformanceTest, MapOperationOverhead) {
    const int iterations = 10000;
    std::vector<std::chrono::nanoseconds> latencies;
    latencies.reserve(iterations);

    auto result = Result<int>::ok(10);

    for (int i = 0; i < iterations; ++i) {
        auto latency = measure_execution_time([&]() {
            auto mapped = result.map([](int x) { return x * 2; });
            (void)mapped;
        });
        latencies.push_back(latency);
    }

    auto p50 = calculate_percentile(latencies, 50);
    auto p95 = calculate_percentile(latencies, 95);
    auto p99 = calculate_percentile(latencies, 99);

    std::cout << "Map operation latency (ns):\n"
              << "  P50: " << p50.count() << "\n"
              << "  P95: " << p95.count() << "\n"
              << "  P99: " << p99.count() << "\n";

    EXPECT_LT(p50.count(), 2000) << "P50 map latency too high";
    EXPECT_LT(p99.count(), 20000) << "P99 map latency too high";
}

TEST_F(ResultPerformanceTest, AndThenChainingOverhead) {
    const int iterations = 10000;
    std::vector<std::chrono::nanoseconds> latencies;
    latencies.reserve(iterations);

    auto result = Result<int>::ok(10);

    for (int i = 0; i < iterations; ++i) {
        auto latency = measure_execution_time([&]() {
            auto chained = result.and_then([](int x) -> Result<std::string> {
                return Result<std::string>::ok(std::to_string(x));
            });
            (void)chained;
        });
        latencies.push_back(latency);
    }

    auto p50 = calculate_percentile(latencies, 50);
    auto p95 = calculate_percentile(latencies, 95);
    auto p99 = calculate_percentile(latencies, 99);

    std::cout << "and_then operation latency (ns):\n"
              << "  P50: " << p50.count() << "\n"
              << "  P95: " << p95.count() << "\n"
              << "  P99: " << p99.count() << "\n";

    EXPECT_LT(p50.count(), 3000) << "P50 and_then latency too high";
    EXPECT_LT(p99.count(), 30000) << "P99 and_then latency too high";
}

TEST_F(ResultPerformanceTest, ComplexChainPerformance) {
    const int iterations = 10000;
    std::vector<std::chrono::nanoseconds> latencies;
    latencies.reserve(iterations);

    for (int i = 0; i < iterations; ++i) {
        auto latency = measure_execution_time([&]() {
            auto result = Result<int>::ok(10)
                .map([](int x) { return x + 5; })
                .and_then([](int x) -> Result<int> {
                    return Result<int>::ok(x * 2);
                })
                .map([](int x) { return x - 10; })
                .and_then([](int x) -> Result<std::string> {
                    return Result<std::string>::ok(std::to_string(x));
                });
            (void)result;
        });
        latencies.push_back(latency);
    }

    auto p50 = calculate_percentile(latencies, 50);
    auto p95 = calculate_percentile(latencies, 95);
    auto p99 = calculate_percentile(latencies, 99);

    std::cout << "Complex chain latency (ns):\n"
              << "  P50: " << p50.count() << "\n"
              << "  P95: " << p95.count() << "\n"
              << "  P99: " << p99.count() << "\n";

    EXPECT_LT(p50.count(), 10000) << "P50 complex chain latency too high";
    EXPECT_LT(p99.count(), 100000) << "P99 complex chain latency too high";
}

TEST_F(ResultPerformanceTest, ErrorPathPerformance) {
    const int iterations = 10000;
    std::vector<std::chrono::nanoseconds> latencies;
    latencies.reserve(iterations);

    auto error = Result<int>::err(error_code{1, "test error"});

    for (int i = 0; i < iterations; ++i) {
        auto latency = measure_execution_time([&]() {
            auto recovered = error.or_else([](const error_code&) {
                return Result<int>::ok(99);
            });
            (void)recovered;
        });
        latencies.push_back(latency);
    }

    auto p50 = calculate_percentile(latencies, 50);
    auto p95 = calculate_percentile(latencies, 95);
    auto p99 = calculate_percentile(latencies, 99);

    std::cout << "Error recovery latency (ns):\n"
              << "  P50: " << p50.count() << "\n"
              << "  P95: " << p95.count() << "\n"
              << "  P99: " << p99.count() << "\n";

    EXPECT_LT(p50.count(), 3000) << "P50 error recovery latency too high";
    EXPECT_LT(p99.count(), 30000) << "P99 error recovery latency too high";
}

TEST_F(ResultPerformanceTest, Throughput) {
    const int total_operations = 1000000;
    const auto start = std::chrono::steady_clock::now();

    for (int i = 0; i < total_operations; ++i) {
        auto result = Result<int>::ok(i)
            .map([](int x) { return x + 1; })
            .and_then([](int x) -> Result<int> {
                return Result<int>::ok(x * 2);
            });
        (void)result;
    }

    const auto end = std::chrono::steady_clock::now();
    const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    const double ops_per_second = (total_operations * 1000.0) / duration.count();

    std::cout << "Result throughput:\n"
              << "  Total operations: " << total_operations << "\n"
              << "  Duration: " << duration.count() << " ms\n"
              << "  Throughput: " << ops_per_second << " ops/sec\n";

    // Expect at least 1M ops/sec (adjust based on baseline)
    EXPECT_GT(ops_per_second, 1000000.0) << "Throughput too low";
}

TEST_F(ResultPerformanceTest, MemoryOverhead) {
    // Measure memory overhead of Result<T> vs T
    struct SmallStruct {
        int value;
    };

    struct LargeStruct {
        char data[1024];
    };

    std::cout << "Memory overhead:\n"
              << "  sizeof(int): " << sizeof(int) << " bytes\n"
              << "  sizeof(Result<int>): " << sizeof(Result<int>) << " bytes\n"
              << "  sizeof(SmallStruct): " << sizeof(SmallStruct) << " bytes\n"
              << "  sizeof(Result<SmallStruct>): " << sizeof(Result<SmallStruct>) << " bytes\n"
              << "  sizeof(LargeStruct): " << sizeof(LargeStruct) << " bytes\n"
              << "  sizeof(Result<LargeStruct>): " << sizeof(Result<LargeStruct>) << " bytes\n";

    // Result should have reasonable overhead
    // Note: Result<T> stores both std::optional<T> and std::optional<error_info>
    // error_info contains: int (4 bytes) + 2 std::string (48-64 bytes) +
    // std::optional<std::string> (32-40 bytes), totaling ~96-108 bytes
    // std::optional adds ~8 bytes overhead per optional
    // Total Result overhead = sizeof(std::optional<error_info>) ≈ 104-112 bytes
    EXPECT_LE(sizeof(Result<int>), 128)
        << "Result<int> has excessive overhead";

    // For large types, the overhead is the same absolute amount (std::optional<error_info>)
    // Allow 128 bytes overhead to account for padding and alignment across different platforms
    // (macOS: 104 bytes, Linux: 128 bytes due to different alignment requirements)
    EXPECT_LE(sizeof(Result<LargeStruct>), sizeof(LargeStruct) + 128)
        << "Result<LargeStruct> has excessive overhead";
}

TEST_F(ResultPerformanceTest, MoveVsCopyPerformance) {
    struct LargeData {
        std::vector<int> data;
        LargeData() : data(1000, 42) {}
    };

    const int iterations = 1000;

    // Measure copy performance
    std::vector<std::chrono::nanoseconds> copy_latencies;
    copy_latencies.reserve(iterations);

    for (int i = 0; i < iterations; ++i) {
        LargeData source;
        auto latency = measure_execution_time([&]() {
            auto result = Result<LargeData>::ok(source);
            (void)result;
        });
        copy_latencies.push_back(latency);
    }

    // Measure move performance
    std::vector<std::chrono::nanoseconds> move_latencies;
    move_latencies.reserve(iterations);

    for (int i = 0; i < iterations; ++i) {
        LargeData source;
        auto latency = measure_execution_time([&]() {
            auto result = Result<LargeData>::ok(std::move(source));
            (void)result;
        });
        move_latencies.push_back(latency);
    }

    auto copy_p50 = calculate_percentile(copy_latencies, 50);
    auto move_p50 = calculate_percentile(move_latencies, 50);

    std::cout << "Move vs Copy performance:\n"
              << "  Copy P50: " << copy_p50.count() << " ns\n"
              << "  Move P50: " << move_p50.count() << " ns\n"
              << "  Speedup: " << (static_cast<double>(copy_p50.count()) / move_p50.count()) << "x\n";

    // Move should not be significantly slower than copy for large objects
    // Note: Allow 10% tolerance for measurement noise and compiler optimizations
    // Differences of 1-2 ns are not meaningful and can vary between runs
    auto tolerance = static_cast<int64_t>(copy_p50.count() * 0.1);
    EXPECT_LE(move_p50.count(), copy_p50.count() + tolerance)
        << "Move significantly slower than copy (beyond 10% tolerance)";
}
