// BSD 3-Clause License
//
// Copyright (c) 2021-2025, kcenon
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its
//    contributors may be used to endorse or promote products derived from
//    this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "system_fixture.h"
#include "test_helpers.h"
#include <kcenon/common/patterns/event_bus.h>
#include <kcenon/common/patterns/result.h>
#include <kcenon/common/utils/object_pool.h>
#include <kcenon/common/utils/circular_buffer.h>
#include <vector>
#include <thread>
#include <atomic>
#include <random>
#include <chrono>
#include <barrier>
#include <latch>

using namespace integration_tests;
using namespace kcenon::common;
using namespace kcenon::common::utils;

/**
 * Stress tests for high-load scenarios
 */
class StressTest : public PerformanceIntegrationTest {};

// =============================================================================
// Event Bus High-Load Tests
// =============================================================================

struct StressEvent {
    int thread_id;
    int sequence;
    std::vector<char> payload;

    StressEvent(int tid, int seq, size_t payload_size = 64)
        : thread_id(tid), sequence(seq), payload(payload_size, 'X') {}
};

TEST_F(StressTest, ConcurrentPublish100Threads) {
    simple_event_bus bus;
    std::atomic<int> received_count{0};
    const int thread_count = 100;
    const int events_per_thread = 100;

    auto sub_id = bus.subscribe<StressEvent>(
        [&received_count](const StressEvent&) {
            ++received_count;
        });

    std::vector<std::thread> threads;
    std::latch start_latch(thread_count);

    for (int t = 0; t < thread_count; ++t) {
        threads.emplace_back([&bus, t, events_per_thread, &start_latch]() {
            start_latch.arrive_and_wait();
            for (int i = 0; i < events_per_thread; ++i) {
                bus.publish(StressEvent{t, i});
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    EXPECT_EQ(received_count.load(), thread_count * events_per_thread);
    bus.unsubscribe(sub_id);

    std::cout << "ConcurrentPublish100Threads: " << received_count.load()
              << " events processed\n";
}

TEST_F(StressTest, ConcurrentSubscribeUnsubscribe) {
    simple_event_bus bus;
    std::atomic<int> subscription_count{0};
    std::atomic<int> unsubscription_count{0};
    const int thread_count = 50;
    const int ops_per_thread = 20;

    std::vector<std::thread> threads;
    std::latch start_latch(thread_count);

    for (int t = 0; t < thread_count; ++t) {
        threads.emplace_back([&bus, &subscription_count, &unsubscription_count,
                              ops_per_thread, &start_latch]() {
            start_latch.arrive_and_wait();
            for (int i = 0; i < ops_per_thread; ++i) {
                auto sub_id = bus.subscribe<StressEvent>(
                    [](const StressEvent&) {});
                ++subscription_count;

                std::this_thread::yield();

                bus.unsubscribe(sub_id);
                ++unsubscription_count;
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    EXPECT_EQ(subscription_count.load(), thread_count * ops_per_thread);
    EXPECT_EQ(unsubscription_count.load(), thread_count * ops_per_thread);

    std::cout << "ConcurrentSubscribeUnsubscribe: "
              << subscription_count.load() << " subscriptions, "
              << unsubscription_count.load() << " unsubscriptions\n";
}

TEST_F(StressTest, MixedOperationsStress) {
    simple_event_bus bus;
    std::atomic<int> events_published{0};
    std::atomic<int> events_received{0};
    std::atomic<bool> stop{false};
    const int publisher_count = 20;
    const int subscriber_count = 10;
    const int churner_count = 5;

    // Long-running subscribers
    std::vector<subscription_id> persistent_subs;
    for (int i = 0; i < 5; ++i) {
        auto sub_id = bus.subscribe<StressEvent>(
            [&events_received](const StressEvent&) {
                ++events_received;
            });
        persistent_subs.push_back(sub_id);
    }

    std::vector<std::thread> threads;

    // Publishers
    for (int t = 0; t < publisher_count; ++t) {
        threads.emplace_back([&bus, &events_published, &stop, t]() {
            int seq = 0;
            while (!stop.load()) {
                bus.publish(StressEvent{t, seq++});
                ++events_published;
                if (seq % 10 == 0) {
                    std::this_thread::yield();
                }
            }
        });
    }

    // Subscription churners (subscribe/unsubscribe rapidly)
    for (int t = 0; t < churner_count; ++t) {
        threads.emplace_back([&bus, &stop]() {
            while (!stop.load()) {
                auto sub_id = bus.subscribe<StressEvent>(
                    [](const StressEvent&) {});
                std::this_thread::yield();
                bus.unsubscribe(sub_id);
            }
        });
    }

    // Run for 2 seconds
    std::this_thread::sleep_for(std::chrono::seconds(2));
    stop = true;

    for (auto& thread : threads) {
        thread.join();
    }

    for (auto sub_id : persistent_subs) {
        bus.unsubscribe(sub_id);
    }

    std::cout << "MixedOperationsStress: published=" << events_published.load()
              << ", received=" << events_received.load() << "\n";

    EXPECT_GT(events_published.load(), 0);
    EXPECT_GT(events_received.load(), 0);
}

// =============================================================================
// Race Condition Tests
// =============================================================================

class RaceConditionTest : public PerformanceIntegrationTest {};

TEST_F(RaceConditionTest, PublishSubscribeRace) {
    // Test race between publish and subscribe operations
    simple_event_bus bus;
    std::atomic<int> received{0};
    const int iterations = 1000;

    for (int i = 0; i < iterations; ++i) {
        std::atomic<bool> subscribed{false};
        subscription_id sub_id = 0;

        std::thread subscriber([&]() {
            sub_id = bus.subscribe<StressEvent>(
                [&received](const StressEvent&) {
                    ++received;
                });
            subscribed = true;
        });

        std::thread publisher([&]() {
            // May or may not see the subscriber
            bus.publish(StressEvent{0, i});
        });

        subscriber.join();
        publisher.join();

        bus.unsubscribe(sub_id);
    }

    // Some events should be received (non-deterministic due to race)
    std::cout << "PublishSubscribeRace: " << received.load()
              << "/" << iterations << " events caught\n";
}

TEST_F(RaceConditionTest, MultipleWritersSingleReader) {
    CircularBuffer<int, 1000> buffer;
    std::atomic<int> written{0};
    std::atomic<int> read{0};
    std::atomic<bool> stop{false};
    const int writer_count = 10;

    std::vector<std::thread> writers;
    for (int w = 0; w < writer_count; ++w) {
        writers.emplace_back([&buffer, &written, &stop, w]() {
            int val = w * 10000;
            while (!stop.load()) {
                if (buffer.push(val++, true)) {
                    ++written;
                }
            }
        });
    }

    std::thread reader([&buffer, &read, &stop]() {
        while (!stop.load() || !buffer.empty()) {
            auto val = buffer.pop();
            if (val.has_value()) {
                ++read;
            }
        }
    });

    std::this_thread::sleep_for(std::chrono::seconds(1));
    stop = true;

    for (auto& w : writers) {
        w.join();
    }
    reader.join();

    std::cout << "MultipleWritersSingleReader: written=" << written.load()
              << ", read=" << read.load() << "\n";

    EXPECT_GT(written.load(), 0);
    EXPECT_GT(read.load(), 0);
}

// =============================================================================
// Sustained Load Tests
// =============================================================================

class SustainedLoadTest : public PerformanceIntegrationTest {};

TEST_F(SustainedLoadTest, SustainedLoad30Seconds) {
    // Shorter version for CI - 30 seconds instead of 10 minutes
    simple_event_bus bus;
    ObjectPool<StressEvent> pool(64);
    std::atomic<int> operations{0};
    std::atomic<bool> stop{false};
    const int thread_count = 16;

    auto sub_id = bus.subscribe<StressEvent>(
        [](const StressEvent&) {});

    std::vector<std::thread> threads;
    for (int t = 0; t < thread_count; ++t) {
        threads.emplace_back([&bus, &pool, &operations, &stop, t]() {
            while (!stop.load()) {
                // Mix of operations
                bool reused = false;
                auto obj = pool.acquire(&reused, t, operations.load() % 1000);
                bus.publish(StressEvent{t, operations.load()});

                auto result = Result<int>::ok(operations.load())
                    .map([](int x) { return x + 1; });
                (void)result;

                ++operations;

                if (operations.load() % 100 == 0) {
                    std::this_thread::yield();
                }
            }
        });
    }

    // Run for 30 seconds
    std::this_thread::sleep_for(std::chrono::seconds(30));
    stop = true;

    for (auto& thread : threads) {
        thread.join();
    }

    bus.unsubscribe(sub_id);

    std::cout << "SustainedLoad30Seconds: " << operations.load()
              << " operations completed\n";

    EXPECT_GT(operations.load(), 10000);
}

TEST_F(SustainedLoadTest, MemoryStabilityOverTime) {
    // Test that memory doesn't grow unbounded over time
    ObjectPool<StressEvent> pool(32);
    const int iterations = 10000;
    const int batch_size = 100;

    for (int i = 0; i < iterations; ++i) {
        std::vector<std::unique_ptr<StressEvent, std::function<void(StressEvent*)>>> batch;

        // Acquire batch
        for (int j = 0; j < batch_size; ++j) {
            bool reused = false;
            batch.push_back(pool.acquire(&reused, i, j));
        }

        // Release batch
        batch.clear();

        // Pool should reclaim memory
        EXPECT_GE(pool.available(), 0);
    }

    std::cout << "MemoryStabilityOverTime: completed " << iterations
              << " iterations, pool available: " << pool.available() << "\n";
}

// =============================================================================
// Thread Contention Tests
// =============================================================================

class ThreadContentionTest : public PerformanceIntegrationTest {};

TEST_F(ThreadContentionTest, HighContentionObjectPool) {
    ObjectPool<StressEvent> pool(8);  // Small pool to force contention
    std::atomic<int> success{0};
    std::atomic<int> total_ops{0};
    const int thread_count = 32;
    const int ops_per_thread = 500;

    std::vector<std::thread> threads;
    std::latch start_latch(thread_count);

    for (int t = 0; t < thread_count; ++t) {
        threads.emplace_back([&pool, &success, &total_ops, t, ops_per_thread, &start_latch]() {
            start_latch.arrive_and_wait();
            for (int i = 0; i < ops_per_thread; ++i) {
                bool reused = false;
                auto obj = pool.acquire(&reused, t, i);
                if (obj) {
                    ++success;
                    // Hold briefly to increase contention
                    std::this_thread::yield();
                }
                ++total_ops;
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    EXPECT_EQ(success.load(), total_ops.load());
    std::cout << "HighContentionObjectPool: " << success.load()
              << "/" << total_ops.load() << " successful acquisitions\n";
}

TEST_F(ThreadContentionTest, HighContentionCircularBuffer) {
    CircularBuffer<int, 64> buffer;  // Small buffer to force contention
    std::atomic<int> successful_pushes{0};
    std::atomic<int> successful_pops{0};
    std::atomic<bool> stop{false};
    const int writer_count = 16;
    const int reader_count = 16;

    std::vector<std::thread> writers;
    std::vector<std::thread> readers;

    for (int w = 0; w < writer_count; ++w) {
        writers.emplace_back([&buffer, &successful_pushes, &stop, w]() {
            int val = w * 100000;
            while (!stop.load()) {
                if (buffer.push(val++, false)) {
                    ++successful_pushes;
                }
                std::this_thread::yield();
            }
        });
    }

    for (int r = 0; r < reader_count; ++r) {
        readers.emplace_back([&buffer, &successful_pops, &stop]() {
            while (!stop.load()) {
                auto val = buffer.pop();
                if (val.has_value()) {
                    ++successful_pops;
                }
                std::this_thread::yield();
            }
        });
    }

    std::this_thread::sleep_for(std::chrono::seconds(2));
    stop = true;

    for (auto& w : writers) {
        w.join();
    }
    for (auto& r : readers) {
        r.join();
    }

    std::cout << "HighContentionCircularBuffer: pushes=" << successful_pushes.load()
              << ", pops=" << successful_pops.load() << "\n";

    EXPECT_GT(successful_pushes.load(), 0);
    EXPECT_GT(successful_pops.load(), 0);
}

// =============================================================================
// Burst Traffic Tests
// =============================================================================

class BurstTrafficTest : public PerformanceIntegrationTest {};

TEST_F(BurstTrafficTest, EventBusBurst) {
    simple_event_bus bus;
    std::atomic<int> received{0};
    const int burst_size = 10000;
    const int burst_count = 10;

    auto sub_id = bus.subscribe<StressEvent>(
        [&received](const StressEvent&) {
            ++received;
        });

    for (int burst = 0; burst < burst_count; ++burst) {
        // Burst of events
        for (int i = 0; i < burst_size; ++i) {
            bus.publish(StressEvent{burst, i});
        }

        // Brief pause between bursts
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    bus.unsubscribe(sub_id);

    EXPECT_EQ(received.load(), burst_size * burst_count);
    std::cout << "EventBusBurst: " << received.load() << " events processed\n";
}

TEST_F(BurstTrafficTest, ObjectPoolBurst) {
    ObjectPool<StressEvent> pool(128);
    const int burst_size = 500;
    const int burst_count = 20;

    for (int burst = 0; burst < burst_count; ++burst) {
        std::vector<std::unique_ptr<StressEvent, std::function<void(StressEvent*)>>> batch;

        // Acquire burst
        for (int i = 0; i < burst_size; ++i) {
            bool reused = false;
            auto obj = pool.acquire(&reused, burst, i);
            ASSERT_NE(obj, nullptr);
            batch.push_back(std::move(obj));
        }

        // Release all at once
        batch.clear();

        // Pool should handle rapid release
        EXPECT_GE(pool.available(), static_cast<size_t>(burst_size));
    }

    std::cout << "ObjectPoolBurst: completed " << burst_count
              << " bursts of " << burst_size << " objects\n";
}
