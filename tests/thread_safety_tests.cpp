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

using namespace common;
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
                    Result<int> result;

                    if (j % 3 == 0) {
                        result = Ok(thread_id * 1000 + j);
                    } else {
                        result = Err<int>("Error in thread " + std::to_string(thread_id));
                    }

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
                    auto result = Ok(j)
                        .map([](int x) { return x * 2; })
                        .and_then([](int x) { return Ok(x + 1); })
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
                Result<int> result;

                if (j % 4 == 0) {
                    result = Err<int>("Test error");
                } else {
                    result = Ok(thread_id * 1000 + j);
                }

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
                        auto result = Ok(j).map([](int x) { return x * 2; });
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
