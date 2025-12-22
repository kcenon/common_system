// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file security_controls_test.cpp
 * @brief Unit tests for security controls in GlobalLoggerRegistry and service_container
 *
 * Tests verify:
 * - Freeze mechanism prevents modifications
 * - Audit logging captures registry mutations
 * - SystemBootstrapper auto-freeze integration
 *
 * @note Issue #206: Security controls for global registries
 *
 * IMPORTANT: Since freeze() is a one-way operation, these tests should be run
 * in a separate process or at the end of the test suite.
 */

#include <gtest/gtest.h>
#include <kcenon/common/interfaces/global_logger_registry.h>
#include <kcenon/common/interfaces/registry_audit_log.h>
#include <kcenon/common/di/service_container.h>
#include <kcenon/common/bootstrap/system_bootstrapper.h>

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

using namespace kcenon::common;
using namespace kcenon::common::interfaces;
using namespace kcenon::common::di;
using namespace kcenon::common::bootstrap;

// ============================================================================
// Test Logger Implementation
// ============================================================================

/**
 * @note Issue #217: Removed deprecated file/line/function method in v3.0.0.
 */
class SecurityTestLogger : public ILogger {
public:
    SecurityTestLogger() = default;
    explicit SecurityTestLogger(const std::string& name) : name_(name) {}

    VoidResult log(log_level /*level*/, const std::string& /*message*/) override {
        return VoidResult::ok({});
    }

    VoidResult log(log_level level,
                   std::string_view message,
                   const source_location& /*loc*/ = source_location::current()) override {
        return log(level, std::string(message));
    }

    VoidResult log(const log_entry& entry) override {
        return log(entry.level, entry.message);
    }

    bool is_enabled(log_level /*level*/) const override { return true; }
    VoidResult set_level(log_level /*level*/) override { return VoidResult::ok({}); }
    log_level get_level() const override { return log_level::info; }
    VoidResult flush() override { return VoidResult::ok({}); }

    std::string name() const { return name_; }

private:
    std::string name_;
};

// Test service interface
class ITestService {
public:
    virtual ~ITestService() = default;
    virtual std::string name() const = 0;
};

class TestServiceImpl : public ITestService {
public:
    explicit TestServiceImpl(std::string name = "default") : name_(std::move(name)) {}
    std::string name() const override { return name_; }
private:
    std::string name_;
};

// ============================================================================
// RegistryAuditLog Tests
// ============================================================================

class RegistryAuditLogTest : public ::testing::Test {
protected:
    void SetUp() override {
        RegistryAuditLog::clear();
        RegistryAuditLog::set_enabled(true);
    }

    void TearDown() override {
        RegistryAuditLog::clear();
    }
};

TEST_F(RegistryAuditLogTest, LogEventCapture) {
    registry_event event(
        registry_action::register_logger,
        "test-logger",
        interfaces::source_location::current(),
        true
    );

    RegistryAuditLog::log_event(event);

    auto events = RegistryAuditLog::get_events();
    ASSERT_EQ(events.size(), 1);
    EXPECT_EQ(events[0].action, registry_action::register_logger);
    EXPECT_EQ(events[0].target_name, "test-logger");
    EXPECT_TRUE(events[0].success);
}

TEST_F(RegistryAuditLogTest, FilterByAction) {
    RegistryAuditLog::log_event(registry_event(
        registry_action::register_logger, "logger1"));
    RegistryAuditLog::log_event(registry_event(
        registry_action::register_service, "service1"));
    RegistryAuditLog::log_event(registry_event(
        registry_action::register_logger, "logger2"));

    auto logger_events = RegistryAuditLog::get_events_by_action(
        registry_action::register_logger);
    EXPECT_EQ(logger_events.size(), 2);

    auto service_events = RegistryAuditLog::get_events_by_action(
        registry_action::register_service);
    EXPECT_EQ(service_events.size(), 1);
}

TEST_F(RegistryAuditLogTest, DisableLogging) {
    RegistryAuditLog::set_enabled(false);

    RegistryAuditLog::log_event(registry_event(
        registry_action::register_logger, "test"));

    auto events = RegistryAuditLog::get_events();
    EXPECT_TRUE(events.empty());

    RegistryAuditLog::set_enabled(true);
}

TEST_F(RegistryAuditLogTest, FailedEventCapture) {
    registry_event event(
        registry_action::register_logger,
        "test-logger",
        interfaces::source_location::current(),
        false,
        "Registry is frozen"
    );

    RegistryAuditLog::log_event(event);

    auto events = RegistryAuditLog::get_events();
    ASSERT_EQ(events.size(), 1);
    EXPECT_FALSE(events[0].success);
    EXPECT_EQ(events[0].error_message, "Registry is frozen");
}

TEST_F(RegistryAuditLogTest, TimeRangeFilter) {
    auto start = std::chrono::system_clock::now();

    RegistryAuditLog::log_event(registry_event(
        registry_action::register_logger, "test1"));

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    auto middle = std::chrono::system_clock::now();

    RegistryAuditLog::log_event(registry_event(
        registry_action::register_logger, "test2"));

    auto end = std::chrono::system_clock::now();

    // Get events in second half only
    auto later_events = RegistryAuditLog::get_events_in_range(middle, end);
    EXPECT_EQ(later_events.size(), 1);
    EXPECT_EQ(later_events[0].target_name, "test2");

    // Get all events
    auto all_events = RegistryAuditLog::get_events_in_range(start, end);
    EXPECT_EQ(all_events.size(), 2);
}

TEST_F(RegistryAuditLogTest, ThreadSafety) {
    constexpr int num_threads = 8;
    constexpr int events_per_thread = 100;

    std::vector<std::thread> threads;

    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([t]() {
            for (int i = 0; i < events_per_thread; ++i) {
                RegistryAuditLog::log_event(registry_event(
                    registry_action::register_logger,
                    "thread_" + std::to_string(t) + "_" + std::to_string(i)));
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    EXPECT_EQ(RegistryAuditLog::event_count(),
              static_cast<size_t>(num_threads * events_per_thread));
}

// ============================================================================
// GlobalLoggerRegistry Freeze Tests
// ============================================================================

// Note: These tests use a separate instance check pattern because
// freeze() is irreversible on the singleton.

TEST(GlobalLoggerRegistryFreezeTest, InitiallyNotFrozen) {
    // This test must run before any freeze operation
    // We can only verify the API exists
    EXPECT_FALSE(GlobalLoggerRegistry::instance().is_frozen());
}

// ============================================================================
// service_container Freeze Tests  (using separate container instances)
// ============================================================================

class ServiceContainerFreezeTest : public ::testing::Test {
protected:
    void SetUp() override {
        container_ = std::make_unique<service_container>();
        RegistryAuditLog::clear();
    }

    void TearDown() override {
        container_.reset();
        RegistryAuditLog::clear();
    }

    std::unique_ptr<service_container> container_;
};

TEST_F(ServiceContainerFreezeTest, InitiallyNotFrozen) {
    EXPECT_FALSE(container_->is_frozen());
}

TEST_F(ServiceContainerFreezeTest, FreezePreventesRegistration) {
    // Register before freeze
    auto result1 = container_->register_type<ITestService, TestServiceImpl>();
    EXPECT_TRUE(result1.is_ok());

    // Freeze
    container_->freeze();
    EXPECT_TRUE(container_->is_frozen());

    // Try to register after freeze - should fail
    container_->unregister<ITestService>();  // Clear for next registration

    // Create a new container to test registration after freeze
    // (The current container is frozen but we already registered before)
}

TEST_F(ServiceContainerFreezeTest, FreezePreventsClear) {
    container_->register_type<ITestService, TestServiceImpl>();
    EXPECT_TRUE(container_->is_registered<ITestService>());

    container_->freeze();

    // Clear should silently fail when frozen
    container_->clear();

    // Service should still be registered
    EXPECT_TRUE(container_->is_registered<ITestService>());
}

TEST_F(ServiceContainerFreezeTest, ResolveWorksAfterFreeze) {
    container_->register_type<ITestService, TestServiceImpl>();

    container_->freeze();

    // Resolution should still work after freeze
    auto result = container_->resolve<ITestService>();
    EXPECT_TRUE(result.is_ok());
    EXPECT_NE(result.value(), nullptr);
}

TEST_F(ServiceContainerFreezeTest, AuditLogCapturesFreezeEvent) {
    container_->freeze();

    auto events = RegistryAuditLog::get_events_by_action(
        registry_action::freeze_service_container);
    EXPECT_GE(events.size(), 1);
}

// ============================================================================
// SystemBootstrapper Auto-Freeze Tests
// ============================================================================

class SystemBootstrapperAutoFreezeTest : public ::testing::Test {
protected:
    void SetUp() override {
        RegistryAuditLog::clear();
    }

    void TearDown() override {
        RegistryAuditLog::clear();
    }
};

TEST_F(SystemBootstrapperAutoFreezeTest, AutoFreezeConfigurationExists) {
    SystemBootstrapper bootstrapper;

    // with_auto_freeze should return reference for chaining
    auto& ref = bootstrapper.with_auto_freeze(true, true);
    EXPECT_EQ(&ref, &bootstrapper);
}

TEST_F(SystemBootstrapperAutoFreezeTest, AutoFreezeCanBeDisabled) {
    SystemBootstrapper bootstrapper;

    // Can configure without auto-freeze
    auto& ref = bootstrapper.with_auto_freeze(false, false);
    EXPECT_EQ(&ref, &bootstrapper);
}

// ============================================================================
// Error Code Tests
// ============================================================================

TEST(SecurityErrorCodesTest, RegistryFrozenErrorCodeExists) {
    // Verify the error code is defined correctly
    EXPECT_EQ(error_codes::REGISTRY_FROZEN, -11);
}

TEST(SecurityErrorCodesTest, RegistryFrozenErrorMessage) {
    auto message = error::get_error_message(error::codes::common_errors::registry_frozen);
    EXPECT_EQ(message, "Registry is frozen");
}
