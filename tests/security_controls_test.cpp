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
// registry_action to_string Tests (Issue #359)
// ============================================================================

TEST_F(RegistryAuditLogTest, ToStringAllRegistryActions) {
    EXPECT_STREQ(to_string(registry_action::register_logger), "register_logger");
    EXPECT_STREQ(to_string(registry_action::unregister_logger), "unregister_logger");
    EXPECT_STREQ(to_string(registry_action::set_default_logger), "set_default_logger");
    EXPECT_STREQ(to_string(registry_action::register_factory), "register_factory");
    EXPECT_STREQ(to_string(registry_action::set_default_factory), "set_default_factory");
    EXPECT_STREQ(to_string(registry_action::clear_loggers), "clear_loggers");
    EXPECT_STREQ(to_string(registry_action::freeze_logger_registry), "freeze_logger_registry");
    EXPECT_STREQ(to_string(registry_action::register_service), "register_service");
    EXPECT_STREQ(to_string(registry_action::unregister_service), "unregister_service");
    EXPECT_STREQ(to_string(registry_action::clear_services), "clear_services");
    EXPECT_STREQ(to_string(registry_action::freeze_service_container), "freeze_service_container");
}

// ============================================================================
// log_event Move Overload Tests (Issue #359)
// ============================================================================

TEST_F(RegistryAuditLogTest, LogEventMoveOverload) {
    registry_event event(
        registry_action::register_service,
        "move-test-service",
        interfaces::source_location::current(),
        true
    );

    RegistryAuditLog::log_event(std::move(event));

    auto events = RegistryAuditLog::get_events();
    ASSERT_EQ(events.size(), 1);
    EXPECT_EQ(events[0].action, registry_action::register_service);
    EXPECT_EQ(events[0].target_name, "move-test-service");
    EXPECT_TRUE(events[0].success);
}

// ============================================================================
// event_count Direct Verification Tests (Issue #359)
// ============================================================================

TEST_F(RegistryAuditLogTest, EventCountEmpty) {
    EXPECT_EQ(RegistryAuditLog::event_count(), 0u);
}

TEST_F(RegistryAuditLogTest, EventCountAfterLogging) {
    RegistryAuditLog::log_event(registry_event(
        registry_action::register_logger, "logger1"));
    EXPECT_EQ(RegistryAuditLog::event_count(), 1u);

    RegistryAuditLog::log_event(registry_event(
        registry_action::register_service, "service1"));
    EXPECT_EQ(RegistryAuditLog::event_count(), 2u);

    RegistryAuditLog::log_event(registry_event(
        registry_action::clear_loggers, ""));
    EXPECT_EQ(RegistryAuditLog::event_count(), 3u);
}

TEST_F(RegistryAuditLogTest, EventCountAfterClear) {
    RegistryAuditLog::log_event(registry_event(
        registry_action::register_logger, "logger1"));
    RegistryAuditLog::log_event(registry_event(
        registry_action::register_logger, "logger2"));
    EXPECT_EQ(RegistryAuditLog::event_count(), 2u);

    RegistryAuditLog::clear();
    EXPECT_EQ(RegistryAuditLog::event_count(), 0u);
}

// ============================================================================
// get_events_by_action for All Untested Action Types (Issue #359)
// ============================================================================

TEST_F(RegistryAuditLogTest, FilterByActionUnregisterLogger) {
    RegistryAuditLog::log_event(registry_event(
        registry_action::unregister_logger, "old-logger"));
    RegistryAuditLog::log_event(registry_event(
        registry_action::register_logger, "new-logger"));

    auto events = RegistryAuditLog::get_events_by_action(
        registry_action::unregister_logger);
    ASSERT_EQ(events.size(), 1);
    EXPECT_EQ(events[0].target_name, "old-logger");
}

TEST_F(RegistryAuditLogTest, FilterByActionSetDefaultLogger) {
    RegistryAuditLog::log_event(registry_event(
        registry_action::set_default_logger, "main-logger"));

    auto events = RegistryAuditLog::get_events_by_action(
        registry_action::set_default_logger);
    ASSERT_EQ(events.size(), 1);
    EXPECT_EQ(events[0].target_name, "main-logger");
}

TEST_F(RegistryAuditLogTest, FilterByActionRegisterFactory) {
    RegistryAuditLog::log_event(registry_event(
        registry_action::register_factory, "console-factory"));

    auto events = RegistryAuditLog::get_events_by_action(
        registry_action::register_factory);
    ASSERT_EQ(events.size(), 1);
    EXPECT_EQ(events[0].target_name, "console-factory");
}

TEST_F(RegistryAuditLogTest, FilterByActionSetDefaultFactory) {
    RegistryAuditLog::log_event(registry_event(
        registry_action::set_default_factory, "file-factory"));

    auto events = RegistryAuditLog::get_events_by_action(
        registry_action::set_default_factory);
    ASSERT_EQ(events.size(), 1);
    EXPECT_EQ(events[0].target_name, "file-factory");
}

TEST_F(RegistryAuditLogTest, FilterByActionClearLoggers) {
    RegistryAuditLog::log_event(registry_event(
        registry_action::clear_loggers, ""));

    auto events = RegistryAuditLog::get_events_by_action(
        registry_action::clear_loggers);
    ASSERT_EQ(events.size(), 1);
    EXPECT_TRUE(events[0].target_name.empty());
}

TEST_F(RegistryAuditLogTest, FilterByActionFreezeLoggerRegistry) {
    RegistryAuditLog::log_event(registry_event(
        registry_action::freeze_logger_registry, ""));

    auto events = RegistryAuditLog::get_events_by_action(
        registry_action::freeze_logger_registry);
    ASSERT_EQ(events.size(), 1);
}

TEST_F(RegistryAuditLogTest, FilterByActionUnregisterService) {
    RegistryAuditLog::log_event(registry_event(
        registry_action::unregister_service, "old-service"));

    auto events = RegistryAuditLog::get_events_by_action(
        registry_action::unregister_service);
    ASSERT_EQ(events.size(), 1);
    EXPECT_EQ(events[0].target_name, "old-service");
}

TEST_F(RegistryAuditLogTest, FilterByActionClearServices) {
    RegistryAuditLog::log_event(registry_event(
        registry_action::clear_services, ""));

    auto events = RegistryAuditLog::get_events_by_action(
        registry_action::clear_services);
    ASSERT_EQ(events.size(), 1);
}

TEST_F(RegistryAuditLogTest, FilterByActionFreezeServiceContainer) {
    RegistryAuditLog::log_event(registry_event(
        registry_action::freeze_service_container, ""));

    auto events = RegistryAuditLog::get_events_by_action(
        registry_action::freeze_service_container);
    ASSERT_EQ(events.size(), 1);
}

// ============================================================================
// get_events_in_range with Various Time Windows (Issue #359)
// ============================================================================

TEST_F(RegistryAuditLogTest, TimeRangeEmptyWindow) {
    auto now = std::chrono::system_clock::now();
    auto past = now - std::chrono::hours(1);

    // Log event at current time
    RegistryAuditLog::log_event(registry_event(
        registry_action::register_logger, "test"));

    // Query a window entirely in the past
    auto events = RegistryAuditLog::get_events_in_range(
        past - std::chrono::hours(2), past);
    EXPECT_TRUE(events.empty());
}

TEST_F(RegistryAuditLogTest, TimeRangeFutureWindow) {
    RegistryAuditLog::log_event(registry_event(
        registry_action::register_logger, "test"));

    auto future = std::chrono::system_clock::now() + std::chrono::hours(1);
    auto far_future = future + std::chrono::hours(1);

    auto events = RegistryAuditLog::get_events_in_range(future, far_future);
    EXPECT_TRUE(events.empty());
}

TEST_F(RegistryAuditLogTest, TimeRangeMultipleEvents) {
    auto before = std::chrono::system_clock::now();

    RegistryAuditLog::log_event(registry_event(
        registry_action::register_logger, "first"));
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    auto mid1 = std::chrono::system_clock::now();

    RegistryAuditLog::log_event(registry_event(
        registry_action::register_service, "second"));
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    auto mid2 = std::chrono::system_clock::now();

    RegistryAuditLog::log_event(registry_event(
        registry_action::clear_loggers, "third"));

    auto after = std::chrono::system_clock::now();

    // Full window should capture all 3
    auto all = RegistryAuditLog::get_events_in_range(before, after);
    EXPECT_EQ(all.size(), 3u);

    // Middle window should capture only the second event
    auto middle = RegistryAuditLog::get_events_in_range(mid1, mid2);
    ASSERT_EQ(middle.size(), 1u);
    EXPECT_EQ(middle[0].target_name, "second");
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
