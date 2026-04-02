// BSD 3-Clause License
// Copyright (c) 2025, 🍀☀🌕🌥 🌊
// See the LICENSE file in the project root for full license information.

/**
 * @file scaling_benchmark.cpp
 * @brief Benchmark 4: Thread scaling and contention (Phase 4)
 *
 * Measures how the pipeline scales across thread counts and identifies
 * contention patterns when multiple threads share interface implementations.
 *
 * Benchmarks in this file:
 * - BM_ExecutorScaling              : async executor throughput vs thread count
 * - BM_SharedLogger_Contention      : logger throughput under shared access
 * - BM_SharedCollector_Contention   : collector throughput under shared access
 * - BM_SharedEventBus_Contention    : event bus publish throughput under concurrency
 * - BM_MixedWorkload_Pipeline       : realistic mixed read/write/monitor workload
 */

#include "ecosystem_harness.h"

#include <atomic>
#include <cstdint>
#include <thread>
#include <vector>

using namespace kcenon::benchmark::ecosystem;
using namespace kcenon::common;

// =============================================================================
// Executor thread scaling
// =============================================================================

namespace {
struct compute_job : public kcenon::common::interfaces::IJob {
    std::atomic<int64_t>* total;
    int value;

    compute_job(std::atomic<int64_t>* t, int v) : total(t), value(v) {}

    kcenon::common::VoidResult execute() override {
        // Simulate minimal compute work (prevents trivial optimization)
        volatile int result = value * 37 + 13;
        total->fetch_add(static_cast<int>(result),
                          std::memory_order_relaxed);
        return kcenon::common::VoidResult::ok({});
    }
};
}  // namespace

static void BM_ExecutorScaling(benchmark::State& state) {
    const int thread_count = state.range(0);
    const int jobs_per_thread = 100;
    async_mock_executor executor(thread_count);
    std::atomic<int64_t> total{0};

    for (auto _ : state) {
        std::vector<std::future<void>> futures;
        futures.reserve(thread_count * jobs_per_thread);

        for (int t = 0; t < thread_count; ++t) {
            for (int j = 0; j < jobs_per_thread; ++j) {
                auto job = std::make_unique<compute_job>(&total, t * 1000 + j);
                auto result = executor.execute(std::move(job));
                if (result.is_ok()) {
                    futures.push_back(std::move(result.value()));
                }
            }
        }

        for (auto& f : futures) {
            if (f.valid()) f.get();
        }
        benchmark::DoNotOptimize(total.load());
    }

    state.SetItemsProcessed(state.iterations() * thread_count * jobs_per_thread);
    state.counters["threads"] = static_cast<double>(thread_count);
    state.counters["jobs_total"] =
        static_cast<double>(thread_count * jobs_per_thread);
    state.SetLabel("jobs/s vs thread count");
    report_hardware_profile(state);
}
BENCHMARK(BM_ExecutorScaling)->Arg(1)->Arg(2)->Arg(4)->Arg(8)->Arg(16);

// =============================================================================
// Shared logger contention (multiple threads writing to one logger)
// =============================================================================

static void BM_SharedLogger_Contention(benchmark::State& state) {
    const int thread_count = state.range(0);
    const int logs_per_thread = 1000;
    null_mock_logger shared_logger;  // shared across all threads

    for (auto _ : state) {
        std::vector<std::thread> threads;
        threads.reserve(thread_count);

        for (int t = 0; t < thread_count; ++t) {
            threads.emplace_back([&]() {
                for (int i = 0; i < logs_per_thread; ++i) {
                    shared_logger.log(interfaces::log_level::info,
                                      "concurrent log entry");
                }
            });
        }

        for (auto& t : threads) t.join();
    }

    state.SetItemsProcessed(state.iterations() * thread_count * logs_per_thread);
    state.counters["threads"] = static_cast<double>(thread_count);
    state.SetLabel("shared logger, concurrent writes");
}
BENCHMARK(BM_SharedLogger_Contention)->Arg(1)->Arg(2)->Arg(4)->Arg(8);

// =============================================================================
// Shared metric collector contention
// =============================================================================

static void BM_SharedCollector_Contention(benchmark::State& state) {
    const int thread_count = state.range(0);
    const int metrics_per_thread = 1000;
    null_mock_collector shared_collector;  // shared across all threads

    for (auto _ : state) {
        std::vector<std::thread> threads;
        threads.reserve(thread_count);

        for (int t = 0; t < thread_count; ++t) {
            threads.emplace_back([&, t]() {
                for (int i = 0; i < metrics_per_thread; ++i) {
                    shared_collector.increment("pipeline.events",
                                               static_cast<double>(t));
                    shared_collector.histogram("pipeline.latency_ns",
                                               static_cast<double>(i * 10));
                }
            });
        }

        for (auto& t : threads) t.join();
    }

    state.SetItemsProcessed(state.iterations() * thread_count *
                            metrics_per_thread * 2);
    state.counters["threads"] = static_cast<double>(thread_count);
    state.SetLabel("shared collector, concurrent increments");
}
BENCHMARK(BM_SharedCollector_Contention)->Arg(1)->Arg(2)->Arg(4)->Arg(8);

// =============================================================================
// EventBus concurrent publish contention
// =============================================================================

namespace {
struct pipeline_event {
    int thread_id;
    int sequence;
};
}  // namespace

static void BM_SharedEventBus_Contention(benchmark::State& state) {
    const int thread_count = state.range(0);
    const int events_per_thread = 500;
    simple_event_bus bus;
    bus.start();
    std::atomic<int64_t> received{0};

    bus.subscribe<pipeline_event>([&](const pipeline_event&) {
        received.fetch_add(1, std::memory_order_relaxed);
    });

    for (auto _ : state) {
        received.store(0);
        std::vector<std::thread> threads;
        threads.reserve(thread_count);

        for (int t = 0; t < thread_count; ++t) {
            threads.emplace_back([&, t]() {
                for (int i = 0; i < events_per_thread; ++i) {
                    bus.publish(pipeline_event{t, i});
                }
            });
        }

        for (auto& t : threads) t.join();
        benchmark::DoNotOptimize(received.load());
    }

    state.SetItemsProcessed(state.iterations() * thread_count * events_per_thread);
    state.counters["threads"] = static_cast<double>(thread_count);
    state.SetLabel("shared event bus, concurrent publish");
}
BENCHMARK(BM_SharedEventBus_Contention)->Arg(1)->Arg(2)->Arg(4)->Arg(8);

// =============================================================================
// Mixed workload: concurrent reads, writes, and monitoring
// =============================================================================

static void BM_MixedWorkload_Pipeline(benchmark::State& state) {
    const int thread_count = state.range(0);
    const int ops_per_thread = 300;

    null_mock_logger logger;
    null_mock_collector collector;
    simple_event_bus bus;
    bus.start();

    std::atomic<int64_t> processed{0};

    // Simulate: 60% read (deserialize), 30% write (serialize+log),
    //           10% monitoring (metric batch)
    bus.subscribe<serialized_payload>([&](const serialized_payload& p) {
        auto result = serialized_payload::deserialize(p);
        collector.increment("bus.received");
        processed.fetch_add(1, std::memory_order_relaxed);
        benchmark::DoNotOptimize(result);
    });

    const std::vector<uint8_t> data(256, 0xBB);

    for (auto _ : state) {
        processed.store(0);
        std::vector<std::thread> threads;
        threads.reserve(thread_count);

        for (int t = 0; t < thread_count; ++t) {
            threads.emplace_back([&, t]() {
                for (int i = 0; i < ops_per_thread; ++i) {
                    const int op_type = (t * ops_per_thread + i) % 10;

                    if (op_type < 6) {
                        // 60%: read path (deserialize only)
                        const auto payload = serialized_payload::serialize(data);
                        auto result = serialized_payload::deserialize(payload);
                        benchmark::DoNotOptimize(result);
                    } else if (op_type < 9) {
                        // 30%: write path (serialize + log + event)
                        const auto payload = serialized_payload::serialize(data);
                        logger.log(interfaces::log_level::info, "write op");
                        bus.publish(payload);
                    } else {
                        // 10%: monitoring batch
                        collector.increment("ops.total", 10.0);
                        collector.gauge("threads.active",
                                        static_cast<double>(thread_count));
                        collector.histogram("op.payload_bytes", 256.0);
                    }
                }
            });
        }

        for (auto& t : threads) t.join();
        benchmark::DoNotOptimize(processed.load());
    }

    state.SetItemsProcessed(state.iterations() * thread_count * ops_per_thread);
    state.counters["threads"] = static_cast<double>(thread_count);
    state.SetLabel("60% read, 30% write, 10% monitor");
    report_hardware_profile(state);
}
BENCHMARK(BM_MixedWorkload_Pipeline)->Arg(1)->Arg(2)->Arg(4)->Arg(8);
