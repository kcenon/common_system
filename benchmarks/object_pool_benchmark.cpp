// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

#include <benchmark/benchmark.h>
#include <kcenon/common/utils/object_pool.h>
#include <memory>
#include <string>
#include <vector>

using namespace kcenon::common::utils;

// Simple object for pool testing
struct SimpleObject {
    int value;
    explicit SimpleObject(int v = 0) : value(v) {}
};

// Complex object for pool testing
struct ComplexObject {
    std::string name;
    std::vector<int> data;

    ComplexObject() : name("default"), data(100, 0) {}
    explicit ComplexObject(const std::string& n, int size)
        : name(n), data(size, 42) {}
};

// Large object for pool testing
struct LargeObject {
    std::array<char, 4096> buffer;
    int id;

    LargeObject() : buffer{}, id(0) {}
    explicit LargeObject(int i) : buffer{}, id(i) {
        std::fill(buffer.begin(), buffer.end(), static_cast<char>(i % 256));
    }
};

// Basic acquire/release benchmarks
static void BM_ObjectPoolAcquireRelease(benchmark::State& state) {
    ObjectPool<SimpleObject> pool;

    for (auto _ : state) {
        auto obj = pool.acquire(42);
        benchmark::DoNotOptimize(obj);
    }
}
BENCHMARK(BM_ObjectPoolAcquireRelease);

static void BM_NewDeleteSimpleObject(benchmark::State& state) {
    for (auto _ : state) {
        auto obj = std::make_unique<SimpleObject>(42);
        benchmark::DoNotOptimize(obj);
    }
}
BENCHMARK(BM_NewDeleteSimpleObject);

// Complex object benchmarks
static void BM_ObjectPoolComplexAcquireRelease(benchmark::State& state) {
    ObjectPool<ComplexObject> pool;

    for (auto _ : state) {
        auto obj = pool.acquire("test", 100);
        benchmark::DoNotOptimize(obj);
    }
}
BENCHMARK(BM_ObjectPoolComplexAcquireRelease);

static void BM_NewDeleteComplexObject(benchmark::State& state) {
    for (auto _ : state) {
        auto obj = std::make_unique<ComplexObject>("test", 100);
        benchmark::DoNotOptimize(obj);
    }
}
BENCHMARK(BM_NewDeleteComplexObject);

// Large object benchmarks
static void BM_ObjectPoolLargeAcquireRelease(benchmark::State& state) {
    ObjectPool<LargeObject> pool;

    for (auto _ : state) {
        auto obj = pool.acquire(42);
        benchmark::DoNotOptimize(obj);
    }
}
BENCHMARK(BM_ObjectPoolLargeAcquireRelease);

static void BM_NewDeleteLargeObject(benchmark::State& state) {
    for (auto _ : state) {
        auto obj = std::make_unique<LargeObject>(42);
        benchmark::DoNotOptimize(obj);
    }
}
BENCHMARK(BM_NewDeleteLargeObject);

// Pre-reserved pool benchmarks
static void BM_ObjectPoolPreReserved(benchmark::State& state) {
    ObjectPool<SimpleObject> pool;
    pool.reserve(1000);

    for (auto _ : state) {
        auto obj = pool.acquire(42);
        benchmark::DoNotOptimize(obj);
    }
}
BENCHMARK(BM_ObjectPoolPreReserved);

// Reuse rate benchmark
static void BM_ObjectPoolReuseRate(benchmark::State& state) {
    ObjectPool<SimpleObject> pool;
    int reused_count = 0;
    int total_count = 0;

    for (auto _ : state) {
        bool reused = false;
        auto obj = pool.acquire(&reused, 42);
        if (reused) ++reused_count;
        ++total_count;
        benchmark::DoNotOptimize(obj);
    }

    state.counters["reuse_rate"] = benchmark::Counter(
        static_cast<double>(reused_count) / total_count * 100,
        benchmark::Counter::kDefaults,
        benchmark::Counter::kIs1000
    );
}
BENCHMARK(BM_ObjectPoolReuseRate);

// Batch operations benchmark
static void BM_ObjectPoolBatchAcquire(benchmark::State& state) {
    ObjectPool<SimpleObject> pool;
    const int batch_size = state.range(0);

    for (auto _ : state) {
        std::vector<std::unique_ptr<SimpleObject, std::function<void(SimpleObject*)>>> objects;
        objects.reserve(batch_size);

        for (int i = 0; i < batch_size; ++i) {
            objects.push_back(pool.acquire(i));
        }
        benchmark::DoNotOptimize(objects);
    }
    state.SetItemsProcessed(state.iterations() * batch_size);
}
BENCHMARK(BM_ObjectPoolBatchAcquire)->Range(10, 1000);

static void BM_NewDeleteBatch(benchmark::State& state) {
    const int batch_size = state.range(0);

    for (auto _ : state) {
        std::vector<std::unique_ptr<SimpleObject>> objects;
        objects.reserve(batch_size);

        for (int i = 0; i < batch_size; ++i) {
            objects.push_back(std::make_unique<SimpleObject>(i));
        }
        benchmark::DoNotOptimize(objects);
    }
    state.SetItemsProcessed(state.iterations() * batch_size);
}
BENCHMARK(BM_NewDeleteBatch)->Range(10, 1000);

// Growth factor benchmark
static void BM_ObjectPoolGrowth(benchmark::State& state) {
    const int growth_factor = state.range(0);
    ObjectPool<SimpleObject> pool(growth_factor);

    const int total_acquires = 1000;
    for (auto _ : state) {
        std::vector<std::unique_ptr<SimpleObject, std::function<void(SimpleObject*)>>> objects;
        objects.reserve(total_acquires);

        for (int i = 0; i < total_acquires; ++i) {
            objects.push_back(pool.acquire(i));
        }
        benchmark::DoNotOptimize(objects);
    }
    state.SetItemsProcessed(state.iterations() * total_acquires);
}
BENCHMARK(BM_ObjectPoolGrowth)->Arg(1)->Arg(8)->Arg(32)->Arg(128);

// Concurrent-like access pattern (sequential but simulating contention pattern)
static void BM_ObjectPoolSequentialPattern(benchmark::State& state) {
    ObjectPool<SimpleObject> pool;
    pool.reserve(100);

    for (auto _ : state) {
        // Acquire multiple objects
        auto obj1 = pool.acquire(1);
        auto obj2 = pool.acquire(2);
        auto obj3 = pool.acquire(3);

        benchmark::DoNotOptimize(obj1);
        benchmark::DoNotOptimize(obj2);
        benchmark::DoNotOptimize(obj3);

        // Release happens automatically when unique_ptr goes out of scope
    }
}
BENCHMARK(BM_ObjectPoolSequentialPattern);

// Memory overhead benchmark (informational)
static void BM_ObjectPoolMemoryOverhead(benchmark::State& state) {
    const int pool_size = state.range(0);
    ObjectPool<SimpleObject> pool;
    pool.reserve(pool_size);

    for (auto _ : state) {
        auto available = pool.available();
        benchmark::DoNotOptimize(available);
    }

    state.counters["pool_size"] = pool_size;
    state.counters["object_size"] = sizeof(SimpleObject);
}
BENCHMARK(BM_ObjectPoolMemoryOverhead)->Range(100, 10000);
