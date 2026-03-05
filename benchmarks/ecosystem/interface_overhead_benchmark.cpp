// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file interface_overhead_benchmark.cpp
 * @brief Benchmark 1: Interface call overhead (Phase 1 baseline)
 *
 * Measures the bare cost of virtual dispatch through each ecosystem interface.
 * These are the baseline numbers that all integration overhead is measured
 * against. Results identify the "floor" cost below which no implementation
 * can go.
 *
 * Benchmarks in this file:
 * - BM_ExecutorDispatch_Inline      : IExecutor::execute() vtable cost
 * - BM_LoggerCall_SimpleMessage     : ILogger::log() with small string
 * - BM_LoggerCall_LargeMessage      : ILogger::log() with 1KB message
 * - BM_LoggerCall_LogEntry          : ILogger::log(log_entry) with entry construction
 * - BM_MetricCollectorIncrement     : IMetricCollector::increment() call cost
 * - BM_MetricCollectorGauge         : IMetricCollector::gauge() call cost
 * - BM_MetricCollectorHistogram     : IMetricCollector::histogram() call cost
 * - BM_MetricCollectorTiming        : IMetricCollector::timing() call cost
 * - BM_EventBusPublishToExecutor    : EventBus publish + executor dispatch chain
 */

#include "ecosystem_harness.h"

#include <kcenon/common/interfaces/logger_interface.h>
#include <kcenon/common/interfaces/monitoring/metric_collector_interface.h>
#include <kcenon/common/patterns/event_bus.h>

#include <memory>
#include <string>

using namespace kcenon::benchmark::ecosystem;
using namespace kcenon::common;

// =============================================================================
// IExecutor dispatch overhead
// =============================================================================

namespace {
// Simple no-op job for measuring dispatch cost
struct noop_job : public interfaces::IJob {
    VoidResult execute() override { return VoidResult::ok({}); }
    std::string get_name() const override { return "noop"; }
};
}  // namespace

static void BM_ExecutorDispatch_Inline(benchmark::State& state) {
    inline_mock_executor executor;
    for (auto _ : state) {
        auto job = std::make_unique<noop_job>();
        auto result = executor.execute(std::move(job));
        benchmark::DoNotOptimize(result);
    }
    state.SetLabel("vtable+future overhead");
}
BENCHMARK(BM_ExecutorDispatch_Inline);

// =============================================================================
// ILogger call overhead
// =============================================================================

static void BM_LoggerCall_SimpleMessage(benchmark::State& state) {
    null_mock_logger logger;
    for (auto _ : state) {
        auto result = logger.log(interfaces::log_level::info, "pipeline processed");
        benchmark::DoNotOptimize(result);
    }
    state.SetLabel("string copy + vtable");
}
BENCHMARK(BM_LoggerCall_SimpleMessage);

static void BM_LoggerCall_LargeMessage(benchmark::State& state) {
    null_mock_logger logger;
    const std::string large_msg(1024, 'x');
    for (auto _ : state) {
        auto result = logger.log(interfaces::log_level::info, large_msg);
        benchmark::DoNotOptimize(result);
    }
    state.SetBytesProcessed(state.iterations() * 1024);
    state.SetLabel("1KB string copy + vtable");
}
BENCHMARK(BM_LoggerCall_LargeMessage);

static void BM_LoggerCall_LogEntry(benchmark::State& state) {
    null_mock_logger logger;
    for (auto _ : state) {
        auto entry = interfaces::log_entry::create(
            interfaces::log_level::info, "pipeline processed");
        auto result = logger.log(entry);
        benchmark::DoNotOptimize(result);
    }
    state.SetLabel("log_entry construction + vtable");
}
BENCHMARK(BM_LoggerCall_LogEntry);

// =============================================================================
// IMetricCollector call overhead
// =============================================================================

static void BM_MetricCollectorIncrement(benchmark::State& state) {
    null_mock_collector collector;
    for (auto _ : state) {
        collector.increment("pipeline.events_processed", 1.0);
    }
    state.SetLabel("string_view + atomic increment");
}
BENCHMARK(BM_MetricCollectorIncrement);

static void BM_MetricCollectorGauge(benchmark::State& state) {
    null_mock_collector collector;
    for (auto _ : state) {
        collector.gauge("pipeline.queue_depth", 42.0);
    }
    state.SetLabel("gauge vtable cost");
}
BENCHMARK(BM_MetricCollectorGauge);

static void BM_MetricCollectorHistogram(benchmark::State& state) {
    null_mock_collector collector;
    double value = 0.0;
    for (auto _ : state) {
        collector.histogram("pipeline.message_size_bytes", value);
        value += 1.0;
    }
    state.SetLabel("histogram vtable cost");
}
BENCHMARK(BM_MetricCollectorHistogram);

static void BM_MetricCollectorTiming(benchmark::State& state) {
    null_mock_collector collector;
    for (auto _ : state) {
        collector.timing("pipeline.stage_latency_ns",
                         std::chrono::nanoseconds{100});
    }
    state.SetLabel("timing vtable cost");
}
BENCHMARK(BM_MetricCollectorTiming);

// =============================================================================
// EventBus -> Executor dispatch chain
// =============================================================================

namespace {
struct pipeline_event {
    int sequence_id;
    std::string payload;
};
}  // namespace

static void BM_EventBusPublishToExecutor(benchmark::State& state) {
    simple_event_bus bus;
    bus.start();
    inline_mock_executor executor;

    int dispatched = 0;
    bus.subscribe<pipeline_event>([&](const pipeline_event& e) {
        auto job = std::make_unique<noop_job>();
        executor.execute(std::move(job));
        dispatched = e.sequence_id;
    });

    int seq = 0;
    for (auto _ : state) {
        bus.publish(pipeline_event{seq++, "data"});
        benchmark::DoNotOptimize(dispatched);
    }
    state.SetItemsProcessed(state.iterations());
    state.SetLabel("eventbus + vtable dispatch chain");
}
BENCHMARK(BM_EventBusPublishToExecutor);
