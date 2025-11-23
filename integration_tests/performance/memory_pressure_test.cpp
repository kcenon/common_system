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
#include <kcenon/common/utils/object_pool.h>
#include <kcenon/common/utils/circular_buffer.h>
#include <kcenon/common/patterns/event_bus.h>
#include <kcenon/common/patterns/result.h>
#include <vector>
#include <string>
#include <thread>
#include <atomic>
#include <memory>

using namespace integration_tests;
using namespace kcenon::common;
using namespace kcenon::common::utils;

/**
 * Memory pressure tests for common system components
 */
class MemoryPressureTest : public PerformanceIntegrationTest {};

// =============================================================================
// Object Pool Pressure Tests
// =============================================================================

struct ExpensiveObject {
    std::vector<int> data;
    std::string name;

    ExpensiveObject() : data(1000, 42), name("expensive") {}
    explicit ExpensiveObject(int size) : data(size, 42), name("expensive") {}
};

TEST_F(MemoryPressureTest, ObjectPoolExhaustion) {
    ObjectPool<ExpensiveObject> pool(4);

    // Acquire all pre-allocated objects
    std::vector<std::unique_ptr<ExpensiveObject, std::function<void(ExpensiveObject*)>>> acquired;

    // Acquire more than initial capacity to trigger growth
    const size_t acquire_count = 20;
    for (size_t i = 0; i < acquire_count; ++i) {
        bool reused = false;
        auto obj = pool.acquire(&reused);
        ASSERT_NE(obj, nullptr) << "Failed to acquire object at iteration " << i;
        acquired.push_back(std::move(obj));
    }

    EXPECT_EQ(acquired.size(), acquire_count);

    // Release all - should return to pool
    acquired.clear();

    // Pool should now have all objects available
    EXPECT_EQ(pool.available(), acquire_count);
}

TEST_F(MemoryPressureTest, ObjectPoolRecovery) {
    ObjectPool<ExpensiveObject> pool(8);

    // Stress test: acquire and release in cycles
    const int cycles = 100;
    const size_t objects_per_cycle = 10;

    for (int cycle = 0; cycle < cycles; ++cycle) {
        std::vector<std::unique_ptr<ExpensiveObject, std::function<void(ExpensiveObject*)>>> batch;

        for (size_t i = 0; i < objects_per_cycle; ++i) {
            bool reused = false;
            batch.push_back(pool.acquire(&reused));
        }

        // All should be valid
        for (const auto& obj : batch) {
            ASSERT_NE(obj, nullptr);
            EXPECT_EQ(obj->data.size(), 1000);
        }

        batch.clear(); // Release all back to pool
    }

    // Pool should still be functional
    bool reused = false;
    auto final_obj = pool.acquire(&reused);
    ASSERT_NE(final_obj, nullptr);
    EXPECT_EQ(final_obj->data.size(), 1000);
}

TEST_F(MemoryPressureTest, ObjectPoolFragmentation) {
    ObjectPool<ExpensiveObject> pool(16);

    // Simulate fragmentation: acquire/release in random patterns
    std::vector<std::unique_ptr<ExpensiveObject, std::function<void(ExpensiveObject*)>>> held;

    for (int i = 0; i < 100; ++i) {
        // Acquire some
        for (int j = 0; j < 5; ++j) {
            bool reused = false;
            held.push_back(pool.acquire(&reused));
        }

        // Release half (every other)
        for (size_t k = 0; k < held.size(); k += 2) {
            if (k < held.size()) {
                held[k].reset();
            }
        }

        // Remove nullptrs
        held.erase(std::remove_if(held.begin(), held.end(),
            [](const auto& ptr) { return ptr == nullptr; }), held.end());
    }

    // Pool should handle fragmented state
    bool reused = false;
    auto obj = pool.acquire(&reused);
    ASSERT_NE(obj, nullptr);

    held.clear();

    // Verify pool recovers all memory
    size_t available_after = pool.available();
    EXPECT_GT(available_after, 0);
}

TEST_F(MemoryPressureTest, ObjectPoolConcurrentAccess) {
    ObjectPool<ExpensiveObject> pool(32);
    std::atomic<int> success_count{0};
    std::atomic<int> failure_count{0};
    const int thread_count = 8;
    const int ops_per_thread = 100;

    std::vector<std::thread> threads;
    for (int t = 0; t < thread_count; ++t) {
        threads.emplace_back([&pool, &success_count, &failure_count, ops_per_thread]() {
            for (int i = 0; i < ops_per_thread; ++i) {
                bool reused = false;
                auto obj = pool.acquire(&reused);
                if (obj) {
                    ++success_count;
                    obj->data[0] = i; // Use the object
                } else {
                    ++failure_count;
                }
                // Object released when going out of scope
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_EQ(success_count.load(), thread_count * ops_per_thread);
    EXPECT_EQ(failure_count.load(), 0);
}

// =============================================================================
// Circular Buffer Pressure Tests
// =============================================================================

TEST_F(MemoryPressureTest, CircularBufferOverflow) {
    constexpr size_t capacity = 100;
    CircularBuffer<int, capacity> buffer;

    // Fill to capacity
    for (size_t i = 0; i < capacity; ++i) {
        EXPECT_TRUE(buffer.push(static_cast<int>(i)));
    }

    EXPECT_TRUE(buffer.full());
    EXPECT_EQ(buffer.size(), capacity);

    // Attempt to push without overwrite - should fail
    EXPECT_FALSE(buffer.push(999, false));
    EXPECT_EQ(buffer.size(), capacity);

    // Push with overwrite - should succeed and drop oldest
    EXPECT_TRUE(buffer.push(999, true));
    EXPECT_EQ(buffer.size(), capacity);

    // First element should now be 1 (0 was dropped)
    auto first = buffer.pop();
    ASSERT_TRUE(first.has_value());
    EXPECT_EQ(first.value(), 1);
}

TEST_F(MemoryPressureTest, CircularBufferUnderPressure) {
    constexpr size_t capacity = 50;
    CircularBuffer<std::string, capacity> buffer;

    // Simulate high-frequency push/pop
    const int iterations = 10000;
    int pushed = 0;
    int popped = 0;

    for (int i = 0; i < iterations; ++i) {
        std::string data = "message_" + std::to_string(i);

        if (buffer.push(data, true)) {
            ++pushed;
        }

        // Pop every other iteration
        if (i % 2 == 0) {
            auto result = buffer.pop();
            if (result.has_value()) {
                ++popped;
            }
        }
    }

    EXPECT_EQ(pushed, iterations);
    EXPECT_GT(popped, 0);

    // Drain remaining
    while (!buffer.empty()) {
        (void)buffer.pop();
    }

    EXPECT_TRUE(buffer.empty());
    EXPECT_EQ(buffer.size(), 0);
}

TEST_F(MemoryPressureTest, CircularBufferConcurrentReadWrite) {
    constexpr size_t capacity = 100;
    CircularBuffer<int, capacity> buffer;
    std::atomic<int> produced{0};
    std::atomic<int> consumed{0};
    std::atomic<bool> stop{false};

    // Producer thread
    std::thread producer([&]() {
        for (int i = 0; i < 1000 && !stop; ++i) {
            if (buffer.push(i, true)) {
                ++produced;
            }
            std::this_thread::yield();
        }
    });

    // Consumer thread
    std::thread consumer([&]() {
        while (!stop || !buffer.empty()) {
            auto val = buffer.pop();
            if (val.has_value()) {
                ++consumed;
            }
            std::this_thread::yield();
        }
    });

    producer.join();
    stop = true;
    consumer.join();

    EXPECT_EQ(produced.load(), 1000);
    EXPECT_GT(consumed.load(), 0);
    std::cout << "CircularBuffer concurrent: produced=" << produced.load()
              << ", consumed=" << consumed.load() << "\n";
}

// =============================================================================
// Event Bus Memory Tests
// =============================================================================

struct LargePayloadEvent {
    std::vector<char> payload;
    int id;

    explicit LargePayloadEvent(int event_id, size_t payload_size = 1024)
        : payload(payload_size, 'X'), id(event_id) {}
};

TEST_F(MemoryPressureTest, MassiveEventPublish) {
    simple_event_bus bus;
    std::atomic<int> received_count{0};

    auto sub_id = bus.subscribe<LargePayloadEvent>(
        [&received_count](const LargePayloadEvent& evt) {
            ++received_count;
            (void)evt;
        });

    // Publish many events
    const int event_count = 1000;
    for (int i = 0; i < event_count; ++i) {
        bus.publish(LargePayloadEvent{i, 512});
    }

    EXPECT_EQ(received_count.load(), event_count);

    bus.unsubscribe(sub_id);
}

TEST_F(MemoryPressureTest, LargeEventPayload) {
    simple_event_bus bus;
    bool received = false;
    size_t received_size = 0;

    auto sub_id = bus.subscribe<LargePayloadEvent>(
        [&received, &received_size](const LargePayloadEvent& evt) {
            received = true;
            received_size = evt.payload.size();
        });

    // Publish event with large payload (1MB)
    const size_t large_size = 1024 * 1024;
    bus.publish(LargePayloadEvent{1, large_size});

    EXPECT_TRUE(received);
    EXPECT_EQ(received_size, large_size);

    bus.unsubscribe(sub_id);
}

TEST_F(MemoryPressureTest, EventBusMemoryGrowth) {
    simple_event_bus bus;
    std::atomic<int> total_received{0};

    // Subscribe with multiple handlers
    std::vector<subscription_id> subscriptions;
    const int handler_count = 10;

    for (int h = 0; h < handler_count; ++h) {
        auto sub_id = bus.subscribe<LargePayloadEvent>(
            [&total_received](const LargePayloadEvent&) {
                ++total_received;
            });
        subscriptions.push_back(sub_id);
    }

    // Publish events
    const int event_count = 100;
    for (int i = 0; i < event_count; ++i) {
        bus.publish(LargePayloadEvent{i, 256});
    }

    EXPECT_EQ(total_received.load(), event_count * handler_count);

    // Unsubscribe all
    for (auto sub_id : subscriptions) {
        bus.unsubscribe(sub_id);
    }

    // Publish again - should not trigger handlers
    total_received = 0;
    bus.publish(LargePayloadEvent{999, 256});
    EXPECT_EQ(total_received.load(), 0);
}

// =============================================================================
// Result<T> Memory Tests
// =============================================================================

TEST_F(MemoryPressureTest, ResultChainMemory) {
    // Test memory behavior of chained Result operations
    const int chain_count = 1000;
    std::vector<Result<std::string>> results;
    results.reserve(chain_count);

    for (int i = 0; i < chain_count; ++i) {
        auto result = Result<int>::ok(i)
            .map([](int x) { return x * 2; })
            .and_then([](int x) -> Result<std::string> {
                return Result<std::string>::ok(std::to_string(x));
            })
            .map([](const std::string& s) { return s + "_processed"; });

        results.push_back(std::move(result));
    }

    // Verify all results are valid
    for (int i = 0; i < chain_count; ++i) {
        ASSERT_TRUE(results[i].is_ok()) << "Result " << i << " should be ok";
        auto expected = std::to_string(i * 2) + "_processed";
        EXPECT_EQ(results[i].value(), expected);
    }
}

TEST_F(MemoryPressureTest, LargeErrorMessage) {
    // Test Result with large error messages
    const size_t message_size = 10000;
    std::string large_message(message_size, 'E');

    auto result = Result<int>::err(error_code{-1, large_message});

    ASSERT_TRUE(result.is_err());
    EXPECT_EQ(result.error().message.size(), message_size);

    // Chain operations on error result
    auto chained = result
        .map([](int x) { return x * 2; })
        .or_else([](const error_code& err) -> Result<int> {
            return Result<int>::err(error_code{err.code, "recovered: " + err.message});
        });

    ASSERT_TRUE(chained.is_err());
    EXPECT_GT(chained.error().message.size(), message_size);
}

TEST_F(MemoryPressureTest, ResultWithLargeValue) {
    struct LargeStruct {
        std::array<char, 4096> data;
        int id;

        LargeStruct() : id(0) { data.fill('X'); }
        explicit LargeStruct(int i) : id(i) { data.fill('X'); }
    };

    // Create many Results with large values
    const int count = 100;
    std::vector<Result<LargeStruct>> results;
    results.reserve(count);

    for (int i = 0; i < count; ++i) {
        results.push_back(Result<LargeStruct>::ok(LargeStruct{i}));
    }

    // Verify
    for (int i = 0; i < count; ++i) {
        ASSERT_TRUE(results[i].is_ok());
        EXPECT_EQ(results[i].value().id, i);
    }

    // Move semantics test
    auto moved_result = std::move(results[0]);
    EXPECT_TRUE(moved_result.is_ok());
}

TEST_F(MemoryPressureTest, ResultMemoryReuse) {
    // Test that Result doesn't leak memory on reassignment
    Result<std::vector<int>> result = Result<std::vector<int>>::ok(std::vector<int>(1000, 42));

    for (int i = 0; i < 100; ++i) {
        // Reassign with new value
        result = Result<std::vector<int>>::ok(std::vector<int>(1000, i));
        ASSERT_TRUE(result.is_ok());
        EXPECT_EQ(result.value().size(), 1000);
        EXPECT_EQ(result.value()[0], i);
    }

    // Reassign with error
    result = Result<std::vector<int>>::err(error_code{-1, "test error"});
    ASSERT_TRUE(result.is_err());
}

// =============================================================================
// Combined Memory Pressure Tests
// =============================================================================

TEST_F(MemoryPressureTest, CombinedHighLoadScenario) {
    // Simulate realistic high-load scenario
    ObjectPool<ExpensiveObject> pool(16);
    CircularBuffer<int, 100> buffer;
    simple_event_bus bus;

    std::atomic<int> events_processed{0};
    auto sub_id = bus.subscribe<LargePayloadEvent>(
        [&events_processed](const LargePayloadEvent&) {
            ++events_processed;
        });

    const int iterations = 500;

    for (int i = 0; i < iterations; ++i) {
        // Use object pool
        bool reused = false;
        auto obj = pool.acquire(&reused, i % 100 + 1);
        ASSERT_NE(obj, nullptr);

        // Use circular buffer
        buffer.push(i, true);

        // Publish event
        bus.publish(LargePayloadEvent{i, 128});

        // Create Result chain
        auto result = Result<int>::ok(i)
            .map([](int x) { return x + 1; })
            .and_then([](int x) -> Result<int> {
                return Result<int>::ok(x * 2);
            });
        EXPECT_TRUE(result.is_ok());
    }

    EXPECT_EQ(events_processed.load(), iterations);
    bus.unsubscribe(sub_id);

    // Drain buffer
    while (!buffer.empty()) {
        (void)buffer.pop();
    }

    std::cout << "Combined high-load scenario completed successfully\n";
}
