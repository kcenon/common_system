// BSD 3-Clause License
//
// Copyright (c) 2025, kcenon
// All rights reserved.

/**
 * @file circular_buffer_test.cpp
 * @brief Unit tests for CircularBuffer<T, Capacity>
 * @since 4.0.0
 */

#include <gtest/gtest.h>

#include <kcenon/common/utils/circular_buffer.h>

#include <string>
#include <thread>
#include <vector>
#include <atomic>

using namespace kcenon::common::utils;

// =============================================================================
// Basic operations
// =============================================================================

TEST(CircularBufferTest, DefaultConstructionIsEmpty) {
    CircularBuffer<int, 8> buf;
    EXPECT_TRUE(buf.empty());
    EXPECT_FALSE(buf.full());
    EXPECT_EQ(buf.size(), 0u);
    EXPECT_EQ(buf.capacity(), 8u);
}

TEST(CircularBufferTest, PushIncreasesSize) {
    CircularBuffer<int, 4> buf;
    EXPECT_TRUE(buf.push(10));
    EXPECT_EQ(buf.size(), 1u);
    EXPECT_FALSE(buf.empty());
}

TEST(CircularBufferTest, PopReturnsValueInFifoOrder) {
    CircularBuffer<int, 4> buf;
    buf.push(1);
    buf.push(2);
    buf.push(3);

    EXPECT_EQ(buf.pop().value(), 1);
    EXPECT_EQ(buf.pop().value(), 2);
    EXPECT_EQ(buf.pop().value(), 3);
}

TEST(CircularBufferTest, PopOnEmptyReturnsNullopt) {
    CircularBuffer<int, 4> buf;
    auto result = buf.pop();
    EXPECT_FALSE(result.has_value());
}

// =============================================================================
// Capacity and full behavior
// =============================================================================

TEST(CircularBufferTest, FillToCapacity) {
    CircularBuffer<int, 3> buf;
    EXPECT_TRUE(buf.push(1));
    EXPECT_TRUE(buf.push(2));
    EXPECT_TRUE(buf.push(3));
    EXPECT_TRUE(buf.full());
    EXPECT_EQ(buf.size(), 3u);
}

TEST(CircularBufferTest, PushWhenFullWithoutOverwriteFails) {
    CircularBuffer<int, 2> buf;
    buf.push(1);
    buf.push(2);
    EXPECT_FALSE(buf.push(3, false));
    EXPECT_EQ(buf.size(), 2u);
    // Original values preserved
    EXPECT_EQ(buf.pop().value(), 1);
    EXPECT_EQ(buf.pop().value(), 2);
}

TEST(CircularBufferTest, PushWhenFullWithOverwriteSucceeds) {
    CircularBuffer<int, 2> buf;
    buf.push(1);
    buf.push(2);
    EXPECT_TRUE(buf.push(3, true));
    // Oldest element (1) should be overwritten
    EXPECT_EQ(buf.size(), 2u);
    EXPECT_EQ(buf.pop().value(), 2);
    EXPECT_EQ(buf.pop().value(), 3);
}

// =============================================================================
// Circular wraparound
// =============================================================================

TEST(CircularBufferTest, WraparoundMaintainsFifo) {
    CircularBuffer<int, 3> buf;
    // Fill
    buf.push(1);
    buf.push(2);
    buf.push(3);
    // Pop two
    EXPECT_EQ(buf.pop().value(), 1);
    EXPECT_EQ(buf.pop().value(), 2);
    // Push two more (wraps around)
    buf.push(4);
    buf.push(5);
    // Should get 3, 4, 5
    EXPECT_EQ(buf.pop().value(), 3);
    EXPECT_EQ(buf.pop().value(), 4);
    EXPECT_EQ(buf.pop().value(), 5);
    EXPECT_TRUE(buf.empty());
}

TEST(CircularBufferTest, MultipleWraparound) {
    CircularBuffer<int, 2> buf;
    for (int cycle = 0; cycle < 5; ++cycle) {
        buf.push(cycle * 2);
        buf.push(cycle * 2 + 1);
        EXPECT_EQ(buf.pop().value(), cycle * 2);
        EXPECT_EQ(buf.pop().value(), cycle * 2 + 1);
    }
    EXPECT_TRUE(buf.empty());
}

// =============================================================================
// Overwrite mode stress
// =============================================================================

TEST(CircularBufferTest, OverwriteModeMaintainsLatestN) {
    CircularBuffer<int, 3> buf;
    for (int i = 0; i < 10; ++i) {
        buf.push(i, true);
    }
    // Should contain last 3: 7, 8, 9
    EXPECT_EQ(buf.size(), 3u);
    EXPECT_EQ(buf.pop().value(), 7);
    EXPECT_EQ(buf.pop().value(), 8);
    EXPECT_EQ(buf.pop().value(), 9);
}

// =============================================================================
// Move semantics
// =============================================================================

TEST(CircularBufferTest, PushMoveOnly) {
    CircularBuffer<std::string, 4> buf;
    std::string s = "hello";
    buf.push(std::move(s));
    EXPECT_EQ(buf.pop().value(), "hello");
}

TEST(CircularBufferTest, PushCopy) {
    CircularBuffer<std::string, 4> buf;
    const std::string s = "world";
    buf.push(s);
    EXPECT_EQ(buf.pop().value(), "world");
    EXPECT_EQ(s, "world");  // original unchanged
}

// =============================================================================
// Capacity of 1
// =============================================================================

TEST(CircularBufferTest, CapacityOfOne) {
    CircularBuffer<int, 1> buf;
    EXPECT_EQ(buf.capacity(), 1u);
    buf.push(42);
    EXPECT_TRUE(buf.full());
    EXPECT_FALSE(buf.push(99, false));
    EXPECT_TRUE(buf.push(99, true));
    EXPECT_EQ(buf.pop().value(), 99);
}

// =============================================================================
// Thread safety
// =============================================================================

TEST(CircularBufferTest, ConcurrentPushPop) {
    CircularBuffer<int, 64> buf;
    std::atomic<int> push_count{0};
    std::atomic<int> pop_count{0};
    const int ops_per_thread = 500;

    std::vector<std::thread> producers;
    std::vector<std::thread> consumers;

    for (int t = 0; t < 4; ++t) {
        producers.emplace_back([&, t]() {
            for (int i = 0; i < ops_per_thread; ++i) {
                if (buf.push(t * 1000 + i)) {
                    push_count++;
                }
            }
        });
    }

    for (int t = 0; t < 4; ++t) {
        consumers.emplace_back([&]() {
            for (int i = 0; i < ops_per_thread; ++i) {
                if (buf.pop().has_value()) {
                    pop_count++;
                }
            }
        });
    }

    for (auto& th : producers) th.join();
    for (auto& th : consumers) th.join();

    // Drain remaining
    while (buf.pop().has_value()) {
        pop_count++;
    }

    EXPECT_EQ(push_count.load(), pop_count.load());
}

TEST(CircularBufferTest, ConcurrentOverwrite) {
    CircularBuffer<int, 8> buf;
    std::atomic<int> errors{0};

    std::vector<std::thread> threads;
    for (int t = 0; t < 8; ++t) {
        threads.emplace_back([&, t]() {
            for (int i = 0; i < 200; ++i) {
                buf.push(t * 1000 + i, true);
                auto val = buf.pop();
                if (val.has_value() && val.value() < 0) {
                    errors++;
                }
            }
        });
    }

    for (auto& th : threads) th.join();
    EXPECT_EQ(errors.load(), 0);
}

// =============================================================================
// Different types
// =============================================================================

TEST(CircularBufferTest, WithDoubles) {
    CircularBuffer<double, 4> buf;
    buf.push(3.14);
    buf.push(2.71);
    EXPECT_DOUBLE_EQ(buf.pop().value(), 3.14);
    EXPECT_DOUBLE_EQ(buf.pop().value(), 2.71);
}

TEST(CircularBufferTest, WithStrings) {
    CircularBuffer<std::string, 4> buf;
    buf.push("alpha");
    buf.push("beta");
    buf.push("gamma");
    EXPECT_EQ(buf.size(), 3u);
    EXPECT_EQ(buf.pop().value(), "alpha");
}
