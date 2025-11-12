// BSD 3-Clause License
//
// Copyright (c) 2021-2025, 🍀☀🌕🌥 🌊
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
#include <string>
#include <atomic>
#include <chrono>
#include <thread>

using namespace integration_tests;
using namespace kcenon::common;

/**
 * Integration tests for event bus pattern
 */
class EventBusIntegrationTest : public SystemFixture {
protected:
    struct TestEvent {
        std::string message;
        int value;
    };

    struct CounterEvent {
        int increment;
    };
};

TEST_F(EventBusIntegrationTest, BasicPublishSubscribe) {
    auto& bus = get_event_bus();

    bool event_received = false;
    std::string received_message;
    int received_value = 0;

    // Subscribe to event
    auto subscription_id = bus.subscribe<TestEvent>([&](const TestEvent& event) {
        event_received = true;
        received_message = event.message;
        received_value = event.value;
    });

    // Publish event
    TestEvent event{"test message", 42};
    bus.publish(event);

    // Give time for async processing if needed
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    // Verify event was received
    EXPECT_TRUE(event_received);
    EXPECT_EQ(received_message, "test message");
    EXPECT_EQ(received_value, 42);

    // Cleanup
    bus.unsubscribe(subscription_id);
}

TEST_F(EventBusIntegrationTest, MultipleSubscribers) {
    auto& bus = get_event_bus();

    std::atomic<int> call_count{0};

    // Subscribe multiple handlers
    auto sub1 = bus.subscribe<TestEvent>([&](const TestEvent&) {
        call_count++;
    });

    auto sub2 = bus.subscribe<TestEvent>([&](const TestEvent&) {
        call_count++;
    });

    auto sub3 = bus.subscribe<TestEvent>([&](const TestEvent&) {
        call_count++;
    });

    // Publish event
    TestEvent event{"test", 1};
    bus.publish(event);

    // Give time for async processing
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    // All three subscribers should have been called
    EXPECT_EQ(call_count.load(), 3);

    // Cleanup
    bus.unsubscribe(sub1);
    bus.unsubscribe(sub2);
    bus.unsubscribe(sub3);
}

TEST_F(EventBusIntegrationTest, UnsubscribePreventsDelivery) {
    auto& bus = get_event_bus();

    bool event_received = false;

    // Subscribe
    auto sub_id = bus.subscribe<TestEvent>([&](const TestEvent&) {
        event_received = true;
    });

    // Unsubscribe before publishing
    bus.unsubscribe(sub_id);

    // Publish event
    TestEvent event{"test", 1};
    bus.publish(event);

    // Give time for async processing
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    // Event should not be received
    EXPECT_FALSE(event_received);
}

TEST_F(EventBusIntegrationTest, DifferentEventTypes) {
    auto& bus = get_event_bus();

    bool test_event_received = false;
    bool counter_event_received = false;

    // Subscribe to different event types
    auto sub1 = bus.subscribe<TestEvent>([&](const TestEvent&) {
        test_event_received = true;
    });

    auto sub2 = bus.subscribe<CounterEvent>([&](const CounterEvent&) {
        counter_event_received = true;
    });

    // Publish TestEvent
    TestEvent test_event{"test", 1};
    bus.publish(test_event);

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    // Only TestEvent handler should be called
    EXPECT_TRUE(test_event_received);
    EXPECT_FALSE(counter_event_received);

    // Reset
    test_event_received = false;

    // Publish CounterEvent
    CounterEvent counter_event{5};
    bus.publish(counter_event);

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    // Only CounterEvent handler should be called
    EXPECT_FALSE(test_event_received);
    EXPECT_TRUE(counter_event_received);

    // Cleanup
    bus.unsubscribe(sub1);
    bus.unsubscribe(sub2);
}

TEST_F(EventBusIntegrationTest, HighVolumePublishing) {
    auto& bus = get_event_bus();

    std::atomic<int> event_count{0};
    const int num_events = 1000;

    auto sub_id = bus.subscribe<CounterEvent>([&](const CounterEvent& event) {
        event_count += event.increment;
    });

    // Publish many events
    for (int i = 0; i < num_events; ++i) {
        CounterEvent event{1};
        bus.publish(event);
    }

    // Wait for all events to be processed
    auto wait_result = helpers::wait_for_condition(
        [&]() { return event_count.load() == num_events; },
        std::chrono::seconds(5)
    );

    EXPECT_TRUE(wait_result);
    EXPECT_EQ(event_count.load(), num_events);

    // Cleanup
    bus.unsubscribe(sub_id);
}

TEST_F(EventBusIntegrationTest, ThreadSafety) {
    auto& bus = get_event_bus();

    std::atomic<int> total_count{0};
    const int num_threads = 4;
    const int events_per_thread = 250;

    auto sub_id = bus.subscribe<CounterEvent>([&](const CounterEvent& event) {
        total_count += event.increment;
    });

    // Publish from multiple threads
    std::vector<std::thread> threads;
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&]() {
            for (int i = 0; i < events_per_thread; ++i) {
                CounterEvent event{1};
                bus.publish(event);
            }
        });
    }

    // Wait for all threads
    for (auto& thread : threads) {
        thread.join();
    }

    // Wait for all events to be processed
    auto wait_result = helpers::wait_for_condition(
        [&]() { return total_count.load() == num_threads * events_per_thread; },
        std::chrono::seconds(5)
    );

    EXPECT_TRUE(wait_result);
    EXPECT_EQ(total_count.load(), num_threads * events_per_thread);

    // Cleanup
    bus.unsubscribe(sub_id);
}

TEST_F(EventBusIntegrationTest, EventDataIntegrity) {
    auto& bus = get_event_bus();

    std::vector<std::string> received_messages;

    auto sub_id = bus.subscribe<TestEvent>([&](const TestEvent& event) {
        received_messages.push_back(event.message);
    });

    // Publish events with different data
    std::vector<std::string> sent_messages = {
        "first", "second", "third", "fourth", "fifth"
    };

    for (const auto& msg : sent_messages) {
        TestEvent event{msg, 0};
        bus.publish(event);
    }

    // Wait for processing
    auto wait_result = helpers::wait_for_condition(
        [&]() { return received_messages.size() == sent_messages.size(); },
        std::chrono::seconds(2)
    );

    EXPECT_TRUE(wait_result);
    EXPECT_EQ(received_messages.size(), sent_messages.size());

    // Verify all messages were received (order may vary with async processing)
    for (const auto& msg : sent_messages) {
        EXPECT_TRUE(std::find(received_messages.begin(), received_messages.end(), msg)
                   != received_messages.end());
    }

    // Cleanup
    bus.unsubscribe(sub_id);
}
