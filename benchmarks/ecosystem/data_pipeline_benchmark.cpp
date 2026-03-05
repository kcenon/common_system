// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file data_pipeline_benchmark.cpp
 * @brief Benchmark 2: Data pipeline stage costs (Phase 2)
 *
 * Measures the integration overhead of realistic data pipelines that chain
 * multiple ecosystem stages together using common_system interfaces.
 *
 * Simulated pipeline:
 *   [Serialize] -> [Transmit (in-memory)] -> [Deserialize]
 *                                                 |
 *                                          [Dispatch to Executor]
 *                                                 |
 *                                           [Log result]
 *                                                 |
 *                                          [Record metric]
 *
 * Benchmarks in this file:
 * - BM_Serialize_SmallPayload          : serialize 64B payload
 * - BM_Serialize_MediumPayload         : serialize 1KB payload
 * - BM_Serialize_LargePayload          : serialize 64KB payload
 * - BM_Deserialize_ValidPayload        : deserialize + Result<T> propagation
 * - BM_SerializeDeserializeRoundtrip   : full roundtrip cost
 * - BM_DispatchLogMetric               : executor -> logger -> collector chain
 * - BM_FullDataPipeline_Small          : end-to-end with 64B payload
 * - BM_FullDataPipeline_Medium         : end-to-end with 1KB payload
 * - BM_LogAndCollect_Throughput        : logger + collector throughput (items/s)
 */

#include "ecosystem_harness.h"

#include <kcenon/common/patterns/result.h>

#include <cstdint>
#include <string>
#include <vector>

using namespace kcenon::benchmark::ecosystem;
using namespace kcenon::common;

// =============================================================================
// Serialization cost
// =============================================================================

static void BM_Serialize_SmallPayload(benchmark::State& state) {
    const std::vector<uint8_t> data(64, 0xAA);
    for (auto _ : state) {
        auto payload = serialized_payload::serialize(data);
        benchmark::DoNotOptimize(payload.bytes.data());
    }
    state.SetBytesProcessed(state.iterations() *
                            static_cast<int64_t>(data.size()));
    state.SetLabel("64B payload");
}
BENCHMARK(BM_Serialize_SmallPayload);

static void BM_Serialize_MediumPayload(benchmark::State& state) {
    const std::vector<uint8_t> data(1024, 0xBB);
    for (auto _ : state) {
        auto payload = serialized_payload::serialize(data);
        benchmark::DoNotOptimize(payload.bytes.data());
    }
    state.SetBytesProcessed(state.iterations() *
                            static_cast<int64_t>(data.size()));
    state.SetLabel("1KB payload");
}
BENCHMARK(BM_Serialize_MediumPayload);

static void BM_Serialize_LargePayload(benchmark::State& state) {
    const std::vector<uint8_t> data(65536, 0xCC);
    for (auto _ : state) {
        auto payload = serialized_payload::serialize(data);
        benchmark::DoNotOptimize(payload.bytes.data());
    }
    state.SetBytesProcessed(state.iterations() *
                            static_cast<int64_t>(data.size()));
    state.SetLabel("64KB payload");
}
BENCHMARK(BM_Serialize_LargePayload);

// =============================================================================
// Deserialization cost (with Result<T> propagation)
// =============================================================================

static void BM_Deserialize_ValidPayload(benchmark::State& state) {
    const std::vector<uint8_t> data(1024, 0xDD);
    const auto payload = serialized_payload::serialize(data);
    for (auto _ : state) {
        auto result = serialized_payload::deserialize(payload);
        benchmark::DoNotOptimize(result);
    }
    state.SetBytesProcessed(state.iterations() *
                            static_cast<int64_t>(payload.total_size()));
    state.SetLabel("1KB + header validation");
}
BENCHMARK(BM_Deserialize_ValidPayload);

static void BM_SerializeDeserializeRoundtrip(benchmark::State& state) {
    const int payload_size = state.range(0);
    const std::vector<uint8_t> original(payload_size, 0xEE);
    for (auto _ : state) {
        auto serialized = serialized_payload::serialize(original);
        auto result = serialized_payload::deserialize(serialized);
        benchmark::DoNotOptimize(result);
    }
    state.SetBytesProcessed(state.iterations() *
                            static_cast<int64_t>(payload_size) * 2);
}
BENCHMARK(BM_SerializeDeserializeRoundtrip)->Range(64, 65536);

// =============================================================================
// Executor -> Logger -> Collector chain
// =============================================================================

namespace {

struct process_job : public kcenon::common::interfaces::IJob {
    null_mock_logger* logger;
    null_mock_collector* collector;
    int id;

    process_job(null_mock_logger* l, null_mock_collector* c, int i)
        : logger(l), collector(c), id(i) {}

    kcenon::common::VoidResult execute() override {
        logger->log(kcenon::common::interfaces::log_level::info, "job processed");
        collector->increment("jobs.completed");
        return kcenon::common::VoidResult::ok({});
    }
};

// Pipeline job for small payload (64B) end-to-end benchmark
struct small_pipeline_job : public kcenon::common::interfaces::IJob {
    const serialized_payload* payload;
    null_mock_logger* logger;
    null_mock_collector* collector;
    int* processed_count;

    kcenon::common::VoidResult execute() override {
        auto result = serialized_payload::deserialize(*payload);
        logger->log(kcenon::common::interfaces::log_level::info,
                    "payload processed");
        collector->increment("pipeline.messages_processed");
        collector->histogram("pipeline.payload_size_bytes",
                              static_cast<double>(payload->total_size()));
        ++(*processed_count);
        return kcenon::common::VoidResult::ok({});
    }
};

// Pipeline job for medium payload (1KB) end-to-end benchmark
struct medium_pipeline_job : public kcenon::common::interfaces::IJob {
    const serialized_payload* payload;
    null_mock_logger* logger;
    null_mock_collector* collector;
    int* processed_count;

    kcenon::common::VoidResult execute() override {
        auto result = serialized_payload::deserialize(*payload);
        logger->log(kcenon::common::interfaces::log_level::info,
                    "1KB payload processed");
        collector->increment("pipeline.messages_processed");
        collector->histogram("pipeline.payload_size_bytes",
                              static_cast<double>(payload->total_size()));
        ++(*processed_count);
        return kcenon::common::VoidResult::ok({});
    }
};

}  // namespace

static void BM_DispatchLogMetric(benchmark::State& state) {
    inline_mock_executor executor;
    auto logger = std::make_shared<null_mock_logger>();
    auto collector = std::make_shared<null_mock_collector>();

    int id = 0;
    for (auto _ : state) {
        auto job = std::make_unique<process_job>(logger.get(),
                                                  collector.get(), id++);
        auto result = executor.execute(std::move(job));
        benchmark::DoNotOptimize(result);
    }
    state.SetItemsProcessed(state.iterations());
    state.SetLabel("dispatch + log + metric");
}
BENCHMARK(BM_DispatchLogMetric);

// =============================================================================
// Full data pipeline: serialize -> dispatch -> log -> collect metric
// =============================================================================

static void BM_FullDataPipeline_Small(benchmark::State& state) {
    pipeline_context ctx;
    const std::vector<uint8_t> input_data(64, 0xFF);
    int processed = 0;

    for (auto _ : state) {
        // Stage 1: Serialize (simulates container_system)
        auto payload = serialized_payload::serialize(input_data);

        // Stage 2-5: Dispatch + Deserialize + Log + Metric
        auto job = std::make_unique<small_pipeline_job>();
        job->payload = &payload;
        job->logger = ctx.logger.get();
        job->collector = ctx.collector.get();
        job->processed_count = &processed;

        ctx.executor->execute(std::move(job));
        benchmark::DoNotOptimize(processed);
    }

    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(state.iterations() *
                            static_cast<int64_t>(64 +
                                serialized_payload::header_size));
    state.SetLabel("serialize+dispatch+deserialize+log+metric (64B)");
    report_hardware_profile(state);
}
BENCHMARK(BM_FullDataPipeline_Small);

static void BM_FullDataPipeline_Medium(benchmark::State& state) {
    pipeline_context ctx;
    const std::vector<uint8_t> input_data(1024, 0xFF);
    int processed = 0;

    for (auto _ : state) {
        auto payload = serialized_payload::serialize(input_data);

        auto job = std::make_unique<medium_pipeline_job>();
        job->payload = &payload;
        job->logger = ctx.logger.get();
        job->collector = ctx.collector.get();
        job->processed_count = &processed;

        ctx.executor->execute(std::move(job));
        benchmark::DoNotOptimize(processed);
    }

    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(state.iterations() *
                            static_cast<int64_t>(1024 +
                                serialized_payload::header_size));
    state.SetLabel("serialize+dispatch+deserialize+log+metric (1KB)");
    report_hardware_profile(state);
}
BENCHMARK(BM_FullDataPipeline_Medium);

// =============================================================================
// Throughput: logger + collector at saturation
// =============================================================================

static void BM_LogAndCollect_Throughput(benchmark::State& state) {
    const int batch_size = state.range(0);
    null_mock_logger logger;
    null_mock_collector collector;

    for (auto _ : state) {
        for (int i = 0; i < batch_size; ++i) {
            logger.log(interfaces::log_level::info, "event");
            collector.increment("events.total");
        }
    }
    state.SetItemsProcessed(state.iterations() * batch_size);
    state.SetLabel("log + metric per item");
}
BENCHMARK(BM_LogAndCollect_Throughput)->Range(100, 10000);
