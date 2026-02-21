// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

#include <benchmark/benchmark.h>
#include <kcenon/common/interfaces/global_logger_registry.h>
#include <memory>
#include <string>
#include <thread>
#include <vector>

using namespace kcenon::common;
using namespace kcenon::common::interfaces;

// Minimal logger for benchmarking
class BenchLogger : public ILogger {
public:
    VoidResult log(log_level /*level*/, const std::string& /*message*/) override {
        return VoidResult::ok({});
    }

    VoidResult log(log_level level,
                   std::string_view message,
                   const source_location& /*loc*/ = source_location::current()) override {
        return log(level, std::string(message));
    }

    VoidResult log(const log_entry& entry) override {
        return log(entry.level, entry.message);
    }

    bool is_enabled(log_level /*level*/) const override { return true; }
    VoidResult set_level(log_level /*level*/) override { return VoidResult::ok({}); }
    log_level get_level() const override { return log_level::info; }
    VoidResult flush() override { return VoidResult::ok({}); }
};

// get_logger() throughput for registered logger
static void BM_GlobalLoggerRegistryGetLogger(benchmark::State& state) {
    auto& registry = GlobalLoggerRegistry::instance();
    registry.register_logger("bench-logger", std::make_shared<BenchLogger>());

    for (auto _ : state) {
        auto logger = registry.get_logger("bench-logger");
        benchmark::DoNotOptimize(logger);
    }

    registry.unregister_logger("bench-logger");
}
BENCHMARK(BM_GlobalLoggerRegistryGetLogger);

// get_logger() for non-existent name (returns null_logger)
static void BM_GlobalLoggerRegistryGetNullLogger(benchmark::State& state) {
    auto& registry = GlobalLoggerRegistry::instance();

    for (auto _ : state) {
        auto logger = registry.get_logger("nonexistent-bench-logger");
        benchmark::DoNotOptimize(logger);
    }
}
BENCHMARK(BM_GlobalLoggerRegistryGetNullLogger);

// get_default_logger() throughput
static void BM_GlobalLoggerRegistryGetDefaultLogger(benchmark::State& state) {
    auto& registry = GlobalLoggerRegistry::instance();

    for (auto _ : state) {
        auto logger = registry.get_default_logger();
        benchmark::DoNotOptimize(logger);
    }
}
BENCHMARK(BM_GlobalLoggerRegistryGetDefaultLogger);

// has_logger() check throughput
static void BM_GlobalLoggerRegistryHasLogger(benchmark::State& state) {
    auto& registry = GlobalLoggerRegistry::instance();
    registry.register_logger("bench-has-logger", std::make_shared<BenchLogger>());

    for (auto _ : state) {
        benchmark::DoNotOptimize(registry.has_logger("bench-has-logger"));
    }

    registry.unregister_logger("bench-has-logger");
}
BENCHMARK(BM_GlobalLoggerRegistryHasLogger);

// Concurrent get_logger() throughput
static void BM_GlobalLoggerRegistryConcurrentGetLogger(benchmark::State& state) {
    auto& registry = GlobalLoggerRegistry::instance();
    registry.register_logger("bench-concurrent", std::make_shared<BenchLogger>());
    const int num_threads = state.range(0);

    for (auto _ : state) {
        std::vector<std::thread> threads;
        for (int t = 0; t < num_threads; ++t) {
            threads.emplace_back([&registry]() {
                for (int i = 0; i < 1000; ++i) {
                    auto logger = registry.get_logger("bench-concurrent");
                    benchmark::DoNotOptimize(logger);
                }
            });
        }
        for (auto& t : threads) {
            t.join();
        }
    }
    state.SetItemsProcessed(state.iterations() * num_threads * 1000);

    registry.unregister_logger("bench-concurrent");
}
BENCHMARK(BM_GlobalLoggerRegistryConcurrentGetLogger)->Arg(1)->Arg(2)->Arg(4)->Arg(8);
