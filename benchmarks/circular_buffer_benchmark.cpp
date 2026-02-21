// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

#include <benchmark/benchmark.h>
#include <kcenon/common/utils/circular_buffer.h>
#include <thread>
#include <vector>

using namespace kcenon::common::utils;

// Push throughput on non-full buffer
static void BM_CircularBufferPush(benchmark::State& state) {
    CircularBuffer<int, 1024> buffer;

    for (auto _ : state) {
        state.PauseTiming();
        while (buffer.pop().has_value()) {} // drain
        state.ResumeTiming();

        for (int i = 0; i < 1024; ++i) {
            benchmark::DoNotOptimize(buffer.push(i));
        }
    }
    state.SetItemsProcessed(state.iterations() * 1024);
}
BENCHMARK(BM_CircularBufferPush);

// Pop throughput from full buffer
static void BM_CircularBufferPop(benchmark::State& state) {
    CircularBuffer<int, 1024> buffer;

    for (auto _ : state) {
        state.PauseTiming();
        while (buffer.pop().has_value()) {} // drain
        for (int i = 0; i < 1024; ++i) {
            buffer.push(i);
        }
        state.ResumeTiming();

        for (int i = 0; i < 1024; ++i) {
            benchmark::DoNotOptimize(buffer.pop());
        }
    }
    state.SetItemsProcessed(state.iterations() * 1024);
}
BENCHMARK(BM_CircularBufferPop);

// Push with overwrite on full buffer
static void BM_CircularBufferPushOverwrite(benchmark::State& state) {
    CircularBuffer<int, 512> buffer;
    // Fill buffer
    for (int i = 0; i < 512; ++i) {
        buffer.push(i);
    }

    for (auto _ : state) {
        benchmark::DoNotOptimize(buffer.push(42, true));
    }
}
BENCHMARK(BM_CircularBufferPushOverwrite);

// Push/pop interleaved (producer-consumer pattern)
static void BM_CircularBufferPushPopInterleaved(benchmark::State& state) {
    CircularBuffer<int, 256> buffer;

    for (auto _ : state) {
        buffer.push(42);
        benchmark::DoNotOptimize(buffer.pop());
    }
}
BENCHMARK(BM_CircularBufferPushPopInterleaved);

// Size and empty checks (read-only hot path)
static void BM_CircularBufferSizeCheck(benchmark::State& state) {
    CircularBuffer<int, 1024> buffer;
    for (int i = 0; i < 512; ++i) {
        buffer.push(i);
    }

    for (auto _ : state) {
        benchmark::DoNotOptimize(buffer.size());
        benchmark::DoNotOptimize(buffer.empty());
        benchmark::DoNotOptimize(buffer.full());
    }
}
BENCHMARK(BM_CircularBufferSizeCheck);

// Concurrent push from multiple threads
static void BM_CircularBufferConcurrentPush(benchmark::State& state) {
    CircularBuffer<int, 4096> buffer;
    const int num_threads = state.range(0);

    for (auto _ : state) {
        state.PauseTiming();
        while (buffer.pop().has_value()) {} // drain
        state.ResumeTiming();

        std::vector<std::thread> threads;
        const int items_per_thread = 4096 / num_threads;

        for (int t = 0; t < num_threads; ++t) {
            threads.emplace_back([&buffer, items_per_thread, t]() {
                for (int i = 0; i < items_per_thread; ++i) {
                    buffer.push(t * items_per_thread + i, true);
                }
            });
        }

        for (auto& t : threads) {
            t.join();
        }
    }
    state.SetItemsProcessed(state.iterations() * 4096);
}
BENCHMARK(BM_CircularBufferConcurrentPush)->Arg(1)->Arg(2)->Arg(4)->Arg(8);
