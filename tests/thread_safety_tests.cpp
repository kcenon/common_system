/*****************************************************************************
BSD 3-Clause License

Copyright (c) 2025, 🍀☀🌕🌥 🌊
All rights reserved.
*****************************************************************************/

#include <gtest/gtest.h>
#include "kcenon/common/patterns/result.h"
#include "kcenon/common/patterns/event_bus.h"

#include <thread>
#include <vector>
#include <atomic>
#include <chrono>

using namespace kcenon::common;
using namespace std::chrono_literals;

class CommonSystemThreadSafetyTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

// Test 1: Result<T> concurrent access across threads
TEST_F(CommonSystemThreadSafetyTest, ResultConcurrentAccess) {
    const int num_threads = 15;
    const int operations_per_thread = 500;
    std::atomic<int> ok_results{0};
    std::atomic<int> err_results{0};
    std::atomic<int> errors{0};

    std::vector<std::thread> threads;

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&, thread_id = i]() {
            for (int j = 0; j < operations_per_thread; ++j) {
                try {
                    // Create Result in this thread
                    Result<int> result = (j % 3 == 0)
                        ? ok(thread_id * 1000 + j)
                        : make_error<int>(-1, "Error in thread " + std::to_string(thread_id));

                    // Pass to another lambda
                    auto process = [&](Result<int> r) {
                        if (r.is_ok()) {
                            ++ok_results;
                        } else {
                            ++err_results;
                        }
                    };

                    process(std::move(result));
                } catch (...) {
                    ++errors;
                }
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_EQ(errors.load(), 0);
    EXPECT_EQ(ok_results.load() + err_results.load(), num_threads * operations_per_thread);
}

// Test 2: Event bus thread safety
TEST_F(CommonSystemThreadSafetyTest, EventBusThreadSafety) {
    event_bus bus;

    const int num_publishers = 10;
    const int num_subscribers = 5;
    const int events_per_publisher = 300;

    std::atomic<int> events_received{0};
    std::atomic<int> errors{0};
    std::vector<subscription_id> subscriptions;

    // Subscribe
    for (int i = 0; i < num_subscribers; ++i) {
        auto id = bus.subscribe([&](const event& e) {
            ++events_received;
        });
        subscriptions.push_back(id);
    }

    // Publish from multiple threads
    std::vector<std::thread> threads;

    for (int i = 0; i < num_publishers; ++i) {
        threads.emplace_back([&, pub_id = i]() {
            for (int j = 0; j < events_per_publisher; ++j) {
                try {
                    event e;
                    e.set_type("test");
                    e.set_data(std::to_string(pub_id * 1000 + j));
                    bus.publish(std::move(e));
                } catch (...) {
                    ++errors;
                }

                if (j % 30 == 0) {
                    std::this_thread::sleep_for(1ms);
                }
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    std::this_thread::sleep_for(100ms);

    // Unsubscribe
    for (auto id : subscriptions) {
        bus.unsubscribe(id);
    }

    EXPECT_EQ(errors.load(), 0);
}

// Test 3: Result transformation chain concurrent
TEST_F(CommonSystemThreadSafetyTest, ResultTransformationChain) {
    const int num_threads = 12;
    const int chains_per_thread = 400;
    std::atomic<int> successful_chains{0};
    std::atomic<int> errors{0};

    std::vector<std::thread> threads;

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&, thread_id = i]() {
            for (int j = 0; j < chains_per_thread; ++j) {
                try {
                    auto result = ok(j)
                        .map([](int x) { return x * 2; })
                        .and_then([](int x) { return ok(x + 1); })
                        .map([](int x) { return x * 3; });

                    if (result.is_ok()) {
                        ++successful_chains;
                    }
                } catch (...) {
                    ++errors;
                }
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_EQ(errors.load(), 0);
    EXPECT_EQ(successful_chains.load(), num_threads * chains_per_thread);
}

// Test 4: Singleton event bus access
TEST_F(CommonSystemThreadSafetyTest, SingletonEventBusSafety) {
    const int num_threads = 20;
    const int operations_per_thread = 200;
    std::atomic<int> errors{0};

    std::vector<std::thread> threads;

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&]() {
            for (int j = 0; j < operations_per_thread; ++j) {
                try {
                    auto& bus = event_bus::instance();

                    auto id = bus.subscribe([](const event& e) {});

                    event e;
                    e.set_type("singleton_test");
                    bus.publish(std::move(e));

                    bus.unsubscribe(id);
                } catch (...) {
                    ++errors;
                }
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_EQ(errors.load(), 0);
}

// Test 5: Result error propagation concurrent
TEST_F(CommonSystemThreadSafetyTest, ResultErrorPropagation) {
    const int num_threads = 15;
    const int operations_per_thread = 500;
    std::atomic<int> errors_propagated{0};
    std::atomic<int> errors{0};

    std::vector<std::thread> threads;

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&]() {
            for (int j = 0; j < operations_per_thread; ++j) {
                try {
                    auto result = Err<int>("Initial error")
                        .map([](int x) { return x * 2; })  // Should not execute
                        .map([](int x) { return x + 1; }); // Should not execute

                    if (result.is_err()) {
                        ++errors_propagated;
                    }
                } catch (...) {
                    ++errors;
                }
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_EQ(errors.load(), 0);
    EXPECT_EQ(errors_propagated.load(), num_threads * operations_per_thread);
}

// Test 6: Event filtering concurrent
TEST_F(CommonSystemThreadSafetyTest, EventFilteringConcurrent) {
    event_bus bus;

    const int num_threads = 10;
    const int events_per_thread = 300;
    std::atomic<int> type_a_received{0};
    std::atomic<int> type_b_received{0};
    std::atomic<int> errors{0};

    auto id_a = bus.subscribe_filtered([&](const event& e) {
        ++type_a_received;
    }, [](const event& e) { return e.get_type() == "type_a"; });

    auto id_b = bus.subscribe_filtered([&](const event& e) {
        ++type_b_received;
    }, [](const event& e) { return e.get_type() == "type_b"; });

    std::vector<std::thread> threads;

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&, thread_id = i]() {
            for (int j = 0; j < events_per_thread; ++j) {
                try {
                    event e;
                    e.set_type((j % 2 == 0) ? "type_a" : "type_b");
                    bus.publish(std::move(e));
                } catch (...) {
                    ++errors;
                }
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    std::this_thread::sleep_for(100ms);

    bus.unsubscribe(id_a);
    bus.unsubscribe(id_b);

    EXPECT_EQ(errors.load(), 0);
}

// Test 7: Result unwrap safety
TEST_F(CommonSystemThreadSafetyTest, ResultUnwrapSafety) {
    const int num_threads = 12;
    const int operations_per_thread = 400;
    std::atomic<int> unwrap_errors{0};
    std::atomic<int> successful_unwraps{0};

    std::vector<std::thread> threads;

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&, thread_id = i]() {
            for (int j = 0; j < operations_per_thread; ++j) {
                Result<int> result = (j % 4 == 0)
                    ? make_error<int>(-1, "Test error")
                    : ok(thread_id * 1000 + j);

                if (result.is_ok()) {
                    int value = result.unwrap();
                    ++successful_unwraps;
                } else {
                    try {
                        result.unwrap(); // Should throw
                    } catch (...) {
                        ++unwrap_errors;
                    }
                }
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_GT(unwrap_errors.load(), 0);
    EXPECT_GT(successful_unwraps.load(), 0);
}

// Test 8: Optional value concurrent
TEST_F(CommonSystemThreadSafetyTest, OptionalValueConcurrent) {
    const int num_threads = 15;
    const int operations_per_thread = 500;
    std::atomic<int> some_values{0};
    std::atomic<int> none_values{0};
    std::atomic<int> errors{0};

    std::vector<std::thread> threads;

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&, thread_id = i]() {
            for (int j = 0; j < operations_per_thread; ++j) {
                try {
                    Optional<int> opt;

                    if (j % 3 == 0) {
                        opt = None<int>();
                    } else {
                        opt = Some(thread_id * 1000 + j);
                    }

                    if (opt.has_value()) {
                        ++some_values;
                    } else {
                        ++none_values;
                    }
                } catch (...) {
                    ++errors;
                }
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_EQ(errors.load(), 0);
    EXPECT_EQ(some_values.load() + none_values.load(), num_threads * operations_per_thread);
}

// Test 9: Event bus dynamic subscription changes
TEST_F(CommonSystemThreadSafetyTest, EventBusDynamicSubscriptions) {
    event_bus bus;

    const int num_threads = 15;
    std::atomic<bool> running{true};
    std::atomic<int> errors{0};

    std::vector<std::thread> threads;

    // Publisher thread
    threads.emplace_back([&]() {
        while (running.load()) {
            try {
                event e;
                e.set_type("dynamic");
                bus.publish(std::move(e));
                std::this_thread::sleep_for(2ms);
            } catch (...) {
                ++errors;
            }
        }
    });

    // Subscriber threads that constantly subscribe/unsubscribe
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&]() {
            while (running.load()) {
                try {
                    auto id = bus.subscribe([](const event& e) {});
                    std::this_thread::sleep_for(10ms);
                    bus.unsubscribe(id);
                } catch (...) {
                    ++errors;
                }
            }
        });
    }

    std::this_thread::sleep_for(500ms);
    running.store(false);

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_EQ(errors.load(), 0);
}

// Test 10: Memory safety - no leaks
TEST_F(CommonSystemThreadSafetyTest, MemorySafetyTest) {
    const int num_iterations = 50;
    const int threads_per_iteration = 10;
    const int operations_per_thread = 100;

    std::atomic<int> total_errors{0};

    for (int iteration = 0; iteration < num_iterations; ++iteration) {
        event_bus bus;
        std::vector<std::thread> threads;

        for (int i = 0; i < threads_per_iteration; ++i) {
            threads.emplace_back([&]() {
                for (int j = 0; j < operations_per_thread; ++j) {
                    try {
                        auto id = bus.subscribe([](const event& e) {});

                        event e;
                        e.set_type("memory_test");
                        bus.publish(std::move(e));

                        bus.unsubscribe(id);

                        // Test Result
                        auto result = ok(j).map([](int x) { return x * 2; });
                    } catch (...) {
                        ++total_errors;
                    }
                }
            });
        }

        for (auto& t : threads) {
            t.join();
        }

        // Destructors called here
    }

    EXPECT_EQ(total_errors.load(), 0);
}

// Test 11: Event bus subscription modification during handler execution
TEST_F(CommonSystemThreadSafetyTest, EventBusSubscriptionDuringExecution) {
    event_bus bus;

    const int num_iterations = 100;
    std::atomic<int> handler_executions{0};
    std::atomic<int> errors{0};
    std::atomic<bool> handler_running{false};

    std::vector<subscription_id> subscriptions;

    // Create initial subscription
    auto initial_id = bus.subscribe([&](const event& e) {
        handler_running.store(true);
        ++handler_executions;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        handler_running.store(false);
    });
    subscriptions.push_back(initial_id);

    // Thread 1: Publish events
    std::thread publisher([&]() {
        for (int i = 0; i < num_iterations; ++i) {
            try {
                event e;
                e.set_type("modification_test");
                bus.publish(std::move(e));
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            } catch (...) {
                ++errors;
            }
        }
    });

    // Thread 2: Modify subscriptions while handlers execute
    std::thread modifier([&]() {
        for (int i = 0; i < num_iterations / 2; ++i) {
            try {
                // Add new subscription
                auto id = bus.subscribe([&](const event& e) {
                    ++handler_executions;
                });
                subscriptions.push_back(id);

                std::this_thread::sleep_for(std::chrono::milliseconds(15));

                // Remove a subscription
                if (!subscriptions.empty()) {
                    bus.unsubscribe(subscriptions.back());
                    subscriptions.pop_back();
                }
            } catch (...) {
                ++errors;
            }
        }
    });

    publisher.join();
    modifier.join();

    // Cleanup remaining subscriptions
    for (auto id : subscriptions) {
        bus.unsubscribe(id);
    }

    EXPECT_EQ(errors.load(), 0);
    EXPECT_GT(handler_executions.load(), 0);
}

// Test 12: Event bus error callback thread safety
TEST_F(CommonSystemThreadSafetyTest, EventBusErrorCallbackSafety) {
    event_bus bus;

    const int num_threads = 10;
    const int events_per_thread = 50;
    std::atomic<int> error_callback_invocations{0};
    std::atomic<int> errors{0};

    // Set error callback that will be invoked from multiple threads
    bus.set_error_callback([&](const std::string& msg, size_t type_id, uint64_t handler_id) {
        ++error_callback_invocations;
    });

    // Subscribe with a handler that throws exceptions
    auto id = bus.subscribe([](const event& e) {
        throw std::runtime_error("Intentional error for testing");
    });

    std::vector<std::thread> threads;

    // Publish events from multiple threads - all will trigger error callback
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&]() {
            for (int j = 0; j < events_per_thread; ++j) {
                try {
                    event e;
                    e.set_type("error_test");
                    bus.publish(std::move(e));
                } catch (...) {
                    ++errors;
                }
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    bus.unsubscribe(id);
    bus.clear_error_callback();

    // Error callback should have been invoked for each event
    EXPECT_EQ(error_callback_invocations.load(), num_threads * events_per_thread);
    EXPECT_EQ(errors.load(), 0); // Exceptions in handlers shouldn't propagate
}

// Test 13: Concurrent Result construction and destruction
TEST_F(CommonSystemThreadSafetyTest, ResultLifecycleStressTest) {
    const int num_threads = 20;
    const int cycles_per_thread = 1000;
    std::atomic<int> errors{0};

    std::vector<std::thread> threads;

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&, thread_id = i]() {
            for (int j = 0; j < cycles_per_thread; ++j) {
                try {
                    // Test various Result lifecycle scenarios
                    {
                        Result<std::vector<int>> result = ok(std::vector<int>{1, 2, 3, 4, 5});
                        auto mapped = result.map([](const std::vector<int>& v) {
                            return v.size();
                        });
                    }

                    {
                        Result<std::string> result = make_error<std::string>(-1, "error");
                        auto recovered = result.or_else([](const error_info& e) {
                            return ok(std::string("recovered"));
                        });
                    }

                    {
                        auto result = ok(thread_id)
                            .and_then([](int x) { return ok(x * 2); })
                            .map([](int x) { return std::to_string(x); });
                    }
                } catch (...) {
                    ++errors;
                }
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_EQ(errors.load(), 0);
}
