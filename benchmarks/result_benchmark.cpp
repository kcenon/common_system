// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

#include <benchmark/benchmark.h>
#include <kcenon/common/patterns/result.h>
#include <string>
#include <vector>

using namespace kcenon::common;

// Result creation benchmarks
static void BM_ResultOkCreation(benchmark::State& state) {
    for (auto _ : state) {
        auto result = Result<int>::ok(42);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_ResultOkCreation);

static void BM_ResultErrorCreation(benchmark::State& state) {
    for (auto _ : state) {
        auto result = Result<int>::err(error_code{1, "test error"});
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_ResultErrorCreation);

// Map operation benchmarks
static void BM_ResultMapSingle(benchmark::State& state) {
    auto result = Result<int>::ok(10);
    for (auto _ : state) {
        auto mapped = result.map([](int x) { return x * 2; });
        benchmark::DoNotOptimize(mapped);
    }
}
BENCHMARK(BM_ResultMapSingle);

static void BM_ResultMapChain(benchmark::State& state) {
    auto result = Result<int>::ok(10);
    for (auto _ : state) {
        auto mapped = result
            .map([](int x) { return x + 1; })
            .map([](int x) { return x * 2; })
            .map([](int x) { return x - 5; });
        benchmark::DoNotOptimize(mapped);
    }
}
BENCHMARK(BM_ResultMapChain);

// and_then operation benchmarks
static void BM_ResultAndThenSingle(benchmark::State& state) {
    auto result = Result<int>::ok(10);
    for (auto _ : state) {
        auto chained = result.and_then([](int x) -> Result<std::string> {
            return Result<std::string>::ok(std::to_string(x));
        });
        benchmark::DoNotOptimize(chained);
    }
}
BENCHMARK(BM_ResultAndThenSingle);

static void BM_ResultAndThenChain(benchmark::State& state) {
    auto result = Result<int>::ok(10);
    for (auto _ : state) {
        auto chained = result
            .and_then([](int x) -> Result<int> {
                return Result<int>::ok(x + 1);
            })
            .and_then([](int x) -> Result<int> {
                return Result<int>::ok(x * 2);
            })
            .and_then([](int x) -> Result<std::string> {
                return Result<std::string>::ok(std::to_string(x));
            });
        benchmark::DoNotOptimize(chained);
    }
}
BENCHMARK(BM_ResultAndThenChain);

// Error path benchmarks
static void BM_ResultOrElse(benchmark::State& state) {
    auto error = Result<int>::err(error_code{1, "error"});
    for (auto _ : state) {
        auto recovered = error.or_else([](const error_code&) {
            return Result<int>::ok(99);
        });
        benchmark::DoNotOptimize(recovered);
    }
}
BENCHMARK(BM_ResultOrElse);

// Complex chain benchmark
static void BM_ResultComplexChain(benchmark::State& state) {
    for (auto _ : state) {
        auto result = Result<int>::ok(10)
            .map([](int x) { return x + 5; })
            .and_then([](int x) -> Result<int> {
                return Result<int>::ok(x * 2);
            })
            .map([](int x) { return x - 10; })
            .and_then([](int x) -> Result<std::string> {
                return Result<std::string>::ok(std::to_string(x));
            });
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_ResultComplexChain);

// Large data benchmarks
struct LargeData {
    std::vector<int> data;
    LargeData() : data(1000, 42) {}
    LargeData(const LargeData&) = default;
    LargeData(LargeData&&) = default;
    LargeData& operator=(const LargeData&) = default;
    LargeData& operator=(LargeData&&) = default;
};

static void BM_ResultLargeDataCopy(benchmark::State& state) {
    LargeData source;
    for (auto _ : state) {
        auto result = Result<LargeData>::ok(source);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_ResultLargeDataCopy);

static void BM_ResultLargeDataMove(benchmark::State& state) {
    for (auto _ : state) {
        LargeData source;
        auto result = Result<LargeData>::ok(std::move(source));
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_ResultLargeDataMove);

// Throughput benchmark with varying iteration counts
static void BM_ResultThroughput(benchmark::State& state) {
    const int batch_size = state.range(0);
    for (auto _ : state) {
        for (int i = 0; i < batch_size; ++i) {
            auto result = Result<int>::ok(i)
                .map([](int x) { return x + 1; })
                .and_then([](int x) -> Result<int> {
                    return Result<int>::ok(x * 2);
                });
            benchmark::DoNotOptimize(result);
        }
    }
    state.SetItemsProcessed(state.iterations() * batch_size);
}
BENCHMARK(BM_ResultThroughput)->Range(100, 10000);
