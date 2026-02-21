// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

#include <benchmark/benchmark.h>
#include <kcenon/common/resilience/circuit_breaker.h>
#include <thread>
#include <vector>

using namespace kcenon::common::resilience;

// allow_request() hot path in CLOSED state (normal operation)
static void BM_CircuitBreakerAllowRequestClosed(benchmark::State& state) {
    circuit_breaker breaker;

    for (auto _ : state) {
        benchmark::DoNotOptimize(breaker.allow_request());
    }
}
BENCHMARK(BM_CircuitBreakerAllowRequestClosed);

// allow_request() in OPEN state (fast rejection)
static void BM_CircuitBreakerAllowRequestOpen(benchmark::State& state) {
    circuit_breaker_config config{
        .failure_threshold = 1,
        .timeout = std::chrono::hours(1)
    };
    circuit_breaker breaker(config);
    breaker.record_failure(); // trip to OPEN

    for (auto _ : state) {
        benchmark::DoNotOptimize(breaker.allow_request());
    }
}
BENCHMARK(BM_CircuitBreakerAllowRequestOpen);

// record_success() in CLOSED state
static void BM_CircuitBreakerRecordSuccess(benchmark::State& state) {
    circuit_breaker breaker;

    for (auto _ : state) {
        breaker.record_success();
    }
}
BENCHMARK(BM_CircuitBreakerRecordSuccess);

// record_failure() without tripping
static void BM_CircuitBreakerRecordFailure(benchmark::State& state) {
    circuit_breaker_config config{
        .failure_threshold = static_cast<std::size_t>(state.max_iterations + 1),
        .failure_window = std::chrono::milliseconds(100)
    };
    circuit_breaker breaker(config);

    for (auto _ : state) {
        breaker.record_failure();
    }
}
BENCHMARK(BM_CircuitBreakerRecordFailure);

// get_state() read throughput
static void BM_CircuitBreakerGetState(benchmark::State& state) {
    circuit_breaker breaker;

    for (auto _ : state) {
        benchmark::DoNotOptimize(breaker.get_state());
    }
}
BENCHMARK(BM_CircuitBreakerGetState);

// RAII guard creation and destruction (success path)
static void BM_CircuitBreakerGuardSuccess(benchmark::State& state) {
    circuit_breaker breaker;

    for (auto _ : state) {
        auto guard = breaker.make_guard();
        guard.record_success();
    }
}
BENCHMARK(BM_CircuitBreakerGuardSuccess);

// Concurrent allow_request (simulating real-world contention)
static void BM_CircuitBreakerConcurrentAllowRequest(benchmark::State& state) {
    circuit_breaker breaker;
    const int num_threads = state.range(0);

    for (auto _ : state) {
        std::vector<std::thread> threads;
        for (int t = 0; t < num_threads; ++t) {
            threads.emplace_back([&breaker]() {
                for (int i = 0; i < 1000; ++i) {
                    if (breaker.allow_request()) {
                        breaker.record_success();
                    }
                }
            });
        }
        for (auto& t : threads) {
            t.join();
        }
    }
    state.SetItemsProcessed(state.iterations() * num_threads * 1000);
}
BENCHMARK(BM_CircuitBreakerConcurrentAllowRequest)->Arg(1)->Arg(2)->Arg(4)->Arg(8);
