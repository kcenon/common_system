// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

#include <benchmark/benchmark.h>
#include <kcenon/common/patterns/event_bus.h>
#include <string>
#include <vector>

using namespace kcenon::common;

// Custom event types for benchmarking
struct SimpleEvent {
    int value;
};

struct StringEvent {
    std::string message;
};

struct LargeEvent {
    std::vector<int> data;
    LargeEvent() : data(100, 42) {}
    explicit LargeEvent(int size) : data(size, 42) {}
};

// Event publish benchmarks
static void BM_EventBusPublishNoSubscribers(benchmark::State& state) {
    simple_event_bus bus;
    bus.start();

    for (auto _ : state) {
        bus.publish(SimpleEvent{42});
    }
}
BENCHMARK(BM_EventBusPublishNoSubscribers);

static void BM_EventBusPublishSingleSubscriber(benchmark::State& state) {
    simple_event_bus bus;
    bus.start();

    int received = 0;
    bus.subscribe<SimpleEvent>([&received](const SimpleEvent& e) {
        received = e.value;
    });

    for (auto _ : state) {
        bus.publish(SimpleEvent{42});
        benchmark::DoNotOptimize(received);
    }
}
BENCHMARK(BM_EventBusPublishSingleSubscriber);

static void BM_EventBusPublishMultipleSubscribers(benchmark::State& state) {
    simple_event_bus bus;
    bus.start();

    const int subscriber_count = state.range(0);
    std::vector<int> received(subscriber_count, 0);

    for (int i = 0; i < subscriber_count; ++i) {
        bus.subscribe<SimpleEvent>([&received, i](const SimpleEvent& e) {
            received[i] = e.value;
        });
    }

    for (auto _ : state) {
        bus.publish(SimpleEvent{42});
        benchmark::DoNotOptimize(received);
    }
}
BENCHMARK(BM_EventBusPublishMultipleSubscribers)->Range(1, 100);

// Subscribe/unsubscribe benchmarks
static void BM_EventBusSubscribe(benchmark::State& state) {
    simple_event_bus bus;
    bus.start();

    for (auto _ : state) {
        auto id = bus.subscribe<SimpleEvent>([](const SimpleEvent&) {});
        benchmark::DoNotOptimize(id);
        bus.unsubscribe(id);
    }
}
BENCHMARK(BM_EventBusSubscribe);

static void BM_EventBusSubscribeFiltered(benchmark::State& state) {
    simple_event_bus bus;
    bus.start();

    for (auto _ : state) {
        auto id = bus.subscribe_filtered<SimpleEvent>(
            [](const SimpleEvent&) {},
            [](const SimpleEvent& e) { return e.value > 0; }
        );
        benchmark::DoNotOptimize(id);
        bus.unsubscribe(id);
    }
}
BENCHMARK(BM_EventBusSubscribeFiltered);

// Large event benchmarks
static void BM_EventBusPublishLargeEvent(benchmark::State& state) {
    simple_event_bus bus;
    bus.start();

    LargeEvent received;
    bus.subscribe<LargeEvent>([&received](const LargeEvent& e) {
        received = e;
    });

    LargeEvent evt(state.range(0));

    for (auto _ : state) {
        bus.publish(evt);
        benchmark::DoNotOptimize(received);
    }
    state.SetBytesProcessed(state.iterations() * evt.data.size() * sizeof(int));
}
BENCHMARK(BM_EventBusPublishLargeEvent)->Range(100, 10000);

// String event benchmarks
static void BM_EventBusPublishStringEvent(benchmark::State& state) {
    simple_event_bus bus;
    bus.start();

    std::string received;
    bus.subscribe<StringEvent>([&received](const StringEvent& e) {
        received = e.message;
    });

    StringEvent evt{std::string(state.range(0), 'x')};

    for (auto _ : state) {
        bus.publish(evt);
        benchmark::DoNotOptimize(received);
    }
    state.SetBytesProcessed(state.iterations() * evt.message.size());
}
BENCHMARK(BM_EventBusPublishStringEvent)->Range(10, 1000);

// Filtered event benchmarks
static void BM_EventBusFilteredPublish(benchmark::State& state) {
    simple_event_bus bus;
    bus.start();

    int received = 0;
    bus.subscribe_filtered<SimpleEvent>(
        [&received](const SimpleEvent& e) { received = e.value; },
        [](const SimpleEvent& e) { return e.value > 20; }
    );

    for (auto _ : state) {
        bus.publish(SimpleEvent{42});
        benchmark::DoNotOptimize(received);
    }
}
BENCHMARK(BM_EventBusFilteredPublish);

static void BM_EventBusFilteredPublishFiltered(benchmark::State& state) {
    simple_event_bus bus;
    bus.start();

    int received = 0;
    bus.subscribe_filtered<SimpleEvent>(
        [&received](const SimpleEvent& e) { received = e.value; },
        [](const SimpleEvent& e) { return e.value > 50; }
    );

    // This event should be filtered out
    for (auto _ : state) {
        bus.publish(SimpleEvent{10});
        benchmark::DoNotOptimize(received);
    }
}
BENCHMARK(BM_EventBusFilteredPublishFiltered);

// Throughput benchmark
static void BM_EventBusThroughput(benchmark::State& state) {
    simple_event_bus bus;
    bus.start();

    int total = 0;
    bus.subscribe<SimpleEvent>([&total](const SimpleEvent& e) {
        total += e.value;
    });

    const int batch_size = state.range(0);
    for (auto _ : state) {
        for (int i = 0; i < batch_size; ++i) {
            bus.publish(SimpleEvent{i});
        }
        benchmark::DoNotOptimize(total);
    }
    state.SetItemsProcessed(state.iterations() * batch_size);
}
BENCHMARK(BM_EventBusThroughput)->Range(100, 10000);
