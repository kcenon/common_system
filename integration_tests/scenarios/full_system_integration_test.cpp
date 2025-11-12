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
#include <kcenon/common/patterns/result.h>
#include <kcenon/common/patterns/event_bus.h>
#include <thread>
#include <vector>
#include <atomic>

using namespace integration_tests;
using namespace kcenon::common;

/**
 * Full system integration tests covering multiple components working together
 */
class FullSystemIntegrationTest : public MultiSystemFixture {
protected:
    struct SystemEvent {
        std::string component;
        std::string message;
        int priority;
    };

    struct MetricsEvent {
        std::string metric_name;
        double value;
    };
};

TEST_F(FullSystemIntegrationTest, CompleteWorkflow) {
    // Test complete workflow: Result -> EventBus -> Processing
    auto& bus = get_event_bus();

    std::atomic<int> events_processed{0};
    std::vector<std::string> processed_messages;

    // Subscribe to system events
    auto sub_id = bus.subscribe<SystemEvent>([&](const SystemEvent& event) {
        events_processed++;
        processed_messages.push_back(event.component + ": " + event.message);
    });

    // Simulate system operations
    auto operation1 = []() -> Result<std::string> {
        return Result<std::string>::ok("operation1 complete");
    };

    auto operation2 = [](const std::string& msg) -> Result<SystemEvent> {
        SystemEvent event{"component1", msg, 1};
        return Result<SystemEvent>::ok(event);
    };

    // Execute workflow
    auto result = operation1()
        .and_then(operation2)
        .map([&bus](const SystemEvent& event) {
            bus.publish(event);
            return event;
        });

    EXPECT_TRUE(result.is_ok());

    // Wait for event processing
    helpers::wait_for_condition(
        [&]() { return events_processed.load() > 0; },
        std::chrono::seconds(1)
    );

    EXPECT_EQ(events_processed.load(), 1);
    EXPECT_EQ(processed_messages.size(), 1);

    bus.unsubscribe(sub_id);
}

TEST_F(FullSystemIntegrationTest, MultiComponentCoordination) {
    // Test multiple components coordinating through events
    auto& bus = get_event_bus();

    std::atomic<int> system_events{0};
    std::atomic<int> metrics_events{0};

    auto sub1 = bus.subscribe<SystemEvent>([&](const SystemEvent&) {
        system_events++;
    });

    auto sub2 = bus.subscribe<MetricsEvent>([&](const MetricsEvent&) {
        metrics_events++;
    });

    // Simulate multi-component workflow
    std::vector<std::thread> workers;

    // Component 1: System events
    workers.emplace_back([&]() {
        for (int i = 0; i < 10; ++i) {
            SystemEvent event{"component1", "event" + std::to_string(i), i};
            bus.publish(event);
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    });

    // Component 2: Metrics events
    workers.emplace_back([&]() {
        for (int i = 0; i < 10; ++i) {
            MetricsEvent event{"metric" + std::to_string(i), static_cast<double>(i)};
            bus.publish(event);
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    });

    // Wait for workers
    for (auto& worker : workers) {
        worker.join();
    }

    // Wait for all events
    helpers::wait_for_condition(
        [&]() {
            return system_events.load() == 10 && metrics_events.load() == 10;
        },
        std::chrono::seconds(2)
    );

    EXPECT_EQ(system_events.load(), 10);
    EXPECT_EQ(metrics_events.load(), 10);

    bus.unsubscribe(sub1);
    bus.unsubscribe(sub2);
}

TEST_F(FullSystemIntegrationTest, ErrorHandlingAcrossComponents) {
    // Test error handling propagation across components
    auto& bus = get_event_bus();

    std::atomic<int> error_events{0};

    struct ErrorEvent {
        error_code error;
        std::string source;
    };

    auto sub_id = bus.subscribe<ErrorEvent>([&](const ErrorEvent&) {
        error_events++;
    });

    // Component with error handling
    auto component_operation = [](int value) -> Result<int> {
        if (value < 0) {
            return Result<int>::err(error_code{1, "negative value"});
        }
        return Result<int>::ok(value * 2);
    };

    // Process values and publish errors
    std::vector<int> test_values = {5, -1, 10, -2, 15};

    for (int value : test_values) {
        auto result = component_operation(value);

        if (result.is_err()) {
            ErrorEvent error_event{result.error(), "component_operation"};
            bus.publish(error_event);
        }
    }

    // Wait for error events
    helpers::wait_for_condition(
        [&]() { return error_events.load() == 2; },
        std::chrono::seconds(1)
    );

    EXPECT_EQ(error_events.load(), 2);

    bus.unsubscribe(sub_id);
}

TEST_F(FullSystemIntegrationTest, ConcurrentOperationsWithSharedState) {
    // Test concurrent operations with proper synchronization
    auto& bus = get_event_bus();

    std::atomic<int> total_processed{0};
    std::mutex results_mutex;
    std::vector<int> results;

    auto sub_id = bus.subscribe<SystemEvent>([&](const SystemEvent& event) {
        std::lock_guard<std::mutex> lock(results_mutex);
        results.push_back(event.priority);
        total_processed++;
    });

    // Launch concurrent workers
    const int num_workers = 4;
    const int events_per_worker = 25;
    std::vector<std::thread> workers;

    for (int w = 0; w < num_workers; ++w) {
        workers.emplace_back([&, w]() {
            for (int i = 0; i < events_per_worker; ++i) {
                SystemEvent event{
                    "worker" + std::to_string(w),
                    "event" + std::to_string(i),
                    w * events_per_worker + i
                };
                bus.publish(event);
            }
        });
    }

    // Wait for workers
    for (auto& worker : workers) {
        worker.join();
    }

    // Wait for all events
    helpers::wait_for_condition(
        [&]() { return total_processed.load() == num_workers * events_per_worker; },
        std::chrono::seconds(3)
    );

    EXPECT_EQ(total_processed.load(), num_workers * events_per_worker);

    {
        std::lock_guard<std::mutex> lock(results_mutex);
        EXPECT_EQ(results.size(), num_workers * events_per_worker);
    }

    bus.unsubscribe(sub_id);
}

TEST_F(FullSystemIntegrationTest, ResourceCleanupSequence) {
    // Test proper resource cleanup in correct order
    std::vector<std::string> cleanup_order;

    {
        auto cleanup1 = helpers::make_scoped_cleanup([&]() {
            cleanup_order.push_back("cleanup1");
        });

        {
            auto cleanup2 = helpers::make_scoped_cleanup([&]() {
                cleanup_order.push_back("cleanup2");
            });

            {
                auto cleanup3 = helpers::make_scoped_cleanup([&]() {
                    cleanup_order.push_back("cleanup3");
                });

                // All cleanups registered
                EXPECT_EQ(cleanup_order.size(), 0);
            }

            // cleanup3 should have run
            EXPECT_EQ(cleanup_order.size(), 1);
            EXPECT_EQ(cleanup_order[0], "cleanup3");
        }

        // cleanup2 should have run
        EXPECT_EQ(cleanup_order.size(), 2);
        EXPECT_EQ(cleanup_order[1], "cleanup2");
    }

    // All cleanups should have run in reverse order
    EXPECT_EQ(cleanup_order.size(), 3);
    EXPECT_EQ(cleanup_order[0], "cleanup3");
    EXPECT_EQ(cleanup_order[1], "cleanup2");
    EXPECT_EQ(cleanup_order[2], "cleanup1");
}

TEST_F(FullSystemIntegrationTest, LongRunningWorkflow) {
    // Test long-running workflow with multiple stages
    auto& bus = get_event_bus();

    struct StageEvent {
        int stage;
        std::string status;
    };

    std::atomic<int> stages_completed{0};
    std::vector<int> completed_stages;
    std::mutex stages_mutex;

    auto sub_id = bus.subscribe<StageEvent>([&](const StageEvent& event) {
        if (event.status == "complete") {
            std::lock_guard<std::mutex> lock(stages_mutex);
            completed_stages.push_back(event.stage);
            stages_completed++;
        }
    });

    // Simulate multi-stage workflow
    auto workflow = [&]() {
        const int num_stages = 5;

        for (int stage = 1; stage <= num_stages; ++stage) {
            // Simulate stage processing
            auto result = Result<int>::ok(stage)
                .map([](int s) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    return s;
                })
                .and_then([&bus, stage](int s) -> Result<int> {
                    StageEvent event{stage, "complete"};
                    bus.publish(event);
                    return Result<int>::ok(s);
                });

            EXPECT_TRUE(result.is_ok());
        }
    };

    workflow();

    // Wait for all stages
    helpers::wait_for_condition(
        [&]() { return stages_completed.load() == 5; },
        std::chrono::seconds(2)
    );

    EXPECT_EQ(stages_completed.load(), 5);

    {
        std::lock_guard<std::mutex> lock(stages_mutex);
        EXPECT_EQ(completed_stages.size(), 5);

        // Verify all stages completed
        for (int i = 1; i <= 5; ++i) {
            EXPECT_TRUE(std::find(completed_stages.begin(), completed_stages.end(), i)
                       != completed_stages.end());
        }
    }

    bus.unsubscribe(sub_id);
}

TEST_F(FullSystemIntegrationTest, SystemStartupShutdownSequence) {
    // Test complete system startup and shutdown
    auto& bus = get_event_bus();

    std::vector<std::string> lifecycle_events;
    std::mutex events_mutex;

    struct LifecycleEvent {
        std::string component;
        std::string event;
    };

    auto sub_id = bus.subscribe<LifecycleEvent>([&](const LifecycleEvent& event) {
        std::lock_guard<std::mutex> lock(events_mutex);
        lifecycle_events.push_back(event.component + ":" + event.event);
    });

    // Startup sequence
    std::vector<std::string> components = {"component1", "component2", "component3"};

    for (const auto& component : components) {
        LifecycleEvent event{component, "startup"};
        bus.publish(event);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Shutdown sequence (reverse order)
    for (auto it = components.rbegin(); it != components.rend(); ++it) {
        LifecycleEvent event{*it, "shutdown"};
        bus.publish(event);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    {
        std::lock_guard<std::mutex> lock(events_mutex);

        // Should have 6 events total (3 startups + 3 shutdowns)
        EXPECT_EQ(lifecycle_events.size(), 6);

        // Verify startup sequence
        EXPECT_EQ(lifecycle_events[0], "component1:startup");
        EXPECT_EQ(lifecycle_events[1], "component2:startup");
        EXPECT_EQ(lifecycle_events[2], "component3:startup");

        // Verify shutdown sequence (reverse)
        EXPECT_EQ(lifecycle_events[3], "component3:shutdown");
        EXPECT_EQ(lifecycle_events[4], "component2:shutdown");
        EXPECT_EQ(lifecycle_events[5], "component1:shutdown");
    }

    bus.unsubscribe(sub_id);
}
