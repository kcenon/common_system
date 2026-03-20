// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file ecosystem_harness.h
 * @brief Standard benchmark harness for cross-system ecosystem pipeline benchmarks.
 *
 * Provides lightweight mock implementations of ecosystem interfaces and a
 * standardized pipeline context for measuring cross-system integration overhead.
 *
 * Design rationale:
 * common_system defines the interface contracts (IExecutor, ILogger,
 * IMetricCollector) but does not implement them. These inline mocks measure:
 * 1. Interface-call overhead (vtable dispatch, virtual function cost)
 * 2. Realistic pipeline stage costs without external dependencies
 * 3. Baseline integration overhead for comparison against real implementations
 *
 * Related issue: #365
 */

#pragma once

#include <benchmark/benchmark.h>

#include <kcenon/common/interfaces/executor_interface.h>
#include <kcenon/common/interfaces/logger_interface.h>
#include <kcenon/common/interfaces/monitoring/metric_collector_interface.h>
#include <kcenon/common/patterns/event_bus.h>
#include <kcenon/common/patterns/result.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstring>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

namespace kcenon::benchmark::ecosystem {

// =============================================================================
// Mock: Inline executor (simulates thread_system dispatch interface overhead)
// =============================================================================

/**
 * @brief Synchronous inline executor for benchmarking dispatch overhead.
 *
 * Executes jobs on the calling thread to isolate vtable dispatch cost from
 * thread-scheduling latency. For async benchmarks, use async_mock_executor.
 */
class inline_mock_executor : public common::interfaces::IExecutor {
public:
    common::Result<std::future<void>> execute(
        std::unique_ptr<common::interfaces::IJob>&& job) override {
        job->execute();
        std::promise<void> p;
        p.set_value();
        return common::Result<std::future<void>>::ok(p.get_future());
    }

    common::Result<std::future<void>> execute_delayed(
        std::unique_ptr<common::interfaces::IJob>&& job,
        std::chrono::milliseconds /*delay*/) override {
        job->execute();
        std::promise<void> p;
        p.set_value();
        return common::Result<std::future<void>>::ok(p.get_future());
    }

    size_t worker_count() const override { return 1; }
    bool is_running() const override { return true; }
    size_t pending_tasks() const override { return 0; }
    void shutdown(bool /*wait*/) override {}
};

/**
 * @brief Async thread-pool executor mock for concurrency benchmarks.
 *
 * Uses a real thread pool to measure multi-threaded dispatch latency
 * and throughput under concurrent load.
 */
class async_mock_executor : public common::interfaces::IExecutor {
public:
    explicit async_mock_executor(
        size_t threads = std::thread::hardware_concurrency())
        : running_(true) {
        for (size_t i = 0; i < threads; ++i) {
            workers_.emplace_back([this] { worker_loop(); });
        }
    }

    ~async_mock_executor() override { shutdown(true); }

    common::Result<std::future<void>> execute(
        std::unique_ptr<common::interfaces::IJob>&& job) override {
        auto promise = std::make_shared<std::promise<void>>();
        auto future = promise->get_future();
        {
            std::lock_guard lock(mu_);
            queue_.push({std::move(job), std::move(promise)});
        }
        cv_.notify_one();
        return common::Result<std::future<void>>::ok(std::move(future));
    }

    common::Result<std::future<void>> execute_delayed(
        std::unique_ptr<common::interfaces::IJob>&& job,
        std::chrono::milliseconds /*delay*/) override {
        return execute(std::move(job));
    }

    size_t worker_count() const override { return workers_.size(); }
    bool is_running() const override { return running_.load(); }
    size_t pending_tasks() const override {
        std::lock_guard lock(mu_);
        return queue_.size();
    }

    void shutdown(bool wait) override {
        running_.store(false);
        cv_.notify_all();
        if (wait) {
            for (auto& t : workers_) {
                if (t.joinable()) t.join();
            }
        }
    }

private:
    struct work_item {
        std::unique_ptr<common::interfaces::IJob> job;
        std::shared_ptr<std::promise<void>> promise;
    };

    void worker_loop() {
        while (true) {
            std::unique_lock lock(mu_);
            cv_.wait(lock, [this] {
                return !queue_.empty() || !running_.load();
            });
            if (!running_.load() && queue_.empty()) break;
            if (queue_.empty()) continue;
            auto item = std::move(queue_.front());
            queue_.pop();
            lock.unlock();
            item.job->execute();
            item.promise->set_value();
        }
    }

    std::vector<std::thread> workers_;
    std::atomic<bool> running_;
    mutable std::mutex mu_;
    std::condition_variable cv_;
    std::queue<work_item> queue_;
};

// =============================================================================
// Mock: Null logger (simulates logger_system interface call overhead)
// =============================================================================

/**
 * @brief No-op logger measuring pure interface call and entry construction cost.
 *
 * Discards all log entries without I/O. Measures vtable dispatch overhead,
 * string construction, and atomic counter update — the minimum cost that any
 * logger_system implementation must pay.
 */
class null_mock_logger : public common::interfaces::ILogger {
public:
    common::VoidResult log(common::interfaces::log_level /*level*/,
                           const std::string& /*message*/) override {
        logged_count_.fetch_add(1, std::memory_order_relaxed);
        return common::VoidResult::ok({});
    }

    common::VoidResult log(
        const common::interfaces::log_entry& /*entry*/) override {
        logged_count_.fetch_add(1, std::memory_order_relaxed);
        return common::VoidResult::ok({});
    }

    bool is_enabled(common::interfaces::log_level /*level*/) const override {
        return true;
    }

    common::VoidResult set_level(
        common::interfaces::log_level /*level*/) override {
        return common::VoidResult::ok({});
    }

    common::interfaces::log_level get_level() const override {
        return common::interfaces::log_level::off;
    }

    common::VoidResult flush() override {
        return common::VoidResult::ok({});
    }

    size_t logged_count() const {
        return logged_count_.load(std::memory_order_relaxed);
    }

private:
    std::atomic<size_t> logged_count_{0};
};

// =============================================================================
// Mock: Null metric collector (simulates monitoring_system interface overhead)
// =============================================================================

/**
 * @brief No-op metric collector measuring monitoring interface call overhead.
 *
 * Uses an atomic counter to prevent dead-code elimination while discarding
 * actual metric data. This measures the minimum cost of any metric emission
 * path in the ecosystem.
 */
class null_mock_collector : public common::interfaces::IMetricCollector {
public:
    void increment(std::string_view /*name*/,
                   double /*value*/ = 1.0,
                   const common::interfaces::metric_labels& /*labels*/ = {}) override {
        collected_count_.fetch_add(1, std::memory_order_relaxed);
    }

    void gauge(std::string_view /*name*/,
               double value,
               const common::interfaces::metric_labels& /*labels*/ = {}) override {
        collected_count_.fetch_add(1, std::memory_order_relaxed);
        ::benchmark::DoNotOptimize(value);
    }

    void histogram(std::string_view /*name*/,
                   double value,
                   const common::interfaces::metric_labels& /*labels*/ = {}) override {
        collected_count_.fetch_add(1, std::memory_order_relaxed);
        ::benchmark::DoNotOptimize(value);
    }

    void timing(std::string_view /*name*/,
                std::chrono::nanoseconds duration,
                const common::interfaces::metric_labels& /*labels*/ = {}) override {
        collected_count_.fetch_add(1, std::memory_order_relaxed);
        ::benchmark::DoNotOptimize(duration.count());
    }

    size_t collected_count() const {
        return collected_count_.load(std::memory_order_relaxed);
    }

private:
    std::atomic<size_t> collected_count_{0};
};

// =============================================================================
// Simulated payload: stands in for container_system serialization
// =============================================================================

/**
 * @brief Simulated binary payload for serialization/deserialization benchmarks.
 *
 * Models the data contract between container_system (binary serializer) and
 * network_system (TCP transport) without requiring those libraries.
 *
 * Wire format:
 *   [0..3]  magic bytes (0xAB 0xCD 0xEF 0x01)
 *   [4..5]  version (uint16_t, little-endian)
 *   [6..9]  payload length (uint32_t, little-endian)
 *   [10..15] reserved / checksum placeholder
 *   [16..]  payload bytes
 */
struct serialized_payload {
    static constexpr size_t header_size = 16;

    std::vector<uint8_t> bytes;

    static serialized_payload serialize(const std::vector<uint8_t>& data) {
        serialized_payload p;
        p.bytes.resize(header_size + data.size());
        p.bytes[0] = 0xAB;
        p.bytes[1] = 0xCD;
        p.bytes[2] = 0xEF;
        p.bytes[3] = 0x01;
        const uint16_t version = 1;
        std::memcpy(p.bytes.data() + 4, &version, sizeof(version));
        const auto len = static_cast<uint32_t>(data.size());
        std::memcpy(p.bytes.data() + 6, &len, sizeof(len));
        std::memcpy(p.bytes.data() + header_size, data.data(), data.size());
        return p;
    }

    static common::Result<std::vector<uint8_t>> deserialize(
        const serialized_payload& p) {
        if (p.bytes.size() < header_size) {
            return common::Result<std::vector<uint8_t>>::err(
                common::error_info{1, "payload too small"});
        }
        if (p.bytes[0] != 0xAB || p.bytes[1] != 0xCD) {
            return common::Result<std::vector<uint8_t>>::err(
                common::error_info{2, "invalid magic"});
        }
        uint32_t len = 0;
        std::memcpy(&len, p.bytes.data() + 6, sizeof(len));
        if (p.bytes.size() < header_size + len) {
            return common::Result<std::vector<uint8_t>>::err(
                common::error_info{3, "truncated payload"});
        }
        return common::Result<std::vector<uint8_t>>::ok(
            std::vector<uint8_t>(p.bytes.begin() + header_size,
                                  p.bytes.begin() + header_size + len));
    }

    size_t total_size() const { return bytes.size(); }
};

// =============================================================================
// Pipeline context: shared state for full-pipeline benchmarks
// =============================================================================

/**
 * @brief Shared context holding all pipeline stage mocks.
 *
 * Provides a single setup/teardown point for multi-stage benchmarks.
 * Equivalent to having all ecosystem services available and configured.
 */
struct pipeline_context {
    std::shared_ptr<inline_mock_executor> executor;
    std::shared_ptr<null_mock_logger>     logger;
    std::shared_ptr<null_mock_collector>  collector;
    common::simple_event_bus              event_bus;

    pipeline_context()
        : executor(std::make_shared<inline_mock_executor>()),
          logger(std::make_shared<null_mock_logger>()),
          collector(std::make_shared<null_mock_collector>()) {
        event_bus.start();
    }
};

// =============================================================================
// Hardware profile reporting
// =============================================================================

/**
 * @brief Embed hardware context into benchmark JSON output.
 *
 * Call within benchmarks to attach hardware configuration counters,
 * enabling trend comparison across different machines.
 */
inline void report_hardware_profile(::benchmark::State& state) {
    state.counters["hw_threads"] =
        static_cast<double>(std::thread::hardware_concurrency());
}

}  // namespace kcenon::benchmark::ecosystem
