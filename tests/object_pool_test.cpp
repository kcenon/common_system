// BSD 3-Clause License
//
// Copyright (c) 2025, kcenon
// All rights reserved.

/**
 * @file object_pool_test.cpp
 * @brief Unit tests for ObjectPool<T>
 * @since 4.0.0
 */

#include <gtest/gtest.h>

#include <kcenon/common/utils/object_pool.h>

#include <string>
#include <thread>
#include <vector>
#include <atomic>
#include <set>

using namespace kcenon::common::utils;

// =============================================================================
// Test helper types
// =============================================================================

struct SimpleObj {
    int value;
    explicit SimpleObj(int v = 0) : value(v) {}
};

struct CountedObj {
    static std::atomic<int> ctor_count;
    static std::atomic<int> dtor_count;

    int id;

    explicit CountedObj(int i = 0) : id(i) { ctor_count++; }
    ~CountedObj() { dtor_count++; }

    static void reset_counts() {
        ctor_count = 0;
        dtor_count = 0;
    }
};

std::atomic<int> CountedObj::ctor_count{0};
std::atomic<int> CountedObj::dtor_count{0};

// =============================================================================
// Construction tests
// =============================================================================

TEST(ObjectPoolTest, DefaultConstruction) {
    ObjectPool<SimpleObj> pool;
    EXPECT_EQ(pool.available(), 0u);
}

TEST(ObjectPoolTest, CustomGrowth) {
    ObjectPool<SimpleObj> pool(16);
    EXPECT_EQ(pool.available(), 0u);
}

TEST(ObjectPoolTest, ZeroGrowthBecomesOne) {
    ObjectPool<SimpleObj> pool(0);
    // Should still be able to acquire (growth clamped to 1)
    auto obj = pool.acquire(42);
    EXPECT_NE(obj, nullptr);
    EXPECT_EQ(obj->value, 42);
}

// =============================================================================
// Acquire and release
// =============================================================================

TEST(ObjectPoolTest, AcquireConstructsObject) {
    ObjectPool<SimpleObj> pool;
    auto obj = pool.acquire(99);
    EXPECT_NE(obj, nullptr);
    EXPECT_EQ(obj->value, 99);
}

TEST(ObjectPoolTest, AcquireWithReusedFlag) {
    ObjectPool<SimpleObj> pool(1);

    bool reused = true;
    auto obj1 = pool.acquire(&reused, 1);
    EXPECT_FALSE(reused);  // First acquisition is fresh
    obj1.reset();  // Release back to pool

    reused = false;
    auto obj2 = pool.acquire(&reused, 2);
    EXPECT_TRUE(reused);  // Second should reuse storage
    EXPECT_EQ(obj2->value, 2);
}

TEST(ObjectPoolTest, ReleaseReturnsToPool) {
    ObjectPool<SimpleObj> pool(4);
    auto obj = pool.acquire(1);
    // After acquiring, pool should have (growth - 1) available
    EXPECT_EQ(pool.available(), 3u);

    obj.reset();  // Custom deleter returns to pool
    EXPECT_EQ(pool.available(), 4u);
}

TEST(ObjectPoolTest, CustomDeleterAutoRelease) {
    ObjectPool<SimpleObj> pool(2);
    {
        auto obj = pool.acquire(42);
        EXPECT_EQ(pool.available(), 1u);
    }
    // obj goes out of scope, custom deleter returns it
    EXPECT_EQ(pool.available(), 2u);
}

// =============================================================================
// Object lifecycle
// =============================================================================

TEST(ObjectPoolTest, ConstructorCalledOnAcquire) {
    CountedObj::reset_counts();
    ObjectPool<CountedObj> pool(2);

    auto obj = pool.acquire(10);
    EXPECT_EQ(CountedObj::ctor_count.load(), 1);
    EXPECT_EQ(obj->id, 10);
}

TEST(ObjectPoolTest, DestructorCalledOnRelease) {
    CountedObj::reset_counts();
    ObjectPool<CountedObj> pool(2);

    {
        auto obj = pool.acquire(20);
        EXPECT_EQ(CountedObj::ctor_count.load(), 1);
        EXPECT_EQ(CountedObj::dtor_count.load(), 0);
    }
    // After release, destructor should have been called
    EXPECT_EQ(CountedObj::dtor_count.load(), 1);
}

TEST(ObjectPoolTest, ReacquireCallsConstructorAgain) {
    CountedObj::reset_counts();
    ObjectPool<CountedObj> pool(1);

    auto obj1 = pool.acquire(1);
    obj1.reset();  // Calls destructor
    EXPECT_EQ(CountedObj::ctor_count.load(), 1);
    EXPECT_EQ(CountedObj::dtor_count.load(), 1);

    auto obj2 = pool.acquire(2);
    EXPECT_EQ(CountedObj::ctor_count.load(), 2);
    EXPECT_EQ(obj2->id, 2);
}

// =============================================================================
// Reserve and clear
// =============================================================================

TEST(ObjectPoolTest, ReserveAddsBlocks) {
    ObjectPool<SimpleObj> pool(1);
    EXPECT_EQ(pool.available(), 0u);

    pool.reserve(10);
    EXPECT_EQ(pool.available(), 10u);
}

TEST(ObjectPoolTest, ReserveZeroIsNoOp) {
    ObjectPool<SimpleObj> pool(1);
    pool.reserve(0);
    EXPECT_EQ(pool.available(), 0u);
}

TEST(ObjectPoolTest, ClearRemovesAll) {
    ObjectPool<SimpleObj> pool(1);
    pool.reserve(5);
    EXPECT_EQ(pool.available(), 5u);

    pool.clear();
    EXPECT_EQ(pool.available(), 0u);
}

// =============================================================================
// Multiple acquisitions
// =============================================================================

TEST(ObjectPoolTest, AcquireMultipleObjects) {
    ObjectPool<SimpleObj> pool(4);
    std::vector<std::unique_ptr<SimpleObj, std::function<void(SimpleObj*)>>> objects;

    for (int i = 0; i < 10; ++i) {
        objects.push_back(pool.acquire(i));
        EXPECT_EQ(objects.back()->value, i);
    }

    EXPECT_EQ(objects.size(), 10u);
}

TEST(ObjectPoolTest, UniqueAddressesForConcurrentObjects) {
    ObjectPool<SimpleObj> pool(8);
    std::set<SimpleObj*> addresses;

    std::vector<std::unique_ptr<SimpleObj, std::function<void(SimpleObj*)>>> objects;
    for (int i = 0; i < 8; ++i) {
        auto obj = pool.acquire(i);
        addresses.insert(obj.get());
        objects.push_back(std::move(obj));
    }

    // All 8 live objects should have unique addresses
    EXPECT_EQ(addresses.size(), 8u);
}

// =============================================================================
// String type
// =============================================================================

TEST(ObjectPoolTest, StringObjects) {
    ObjectPool<std::string> pool(4);
    auto s1 = pool.acquire("hello");
    auto s2 = pool.acquire("world");

    EXPECT_EQ(*s1, "hello");
    EXPECT_EQ(*s2, "world");
}

// =============================================================================
// Thread safety
// =============================================================================

TEST(ObjectPoolTest, ConcurrentAcquireRelease) {
    ObjectPool<SimpleObj> pool(8);
    pool.reserve(32);

    std::atomic<int> errors{0};
    std::vector<std::thread> threads;

    for (int t = 0; t < 8; ++t) {
        threads.emplace_back([&pool, &errors, t]() {
            for (int i = 0; i < 100; ++i) {
                auto obj = pool.acquire(t * 1000 + i);
                if (!obj || obj->value != t * 1000 + i) {
                    errors++;
                }
                // obj released automatically on next iteration
            }
        });
    }

    for (auto& th : threads) th.join();
    EXPECT_EQ(errors.load(), 0);
}

TEST(ObjectPoolTest, ConcurrentReserveAndAcquire) {
    ObjectPool<SimpleObj> pool(2);
    std::atomic<int> acquired{0};

    std::vector<std::thread> threads;

    // Threads that reserve
    for (int t = 0; t < 2; ++t) {
        threads.emplace_back([&pool]() {
            for (int i = 0; i < 10; ++i) {
                pool.reserve(4);
            }
        });
    }

    // Threads that acquire
    for (int t = 0; t < 4; ++t) {
        threads.emplace_back([&pool, &acquired]() {
            for (int i = 0; i < 50; ++i) {
                auto obj = pool.acquire(i);
                if (obj) {
                    acquired++;
                }
            }
        });
    }

    for (auto& th : threads) th.join();
    EXPECT_GT(acquired.load(), 0);
}

// =============================================================================
// Growth behavior
// =============================================================================

TEST(ObjectPoolTest, GrowthAllocatesInBatches) {
    ObjectPool<SimpleObj> pool(4);

    // First acquire triggers growth of 4
    auto obj1 = pool.acquire(1);
    EXPECT_EQ(pool.available(), 3u);

    // Acquire all remaining
    auto obj2 = pool.acquire(2);
    auto obj3 = pool.acquire(3);
    auto obj4 = pool.acquire(4);
    EXPECT_EQ(pool.available(), 0u);

    // Next acquire triggers another growth of 4
    auto obj5 = pool.acquire(5);
    EXPECT_EQ(pool.available(), 3u);
}
