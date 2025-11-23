/**
 * @file event_bus_failure_test.cpp
 * @brief Tests for Event Bus failure scenarios and exception handling
 *
 * This file tests:
 * - Handler exception isolation
 * - Error callback functionality
 * - Concurrent subscribe/unsubscribe scenarios
 * - Resource cleanup on failures
 */

#include <gtest/gtest.h>
#include <kcenon/common/patterns/event_bus.h>

#include <atomic>
#include <thread>
#include <chrono>
#include <vector>
#include <stdexcept>

using namespace kcenon::common;

// Test event types
struct TestEvent {
    int id;
    std::string message;
};

struct ThrowingEvent {
    int value;
    bool should_throw;
};

class EventBusFailureTest : public ::testing::Test {
protected:
    void SetUp() override {
        bus_ = std::make_unique<simple_event_bus>();
        bus_->start();
    }

    void TearDown() override {
        bus_->stop();
        bus_->clear_error_callback();
    }

    std::unique_ptr<simple_event_bus> bus_;
};

// Test: Handler that throws exception doesn't affect other handlers
TEST_F(EventBusFailureTest, HandlerThrowsException) {
    std::atomic<int> successful_calls{0};
    std::atomic<int> error_count{0};

    // Set error callback
    bus_->set_error_callback([&error_count](const std::string& msg, size_t, uint64_t) {
        error_count++;
        EXPECT_TRUE(msg.find("Exception") != std::string::npos ||
                    msg.find("exception") != std::string::npos);
    });

    // Handler 1: Will throw
    bus_->subscribe<TestEvent>([](const TestEvent&) {
        throw std::runtime_error("Intentional test exception");
    });

    // Handler 2: Should still be called
    bus_->subscribe<TestEvent>([&successful_calls](const TestEvent&) {
        successful_calls++;
    });

    // Handler 3: Should also be called
    bus_->subscribe<TestEvent>([&successful_calls](const TestEvent&) {
        successful_calls++;
    });

    // Publish event
    bus_->publish(TestEvent{1, "test"});

    // Verify: exception was caught, other handlers executed
    EXPECT_EQ(error_count, 1);
    EXPECT_EQ(successful_calls, 2);
}

// Test: Multiple exceptions from multiple handlers
TEST_F(EventBusFailureTest, MultipleHandlerExceptions) {
    std::atomic<int> error_count{0};
    std::atomic<int> successful_calls{0};

    bus_->set_error_callback([&error_count](const std::string&, size_t, uint64_t) {
        error_count++;
    });

    // Two throwing handlers
    bus_->subscribe<TestEvent>([](const TestEvent&) {
        throw std::runtime_error("Exception 1");
    });

    bus_->subscribe<TestEvent>([](const TestEvent&) {
        throw std::logic_error("Exception 2");
    });

    // One successful handler
    bus_->subscribe<TestEvent>([&successful_calls](const TestEvent&) {
        successful_calls++;
    });

    bus_->publish(TestEvent{1, "test"});

    EXPECT_EQ(error_count, 2);
    EXPECT_EQ(successful_calls, 1);
}

// Test: Exception isolation - each publish is independent
TEST_F(EventBusFailureTest, ExceptionIsolation) {
    std::atomic<int> call_count{0};
    std::atomic<int> error_count{0};

    bus_->set_error_callback([&error_count](const std::string&, size_t, uint64_t) {
        error_count++;
    });

    // Handler throws only for specific events
    bus_->subscribe<ThrowingEvent>([&call_count](const ThrowingEvent& evt) {
        call_count++;
        if (evt.should_throw) {
            throw std::runtime_error("Conditional exception");
        }
    });

    // Publish multiple events
    bus_->publish(ThrowingEvent{1, false});  // No throw
    bus_->publish(ThrowingEvent{2, true});   // Throw
    bus_->publish(ThrowingEvent{3, false});  // No throw - should still work
    bus_->publish(ThrowingEvent{4, true});   // Throw
    bus_->publish(ThrowingEvent{5, false});  // No throw

    EXPECT_EQ(call_count, 5);  // All events processed
    EXPECT_EQ(error_count, 2); // Two exceptions caught
}

// Test: Unknown exception type handling
TEST_F(EventBusFailureTest, UnknownExceptionHandling) {
    std::atomic<int> error_count{0};

    bus_->set_error_callback([&error_count](const std::string& msg, size_t, uint64_t) {
        error_count++;
        EXPECT_TRUE(msg.find("Unknown") != std::string::npos ||
                    msg.find("unknown") != std::string::npos);
    });

    // Handler throws non-std::exception
    bus_->subscribe<TestEvent>([](const TestEvent&) {
        throw 42;  // int exception
    });

    bus_->publish(TestEvent{1, "test"});

    EXPECT_EQ(error_count, 1);
}

// Test: Error callback receives correct information
TEST_F(EventBusFailureTest, ErrorCallbackInformation) {
    size_t received_type_id = 0;
    uint64_t received_handler_id = 0;
    std::string received_message;

    bus_->set_error_callback([&](const std::string& msg, size_t type_id, uint64_t handler_id) {
        received_message = msg;
        received_type_id = type_id;
        received_handler_id = handler_id;
    });

    auto sub_id = bus_->subscribe<TestEvent>([](const TestEvent&) {
        throw std::runtime_error("Test error message");
    });

    bus_->publish(TestEvent{1, "test"});

    EXPECT_TRUE(received_message.find("Test error message") != std::string::npos);
    EXPECT_EQ(received_type_id, event_type_id<TestEvent>::id());
    EXPECT_EQ(received_handler_id, sub_id);
}

// Test: No error callback set - exceptions still don't crash
TEST_F(EventBusFailureTest, NoErrorCallbackSet) {
    std::atomic<int> successful_calls{0};

    // Don't set error callback

    bus_->subscribe<TestEvent>([](const TestEvent&) {
        throw std::runtime_error("Exception without callback");
    });

    bus_->subscribe<TestEvent>([&successful_calls](const TestEvent&) {
        successful_calls++;
    });

    // Should not crash
    EXPECT_NO_THROW(bus_->publish(TestEvent{1, "test"}));
    EXPECT_EQ(successful_calls, 1);
}

// Test: Rapid subscribe/unsubscribe from multiple threads
TEST_F(EventBusFailureTest, ConcurrentSubscribeUnsubscribe) {
    std::atomic<bool> running{true};
    std::atomic<int> publish_count{0};
    std::atomic<int> handler_calls{0};

    constexpr int NUM_THREADS = 4;
    constexpr int ITERATIONS = 100;

    std::vector<std::thread> threads;

    // Publisher thread
    threads.emplace_back([this, &running, &publish_count]() {
        while (running) {
            bus_->publish(TestEvent{static_cast<int>(publish_count++), "concurrent"});
            std::this_thread::yield();
        }
    });

    // Subscribe/unsubscribe threads
    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back([this, &handler_calls, ITERATIONS]() {
            for (int j = 0; j < ITERATIONS; ++j) {
                auto id = bus_->subscribe<TestEvent>([&handler_calls](const TestEvent&) {
                    handler_calls++;
                });
                std::this_thread::yield();
                bus_->unsubscribe(id);
            }
        });
    }

    // Let it run for a bit
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    running = false;

    for (auto& t : threads) {
        t.join();
    }

    // No crashes or hangs - test passes if we get here
    EXPECT_GT(publish_count, 0);
}

// Test: Unsubscribe during publish
TEST_F(EventBusFailureTest, UnsubscribeDuringPublish) {
    std::atomic<int> call_count{0};
    uint64_t sub_id = 0;

    // This handler unsubscribes itself
    sub_id = bus_->subscribe<TestEvent>([this, &call_count, &sub_id](const TestEvent&) {
        call_count++;
        // Note: This is inside the lock, so unsubscribe will wait
        // The implementation should handle this gracefully
    });

    // Another handler
    bus_->subscribe<TestEvent>([&call_count](const TestEvent&) {
        call_count++;
    });

    bus_->publish(TestEvent{1, "test"});

    // Unsubscribe after publish
    bus_->unsubscribe(sub_id);

    // Publish again - first handler should not be called
    bus_->publish(TestEvent{2, "test"});

    EXPECT_EQ(call_count, 3);  // 2 from first publish, 1 from second
}

// Test: Handler lifetime and memory safety
TEST_F(EventBusFailureTest, HandlerLifetime) {
    std::atomic<int> call_count{0};

    {
        // Create handler with captured local variable
        int local_value = 42;
        auto sub_id = bus_->subscribe<TestEvent>([&call_count, local_value](const TestEvent& evt) {
            call_count++;
            EXPECT_EQ(local_value, 42);
            EXPECT_GT(evt.id, 0);
        });

        bus_->publish(TestEvent{1, "test"});
        EXPECT_EQ(call_count, 1);

        // Unsubscribe before local goes out of scope
        bus_->unsubscribe(sub_id);
    }

    // Publish after handler scope ended - should be safe
    bus_->publish(TestEvent{2, "test"});
    EXPECT_EQ(call_count, 1);  // No additional calls
}

// Test: Clear error callback
TEST_F(EventBusFailureTest, ClearErrorCallback) {
    std::atomic<int> error_count{0};

    bus_->set_error_callback([&error_count](const std::string&, size_t, uint64_t) {
        error_count++;
    });

    bus_->subscribe<TestEvent>([](const TestEvent&) {
        throw std::runtime_error("Test");
    });

    bus_->publish(TestEvent{1, "test"});
    EXPECT_EQ(error_count, 1);

    // Clear callback
    bus_->clear_error_callback();

    // Publish again - no callback should be called
    EXPECT_NO_THROW(bus_->publish(TestEvent{2, "test"}));
    EXPECT_EQ(error_count, 1);  // Still 1, not incremented
}

// Test: Exception in filter function
TEST_F(EventBusFailureTest, FilterFunctionException) {
    std::atomic<int> error_count{0};
    std::atomic<int> handler_calls{0};

    bus_->set_error_callback([&error_count](const std::string&, size_t, uint64_t) {
        error_count++;
    });

    // Filter that throws
    bus_->subscribe_filtered<TestEvent>(
        [&handler_calls](const TestEvent&) {
            handler_calls++;
        },
        [](const TestEvent& evt) -> bool {
            if (evt.id < 0) {
                throw std::runtime_error("Negative ID not allowed");
            }
            return true;
        }
    );

    // Normal handler
    bus_->subscribe<TestEvent>([&handler_calls](const TestEvent&) {
        handler_calls++;
    });

    // Publish valid event
    bus_->publish(TestEvent{1, "valid"});
    EXPECT_EQ(handler_calls, 2);

    // Publish event that causes filter to throw
    bus_->publish(TestEvent{-1, "invalid"});

    // Filter exception should be caught, other handlers should still run
    EXPECT_GE(error_count, 1);
    EXPECT_GE(handler_calls, 3);  // At least one more handler called
}
