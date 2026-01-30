/**
 * @file circuit_breaker_test.cpp
 * @brief Unit tests for circuit breaker pattern implementation.
 *
 * Tests the circuit breaker state machine and fault tolerance behavior:
 * - State transitions (CLOSED -> OPEN -> HALF_OPEN -> CLOSED)
 * - Failure threshold enforcement
 * - Success threshold for recovery
 * - Timeout-based state transitions
 * - RAII guard pattern
 * - Thread safety
 *
 * @date 2025-01-31
 */

#include <gtest/gtest.h>
#include <kcenon/common/resilience/circuit_breaker.h>
#include <thread>
#include <vector>

using namespace kcenon::common::resilience;

// Test initial state is CLOSED
TEST(CircuitBreakerTest, InitialStateIsClosed)
{
    circuit_breaker breaker;
    EXPECT_EQ(breaker.get_state(), circuit_state::CLOSED);
    EXPECT_TRUE(breaker.allow_request());
}

// Test state transitions to OPEN after failure threshold
TEST(CircuitBreakerTest, TransitionsToOpenAfterFailureThreshold)
{
    circuit_breaker_config config{
        .failure_threshold = 3,
        .success_threshold = 2,
        .failure_window = std::chrono::seconds(60),
        .timeout = std::chrono::seconds(30),
        .half_open_max_requests = 3
    };
    circuit_breaker breaker(config);

    // Record failures below threshold
    breaker.record_failure();
    breaker.record_failure();
    EXPECT_EQ(breaker.get_state(), circuit_state::CLOSED);

    // Third failure should trip circuit
    breaker.record_failure();
    EXPECT_EQ(breaker.get_state(), circuit_state::OPEN);
    EXPECT_FALSE(breaker.allow_request());
}

// Test OPEN state blocks requests
TEST(CircuitBreakerTest, OpenStateBlocksRequests)
{
    circuit_breaker_config config{
        .failure_threshold = 2,
        .timeout = std::chrono::seconds(60) // Long timeout
    };
    circuit_breaker breaker(config);

    // Trip the circuit
    breaker.record_failure();
    breaker.record_failure();
    EXPECT_EQ(breaker.get_state(), circuit_state::OPEN);

    // All requests should be blocked
    for (int i = 0; i < 10; ++i) {
        EXPECT_FALSE(breaker.allow_request());
    }
}

// Test transition to HALF_OPEN after timeout
TEST(CircuitBreakerTest, TransitionsToHalfOpenAfterTimeout)
{
    circuit_breaker_config config{
        .failure_threshold = 2,
        .timeout = std::chrono::milliseconds(100)
    };
    circuit_breaker breaker(config);

    // Trip the circuit
    breaker.record_failure();
    breaker.record_failure();
    EXPECT_EQ(breaker.get_state(), circuit_state::OPEN);

    // Wait for timeout
    std::this_thread::sleep_for(std::chrono::milliseconds(150));

    // Next request should transition to HALF_OPEN
    EXPECT_TRUE(breaker.allow_request());
    EXPECT_EQ(breaker.get_state(), circuit_state::HALF_OPEN);
}

// Test HALF_OPEN allows limited requests
TEST(CircuitBreakerTest, HalfOpenAllowsLimitedRequests)
{
    circuit_breaker_config config{
        .failure_threshold = 2,
        .timeout = std::chrono::milliseconds(100),
        .half_open_max_requests = 3
    };
    circuit_breaker breaker(config);

    // Trip and wait for timeout
    breaker.record_failure();
    breaker.record_failure();
    std::this_thread::sleep_for(std::chrono::milliseconds(150));

    // Should allow exactly half_open_max_requests
    EXPECT_TRUE(breaker.allow_request()); // 1st request
    EXPECT_TRUE(breaker.allow_request()); // 2nd request
    EXPECT_TRUE(breaker.allow_request()); // 3rd request
    EXPECT_FALSE(breaker.allow_request()); // 4th request blocked
}

// Test recovery to CLOSED after success threshold
TEST(CircuitBreakerTest, RecoverToClosedAfterSuccessThreshold)
{
    circuit_breaker_config config{
        .failure_threshold = 2,
        .success_threshold = 2,
        .timeout = std::chrono::milliseconds(100),
        .half_open_max_requests = 3
    };
    circuit_breaker breaker(config);

    // Trip and wait
    breaker.record_failure();
    breaker.record_failure();
    std::this_thread::sleep_for(std::chrono::milliseconds(150));

    // Transition to HALF_OPEN
    EXPECT_TRUE(breaker.allow_request());
    EXPECT_EQ(breaker.get_state(), circuit_state::HALF_OPEN);

    // Record successful operations
    breaker.record_success();
    EXPECT_EQ(breaker.get_state(), circuit_state::HALF_OPEN);

    breaker.record_success();
    EXPECT_EQ(breaker.get_state(), circuit_state::CLOSED);
    EXPECT_TRUE(breaker.allow_request());
}

// Test HALF_OPEN returns to OPEN on any failure
TEST(CircuitBreakerTest, HalfOpenReturnsToOpenOnFailure)
{
    circuit_breaker_config config{
        .failure_threshold = 2,
        .timeout = std::chrono::milliseconds(100)
    };
    circuit_breaker breaker(config);

    // Trip and wait
    breaker.record_failure();
    breaker.record_failure();
    std::this_thread::sleep_for(std::chrono::milliseconds(150));

    // Transition to HALF_OPEN
    EXPECT_TRUE(breaker.allow_request());
    EXPECT_EQ(breaker.get_state(), circuit_state::HALF_OPEN);

    // Any failure should reopen circuit
    breaker.record_failure();
    EXPECT_EQ(breaker.get_state(), circuit_state::OPEN);
    EXPECT_FALSE(breaker.allow_request());
}

// Test RAII guard automatically records failure
TEST(CircuitBreakerTest, GuardAutoRecordsFailure)
{
    circuit_breaker_config config{
        .failure_threshold = 2
    };
    circuit_breaker breaker(config);

    // Guard destroyed without success call
    {
        auto guard = breaker.make_guard();
        // Simulate failure - guard destructor records it
    }

    EXPECT_EQ(breaker.get_state(), circuit_state::CLOSED);

    // Second failure via guard
    {
        auto guard = breaker.make_guard();
    }

    EXPECT_EQ(breaker.get_state(), circuit_state::OPEN);
}

// Test RAII guard with explicit success
TEST(CircuitBreakerTest, GuardExplicitSuccess)
{
    circuit_breaker_config config{
        .failure_threshold = 2,
        .success_threshold = 1,
        .timeout = std::chrono::milliseconds(100)
    };
    circuit_breaker breaker(config);

    // Trip circuit
    breaker.record_failure();
    breaker.record_failure();
    std::this_thread::sleep_for(std::chrono::milliseconds(150));

    // Use guard with success
    EXPECT_TRUE(breaker.allow_request());
    {
        auto guard = breaker.make_guard();
        // Simulate successful operation
        guard.record_success();
    }

    EXPECT_EQ(breaker.get_state(), circuit_state::CLOSED);
}

// Test failure window expiration
TEST(CircuitBreakerTest, FailureWindowExpiration)
{
    circuit_breaker_config config{
        .failure_threshold = 3,
        .failure_window = std::chrono::milliseconds(200)
    };
    circuit_breaker breaker(config);

    // Record 2 failures
    breaker.record_failure();
    breaker.record_failure();
    EXPECT_EQ(breaker.get_state(), circuit_state::CLOSED);

    // Wait for failures to expire
    std::this_thread::sleep_for(std::chrono::milliseconds(250));

    // New failure should not trip circuit (old ones expired)
    breaker.record_failure();
    EXPECT_EQ(breaker.get_state(), circuit_state::CLOSED);
}

// Test custom configuration
TEST(CircuitBreakerTest, CustomConfiguration)
{
    circuit_breaker_config config{
        .failure_threshold = 10,
        .success_threshold = 5,
        .failure_window = std::chrono::seconds(120),
        .timeout = std::chrono::seconds(60),
        .half_open_max_requests = 5
    };
    circuit_breaker breaker(config);

    // Verify custom thresholds are respected
    for (int i = 0; i < 9; ++i) {
        breaker.record_failure();
    }
    EXPECT_EQ(breaker.get_state(), circuit_state::CLOSED);

    breaker.record_failure(); // 10th failure
    EXPECT_EQ(breaker.get_state(), circuit_state::OPEN);
}

// Test thread safety - concurrent access
TEST(CircuitBreakerTest, ThreadSafety)
{
    circuit_breaker_config config{
        .failure_threshold = 100
    };
    circuit_breaker breaker(config);

    std::vector<std::thread> threads;
    const int thread_count = 10;
    const int operations_per_thread = 100;

    // Launch threads performing concurrent operations
    for (int i = 0; i < thread_count; ++i) {
        threads.emplace_back([&breaker, operations_per_thread]() {
            for (int j = 0; j < operations_per_thread; ++j) {
                if (breaker.allow_request()) {
                    if (j % 2 == 0) {
                        breaker.record_success();
                    } else {
                        breaker.record_failure();
                    }
                }
            }
        });
    }

    // Wait for all threads
    for (auto& t : threads) {
        t.join();
    }

    // Circuit should still be in valid state
    auto state = breaker.get_state();
    EXPECT_TRUE(state == circuit_state::CLOSED || state == circuit_state::OPEN
        || state == circuit_state::HALF_OPEN);
}

// Test circuit_state to_string conversion
TEST(CircuitBreakerTest, StateToString)
{
    EXPECT_EQ(to_string(circuit_state::CLOSED), "CLOSED");
    EXPECT_EQ(to_string(circuit_state::OPEN), "OPEN");
    EXPECT_EQ(to_string(circuit_state::HALF_OPEN), "HALF_OPEN");
}
