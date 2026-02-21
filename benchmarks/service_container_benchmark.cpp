// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

#include <benchmark/benchmark.h>
#include <kcenon/common/di/service_container.h>
#include <memory>
#include <string>
#include <thread>
#include <vector>

using namespace kcenon::common::di;

// Test interfaces for benchmarking
class IBenchService {
public:
    virtual ~IBenchService() = default;
    virtual int value() const = 0;
};

class BenchServiceImpl : public IBenchService {
public:
    explicit BenchServiceImpl(int v = 42) : value_(v) {}
    int value() const override { return value_; }
private:
    int value_;
};

class IBenchService2 {
public:
    virtual ~IBenchService2() = default;
    virtual std::string name() const = 0;
};

class BenchService2Impl : public IBenchService2 {
public:
    std::string name() const override { return "bench"; }
};

// Singleton resolution throughput
static void BM_ServiceContainerResolveSingleton(benchmark::State& state) {
    service_container container;
    container.register_type<IBenchService, BenchServiceImpl>(service_lifetime::singleton);

    for (auto _ : state) {
        auto result = container.resolve<IBenchService>();
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_ServiceContainerResolveSingleton);

// Transient resolution throughput (new instance each time)
static void BM_ServiceContainerResolveTransient(benchmark::State& state) {
    service_container container;
    container.register_type<IBenchService, BenchServiceImpl>(service_lifetime::transient);

    for (auto _ : state) {
        auto result = container.resolve<IBenchService>();
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_ServiceContainerResolveTransient);

// is_registered check throughput
static void BM_ServiceContainerIsRegistered(benchmark::State& state) {
    service_container container;
    container.register_type<IBenchService, BenchServiceImpl>();

    for (auto _ : state) {
        benchmark::DoNotOptimize(container.is_registered<IBenchService>());
    }
}
BENCHMARK(BM_ServiceContainerIsRegistered);

// Registration throughput
static void BM_ServiceContainerRegister(benchmark::State& state) {
    for (auto _ : state) {
        service_container container;
        container.register_type<IBenchService, BenchServiceImpl>();
        benchmark::DoNotOptimize(container.is_registered<IBenchService>());
    }
}
BENCHMARK(BM_ServiceContainerRegister);

// Concurrent singleton resolution
static void BM_ServiceContainerConcurrentResolveSingleton(benchmark::State& state) {
    service_container container;
    container.register_type<IBenchService, BenchServiceImpl>(service_lifetime::singleton);
    const int num_threads = state.range(0);

    for (auto _ : state) {
        std::vector<std::thread> threads;
        for (int t = 0; t < num_threads; ++t) {
            threads.emplace_back([&container]() {
                for (int i = 0; i < 1000; ++i) {
                    auto result = container.resolve<IBenchService>();
                    benchmark::DoNotOptimize(result);
                }
            });
        }
        for (auto& t : threads) {
            t.join();
        }
    }
    state.SetItemsProcessed(state.iterations() * num_threads * 1000);
}
BENCHMARK(BM_ServiceContainerConcurrentResolveSingleton)->Arg(1)->Arg(2)->Arg(4)->Arg(8);
