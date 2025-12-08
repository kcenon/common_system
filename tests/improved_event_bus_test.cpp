/**
 * @file improved_event_bus_test.cpp
 * @brief Tests for improved Event Bus with filtering functionality
 * @date 2025-11-19
 */

#include <gtest/gtest.h>
#include <kcenon/common/patterns/event_bus.h>
#include <atomic>
#include <thread>
#include <chrono>

using namespace kcenon::common;

// Helper to wait for a condition to be true
template<typename Predicate>
bool WaitForCondition(Predicate pred, std::chrono::milliseconds timeout = std::chrono::seconds(1)) {
    auto start = std::chrono::steady_clock::now();
    while (std::chrono::steady_clock::now() - start < timeout) {
        if (pred()) return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return pred();
}

// Test event types
struct TestEvent {
    int id;
    std::string message;
    int priority;
};

struct FilterableEvent {
    int category;
    int value;
    bool active;
};

// Test basic filtering functionality
TEST(ImprovedEventBusTest, BasicFiltering) {
    simple_event_bus bus;
    std::atomic<int> handler_calls{0};

    // Subscribe with filter - only handle events with id > 5
    auto sub_id = bus.subscribe_filtered<TestEvent>(
        [&handler_calls](const TestEvent& evt) {
            handler_calls++;
            EXPECT_GT(evt.id, 5);
        },
        [](const TestEvent& evt) { return evt.id > 5; }
    );

    bus.start();

    // Publish events
    bus.publish(TestEvent{1, "Low ID", 1});    // Should be filtered out
    bus.publish(TestEvent{3, "Low ID", 1});    // Should be filtered out
    bus.publish(TestEvent{6, "High ID", 1});   // Should pass filter
    bus.publish(TestEvent{10, "High ID", 1});  // Should pass filter

    // Give time for events to process
    EXPECT_TRUE(WaitForCondition([&]() { return handler_calls == 2; }));

    EXPECT_EQ(handler_calls, 2);  // Only 2 events should have passed the filter

    bus.unsubscribe(sub_id);
    bus.stop();
}

// Test multiple filters on the same event type
TEST(ImprovedEventBusTest, MultipleFilters) {
    simple_event_bus bus;
    std::atomic<int> high_priority_calls{0};
    std::atomic<int> low_priority_calls{0};

    // Subscribe for high priority events
    auto high_sub = bus.subscribe_filtered<TestEvent>(
        [&high_priority_calls](const TestEvent& evt) {
            high_priority_calls++;
            EXPECT_GE(evt.priority, 5);
        },
        [](const TestEvent& evt) { return evt.priority >= 5; }
    );

    // Subscribe for low priority events
    auto low_sub = bus.subscribe_filtered<TestEvent>(
        [&low_priority_calls](const TestEvent& evt) {
            low_priority_calls++;
            EXPECT_LT(evt.priority, 5);
        },
        [](const TestEvent& evt) { return evt.priority < 5; }
    );

    bus.start();

    // Publish various priority events
    bus.publish(TestEvent{1, "Low", 1});    // Low priority
    bus.publish(TestEvent{2, "Low", 2});    // Low priority
    bus.publish(TestEvent{3, "High", 7});   // High priority
    bus.publish(TestEvent{4, "High", 10});  // High priority
    bus.publish(TestEvent{5, "Med", 5});    // High priority (>= 5)

    EXPECT_TRUE(WaitForCondition([&]() { 
        return low_priority_calls == 2 && high_priority_calls == 3; 
    }));

    EXPECT_EQ(low_priority_calls, 2);   // Events with priority < 5
    EXPECT_EQ(high_priority_calls, 3);  // Events with priority >= 5

    bus.unsubscribe(high_sub);
    bus.unsubscribe(low_sub);
    bus.stop();
}

// Test complex filtering logic
TEST(ImprovedEventBusTest, ComplexFiltering) {
    simple_event_bus bus;
    std::vector<FilterableEvent> received_events;
    std::mutex received_mutex;

    // Subscribe with complex filter: category == 1 AND value > 100 AND active == true
    auto sub_id = bus.subscribe_filtered<FilterableEvent>(
        [&received_events, &received_mutex](const FilterableEvent& evt) {
            std::lock_guard<std::mutex> lock(received_mutex);
            received_events.push_back(evt);
        },
        [](const FilterableEvent& evt) {
            return evt.category == 1 && evt.value > 100 && evt.active;
        }
    );

    bus.start();

    // Publish various events
    bus.publish(FilterableEvent{1, 150, true});   // PASS: all conditions met
    bus.publish(FilterableEvent{2, 150, true});   // FAIL: wrong category
    bus.publish(FilterableEvent{1, 50, true});    // FAIL: value too low
    bus.publish(FilterableEvent{1, 150, false});  // FAIL: not active
    bus.publish(FilterableEvent{1, 200, true});   // PASS: all conditions met

    EXPECT_TRUE(WaitForCondition([&]() {
        std::lock_guard<std::mutex> lock(received_mutex);
        return received_events.size() == 2;
    }));

    {
        std::lock_guard<std::mutex> lock(received_mutex);
        EXPECT_EQ(received_events.size(), 2);
        if (received_events.size() >= 2) {
            EXPECT_EQ(received_events[0].value, 150);
            EXPECT_EQ(received_events[1].value, 200);
        }
    }

    bus.unsubscribe(sub_id);
    bus.stop();
}

// Test that filtering doesn't affect non-filtered subscribers
TEST(ImprovedEventBusTest, MixedFilteredAndNonFiltered) {
    simple_event_bus bus;
    std::atomic<int> filtered_calls{0};
    std::atomic<int> unfiltered_calls{0};

    // Filtered subscriber - only even IDs
    auto filtered_sub = bus.subscribe_filtered<TestEvent>(
        [&filtered_calls](const TestEvent& evt) {
            filtered_calls++;
            EXPECT_EQ(evt.id % 2, 0);
        },
        [](const TestEvent& evt) { return evt.id % 2 == 0; }
    );

    // Unfiltered subscriber - receives all events
    auto unfiltered_sub = bus.subscribe<TestEvent>(
        [&unfiltered_calls](const TestEvent& evt) {
            unfiltered_calls++;
        }
    );

    bus.start();

    // Publish 10 events
    for (int i = 1; i <= 10; ++i) {
        bus.publish(TestEvent{i, "Event", 1});
    }

    EXPECT_TRUE(WaitForCondition([&]() {
        return filtered_calls == 5 && unfiltered_calls == 10;
    }));

    EXPECT_EQ(filtered_calls, 5);     // Only even IDs (2,4,6,8,10)
    EXPECT_EQ(unfiltered_calls, 10);  // All events

    bus.unsubscribe(filtered_sub);
    bus.unsubscribe(unfiltered_sub);
    bus.stop();
}

// Test filter performance (filters should not significantly impact performance)
TEST(ImprovedEventBusTest, FilterPerformance) {
    simple_event_bus bus;
    std::atomic<int> passed_count{0};

    // Heavy filter that checks multiple conditions
    auto sub_id = bus.subscribe_filtered<TestEvent>(
        [&passed_count](const TestEvent& evt) {
            passed_count++;
        },
        [](const TestEvent& evt) {
            // Complex filter logic
            return evt.id > 0 &&
                   evt.id % 2 == 0 &&
                   evt.priority > 0 &&
                   !evt.message.empty() &&
                   evt.message.length() > 3;
        }
    );

    bus.start();

    auto start = std::chrono::high_resolution_clock::now();

    // Publish many events
    for (int i = 0; i < 1000; ++i) {
        bus.publish(TestEvent{i, "TestMessage", i % 10});
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    // Performance check - should complete reasonably quickly
    EXPECT_LT(duration.count(), 100);  // Should take less than 100ms

    // Verify filtering worked
    EXPECT_GT(passed_count, 0);
    EXPECT_LT(passed_count, 1000);  // Not all events should pass

    bus.unsubscribe(sub_id);
    bus.stop();
}

// Test thread safety of filtered subscriptions
TEST(ImprovedEventBusTest, ThreadSafetyWithFilters) {
    simple_event_bus bus;
    std::atomic<int> total_handled{0};
    const int num_threads = 4;
    const int events_per_thread = 100;

    // Subscribe with filter for specific thread IDs
    std::vector<uint64_t> subscriptions;
    for (int thread_id = 0; thread_id < num_threads; ++thread_id) {
        auto sub = bus.subscribe_filtered<TestEvent>(
            [&total_handled, thread_id](const TestEvent& evt) {
                EXPECT_EQ(evt.id % num_threads, thread_id);
                total_handled++;
            },
            [thread_id, num_threads](const TestEvent& evt) {
                return evt.id % num_threads == thread_id;
            }
        );
        subscriptions.push_back(sub);
    }

    bus.start();

    // Multiple threads publishing events
    std::vector<std::thread> threads;
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&bus, t, events_per_thread]() {
            for (int i = 0; i < events_per_thread; ++i) {
                bus.publish(TestEvent{i, "Thread event", t});
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    EXPECT_TRUE(WaitForCondition([&]() {
        return total_handled == num_threads * events_per_thread;
    }));

    // Each subscription should have handled its share of events
    EXPECT_EQ(total_handled, num_threads * events_per_thread);

    for (auto sub : subscriptions) {
        bus.unsubscribe(sub);
    }
    bus.stop();
}

// Test that filter exceptions don't crash the bus
TEST(ImprovedEventBusTest, FilterExceptionHandling) {
    simple_event_bus bus;
    std::atomic<int> handler_calls{0};
    bool exception_logged = false;

    // Set error callback to catch filter exceptions
    bus.set_error_callback(
        [&exception_logged](const std::string& msg, size_t, uint64_t) {
            if (msg.find("Exception") != std::string::npos) {
                exception_logged = true;
            }
        }
    );

    // Subscribe with a filter that throws on certain conditions
    auto sub_id = bus.subscribe_filtered<TestEvent>(
        [&handler_calls](const TestEvent& evt) {
            handler_calls++;
        },
        [](const TestEvent& evt) {
            if (evt.id == 666) {
                throw std::runtime_error("Bad ID!");
            }
            return evt.id > 0;
        }
    );

    bus.start();

    // Publish events including one that causes filter exception
    bus.publish(TestEvent{1, "Normal", 1});
    bus.publish(TestEvent{666, "Bad", 1});  // This will throw in filter
    bus.publish(TestEvent{2, "Normal", 1});

    EXPECT_TRUE(WaitForCondition([&]() {
        return handler_calls >= 2;
    }));

    // Handler should be called for valid events (filter exception caught internally)
    // Note: The actual behavior depends on implementation -
    // if exception is in wrapped handler, it gets caught
    EXPECT_GE(handler_calls, 2);  // At least the non-throwing events

    bus.unsubscribe(sub_id);
    bus.stop();
}