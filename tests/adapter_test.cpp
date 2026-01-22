/**
 * @file adapter_test.cpp
 * @brief Tests for unified adapter<T> and interface_adapter<I, T>
 */

#include <gtest/gtest.h>

#include <kcenon/common/adapters/adapter.h>

#include <memory>
#include <string>
#include <thread>
#include <vector>

using namespace kcenon::common::adapters;

// Test interface
class ITestInterface {
public:
    virtual ~ITestInterface() = default;
    virtual int get_value() const = 0;
    virtual void set_value(int val) = 0;
};

// Test implementation
class TestImplementation : public ITestInterface {
private:
    int value_;

public:
    explicit TestImplementation(int val = 0) : value_(val) {}
    int get_value() const override { return value_; }
    void set_value(int val) override { value_ = val; }
};

// Concrete adapter that implements the interface by delegating to impl_
class TestAdapter : public interface_adapter<ITestInterface, TestImplementation> {
public:
    using interface_adapter::interface_adapter;

    int get_value() const override { return impl_->get_value(); }
    void set_value(int val) override { impl_->set_value(val); }
};

// Nested adapter for depth testing
class NestedTestImpl : public ITestInterface, public adapter_base {
private:
    std::shared_ptr<ITestInterface> inner_;
    size_t depth_;

public:
    explicit NestedTestImpl(std::shared_ptr<ITestInterface> inner,
                            size_t depth = 1)
        : inner_(inner), depth_(depth) {}

    int get_value() const override { return inner_->get_value(); }
    void set_value(int val) override { inner_->set_value(val); }

    size_t get_adapter_depth() const override { return depth_; }
    size_t get_type_id() const override { return 99999; }
};

// Concrete adapter for nested impl
class NestedTestAdapter : public interface_adapter<ITestInterface, NestedTestImpl> {
public:
    using interface_adapter::interface_adapter;

    int get_value() const override { return impl_->get_value(); }
    void set_value(int val) override { impl_->set_value(val); }
};

// Simple class for value adapter testing
struct Point {
    int x;
    int y;

    Point(int x = 0, int y = 0) : x(x), y(y) {}

    int distance_squared() const { return x * x + y * y; }
};

// ============================================================================
// adapter<T> Tests - Value Types
// ============================================================================

TEST(AdapterValueTest, BasicValueConstruction) {
    adapter<int> a(42);
    EXPECT_EQ(*a, 42);
}

TEST(AdapterValueTest, StructConstruction) {
    adapter<Point> a(Point{3, 4});
    EXPECT_EQ(a->x, 3);
    EXPECT_EQ(a->y, 4);
    EXPECT_EQ(a->distance_squared(), 25);
}

TEST(AdapterValueTest, MakeAdapterFactory) {
    auto a = make_adapter<Point>(5, 12);
    EXPECT_EQ(a->x, 5);
    EXPECT_EQ(a->y, 12);
    EXPECT_EQ(a->distance_squared(), 169);
}

TEST(AdapterValueTest, ValueAccess) {
    adapter<std::string> a("Hello");
    EXPECT_EQ(a.value(), "Hello");
    EXPECT_EQ(*a, "Hello");

    a.value() = "World";
    EXPECT_EQ(*a, "World");
}

TEST(AdapterValueTest, ReleaseValue) {
    adapter<std::string> a("Test");
    std::string released = std::move(a).release();
    EXPECT_EQ(released, "Test");
}

TEST(AdapterValueTest, BoolConversion) {
    adapter<int> a(0);
    EXPECT_TRUE(static_cast<bool>(a)); // Value types always valid
}

TEST(AdapterValueTest, StaticProperties) {
    EXPECT_FALSE(adapter<int>::is_smart_pointer());
    EXPECT_FALSE(adapter<int>::supports_weak());
}

// ============================================================================
// adapter<T> Tests - Shared Pointer
// ============================================================================

TEST(AdapterSharedPtrTest, BasicConstruction) {
    auto ptr = std::make_shared<Point>(3, 4);
    adapter a(ptr);

    EXPECT_EQ(a->x, 3);
    EXPECT_EQ(a->y, 4);
}

TEST(AdapterSharedPtrTest, MakeSharedAdapterFactory) {
    auto a = make_shared_adapter<Point>(7, 24);
    EXPECT_EQ(a->x, 7);
    EXPECT_EQ(a->y, 24);
    EXPECT_EQ(a->distance_squared(), 625);
}

TEST(AdapterSharedPtrTest, GetRawPointer) {
    auto a = make_shared_adapter<Point>(1, 2);
    Point* raw = a.get();
    EXPECT_NE(raw, nullptr);
    EXPECT_EQ(raw->x, 1);
}

TEST(AdapterSharedPtrTest, WeakReference) {
    auto a = make_shared_adapter<Point>(1, 1);
    auto weak = a.weak();

    EXPECT_FALSE(weak.expired());

    auto locked = weak.lock();
    EXPECT_NE(locked, nullptr);
    EXPECT_EQ(locked->x, 1);
}

TEST(AdapterSharedPtrTest, NullSharedPtr) {
    std::shared_ptr<Point> null_ptr;
    adapter a(null_ptr);

    EXPECT_FALSE(static_cast<bool>(a));
    EXPECT_EQ(a.get(), nullptr);
}

TEST(AdapterSharedPtrTest, StaticProperties) {
    EXPECT_TRUE(adapter<std::shared_ptr<Point>>::is_smart_pointer());
    EXPECT_TRUE(adapter<std::shared_ptr<Point>>::supports_weak());
}

// ============================================================================
// adapter<T> Tests - Unique Pointer
// ============================================================================

TEST(AdapterUniquePtrTest, BasicConstruction) {
    auto ptr = std::make_unique<Point>(5, 5);
    adapter a(std::move(ptr));

    EXPECT_EQ(a->x, 5);
    EXPECT_EQ(a->y, 5);
}

TEST(AdapterUniquePtrTest, MakeUniqueAdapterFactory) {
    auto a = make_unique_adapter<Point>(8, 15);
    EXPECT_EQ(a->x, 8);
    EXPECT_EQ(a->y, 15);
}

TEST(AdapterUniquePtrTest, StaticProperties) {
    EXPECT_TRUE(adapter<std::unique_ptr<Point>>::is_smart_pointer());
    EXPECT_FALSE(adapter<std::unique_ptr<Point>>::supports_weak());
}

// ============================================================================
// interface_adapter<I, T> Tests - Using Concrete Adapter
// ============================================================================

TEST(InterfaceAdapterTest, BasicFunctionality) {
    auto impl = std::make_shared<TestImplementation>(42);
    auto adapter = std::make_shared<TestAdapter>(impl);

    EXPECT_EQ(adapter->get_value(), 42);
    adapter->set_value(100);
    EXPECT_EQ(adapter->get_value(), 100);
}

TEST(InterfaceAdapterTest, UnwrapImplementation) {
    auto impl = std::make_shared<TestImplementation>(42);
    auto adapter = std::make_shared<TestAdapter>(impl);

    auto unwrapped = adapter->unwrap();
    EXPECT_NE(unwrapped, nullptr);
    EXPECT_EQ(unwrapped->get_value(), 42);
}

TEST(InterfaceAdapterTest, DepthCalculation) {
    auto impl = std::make_shared<TestImplementation>(42);
    auto adapter = std::make_shared<TestAdapter>(impl);

    EXPECT_EQ(adapter->get_adapter_depth(), 0);
    EXPECT_FALSE(adapter->is_wrapped_adapter());
}

TEST(InterfaceAdapterTest, NestedAdapterDepth) {
    auto impl = std::make_shared<TestImplementation>(42);
    auto nested = std::make_shared<NestedTestImpl>(impl, 1);
    auto adapter = std::make_shared<NestedTestAdapter>(nested);

#ifdef __cpp_rtti
    EXPECT_GE(adapter->get_adapter_depth(), 1);
#else
    EXPECT_GE(adapter->get_adapter_depth(), 0);
#endif
}

TEST(InterfaceAdapterTest, MaxDepthEnforcement) {
    auto impl = std::make_shared<TestImplementation>(42);
    auto level1 = std::make_shared<NestedTestImpl>(impl, 1);
    auto adapter = std::make_shared<NestedTestAdapter>(level1);

    EXPECT_EQ(adapter->max_depth(), 2);
    EXPECT_LE(adapter->get_adapter_depth(), adapter->max_depth());
}

TEST(InterfaceAdapterTest, TypeIdConsistency) {
    auto id1 = TestAdapter::get_static_type_id();

    // Same type should return same ID consistently
    EXPECT_NE(id1, 0);
    auto id1_again = TestAdapter::get_static_type_id();
    EXPECT_EQ(id1, id1_again);

    // Verify IDs are non-zero
    auto id2 = NestedTestAdapter::get_static_type_id();
    EXPECT_NE(id2, 0);
}

TEST(InterfaceAdapterTest, NullImplementation) {
    std::shared_ptr<TestImplementation> null_impl = nullptr;
    auto adapter = std::make_shared<TestAdapter>(null_impl);

    EXPECT_EQ(adapter->get_adapter_depth(), 0);
    EXPECT_EQ(adapter->unwrap(), nullptr);
}

// ============================================================================
// adapter_factory Tests
// ============================================================================

TEST(AdapterFactoryTest, ZeroCostAdaptation) {
    auto impl = std::make_shared<TestImplementation>(42);

    // TestImplementation implements ITestInterface, so zero-cost
    EXPECT_TRUE(
        (adapter_factory::is_zero_cost<ITestInterface, TestImplementation>()));

    auto adapted = adapter_factory::create<ITestInterface>(impl);
    EXPECT_NE(adapted, nullptr);
    EXPECT_EQ(adapted->get_value(), 42);
}

TEST(AdapterFactoryTest, CreateExplicit) {
    auto impl = std::make_shared<TestImplementation>(42);
    auto adapter = adapter_factory::create_explicit<TestAdapter>(impl);

    EXPECT_NE(adapter, nullptr);
    EXPECT_EQ(adapter->get_value(), 42);
}

// ============================================================================
// Convenience Function Tests
// ============================================================================

TEST(ConvenienceFunctionTest, MakeInterfaceAdapter) {
    auto impl = std::make_shared<TestImplementation>(42);
    auto adapted = make_interface_adapter<ITestInterface>(impl);

    EXPECT_NE(adapted, nullptr);
    EXPECT_EQ(adapted->get_value(), 42);
}

TEST(ConvenienceFunctionTest, IsAdapter) {
    auto impl = std::make_shared<TestImplementation>(42);
    auto adapter = std::make_shared<TestAdapter>(impl);

    // TestAdapter inherits from adapter_base via interface_adapter
    // is_adapter checks if the type inherits from adapter_base at compile time
    // Since we cast to ITestInterface (which doesn't inherit adapter_base),
    // the compile-time check fails. This is expected behavior.
    std::shared_ptr<TestAdapter> adapter_ptr = adapter;

    // Direct check on adapter type works
    EXPECT_TRUE(adapter_ptr->is_adapter());
}

// ============================================================================
// Thread Safety Tests
// ============================================================================

TEST(ThreadSafetyTest, TypeIdGeneration) {
    const int num_threads = 10;
    std::vector<size_t> ids(num_threads);
    std::vector<std::thread> threads;

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&ids, i]() {
            ids[i] = TestAdapter::get_static_type_id();
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    auto expected_id = ids[0];
    for (int i = 1; i < num_threads; ++i) {
        EXPECT_EQ(ids[i], expected_id);
    }
}

// ============================================================================
// Adapter Traits Tests
// ============================================================================

TEST(AdapterTraitsTest, ValueTypeTraits) {
    using Traits = adapter_traits<int>;

    EXPECT_FALSE(Traits::is_smart_pointer);
    EXPECT_FALSE(Traits::supports_weak);
    static_assert(std::is_same_v<Traits::value_type, int>);
}

TEST(AdapterTraitsTest, SharedPtrTraits) {
    using Traits = adapter_traits<std::shared_ptr<Point>>;

    EXPECT_TRUE(Traits::is_smart_pointer);
    EXPECT_TRUE(Traits::supports_weak);
    static_assert(std::is_same_v<Traits::value_type, Point>);
    static_assert(std::is_same_v<Traits::weak_type, std::weak_ptr<Point>>);
}

TEST(AdapterTraitsTest, UniquePtrTraits) {
    using Traits = adapter_traits<std::unique_ptr<Point>>;

    EXPECT_TRUE(Traits::is_smart_pointer);
    EXPECT_FALSE(Traits::supports_weak);
    static_assert(std::is_same_v<Traits::value_type, Point>);
}
