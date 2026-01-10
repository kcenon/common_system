// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file module_verification_test.cpp
 * @brief Tests for C++20 module functionality verification.
 *
 * This file tests that C++20 modules work correctly when enabled.
 * Tests are conditional on KCENON_USE_MODULES being defined.
 *
 * Part of Phase 3 (C++20 Module Stabilization) from Issue #275.
 * Sub-issue: #277 - Test verification with module builds.
 */

#ifdef KCENON_USE_MODULES
import kcenon.common;
#else
// Header-only fallback for comparison testing
#include <kcenon/common/patterns/result.h>
#include <kcenon/common/patterns/event_bus.h>
#include <kcenon/common/di/service_container.h>
#include <kcenon/common/config/unified_config.h>
#endif

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <string>
#include <thread>

using namespace kcenon::common;

namespace {

// =============================================================================
// Result Module Tests (Using member method API - RECOMMENDED)
// =============================================================================

class module_result_test : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(module_result_test, OkResultWorks) {
    auto result = Result<int>::ok(42);
    EXPECT_TRUE(result.is_ok());
    EXPECT_FALSE(result.is_err());
    EXPECT_EQ(result.unwrap(), 42);
}

TEST_F(module_result_test, ErrorResultWorks) {
    auto result = Result<int>::err(-1, "test error", "test_module");
    EXPECT_FALSE(result.is_ok());
    EXPECT_TRUE(result.is_err());

    const auto& err = result.error();
    EXPECT_EQ(err.code, -1);
    EXPECT_EQ(err.message, "test error");
    EXPECT_EQ(err.module, "test_module");
}

TEST_F(module_result_test, MapTransformation) {
    auto result = Result<int>::ok(10);
    auto mapped = result.map([](int x) { return x * 2; });
    EXPECT_TRUE(mapped.is_ok());
    EXPECT_EQ(mapped.unwrap(), 20);
}

TEST_F(module_result_test, AndThenChaining) {
    auto result = Result<int>::ok(5);
    auto chained = result.and_then([](int x) {
        return Result<std::string>::ok(std::to_string(x));
    });
    EXPECT_TRUE(chained.is_ok());
    EXPECT_EQ(chained.unwrap(), "5");
}

TEST_F(module_result_test, ValueOrDefault) {
    auto success = Result<int>::ok(10);
    auto failure = Result<int>::err(-1, "Error");

    EXPECT_EQ(success.value_or(0), 10);
    EXPECT_EQ(failure.value_or(0), 0);
}

// =============================================================================
// Event Bus Module Tests
// =============================================================================

class module_event_bus_test : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

struct test_module_event {
    int id;
    std::string message;
};

TEST_F(module_event_bus_test, PublishAndSubscribe) {
    simple_event_bus bus;
    std::atomic<int> received_value{0};

    auto token = bus.subscribe<test_module_event>(
        [&received_value](const test_module_event& evt) {
            received_value = evt.id;
        });

    bus.start();
    bus.publish(test_module_event{42, "test"});

    // Wait for event to be processed
    auto start = std::chrono::steady_clock::now();
    while (received_value == 0 &&
           std::chrono::steady_clock::now() - start < std::chrono::seconds(1)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    EXPECT_EQ(received_value, 42);

    bus.unsubscribe(token);
    bus.stop();
}

// =============================================================================
// Service Container Module Tests
// =============================================================================

class module_service_container_test : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

class i_module_test_service {
public:
    i_module_test_service() = default;
    virtual ~i_module_test_service() = default;
    i_module_test_service(const i_module_test_service&) = default;
    i_module_test_service& operator=(const i_module_test_service&) = default;
    i_module_test_service(i_module_test_service&&) = default;
    i_module_test_service& operator=(i_module_test_service&&) = default;

    virtual int get_value() const = 0;
};

class module_test_service_impl : public i_module_test_service {
public:
    int get_value() const override { return 42; }
};

TEST_F(module_service_container_test, RegisterAndResolve) {
    di::service_container container;

    auto result = container.register_type<i_module_test_service, module_test_service_impl>(
        di::service_lifetime::singleton);
    EXPECT_TRUE(result.is_ok());

    auto resolve_result = container.resolve<i_module_test_service>();
    ASSERT_TRUE(resolve_result.is_ok());

    const auto& service = resolve_result.value();
    ASSERT_NE(service, nullptr);
    EXPECT_EQ(service->get_value(), 42);
}

// =============================================================================
// Module Version Test (Only available with module build)
// =============================================================================

#ifdef KCENON_USE_MODULES
TEST(module_version_test, VersionInfoAvailable) {
    EXPECT_EQ(module_version::major, 0);
    EXPECT_EQ(module_version::minor, 2);
    EXPECT_EQ(module_version::patch, 0);
    EXPECT_STREQ(module_version::string, "0.2.0.0");
    EXPECT_STREQ(module_version::module_name, "kcenon.common");
}
#endif

// =============================================================================
// Module Build Verification
// =============================================================================

TEST(module_build_verification_test, ModuleModeDetection) {
#ifdef KCENON_USE_MODULES
    // Module build is active - this test verifies module imports work
    SUCCEED() << "Module build is active";
#else
    // Header-only build is active - this test verifies header includes work
    SUCCEED() << "Header-only build is active";
#endif
}

TEST(module_build_verification_test, CoreTypesAccessible) {
    // Verify core types are accessible regardless of build mode
    auto result = Result<int>::ok(100);
    EXPECT_TRUE(result.is_ok());
    EXPECT_EQ(result.unwrap(), 100);
}

} // namespace
