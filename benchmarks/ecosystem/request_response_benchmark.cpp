// BSD 3-Clause License
// Copyright (c) 2025, 🍀☀🌕🌥 🌊
// See the LICENSE file in the project root for full license information.

/**
 * @file request_response_benchmark.cpp
 * @brief Benchmark 3: Full request/response cycle and concurrent clients (Phase 3)
 *
 * Simulates a complete request/response pipeline traversal and measures
 * latency percentiles under concurrent client load.
 *
 * Simulated full cycle:
 *   network_receive -> deserialize -> thread_dispatch -> DB_query_simulation
 *       -> serialize -> network_send
 *   + logger_system writes at each stage
 *   + monitoring_system records metrics
 *
 * Benchmarks in this file:
 * - BM_RequestResponseCycle_Single    : single-client roundtrip latency
 * - BM_RequestResponseCycle_Batched   : batched request throughput
 * - BM_ConcurrentClients             : N concurrent clients throughput
 * - BM_MonitoringOverhead_Enabled     : pipeline cost WITH monitoring
 * - BM_MonitoringOverhead_Disabled    : pipeline cost WITHOUT monitoring
 * - BM_EventBusPipeline_Chained       : full event-driven pipeline chain
 */

#include "ecosystem_harness.h"

#include <kcenon/common/patterns/event_bus.h>
#include <kcenon/common/patterns/result.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <string>
#include <thread>
#include <vector>

using namespace kcenon::benchmark::ecosystem;
using namespace kcenon::common;

// =============================================================================
// Simulated DB query (models database_system latency floor)
// =============================================================================

namespace {

// Simulates the minimum cost of a key-value DB lookup:
// - Hash computation (~5ns)
// - Cache miss / memory access (modeled with volatile read)
// - Result wrapping
inline Result<int> simulate_db_query(int key) {
    volatile int stored = key * 7 + 13;  // prevent optimization
    return Result<int>::ok(static_cast<int>(stored));
}

// Simulated request/response message
struct request_message {
    int client_id;
    int sequence;
    std::vector<uint8_t> body;
};

struct response_message {
    int client_id;
    int sequence;
    bool success;
    std::vector<uint8_t> body;
};

// Process one request through the full pipeline
response_message process_request(const request_message& req,
                                  null_mock_logger& logger,
                                  null_mock_collector& collector) {
    // Stage 1: Deserialize request (simulates container_system)
    auto payload = serialized_payload::serialize(req.body);
    auto deser = serialized_payload::deserialize(payload);

    // Stage 2: Log receipt (simulates logger_system)
    logger.log(interfaces::log_level::debug, "request received");

    // Stage 3: DB query (simulates database_system)
    auto db_result = simulate_db_query(req.sequence);

    // Stage 4: Record metrics (simulates monitoring_system)
    collector.increment("requests.processed");
    collector.histogram("requests.body_size_bytes",
                        static_cast<double>(req.body.size()));

    // Stage 5: Serialize response (simulates container_system)
    std::vector<uint8_t> resp_body(64, static_cast<uint8_t>(
        db_result.is_ok() ? db_result.value() & 0xFF : 0));
    auto resp_payload = serialized_payload::serialize(resp_body);

    // Stage 6: Log completion (simulates logger_system)
    logger.log(interfaces::log_level::debug, "response sent");

    return response_message{
        req.client_id, req.sequence, deser.is_ok(), resp_body};
}

}  // namespace

// =============================================================================
// Single-client roundtrip latency
// =============================================================================

static void BM_RequestResponseCycle_Single(benchmark::State& state) {
    null_mock_logger logger;
    null_mock_collector collector;
    const std::vector<uint8_t> body(64, 0xAB);

    int seq = 0;
    for (auto _ : state) {
        request_message req{0, seq++, body};
        auto resp = process_request(req, logger, collector);
        benchmark::DoNotOptimize(resp.success);
    }
    state.SetItemsProcessed(state.iterations());
    state.SetLabel("1 client, 64B body: deserialize+db+log+metric+serialize");
    report_hardware_profile(state);
}
BENCHMARK(BM_RequestResponseCycle_Single);

// =============================================================================
// Batched request throughput (simulates N queued requests)
// =============================================================================

static void BM_RequestResponseCycle_Batched(benchmark::State& state) {
    const int batch_size = state.range(0);
    null_mock_logger logger;
    null_mock_collector collector;
    const std::vector<uint8_t> body(256, 0xCD);

    int seq = 0;
    for (auto _ : state) {
        for (int i = 0; i < batch_size; ++i) {
            request_message req{0, seq++, body};
            auto resp = process_request(req, logger, collector);
            benchmark::DoNotOptimize(resp.success);
        }
    }
    state.SetItemsProcessed(state.iterations() * batch_size);
    state.SetLabel("batched 256B requests");
}
BENCHMARK(BM_RequestResponseCycle_Batched)->Range(10, 1000);

// =============================================================================
// Concurrent client simulation (N threads, M requests each)
// =============================================================================

static void BM_ConcurrentClients(benchmark::State& state) {
    const int client_count = state.range(0);
    const int requests_per_client = 100;

    // Each client gets its own logger/collector to avoid lock contention
    // (mirrors real deployment where loggers are per-thread)
    std::vector<std::unique_ptr<null_mock_logger>> loggers;
    std::vector<std::unique_ptr<null_mock_collector>> collectors;
    loggers.reserve(client_count);
    collectors.reserve(client_count);
    for (int i = 0; i < client_count; ++i) {
        loggers.push_back(std::make_unique<null_mock_logger>());
        collectors.push_back(std::make_unique<null_mock_collector>());
    }

    const std::vector<uint8_t> body(512, 0xEF);
    std::atomic<int64_t> total_requests{0};

    for (auto _ : state) {
        std::vector<std::thread> threads;
        threads.reserve(client_count);

        for (int c = 0; c < client_count; ++c) {
            threads.emplace_back([&, c]() {
                for (int r = 0; r < requests_per_client; ++r) {
                    request_message req{c, r, body};
                    auto resp = process_request(
                        req, *loggers[c], *collectors[c]);
                    benchmark::DoNotOptimize(resp.success);
                }
                total_requests.fetch_add(requests_per_client,
                                         std::memory_order_relaxed);
            });
        }

        for (auto& t : threads) t.join();
    }

    state.SetItemsProcessed(state.iterations() *
                            client_count * requests_per_client);
    state.counters["clients"] = static_cast<double>(client_count);
    state.counters["reqs_per_client"] =
        static_cast<double>(requests_per_client);
    state.SetLabel("512B body, concurrent");
    report_hardware_profile(state);
}
BENCHMARK(BM_ConcurrentClients)->Arg(1)->Arg(2)->Arg(4)->Arg(8);

// =============================================================================
// Monitoring overhead: enabled vs disabled comparison
// =============================================================================

static void BM_MonitoringOverhead_Enabled(benchmark::State& state) {
    null_mock_logger logger;
    null_mock_collector collector;  // monitoring enabled
    const std::vector<uint8_t> body(256, 0x11);

    int seq = 0;
    for (auto _ : state) {
        request_message req{0, seq++, body};
        auto resp = process_request(req, logger, collector);
        benchmark::DoNotOptimize(resp.success);
    }
    state.SetItemsProcessed(state.iterations());
    state.SetLabel("with monitoring");
}
BENCHMARK(BM_MonitoringOverhead_Enabled);

namespace {
// Null collector that does nothing at all (simulates monitoring_system disabled)
// Used only for the disabled comparison benchmark.
class truly_disabled_collector : public kcenon::common::interfaces::IMetricCollector {
public:
    void increment(std::string_view, double,
                   const kcenon::common::interfaces::metric_labels&) override {}
    void gauge(std::string_view, double,
               const kcenon::common::interfaces::metric_labels&) override {}
    void histogram(std::string_view, double,
                   const kcenon::common::interfaces::metric_labels&) override {}
    void timing(std::string_view, std::chrono::nanoseconds,
                const kcenon::common::interfaces::metric_labels&) override {}
};

// Null logger that does nothing at all (simulates logger_system disabled)
class truly_disabled_logger : public kcenon::common::interfaces::ILogger {
public:
    kcenon::common::VoidResult log(
        kcenon::common::interfaces::log_level,
        const std::string&) override {
        return kcenon::common::VoidResult::ok({});
    }
    kcenon::common::VoidResult log(
        const kcenon::common::interfaces::log_entry&) override {
        return kcenon::common::VoidResult::ok({});
    }
    bool is_enabled(
        kcenon::common::interfaces::log_level) const override { return false; }
    kcenon::common::VoidResult set_level(
        kcenon::common::interfaces::log_level) override {
        return kcenon::common::VoidResult::ok({});
    }
    kcenon::common::interfaces::log_level get_level() const override {
        return kcenon::common::interfaces::log_level::off;
    }
    kcenon::common::VoidResult flush() override {
        return kcenon::common::VoidResult::ok({});
    }
};
}  // namespace

static void BM_MonitoringOverhead_Disabled(benchmark::State& state) {
    truly_disabled_logger logger;    // logging off
    truly_disabled_collector collector;  // monitoring off
    const std::vector<uint8_t> body(256, 0x22);

    int seq = 0;
    for (auto _ : state) {
        // Process without log/metric calls going through the atomic path
        auto payload = serialized_payload::serialize(body);
        auto deser = serialized_payload::deserialize(payload);
        auto db_result = simulate_db_query(seq++);
        std::vector<uint8_t> resp_body(64, 0x00);
        auto resp_payload = serialized_payload::serialize(resp_body);
        benchmark::DoNotOptimize(resp_payload.bytes.data());
        benchmark::DoNotOptimize(deser);
    }
    state.SetItemsProcessed(state.iterations());
    state.SetLabel("without monitoring");
}
BENCHMARK(BM_MonitoringOverhead_Disabled);

// =============================================================================
// Event-driven pipeline chain (EventBus -> stage -> EventBus -> next stage)
// =============================================================================

namespace {
struct stage_complete_event {
    int stage;
    std::vector<uint8_t> data;
};
}  // namespace

static void BM_EventBusPipeline_Chained(benchmark::State& state) {
    simple_event_bus bus;
    bus.start();
    null_mock_logger logger;
    null_mock_collector collector;

    int completed = 0;

    // Stage 1 subscriber: deserialize, dispatch to stage 2
    bus.subscribe<serialized_payload>([&](const serialized_payload& p) {
        auto result = serialized_payload::deserialize(p);
        logger.log(interfaces::log_level::debug, "stage1: deserialized");
        collector.increment("pipeline.stage1_completed");

        if (result.is_ok()) {
            bus.publish(stage_complete_event{1, result.value()});
        }
    });

    // Stage 2 subscriber: process, record completion
    bus.subscribe<stage_complete_event>([&](const stage_complete_event& e) {
        auto db_result = simulate_db_query(e.stage);
        logger.log(interfaces::log_level::debug, "stage2: db query done");
        collector.increment("pipeline.stage2_completed");
        benchmark::DoNotOptimize(db_result);
        ++completed;
    });

    const std::vector<uint8_t> data(256, 0xAA);
    for (auto _ : state) {
        auto payload = serialized_payload::serialize(data);
        bus.publish(payload);
        benchmark::DoNotOptimize(completed);
    }
    state.SetItemsProcessed(state.iterations());
    state.SetLabel("eventbus 2-stage chained pipeline");
}
BENCHMARK(BM_EventBusPipeline_Chained);
