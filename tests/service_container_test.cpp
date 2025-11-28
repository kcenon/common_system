// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file service_container_test.cpp
 * @brief Unit tests for service_container implementation (TICKET-102)
 */

#include <gtest/gtest.h>
#include <kcenon/common/di/service_container.h>

#include <thread>
#include <vector>
#include <atomic>

using namespace kcenon::common;
using namespace kcenon::common::di;

// Test interfaces
class ITestService {
public:
    virtual ~ITestService() = default;
    virtual int get_value() const = 0;
};

class ITestDependency {
public:
    virtual ~ITestDependency() = default;
    virtual std::string get_name() const = 0;
};

// Test implementations
class TestServiceImpl : public ITestService {
public:
    TestServiceImpl() : value_(42) {}
    explicit TestServiceImpl(int value) : value_(value) {}
    int get_value() const override { return value_; }
private:
    int value_;
};

class TestDependencyImpl : public ITestDependency {
public:
    std::string get_name() const override { return "TestDependency"; }
};

class ServiceWithDependency : public ITestService {
public:
    explicit ServiceWithDependency(std::shared_ptr<ITestDependency> dep)
        : dependency_(std::move(dep)) {}

    int get_value() const override { return 100; }
    const ITestDependency& dependency() const { return *dependency_; }
private:
    std::shared_ptr<ITestDependency> dependency_;
};

// Counter for tracking instantiations
static std::atomic<int> g_instantiation_count{0};

class CountingService : public ITestService {
public:
    CountingService() { g_instantiation_count++; }
    int get_value() const override { return g_instantiation_count.load(); }
};

// ============================================================================
// Test Fixtures
// ============================================================================

class ServiceContainerTest : public ::testing::Test {
protected:
    void SetUp() override {
        container_ = std::make_unique<service_container>();
        g_instantiation_count = 0;
    }

    void TearDown() override {
        container_.reset();
    }

    std::unique_ptr<service_container> container_;
};

// ============================================================================
// Registration Tests
// ============================================================================

TEST_F(ServiceContainerTest, RegisterType_Singleton) {
    auto result = container_->register_type<ITestService, TestServiceImpl>(
        service_lifetime::singleton);

    EXPECT_TRUE(result.is_ok());
    EXPECT_TRUE(container_->is_registered<ITestService>());
}

TEST_F(ServiceContainerTest, RegisterType_Transient) {
    auto result = container_->register_type<ITestService, TestServiceImpl>(
        service_lifetime::transient);

    EXPECT_TRUE(result.is_ok());
    EXPECT_TRUE(container_->is_registered<ITestService>());
}

TEST_F(ServiceContainerTest, RegisterType_Scoped) {
    auto result = container_->register_type<ITestService, TestServiceImpl>(
        service_lifetime::scoped);

    EXPECT_TRUE(result.is_ok());
    EXPECT_TRUE(container_->is_registered<ITestService>());
}

TEST_F(ServiceContainerTest, RegisterFactory) {
    auto result = container_->register_factory<ITestService>(
        [](IServiceContainer&) {
            return std::make_shared<TestServiceImpl>(999);
        },
        service_lifetime::singleton
    );

    EXPECT_TRUE(result.is_ok());

    auto resolved = container_->resolve<ITestService>();
    EXPECT_TRUE(resolved.is_ok());
    EXPECT_EQ(resolved.value()->get_value(), 999);
}

TEST_F(ServiceContainerTest, RegisterSimpleFactory) {
    auto result = container_->register_simple_factory<ITestService>(
        []() {
            return std::make_shared<TestServiceImpl>(123);
        },
        service_lifetime::transient
    );

    EXPECT_TRUE(result.is_ok());

    auto resolved = container_->resolve<ITestService>();
    EXPECT_TRUE(resolved.is_ok());
    EXPECT_EQ(resolved.value()->get_value(), 123);
}

TEST_F(ServiceContainerTest, RegisterInstance) {
    auto instance = std::make_shared<TestServiceImpl>(777);
    auto result = container_->register_instance<ITestService>(instance);

    EXPECT_TRUE(result.is_ok());

    auto resolved = container_->resolve<ITestService>();
    EXPECT_TRUE(resolved.is_ok());
    EXPECT_EQ(resolved.value()->get_value(), 777);
    EXPECT_EQ(resolved.value().get(), instance.get());
}

TEST_F(ServiceContainerTest, RegisterDuplicate_Fails) {
    container_->register_type<ITestService, TestServiceImpl>();

    auto result = container_->register_type<ITestService, TestServiceImpl>();

    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error().code, di_error_codes::already_registered);
}

TEST_F(ServiceContainerTest, RegisterNullInstance_Fails) {
    std::shared_ptr<ITestService> null_instance;
    auto result = container_->register_instance<ITestService>(null_instance);

    EXPECT_TRUE(result.is_err());
}

// ============================================================================
// Resolution Tests
// ============================================================================

TEST_F(ServiceContainerTest, Resolve_Singleton_ReturnsSameInstance) {
    container_->register_type<ITestService, CountingService>(
        service_lifetime::singleton);

    auto result1 = container_->resolve<ITestService>();
    auto result2 = container_->resolve<ITestService>();

    EXPECT_TRUE(result1.is_ok());
    EXPECT_TRUE(result2.is_ok());
    EXPECT_EQ(result1.value().get(), result2.value().get());
    EXPECT_EQ(g_instantiation_count, 1);
}

TEST_F(ServiceContainerTest, Resolve_Transient_ReturnsNewInstance) {
    container_->register_type<ITestService, CountingService>(
        service_lifetime::transient);

    auto result1 = container_->resolve<ITestService>();
    auto result2 = container_->resolve<ITestService>();

    EXPECT_TRUE(result1.is_ok());
    EXPECT_TRUE(result2.is_ok());
    EXPECT_NE(result1.value().get(), result2.value().get());
    EXPECT_EQ(g_instantiation_count, 2);
}

TEST_F(ServiceContainerTest, Resolve_Scoped_FromRootContainer_Fails) {
    container_->register_type<ITestService, TestServiceImpl>(
        service_lifetime::scoped);

    auto result = container_->resolve<ITestService>();

    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error().code, di_error_codes::scoped_from_root);
}

TEST_F(ServiceContainerTest, Resolve_NotRegistered_Fails) {
    auto result = container_->resolve<ITestService>();

    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error().code, di_error_codes::service_not_registered);
}

TEST_F(ServiceContainerTest, ResolveOrNull_ReturnsNullWhenNotRegistered) {
    auto result = container_->resolve_or_null<ITestService>();

    EXPECT_EQ(result, nullptr);
}

TEST_F(ServiceContainerTest, ResolveOrNull_ReturnsInstanceWhenRegistered) {
    container_->register_type<ITestService, TestServiceImpl>();

    auto result = container_->resolve_or_null<ITestService>();

    EXPECT_NE(result, nullptr);
    EXPECT_EQ(result->get_value(), 42);
}

// ============================================================================
// Dependency Resolution Tests
// ============================================================================

TEST_F(ServiceContainerTest, ResolveDependency_FromFactory) {
    container_->register_type<ITestDependency, TestDependencyImpl>();

    container_->register_factory<ITestService>(
        [](IServiceContainer& c) {
            auto dep = c.resolve<ITestDependency>().value();
            return std::make_shared<ServiceWithDependency>(dep);
        },
        service_lifetime::singleton
    );

    auto result = container_->resolve<ITestService>();

    EXPECT_TRUE(result.is_ok());
    auto* service = dynamic_cast<ServiceWithDependency*>(result.value().get());
    EXPECT_NE(service, nullptr);
    EXPECT_EQ(service->dependency().get_name(), "TestDependency");
}

// ============================================================================
// Circular Dependency Tests
// ============================================================================

class ICircularA {
public:
    virtual ~ICircularA() = default;
};

class ICircularB {
public:
    virtual ~ICircularB() = default;
};

class CircularAImpl : public ICircularA {
public:
    explicit CircularAImpl(std::shared_ptr<ICircularB>) {}
};

class CircularBImpl : public ICircularB {
public:
    explicit CircularBImpl(std::shared_ptr<ICircularA>) {}
};

TEST_F(ServiceContainerTest, CircularDependency_Detected) {
    // A depends on B, B depends on A - create actual circular dependency
    container_->register_factory<ICircularA>(
        [](IServiceContainer& c) -> std::shared_ptr<ICircularA> {
            auto b_result = c.resolve<ICircularB>();
            if (b_result.is_err()) {
                // Propagate the error by throwing
                throw std::runtime_error(b_result.error().message);
            }
            return std::make_shared<CircularAImpl>(b_result.value());
        },
        service_lifetime::singleton
    );

    container_->register_factory<ICircularB>(
        [](IServiceContainer& c) -> std::shared_ptr<ICircularB> {
            auto a_result = c.resolve<ICircularA>();
            if (a_result.is_err()) {
                // Propagate the error by throwing
                throw std::runtime_error(a_result.error().message);
            }
            return std::make_shared<CircularBImpl>(a_result.value());
        },
        service_lifetime::singleton
    );

    auto result = container_->resolve<ICircularA>();

    EXPECT_TRUE(result.is_err());
    // The error could be either circular_dependency or factory_error
    // (factory_error wraps the circular_dependency when it throws)
    EXPECT_TRUE(result.error().code == di_error_codes::circular_dependency ||
                result.error().code == di_error_codes::factory_error);
}

// ============================================================================
// Scope Tests
// ============================================================================

TEST_F(ServiceContainerTest, Scope_ScopedService_ReturnsSameInstanceInScope) {
    container_->register_type<ITestService, CountingService>(
        service_lifetime::scoped);

    auto scope = container_->create_scope();

    auto result1 = scope->resolve<ITestService>();
    auto result2 = scope->resolve<ITestService>();

    EXPECT_TRUE(result1.is_ok());
    EXPECT_TRUE(result2.is_ok());
    EXPECT_EQ(result1.value().get(), result2.value().get());
    EXPECT_EQ(g_instantiation_count, 1);
}

TEST_F(ServiceContainerTest, Scope_DifferentScopes_ReturnDifferentInstances) {
    container_->register_type<ITestService, CountingService>(
        service_lifetime::scoped);

    auto scope1 = container_->create_scope();
    auto scope2 = container_->create_scope();

    auto result1 = scope1->resolve<ITestService>();
    auto result2 = scope2->resolve<ITestService>();

    EXPECT_TRUE(result1.is_ok());
    EXPECT_TRUE(result2.is_ok());
    EXPECT_NE(result1.value().get(), result2.value().get());
    EXPECT_EQ(g_instantiation_count, 2);
}

TEST_F(ServiceContainerTest, Scope_SingletonService_SharedAcrossScopes) {
    container_->register_type<ITestService, CountingService>(
        service_lifetime::singleton);

    auto scope1 = container_->create_scope();
    auto scope2 = container_->create_scope();

    auto result1 = scope1->resolve<ITestService>();
    auto result2 = scope2->resolve<ITestService>();

    EXPECT_TRUE(result1.is_ok());
    EXPECT_TRUE(result2.is_ok());
    EXPECT_EQ(result1.value().get(), result2.value().get());
    EXPECT_EQ(g_instantiation_count, 1);
}

TEST_F(ServiceContainerTest, Scope_TransientService_NewInstanceEachTime) {
    container_->register_type<ITestService, CountingService>(
        service_lifetime::transient);

    auto scope = container_->create_scope();

    auto result1 = scope->resolve<ITestService>();
    auto result2 = scope->resolve<ITestService>();

    EXPECT_TRUE(result1.is_ok());
    EXPECT_TRUE(result2.is_ok());
    EXPECT_NE(result1.value().get(), result2.value().get());
    EXPECT_EQ(g_instantiation_count, 2);
}

TEST_F(ServiceContainerTest, Scope_ParentAccess) {
    auto scope = container_->create_scope();

    EXPECT_EQ(&scope->parent(), container_.get());
}

TEST_F(ServiceContainerTest, Scope_NestedScope) {
    container_->register_type<ITestService, CountingService>(
        service_lifetime::scoped);

    auto scope1 = container_->create_scope();
    auto scope2 = scope1->create_scope();

    auto result1 = scope1->resolve<ITestService>();
    auto result2 = scope2->resolve<ITestService>();

    EXPECT_TRUE(result1.is_ok());
    EXPECT_TRUE(result2.is_ok());
    // Nested scopes share the same root parent, so different scoped instances
    EXPECT_NE(result1.value().get(), result2.value().get());
}

// ============================================================================
// Introspection Tests
// ============================================================================

TEST_F(ServiceContainerTest, RegisteredServices_ListsAllServices) {
    container_->register_type<ITestService, TestServiceImpl>(
        service_lifetime::singleton);
    container_->register_type<ITestDependency, TestDependencyImpl>(
        service_lifetime::transient);

    auto services = container_->registered_services();

    EXPECT_EQ(services.size(), 2u);
}

TEST_F(ServiceContainerTest, Unregister_RemovesService) {
    container_->register_type<ITestService, TestServiceImpl>();

    EXPECT_TRUE(container_->is_registered<ITestService>());

    auto result = container_->unregister<ITestService>();

    EXPECT_TRUE(result.is_ok());
    EXPECT_FALSE(container_->is_registered<ITestService>());
}

TEST_F(ServiceContainerTest, Unregister_NotRegistered_Fails) {
    auto result = container_->unregister<ITestService>();

    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error().code, di_error_codes::service_not_registered);
}

TEST_F(ServiceContainerTest, Clear_RemovesAllServices) {
    container_->register_type<ITestService, TestServiceImpl>();
    container_->register_type<ITestDependency, TestDependencyImpl>();

    container_->clear();

    EXPECT_FALSE(container_->is_registered<ITestService>());
    EXPECT_FALSE(container_->is_registered<ITestDependency>());
    EXPECT_EQ(container_->registered_services().size(), 0u);
}

// ============================================================================
// Thread Safety Tests
// ============================================================================

TEST_F(ServiceContainerTest, ThreadSafety_ConcurrentRegistration) {
    constexpr int NUM_THREADS = 10;
    std::vector<std::thread> threads;
    std::atomic<int> success_count{0};

    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back([this, i, &success_count]() {
            auto result = container_->register_factory<ITestService>(
                [](IServiceContainer&) {
                    return std::make_shared<TestServiceImpl>();
                },
                service_lifetime::singleton
            );
            if (result.is_ok()) {
                success_count++;
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    // Only one registration should succeed
    EXPECT_EQ(success_count, 1);
}

TEST_F(ServiceContainerTest, ThreadSafety_ConcurrentResolution) {
    container_->register_type<ITestService, TestServiceImpl>(
        service_lifetime::singleton);

    constexpr int NUM_THREADS = 10;
    constexpr int ITERATIONS = 100;
    std::vector<std::thread> threads;
    std::atomic<int> resolve_count{0};
    std::shared_ptr<ITestService> first_instance;
    std::mutex first_mutex;

    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back([this, &resolve_count, &first_instance, &first_mutex]() {
            for (int j = 0; j < ITERATIONS; ++j) {
                auto result = container_->resolve<ITestService>();
                if (result.is_ok()) {
                    resolve_count++;
                    std::lock_guard<std::mutex> lock(first_mutex);
                    if (!first_instance) {
                        first_instance = result.value();
                    } else {
                        // All instances should be the same (singleton)
                        EXPECT_EQ(result.value().get(), first_instance.get());
                    }
                }
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_EQ(resolve_count, NUM_THREADS * ITERATIONS);
}

TEST_F(ServiceContainerTest, ThreadSafety_ConcurrentScopeResolution) {
    container_->register_type<ITestService, CountingService>(
        service_lifetime::scoped);

    constexpr int NUM_THREADS = 10;
    std::vector<std::thread> threads;
    std::atomic<int> total_instantiations{0};

    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back([this, &total_instantiations]() {
            auto scope = container_->create_scope();

            // Multiple resolutions in same scope should return same instance
            auto result1 = scope->resolve<ITestService>();
            auto result2 = scope->resolve<ITestService>();

            EXPECT_TRUE(result1.is_ok());
            EXPECT_TRUE(result2.is_ok());
            EXPECT_EQ(result1.value().get(), result2.value().get());
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    // Each thread should have created exactly one instance
    EXPECT_EQ(g_instantiation_count, NUM_THREADS);
}

// ============================================================================
// Global Container Tests
// ============================================================================

TEST(GlobalContainerTest, Global_ReturnsSameInstance) {
    auto& container1 = service_container::global();
    auto& container2 = service_container::global();

    EXPECT_EQ(&container1, &container2);
}

TEST(GlobalContainerTest, Global_CanRegisterAndResolve) {
    auto& container = service_container::global();

    // Clean up from other tests
    container.clear();

    container.register_type<ITestService, TestServiceImpl>();

    auto result = container.resolve<ITestService>();

    EXPECT_TRUE(result.is_ok());
    EXPECT_EQ(result.value()->get_value(), 42);

    // Clean up
    container.clear();
}

// ============================================================================
// Factory Exception Handling Tests
// ============================================================================

TEST_F(ServiceContainerTest, Factory_ThrowsException_ReturnsError) {
    container_->register_factory<ITestService>(
        [](IServiceContainer&) -> std::shared_ptr<ITestService> {
            throw std::runtime_error("Factory failed!");
        },
        service_lifetime::singleton
    );

    auto result = container_->resolve<ITestService>();

    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error().code, di_error_codes::factory_error);
    EXPECT_TRUE(result.error().message.find("Factory failed!") != std::string::npos);
}

// ============================================================================
// Lifetime String Conversion Tests
// ============================================================================

TEST(ServiceLifetimeTest, ToString) {
    EXPECT_STREQ(to_string(service_lifetime::singleton), "singleton");
    EXPECT_STREQ(to_string(service_lifetime::transient), "transient");
    EXPECT_STREQ(to_string(service_lifetime::scoped), "scoped");
}
