/**
 * @file stats_snapshot_test.cpp
 * @brief Unit tests for statistics snapshot and IStats interface.
 *
 * Tests the statistics infrastructure:
 * - stats_snapshot serialization to JSON
 * - stats_value variant handling
 * - IStats interface implementation in circuit_breaker
 * - Thread-safe statistics collection
 *
 * @date 2025-01-31
 */

#include <gtest/gtest.h>
#include <kcenon/common/interfaces/stats.h>
#include <kcenon/common/resilience/circuit_breaker.h>
#include <thread>
#include <vector>

using namespace kcenon::common::interfaces;
using namespace kcenon::common::resilience;

// Test stats_value variant with different types
TEST(StatsSnapshotTest, StatsValueVariantTypes)
{
    stats_value int_val = std::int64_t{42};
    stats_value double_val = 3.14159;
    stats_value string_val = std::string{"test"};
    stats_value bool_val = true;

    EXPECT_EQ(std::get<std::int64_t>(int_val), 42);
    EXPECT_DOUBLE_EQ(std::get<double>(double_val), 3.14159);
    EXPECT_EQ(std::get<std::string>(string_val), "test");
    EXPECT_TRUE(std::get<bool>(bool_val));
}

// Test stats_snapshot JSON serialization with all value types
TEST(StatsSnapshotTest, JsonSerializationAllTypes)
{
    stats_snapshot snapshot{
        .component_name = "test_component",
        .timestamp = std::chrono::system_clock::now(),
        .values = {
            {"counter", std::int64_t{100}},
            {"rate", 0.95},
            {"status", std::string{"healthy"}},
            {"enabled", true}
        }
    };

    const auto json = snapshot.to_json();

    // Verify JSON structure
    EXPECT_NE(json.find("\"component\": \"test_component\""), std::string::npos);
    EXPECT_NE(json.find("\"timestamp\":"), std::string::npos);
    EXPECT_NE(json.find("\"metrics\":"), std::string::npos);

    // Verify value serialization
    EXPECT_NE(json.find("\"counter\": 100"), std::string::npos);
    EXPECT_NE(json.find("\"rate\": 0.95"), std::string::npos);
    EXPECT_NE(json.find("\"status\": \"healthy\""), std::string::npos);
    EXPECT_NE(json.find("\"enabled\": true"), std::string::npos);
}

// Test empty stats_snapshot
TEST(StatsSnapshotTest, EmptySnapshot)
{
    stats_snapshot snapshot{
        .component_name = "empty_component",
        .timestamp = std::chrono::system_clock::now(),
        .values = {}
    };

    const auto json = snapshot.to_json();

    EXPECT_NE(json.find("\"component\": \"empty_component\""), std::string::npos);
    EXPECT_NE(json.find("\"metrics\": {"), std::string::npos);
}

// Test circuit_breaker implements IStats
TEST(CircuitBreakerStatsTest, ImplementsIStatsInterface)
{
    circuit_breaker breaker;
    IStats* stats = &breaker;

    // Verify interface methods are available
    EXPECT_EQ(stats->name(), "circuit_breaker");
    EXPECT_FALSE(stats->get_stats().empty());
    EXPECT_FALSE(stats->to_json().empty());
}

// Test circuit_breaker get_stats returns expected metrics
TEST(CircuitBreakerStatsTest, GetStatsReturnsExpectedMetrics)
{
    circuit_breaker_config config{
        .failure_threshold = 5,
        .success_threshold = 3,
        .failure_window = std::chrono::seconds(60),
        .timeout = std::chrono::seconds(30),
        .half_open_max_requests = 3
    };
    circuit_breaker breaker(config);

    const auto stats = breaker.get_stats();

    // Verify all expected metrics exist
    EXPECT_NE(stats.find("current_state"), stats.end());
    EXPECT_NE(stats.find("failure_count"), stats.end());
    EXPECT_NE(stats.find("consecutive_successes"), stats.end());
    EXPECT_NE(stats.find("half_open_requests"), stats.end());
    EXPECT_NE(stats.find("failure_threshold"), stats.end());
    EXPECT_NE(stats.find("is_open"), stats.end());

    // Verify initial values
    EXPECT_EQ(std::get<std::string>(stats.at("current_state")), "CLOSED");
    EXPECT_EQ(std::get<std::int64_t>(stats.at("failure_count")), 0);
    EXPECT_EQ(std::get<std::int64_t>(stats.at("consecutive_successes")), 0);
    EXPECT_EQ(std::get<std::int64_t>(stats.at("failure_threshold")), 5);
    EXPECT_FALSE(std::get<bool>(stats.at("is_open")));
}

// Test circuit_breaker stats reflect state changes
TEST(CircuitBreakerStatsTest, StatsReflectStateChanges)
{
    circuit_breaker_config config{
        .failure_threshold = 2,
        .success_threshold = 2,
        .failure_window = std::chrono::seconds(60),
        .timeout = std::chrono::milliseconds(100),
        .half_open_max_requests = 3
    };
    circuit_breaker breaker(config);

    // Initial CLOSED state
    auto stats = breaker.get_stats();
    EXPECT_EQ(std::get<std::string>(stats.at("current_state")), "CLOSED");
    EXPECT_FALSE(std::get<bool>(stats.at("is_open")));

    // Trip circuit to OPEN
    breaker.record_failure();
    breaker.record_failure();
    stats = breaker.get_stats();
    EXPECT_EQ(std::get<std::string>(stats.at("current_state")), "OPEN");
    EXPECT_TRUE(std::get<bool>(stats.at("is_open")));
    EXPECT_EQ(std::get<std::int64_t>(stats.at("failure_count")), 2);

    // Wait for timeout and transition to HALF_OPEN
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    EXPECT_TRUE(breaker.allow_request()); // Triggers HALF_OPEN
    stats = breaker.get_stats();
    EXPECT_EQ(std::get<std::string>(stats.at("current_state")), "HALF_OPEN");
    EXPECT_FALSE(std::get<bool>(stats.at("is_open")));

    // Record successes to transition to CLOSED
    breaker.record_success();
    breaker.record_success();
    stats = breaker.get_stats();
    EXPECT_EQ(std::get<std::string>(stats.at("current_state")), "CLOSED");
    EXPECT_EQ(std::get<std::int64_t>(stats.at("consecutive_successes")), 0); // Reset on CLOSED
}

// Test circuit_breaker to_json produces valid JSON
TEST(CircuitBreakerStatsTest, ToJsonProducesValidJson)
{
    circuit_breaker breaker;
    const auto json = breaker.to_json();

    // Basic JSON structure validation
    EXPECT_NE(json.find("\"component\": \"circuit_breaker\""), std::string::npos);
    EXPECT_NE(json.find("\"timestamp\":"), std::string::npos);
    EXPECT_NE(json.find("\"metrics\":"), std::string::npos);
    EXPECT_NE(json.find("\"current_state\":"), std::string::npos);
    EXPECT_NE(json.find("\"failure_count\":"), std::string::npos);

    // Verify it's a complete JSON object
    EXPECT_EQ(json.front(), '{');
    EXPECT_EQ(json.back(), '}');
}

// Test get_snapshot returns complete snapshot
TEST(CircuitBreakerStatsTest, GetSnapshotReturnsCompleteData)
{
    circuit_breaker breaker;
    const auto snapshot = breaker.get_snapshot();

    EXPECT_EQ(snapshot.component_name, "circuit_breaker");
    EXPECT_FALSE(snapshot.values.empty());
    EXPECT_NE(snapshot.values.find("current_state"), snapshot.values.end());
    EXPECT_NE(snapshot.values.find("failure_count"), snapshot.values.end());
}

// Test thread-safe stats collection
TEST(CircuitBreakerStatsTest, ThreadSafeStatsCollection)
{
    circuit_breaker_config config{
        .failure_threshold = 100,
        .failure_window = std::chrono::seconds(60)
    };
    circuit_breaker breaker(config);

    // Multiple threads collecting stats simultaneously
    std::vector<std::thread> threads;
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([&breaker]() {
            for (int j = 0; j < 100; ++j) {
                auto stats = breaker.get_stats();
                auto json = breaker.to_json();
                auto snapshot = breaker.get_snapshot();
                // Just verify no crashes - data races would cause undefined behavior
                EXPECT_FALSE(stats.empty());
                EXPECT_FALSE(json.empty());
                EXPECT_FALSE(snapshot.values.empty());
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }
}
