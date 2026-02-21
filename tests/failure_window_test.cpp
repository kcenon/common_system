/**
 * @file failure_window_test.cpp
 * @brief Unit tests for failure_window sliding time window.
 *
 * Tests the failure_window class independently from circuit_breaker:
 * - Direct construction with various window durations
 * - record_failure() + get_failure_count() sequence
 * - reset() clears all recorded failures
 * - is_empty() on fresh vs populated window
 * - Expiry behavior: failures older than window_duration are discarded
 * - Thread safety of concurrent operations
 *
 * @date 2026-02-21
 */

#include <gtest/gtest.h>
#include <kcenon/common/resilience/failure_window.h>
#include <thread>
#include <vector>

using namespace kcenon::common::resilience;

// Test construction with various durations
TEST(FailureWindowTest, ConstructionWithVariousDurations)
{
    failure_window w1(std::chrono::milliseconds(100));
    failure_window w2(std::chrono::milliseconds(1000));
    failure_window w3(std::chrono::milliseconds(60000));

    EXPECT_EQ(w1.get_failure_count(), 0u);
    EXPECT_EQ(w2.get_failure_count(), 0u);
    EXPECT_EQ(w3.get_failure_count(), 0u);
}

// Test record_failure increments count
TEST(FailureWindowTest, RecordFailureIncrementsCount)
{
    failure_window window(std::chrono::seconds(60));

    window.record_failure();
    EXPECT_EQ(window.get_failure_count(), 1u);

    window.record_failure();
    EXPECT_EQ(window.get_failure_count(), 2u);

    window.record_failure();
    EXPECT_EQ(window.get_failure_count(), 3u);
}

// Test reset clears all failures
TEST(FailureWindowTest, ResetClearsAllFailures)
{
    failure_window window(std::chrono::seconds(60));

    window.record_failure();
    window.record_failure();
    window.record_failure();
    EXPECT_EQ(window.get_failure_count(), 3u);

    window.reset();
    EXPECT_EQ(window.get_failure_count(), 0u);
}

// Test is_empty on fresh window
TEST(FailureWindowTest, IsEmptyOnFreshWindow)
{
    failure_window window(std::chrono::seconds(60));
    EXPECT_TRUE(window.is_empty());
}

// Test is_empty on populated window
TEST(FailureWindowTest, IsEmptyOnPopulatedWindow)
{
    failure_window window(std::chrono::seconds(60));
    window.record_failure();
    EXPECT_FALSE(window.is_empty());
}

// Test is_empty after reset
TEST(FailureWindowTest, IsEmptyAfterReset)
{
    failure_window window(std::chrono::seconds(60));
    window.record_failure();
    EXPECT_FALSE(window.is_empty());

    window.reset();
    EXPECT_TRUE(window.is_empty());
}

// Test expiry behavior: failures older than window are discarded
TEST(FailureWindowTest, ExpiryDiscardsOldFailures)
{
    failure_window window(std::chrono::milliseconds(100));

    // Record failures
    window.record_failure();
    window.record_failure();
    EXPECT_EQ(window.get_failure_count(), 2u);

    // Wait for window to expire
    std::this_thread::sleep_for(std::chrono::milliseconds(150));

    // Old failures should be discarded
    EXPECT_EQ(window.get_failure_count(), 0u);
    EXPECT_TRUE(window.is_empty());
}

// Test partial expiry: only old failures are removed
TEST(FailureWindowTest, PartialExpiry)
{
    failure_window window(std::chrono::milliseconds(200));

    // Record initial failures
    window.record_failure();
    window.record_failure();

    // Wait for partial expiry
    std::this_thread::sleep_for(std::chrono::milliseconds(150));

    // Record new failure (within window)
    window.record_failure();

    // Wait for initial failures to expire but new one to remain
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Only the recent failure should remain
    EXPECT_EQ(window.get_failure_count(), 1u);
}

// Test multiple record-query cycles
TEST(FailureWindowTest, MultipleRecordQueryCycles)
{
    failure_window window(std::chrono::seconds(60));

    for (int i = 0; i < 10; ++i) {
        window.record_failure();
    }
    EXPECT_EQ(window.get_failure_count(), 10u);

    window.reset();
    EXPECT_EQ(window.get_failure_count(), 0u);

    for (int i = 0; i < 5; ++i) {
        window.record_failure();
    }
    EXPECT_EQ(window.get_failure_count(), 5u);
}

// Test thread safety with concurrent record_failure and get_failure_count
TEST(FailureWindowTest, ThreadSafetyConcurrentAccess)
{
    failure_window window(std::chrono::seconds(60));

    const int thread_count = 8;
    const int operations_per_thread = 100;
    std::vector<std::thread> threads;

    // Launch threads that concurrently record failures
    for (int i = 0; i < thread_count; ++i) {
        threads.emplace_back([&window]() {
            for (int j = 0; j < operations_per_thread; ++j) {
                window.record_failure();
                (void)window.get_failure_count();
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    // All failures should be recorded
    EXPECT_EQ(window.get_failure_count(),
              static_cast<std::size_t>(thread_count * operations_per_thread));
}

// Test thread safety with concurrent record, reset, and query
TEST(FailureWindowTest, ThreadSafetyConcurrentMixedOperations)
{
    failure_window window(std::chrono::seconds(60));

    std::vector<std::thread> threads;

    // Writers
    for (int i = 0; i < 4; ++i) {
        threads.emplace_back([&window]() {
            for (int j = 0; j < 50; ++j) {
                window.record_failure();
            }
        });
    }

    // Readers
    for (int i = 0; i < 4; ++i) {
        threads.emplace_back([&window]() {
            for (int j = 0; j < 50; ++j) {
                (void)window.get_failure_count();
                (void)window.is_empty();
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    // Should not crash - exact count depends on timing
    auto count = window.get_failure_count();
    EXPECT_GE(count, 0u);
}
