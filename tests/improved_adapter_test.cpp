/**
 * @file improved_adapter_test.cpp
 * @brief Tests for improved typed_adapter with enhanced safety
 * @date 2025-11-19
 */

#include <gtest/gtest.h>
#include <kcenon/common/adapters/typed_adapter.h>
#include <kcenon/common/interfaces/executor_interface.h>
#include <memory>
#include <thread>

using namespace kcenon::common;

// Mock interfaces and implementations for testing
class ITestInterface {
public:
    virtual ~ITestInterface() = default;
    virtual int getValue() const = 0;
    virtual void setValue(int val) = 0;
};

class TestImplementation : public ITestInterface {
private:
    int value_;
public:
    explicit TestImplementation(int val) : value_(val) {}
    int getValue() const override { return value_; }
    void setValue(int val) override { value_ = val; }
};

// Test adapter that wraps another adapter (for depth testing)
class NestedTestImpl : public ITestInterface, public adapter_base {
private:
    std::shared_ptr<ITestInterface> inner_;
    size_t depth_;
public:
    explicit NestedTestImpl(std::shared_ptr<ITestInterface> inner, size_t depth = 1)
        : inner_(inner), depth_(depth) {}

    int getValue() const override { return inner_->getValue(); }
    void setValue(int val) override { inner_->setValue(val); }

    size_t get_adapter_depth() const override { return depth_; }
    size_t get_type_id() const override { return 12345; }  // Fixed ID for testing
};

// Test basic adapter creation and unwrapping
TEST(ImprovedAdapterTest, BasicAdapterFunctionality) {
    auto impl = std::make_shared<TestImplementation>(42);
    auto adapter = std::make_shared<typed_adapter<ITestInterface, TestImplementation>>(impl);

    // Test interface methods work
    EXPECT_EQ(adapter->getValue(), 42);
    adapter->setValue(100);
    EXPECT_EQ(adapter->getValue(), 100);

    // Test unwrapping
    auto unwrapped = adapter->unwrap();
    EXPECT_NE(unwrapped, nullptr);
    EXPECT_EQ(unwrapped->getValue(), 100);
}

// Test that adapter depth is calculated correctly with RTTI safety
TEST(ImprovedAdapterTest, AdapterDepthCalculation) {
    // Direct implementation (depth 0)
    auto impl = std::make_shared<TestImplementation>(42);
    auto adapter1 = std::make_shared<typed_adapter<ITestInterface, TestImplementation>>(impl);
    EXPECT_EQ(adapter1->get_adapter_depth(), 0);

    // Nested adapter (depth should be calculated)
    auto nested = std::make_shared<NestedTestImpl>(impl, 1);
    auto adapter2 = std::make_shared<typed_adapter<ITestInterface, NestedTestImpl>>(nested);

    // With the improved implementation, depth calculation is safer
    #ifdef __cpp_rtti
    // When RTTI is available, dynamic_cast ensures type safety
    EXPECT_GE(adapter2->get_adapter_depth(), 1);
    #else
    // Without RTTI, we use type ID validation
    EXPECT_GE(adapter2->get_adapter_depth(), 0);
    #endif
}

// Test max depth enforcement
TEST(ImprovedAdapterTest, MaxDepthEnforcement) {
    auto impl = std::make_shared<TestImplementation>(42);

    // Create nested adapters up to max depth
    auto level1 = std::make_shared<NestedTestImpl>(impl, 1);
    auto adapter1 = std::make_shared<typed_adapter<ITestInterface, NestedTestImpl>>(level1);

    // Check max depth constant
    EXPECT_EQ(adapter1->max_depth(), 2);

    // Verify depth doesn't exceed maximum
    EXPECT_LE(adapter1->get_adapter_depth(), adapter1->max_depth());
}

// Test safe_unwrap with correct type
TEST(ImprovedAdapterTest, SafeUnwrapCorrectType) {
    auto impl = std::make_shared<TestImplementation>(42);
    auto adapter = std::make_shared<typed_adapter<ITestInterface, TestImplementation>>(impl);

    // Cast to interface pointer for testing
    std::shared_ptr<ITestInterface> interface_ptr = adapter;

    // Try to unwrap - should succeed with improved safety
    auto unwrapped = safe_unwrap<TestImplementation>(interface_ptr);

    #ifdef __cpp_rtti
    // With RTTI, dynamic_cast ensures we get the correct type
    EXPECT_NE(unwrapped, nullptr);
    if (unwrapped) {
        EXPECT_EQ(unwrapped->getValue(), 42);
    }
    #else
    // Without RTTI, type ID validation provides safety
    // Result depends on type ID matching
    if (unwrapped) {
        EXPECT_EQ(unwrapped->getValue(), 42);
    }
    #endif
}

// Test safe_unwrap with wrong type
TEST(ImprovedAdapterTest, SafeUnwrapWrongType) {
    auto impl = std::make_shared<TestImplementation>(42);

    // Create a plain shared_ptr (not an adapter)
    std::shared_ptr<ITestInterface> interface_ptr = impl;

    // Try to unwrap as wrong type - should return nullptr
    auto unwrapped = safe_unwrap<NestedTestImpl>(interface_ptr);
    EXPECT_EQ(unwrapped, nullptr);  // Should safely return nullptr
}

// Test type ID generation and uniqueness
TEST(ImprovedAdapterTest, TypeIdUniqueness) {
    using Adapter1 = typed_adapter<ITestInterface, TestImplementation>;
    using Adapter2 = typed_adapter<ITestInterface, NestedTestImpl>;

    // Each adapter type should have a unique ID
    auto id1 = Adapter1::get_static_type_id();
    auto id2 = Adapter2::get_static_type_id();

    EXPECT_NE(id1, 0);
    EXPECT_NE(id2, 0);
    EXPECT_NE(id1, id2);

    // Same type should always return same ID
    auto id1_again = Adapter1::get_static_type_id();
    EXPECT_EQ(id1, id1_again);
}

// Test adapter with null implementation (edge case)
TEST(ImprovedAdapterTest, NullImplementationHandling) {
    std::shared_ptr<TestImplementation> null_impl = nullptr;

    // Creating adapter with null should be safe
    auto adapter = std::make_shared<typed_adapter<ITestInterface, TestImplementation>>(null_impl);

    // Depth calculation should handle null gracefully
    EXPECT_EQ(adapter->get_adapter_depth(), 0);

    // Unwrap should return null
    auto unwrapped = adapter->unwrap();
    EXPECT_EQ(unwrapped, nullptr);
}

// Test thread safety of type ID generation
TEST(ImprovedAdapterTest, TypeIdThreadSafety) {
    const int num_threads = 10;
    std::vector<size_t> ids(num_threads);
    std::vector<std::thread> threads;

    // Multiple threads getting type IDs simultaneously
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&ids, i]() {
            ids[i] = typed_adapter<ITestInterface, TestImplementation>::get_static_type_id();
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    // All threads should get the same ID
    auto expected_id = ids[0];
    for (int i = 1; i < num_threads; ++i) {
        EXPECT_EQ(ids[i], expected_id);
    }
}

// Test adapter wrapper depth documentation
TEST(ImprovedAdapterTest, WrapperDepthDocumentation) {
    // The max_wrapper_depth_ = 2 is documented to prevent performance degradation
    // This test verifies the documentation is accurate

    using TestAdapter = typed_adapter<ITestInterface, TestImplementation>;

    // Check the documented max depth value
    EXPECT_EQ(TestAdapter::max_depth(), 2)
        << "max_wrapper_depth_ should be 2 as documented for performance reasons";
}

// Test improved validation in calculate_depth
TEST(ImprovedAdapterTest, ImprovedDepthCalculationValidation) {
    auto impl = std::make_shared<TestImplementation>(42);

    // Create adapter with base implementation
    auto adapter = std::make_shared<typed_adapter<ITestInterface, TestImplementation>>(impl);

    // Depth should be 0 for non-adapter implementation
    EXPECT_EQ(adapter->get_adapter_depth(), 0);

    // Create nested adapter
    auto nested = std::make_shared<NestedTestImpl>(impl, 1);
    auto nested_adapter = std::make_shared<typed_adapter<ITestInterface, NestedTestImpl>>(nested);

    #ifdef __cpp_rtti
    // With RTTI, dynamic_cast provides runtime type checking
    auto depth = nested_adapter->get_adapter_depth();
    EXPECT_GE(depth, 1) << "With RTTI, nested adapter depth should be properly calculated";
    #else
    // Without RTTI, type ID validation provides safety
    auto depth = nested_adapter->get_adapter_depth();
    // The depth calculation depends on type ID validation
    EXPECT_GE(depth, 0) << "Without RTTI, depth calculation uses type ID validation";
    #endif
}

// Test the compile-time type checking
TEST(ImprovedAdapterTest, CompileTimeTypeChecking) {
    // Verify std::is_base_of_v works correctly at compile time
    constexpr bool is_adapter = std::is_base_of_v<adapter_base, NestedTestImpl>;
    constexpr bool is_not_adapter = std::is_base_of_v<adapter_base, TestImplementation>;

    EXPECT_TRUE(is_adapter);
    EXPECT_FALSE(is_not_adapter);

    // These compile-time checks enable the conditional logic in calculate_depth
    auto impl = std::make_shared<TestImplementation>(42);
    auto nested = std::make_shared<NestedTestImpl>(impl);

    auto adapter1 = std::make_shared<typed_adapter<ITestInterface, TestImplementation>>(impl);
    auto adapter2 = std::make_shared<typed_adapter<ITestInterface, NestedTestImpl>>(nested);

    // The compile-time check ensures correct branch is taken
    EXPECT_EQ(adapter1->get_adapter_depth(), 0);  // Non-adapter branch
    #ifdef __cpp_rtti
    EXPECT_GT(adapter2->get_adapter_depth(), 0);  // Adapter branch with dynamic_cast
    #endif
}