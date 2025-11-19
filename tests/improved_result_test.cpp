/**
 * @file improved_result_test.cpp
 * @brief Additional tests for improved Result<T> implementation
 * @date 2025-11-19
 */

#include <gtest/gtest.h>
#include <kcenon/common/patterns/result.h>
#include <string>
#include <memory>

using namespace kcenon::common;

// Test that default construction is prohibited
TEST(ImprovedResultTest, DefaultConstructionDeleted) {
    // This should not compile if uncommented:
    // Result<int> r;  // Error: deleted function

    // Instead, we must use factory methods
    auto r1 = Result<int>::ok(42);
    EXPECT_TRUE(r1.is_ok());
    EXPECT_EQ(r1.unwrap(), 42);

    auto r2 = Result<int>::err(-1, "Error message");
    EXPECT_TRUE(r2.is_err());
    EXPECT_EQ(r2.error().code, -1);
}

// Test the new uninitialized() factory method
TEST(ImprovedResultTest, UninitializedFactoryMethod) {
    auto r = Result<int>::uninitialized();
    EXPECT_TRUE(r.is_err());
    EXPECT_EQ(r.error().code, -6);  // not_initialized code
    EXPECT_EQ(r.error().message, "Result not initialized");
    EXPECT_EQ(r.error().module, "common::Result");
}

// Test that Result forces explicit initialization
TEST(ImprovedResultTest, ExplicitInitializationRequired) {
    // OK case
    auto ok_result = Result<std::string>::ok("Hello");
    EXPECT_TRUE(ok_result.is_ok());
    EXPECT_FALSE(ok_result.is_err());
    EXPECT_EQ(ok_result.unwrap(), "Hello");

    // Error case
    auto err_result = Result<std::string>::err(-1, "Failed", "TestModule");
    EXPECT_FALSE(err_result.is_ok());
    EXPECT_TRUE(err_result.is_err());
    EXPECT_EQ(err_result.error().code, -1);
    EXPECT_EQ(err_result.error().module, "TestModule");
}

// Test Result with complex types
TEST(ImprovedResultTest, ComplexTypes) {
    // With unique_ptr
    auto ptr_result = Result<std::unique_ptr<int>>::ok(std::make_unique<int>(100));
    EXPECT_TRUE(ptr_result.is_ok());
    EXPECT_EQ(*ptr_result.unwrap(), 100);

    // With shared_ptr
    auto shared = std::make_shared<std::string>("Shared");
    auto shared_result = Result<std::shared_ptr<std::string>>::ok(shared);
    EXPECT_TRUE(shared_result.is_ok());
    EXPECT_EQ(*shared_result.unwrap(), "Shared");
}

// Test error chaining with the new design
TEST(ImprovedResultTest, ErrorChaining) {
    auto divide = [](int a, int b) -> Result<int> {
        if (b == 0) {
            return Result<int>::err(-1, "Division by zero", "Math");
        }
        return Result<int>::ok(a / b);
    };

    auto result1 = divide(10, 2)
        .and_then([&](int val) { return divide(val, 2); });
    EXPECT_TRUE(result1.is_ok());
    EXPECT_EQ(result1.unwrap(), 2);  // (10/2)/2 = 2

    auto result2 = divide(10, 0)
        .and_then([&](int val) { return divide(val, 2); });
    EXPECT_TRUE(result2.is_err());
    EXPECT_EQ(result2.error().message, "Division by zero");
}

// Test that copy and move still work with private constructors
TEST(ImprovedResultTest, CopyAndMoveSemantics) {
    auto r1 = Result<int>::ok(42);

    // Copy constructor
    Result<int> r2 = r1;
    EXPECT_TRUE(r2.is_ok());
    EXPECT_EQ(r2.unwrap(), 42);

    // Move constructor
    Result<int> r3 = std::move(r1);
    EXPECT_TRUE(r3.is_ok());
    EXPECT_EQ(r3.unwrap(), 42);

    // Copy assignment
    auto r4 = Result<int>::err(-1, "Error");
    r4 = r2;
    EXPECT_TRUE(r4.is_ok());
    EXPECT_EQ(r4.unwrap(), 42);

    // Move assignment
    auto r5 = Result<int>::err(-1, "Error");
    r5 = std::move(r3);
    EXPECT_TRUE(r5.is_ok());
    EXPECT_EQ(r5.unwrap(), 42);
}

// Test thread safety considerations
TEST(ImprovedResultTest, ThreadSafetyConsiderations) {
    // Result is not thread-safe for concurrent writes
    // But passing by value is safe

    auto create_result = []() -> Result<int> {
        return Result<int>::ok(42);
    };

    // Multiple threads can safely receive copies
    std::vector<std::thread> threads;
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([create_result, i]() {
            auto r = create_result();  // Each thread gets its own copy
            EXPECT_TRUE(r.is_ok());
            EXPECT_EQ(r.unwrap(), 42);
        });
    }

    for (auto& t : threads) {
        t.join();
    }
}

// Test the map operations with the new design
TEST(ImprovedResultTest, MapOperations) {
    auto r1 = Result<int>::ok(10);
    auto r2 = r1.map([](int x) { return x * 2; });
    EXPECT_TRUE(r2.is_ok());
    EXPECT_EQ(r2.unwrap(), 20);

    auto r3 = Result<int>::err(-1, "Error");
    auto r4 = r3.map([](int x) { return x * 2; });
    EXPECT_TRUE(r4.is_err());
    EXPECT_EQ(r4.error().code, -1);
}

// Test value_or and unwrap_or with improved design
TEST(ImprovedResultTest, ValueOrMethods) {
    auto ok_result = Result<int>::ok(42);
    EXPECT_EQ(ok_result.value_or(0), 42);
    EXPECT_EQ(ok_result.unwrap_or(0), 42);

    auto err_result = Result<int>::err(-1, "Error");
    EXPECT_EQ(err_result.value_or(99), 99);
    EXPECT_EQ(err_result.unwrap_or(99), 99);
}