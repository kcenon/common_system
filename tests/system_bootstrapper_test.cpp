// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file system_bootstrapper_test.cpp
 * @brief Unit tests for SystemBootstrapper implementation (Issue #176)
 *
 * These tests verify:
 * - Normal initialization and shutdown
 * - Fluent API method chaining
 * - Factory-based logger creation
 * - Callback execution order
 * - RAII automatic shutdown
 * - Exception handling and error cases
 * - Move semantics
 */

#include <gtest/gtest.h>
#include <kcenon/common/bootstrap/system_bootstrapper.h>

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

using namespace kcenon::common;
using namespace kcenon::common::bootstrap;
using namespace kcenon::common::interfaces;

// ============================================================================
// Test Logger Implementation
// ============================================================================

/**
 * @brief Test logger implementation that tracks log messages.
 */
class TestLogger : public ILogger {
public:
    TestLogger() = default;
    explicit TestLogger(const std::string& name) : name_(name) {}

    VoidResult log(log_level level, const std::string& message) override {
        std::lock_guard<std::mutex> lock(mutex_);
        messages_.emplace_back(level, message);
        return VoidResult::ok({});
    }

    VoidResult log(log_level level,
                   const std::string& message,
                   const std::string& /*file*/,
                   int /*line*/,
                   const std::string& /*function*/) override {
        return log(level, message);
    }

    VoidResult log(const log_entry& entry) override {
        return log(entry.level, entry.message);
    }

    bool is_enabled(log_level level) const override {
        return level >= level_;
    }

    VoidResult set_level(log_level level) override {
        level_ = level;
        return VoidResult::ok({});
    }

    log_level get_level() const override {
        return level_;
    }

    VoidResult flush() override {
        flushed_ = true;
        return VoidResult::ok({});
    }

    // Test accessors
    const std::string& name() const { return name_; }
    bool was_flushed() const { return flushed_; }

    size_t message_count() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return messages_.size();
    }

    std::pair<log_level, std::string> last_message() const {
        std::lock_guard<std::mutex> lock(mutex_);
        if (messages_.empty()) {
            return {log_level::off, ""};
        }
        return messages_.back();
    }

    void clear_messages() {
        std::lock_guard<std::mutex> lock(mutex_);
        messages_.clear();
    }

private:
    std::string name_;
    log_level level_ = log_level::info;
    bool flushed_ = false;
    mutable std::mutex mutex_;
    std::vector<std::pair<log_level, std::string>> messages_;
};

// Global counters for tracking callback execution
static std::atomic<int> g_init_callback_count{0};
static std::atomic<int> g_shutdown_callback_count{0};
static std::vector<int> g_callback_order;
static std::mutex g_callback_order_mutex;

// ============================================================================
// Test Fixtures
// ============================================================================

class SystemBootstrapperTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Clear the global registry before each test
        GlobalLoggerRegistry::instance().clear();
        g_init_callback_count = 0;
        g_shutdown_callback_count = 0;
        {
            std::lock_guard<std::mutex> lock(g_callback_order_mutex);
            g_callback_order.clear();
        }
    }

    void TearDown() override {
        // Clean up after each test
        GlobalLoggerRegistry::instance().clear();
    }

    void record_callback_order(int id) {
        std::lock_guard<std::mutex> lock(g_callback_order_mutex);
        g_callback_order.push_back(id);
    }

    std::vector<int> get_callback_order() {
        std::lock_guard<std::mutex> lock(g_callback_order_mutex);
        return g_callback_order;
    }
};

// ============================================================================
// Basic Initialization Tests
// ============================================================================

TEST_F(SystemBootstrapperTest, Initialize_Success) {
    SystemBootstrapper bootstrapper;
    bootstrapper.with_default_logger([]() {
        return std::make_shared<TestLogger>("default");
    });

    auto result = bootstrapper.initialize();

    EXPECT_TRUE(result.is_ok());
    EXPECT_TRUE(bootstrapper.is_initialized());
}

TEST_F(SystemBootstrapperTest, Initialize_WithoutLogger_Success) {
    SystemBootstrapper bootstrapper;

    auto result = bootstrapper.initialize();

    EXPECT_TRUE(result.is_ok());
    EXPECT_TRUE(bootstrapper.is_initialized());
}

TEST_F(SystemBootstrapperTest, Initialize_DuplicateReturnsError) {
    SystemBootstrapper bootstrapper;
    bootstrapper.with_default_logger([]() {
        return std::make_shared<TestLogger>("default");
    });

    auto result1 = bootstrapper.initialize();
    auto result2 = bootstrapper.initialize();

    EXPECT_TRUE(result1.is_ok());
    EXPECT_TRUE(result2.is_err());
    EXPECT_EQ(result2.error().code, error_codes::ALREADY_EXISTS);
}

TEST_F(SystemBootstrapperTest, Shutdown_Success) {
    SystemBootstrapper bootstrapper;
    bootstrapper.with_default_logger([]() {
        return std::make_shared<TestLogger>("default");
    });

    bootstrapper.initialize();
    bootstrapper.shutdown();

    EXPECT_FALSE(bootstrapper.is_initialized());
}

TEST_F(SystemBootstrapperTest, Shutdown_MultipleCallsNoEffect) {
    SystemBootstrapper bootstrapper;
    bootstrapper.with_default_logger([]() {
        return std::make_shared<TestLogger>("default");
    });

    bootstrapper.initialize();
    bootstrapper.shutdown();
    bootstrapper.shutdown();  // Should have no effect
    bootstrapper.shutdown();  // Should have no effect

    EXPECT_FALSE(bootstrapper.is_initialized());
}

TEST_F(SystemBootstrapperTest, Shutdown_BeforeInitialize_NoEffect) {
    SystemBootstrapper bootstrapper;

    // Shutdown before initialize should have no effect
    bootstrapper.shutdown();

    EXPECT_FALSE(bootstrapper.is_initialized());
}

// ============================================================================
// Fluent API Tests
// ============================================================================

TEST_F(SystemBootstrapperTest, FluentAPI_MethodChaining) {
    SystemBootstrapper bootstrapper;

    auto& result = bootstrapper
        .with_default_logger([]() { return std::make_shared<TestLogger>("default"); })
        .with_logger("network", []() { return std::make_shared<TestLogger>("network"); })
        .with_logger("database", []() { return std::make_shared<TestLogger>("database"); })
        .on_initialize([]() { /* init callback */ })
        .on_shutdown([]() { /* shutdown callback */ });

    EXPECT_EQ(&result, &bootstrapper);
}

TEST_F(SystemBootstrapperTest, FluentAPI_WithDefaultLogger) {
    SystemBootstrapper bootstrapper;
    bootstrapper.with_default_logger([]() {
        return std::make_shared<TestLogger>("my-default");
    });

    bootstrapper.initialize();

    auto logger = GlobalLoggerRegistry::instance().get_default_logger();
    auto test_logger = std::dynamic_pointer_cast<TestLogger>(logger);

    EXPECT_NE(test_logger, nullptr);
    EXPECT_EQ(test_logger->name(), "my-default");
}

TEST_F(SystemBootstrapperTest, FluentAPI_WithNamedLoggers) {
    SystemBootstrapper bootstrapper;
    bootstrapper
        .with_logger("network", []() { return std::make_shared<TestLogger>("net-logger"); })
        .with_logger("database", []() { return std::make_shared<TestLogger>("db-logger"); });

    bootstrapper.initialize();

    auto net_logger = GlobalLoggerRegistry::instance().get_logger("network");
    auto db_logger = GlobalLoggerRegistry::instance().get_logger("database");

    auto net_test = std::dynamic_pointer_cast<TestLogger>(net_logger);
    auto db_test = std::dynamic_pointer_cast<TestLogger>(db_logger);

    EXPECT_NE(net_test, nullptr);
    EXPECT_EQ(net_test->name(), "net-logger");

    EXPECT_NE(db_test, nullptr);
    EXPECT_EQ(db_test->name(), "db-logger");
}

TEST_F(SystemBootstrapperTest, FluentAPI_DuplicateNameOverwrites) {
    SystemBootstrapper bootstrapper;
    bootstrapper
        .with_logger("test", []() { return std::make_shared<TestLogger>("first"); })
        .with_logger("test", []() { return std::make_shared<TestLogger>("second"); });

    bootstrapper.initialize();

    auto logger = GlobalLoggerRegistry::instance().get_logger("test");
    auto test_logger = std::dynamic_pointer_cast<TestLogger>(logger);

    EXPECT_NE(test_logger, nullptr);
    EXPECT_EQ(test_logger->name(), "second");
}

// ============================================================================
// Callback Execution Tests
// ============================================================================

TEST_F(SystemBootstrapperTest, InitCallbacks_ExecutedInOrder) {
    SystemBootstrapper bootstrapper;
    bootstrapper
        .on_initialize([this]() { record_callback_order(1); })
        .on_initialize([this]() { record_callback_order(2); })
        .on_initialize([this]() { record_callback_order(3); });

    bootstrapper.initialize();

    auto order = get_callback_order();
    EXPECT_EQ(order.size(), 3);
    EXPECT_EQ(order[0], 1);
    EXPECT_EQ(order[1], 2);
    EXPECT_EQ(order[2], 3);
}

TEST_F(SystemBootstrapperTest, ShutdownCallbacks_ExecutedInReverseOrder) {
    SystemBootstrapper bootstrapper;
    bootstrapper
        .on_shutdown([this]() { record_callback_order(1); })
        .on_shutdown([this]() { record_callback_order(2); })
        .on_shutdown([this]() { record_callback_order(3); });

    bootstrapper.initialize();
    bootstrapper.shutdown();

    auto order = get_callback_order();
    EXPECT_EQ(order.size(), 3);
    EXPECT_EQ(order[0], 3);  // Last registered, first executed (LIFO)
    EXPECT_EQ(order[1], 2);
    EXPECT_EQ(order[2], 1);
}

TEST_F(SystemBootstrapperTest, Callbacks_InitAndShutdownOrder) {
    SystemBootstrapper bootstrapper;
    bootstrapper
        .on_initialize([this]() { record_callback_order(10); })
        .on_initialize([this]() { record_callback_order(20); })
        .on_shutdown([this]() { record_callback_order(100); })
        .on_shutdown([this]() { record_callback_order(200); });

    bootstrapper.initialize();
    bootstrapper.shutdown();

    auto order = get_callback_order();
    EXPECT_EQ(order.size(), 4);
    // Init callbacks in order
    EXPECT_EQ(order[0], 10);
    EXPECT_EQ(order[1], 20);
    // Shutdown callbacks in reverse order
    EXPECT_EQ(order[2], 200);
    EXPECT_EQ(order[3], 100);
}

TEST_F(SystemBootstrapperTest, InitCallbacks_ExecutedAfterLoggerRegistration) {
    std::shared_ptr<TestLogger> captured_logger;

    SystemBootstrapper bootstrapper;
    bootstrapper
        .with_default_logger([]() { return std::make_shared<TestLogger>("default"); })
        .on_initialize([&captured_logger]() {
            // Logger should be available during init callback
            auto logger = GlobalLoggerRegistry::instance().get_default_logger();
            captured_logger = std::dynamic_pointer_cast<TestLogger>(logger);
        });

    bootstrapper.initialize();

    EXPECT_NE(captured_logger, nullptr);
    EXPECT_EQ(captured_logger->name(), "default");
}

TEST_F(SystemBootstrapperTest, ShutdownCallbacks_ExecutedBeforeLoggerClear) {
    std::shared_ptr<TestLogger> captured_logger;

    SystemBootstrapper bootstrapper;
    bootstrapper
        .with_default_logger([]() { return std::make_shared<TestLogger>("default"); })
        .on_shutdown([&captured_logger]() {
            // Logger should still be available during shutdown callback
            auto logger = GlobalLoggerRegistry::instance().get_default_logger();
            captured_logger = std::dynamic_pointer_cast<TestLogger>(logger);
        });

    bootstrapper.initialize();
    bootstrapper.shutdown();

    EXPECT_NE(captured_logger, nullptr);
    EXPECT_EQ(captured_logger->name(), "default");

    // After shutdown, registry should be cleared
    auto logger = GlobalLoggerRegistry::instance().get_default_logger();
    EXPECT_EQ(logger->get_level(), log_level::off);  // NullLogger
}

// ============================================================================
// RAII Tests
// ============================================================================

TEST_F(SystemBootstrapperTest, RAII_DestructorCallsShutdown) {
    bool shutdown_called = false;

    {
        SystemBootstrapper bootstrapper;
        bootstrapper
            .with_default_logger([]() { return std::make_shared<TestLogger>("default"); })
            .on_shutdown([&shutdown_called]() { shutdown_called = true; });

        bootstrapper.initialize();
        // bootstrapper goes out of scope here
    }

    EXPECT_TRUE(shutdown_called);
}

TEST_F(SystemBootstrapperTest, RAII_RegistryClearedOnDestruction) {
    {
        SystemBootstrapper bootstrapper;
        bootstrapper.with_default_logger([]() {
            return std::make_shared<TestLogger>("default");
        });

        bootstrapper.initialize();

        // Logger should be available
        EXPECT_TRUE(GlobalLoggerRegistry::instance().has_default_logger());
    }

    // After destruction, registry should be cleared
    auto logger = GlobalLoggerRegistry::instance().get_default_logger();
    EXPECT_EQ(logger->get_level(), log_level::off);  // NullLogger
}

// ============================================================================
// Move Semantics Tests
// ============================================================================

TEST_F(SystemBootstrapperTest, MoveConstruction_TransfersOwnership) {
    SystemBootstrapper bootstrapper1;
    bootstrapper1.with_default_logger([]() {
        return std::make_shared<TestLogger>("default");
    });
    bootstrapper1.initialize();

    SystemBootstrapper bootstrapper2(std::move(bootstrapper1));

    EXPECT_TRUE(bootstrapper2.is_initialized());
    // Note: bootstrapper1 is now in moved-from state, initialized_ is false
}

TEST_F(SystemBootstrapperTest, MoveAssignment_TransfersOwnership) {
    bool shutdown1_called = false;
    bool shutdown2_called = false;

    SystemBootstrapper bootstrapper1;
    bootstrapper1
        .with_default_logger([]() { return std::make_shared<TestLogger>("logger1"); })
        .on_shutdown([&shutdown1_called]() { shutdown1_called = true; });
    bootstrapper1.initialize();

    SystemBootstrapper bootstrapper2;
    bootstrapper2
        .with_default_logger([]() { return std::make_shared<TestLogger>("logger2"); })
        .on_shutdown([&shutdown2_called]() { shutdown2_called = true; });
    bootstrapper2.initialize();

    // Clear registry for bootstrapper2's shutdown
    GlobalLoggerRegistry::instance().clear();

    // Reset to register logger2 again
    bootstrapper2.reset();
    bootstrapper2.with_default_logger([]() {
        return std::make_shared<TestLogger>("logger2");
    });
    bootstrapper2.on_shutdown([&shutdown2_called]() { shutdown2_called = true; });
    bootstrapper2.initialize();

    // Move assignment should shutdown bootstrapper2's state first
    bootstrapper2 = std::move(bootstrapper1);

    EXPECT_TRUE(shutdown2_called);  // bootstrapper2's shutdown was called
    EXPECT_TRUE(bootstrapper2.is_initialized());
}

TEST_F(SystemBootstrapperTest, MoveConstruction_PreventsDoubleShutdown) {
    int shutdown_count = 0;

    SystemBootstrapper bootstrapper1;
    bootstrapper1
        .with_default_logger([]() { return std::make_shared<TestLogger>("default"); })
        .on_shutdown([&shutdown_count]() { shutdown_count++; });
    bootstrapper1.initialize();

    {
        SystemBootstrapper bootstrapper2(std::move(bootstrapper1));
        // bootstrapper2 goes out of scope
    }

    // Shutdown should only be called once
    EXPECT_EQ(shutdown_count, 1);
}

// ============================================================================
// Error Handling Tests
// ============================================================================

TEST_F(SystemBootstrapperTest, Initialize_NullFactoryResult_ReturnsError) {
    SystemBootstrapper bootstrapper;
    bootstrapper.with_default_logger([]() {
        return std::shared_ptr<ILogger>(nullptr);  // Return null
    });

    auto result = bootstrapper.initialize();

    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error().code, error_codes::INTERNAL_ERROR);
}

TEST_F(SystemBootstrapperTest, Initialize_NamedLoggerNullFactory_ReturnsError) {
    SystemBootstrapper bootstrapper;
    bootstrapper.with_logger("test", []() {
        return std::shared_ptr<ILogger>(nullptr);  // Return null
    });

    auto result = bootstrapper.initialize();

    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error().code, error_codes::INTERNAL_ERROR);
}

TEST_F(SystemBootstrapperTest, NullCallback_Ignored) {
    SystemBootstrapper bootstrapper;

    // Null callbacks should be ignored
    bootstrapper
        .on_initialize(nullptr)
        .on_shutdown(nullptr);

    auto result = bootstrapper.initialize();

    EXPECT_TRUE(result.is_ok());
}

// ============================================================================
// Reset Tests
// ============================================================================

TEST_F(SystemBootstrapperTest, Reset_ClearsConfiguration) {
    SystemBootstrapper bootstrapper;
    bootstrapper
        .with_default_logger([]() { return std::make_shared<TestLogger>("default"); })
        .with_logger("test", []() { return std::make_shared<TestLogger>("test"); })
        .on_initialize([]() { g_init_callback_count++; })
        .on_shutdown([]() { g_shutdown_callback_count++; });

    bootstrapper.reset();

    // After reset, initializing should not register any loggers
    bootstrapper.initialize();

    EXPECT_TRUE(bootstrapper.is_initialized());
    EXPECT_EQ(g_init_callback_count, 0);

    // Registry should not have any custom loggers
    auto logger = GlobalLoggerRegistry::instance().get_default_logger();
    EXPECT_EQ(logger->get_level(), log_level::off);  // NullLogger
}

TEST_F(SystemBootstrapperTest, Reset_ShutdownsIfInitialized) {
    bool shutdown_called = false;

    SystemBootstrapper bootstrapper;
    bootstrapper
        .with_default_logger([]() { return std::make_shared<TestLogger>("default"); })
        .on_shutdown([&shutdown_called]() { shutdown_called = true; });

    bootstrapper.initialize();
    bootstrapper.reset();

    EXPECT_TRUE(shutdown_called);
    EXPECT_FALSE(bootstrapper.is_initialized());
}

TEST_F(SystemBootstrapperTest, Reset_AllowsReinitialization) {
    SystemBootstrapper bootstrapper;
    bootstrapper.with_default_logger([]() {
        return std::make_shared<TestLogger>("first");
    });

    bootstrapper.initialize();
    bootstrapper.reset();

    // Reconfigure
    bootstrapper.with_default_logger([]() {
        return std::make_shared<TestLogger>("second");
    });

    auto result = bootstrapper.initialize();
    EXPECT_TRUE(result.is_ok());

    auto logger = GlobalLoggerRegistry::instance().get_default_logger();
    auto test_logger = std::dynamic_pointer_cast<TestLogger>(logger);
    EXPECT_NE(test_logger, nullptr);
    EXPECT_EQ(test_logger->name(), "second");
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST_F(SystemBootstrapperTest, Integration_CompleteLifecycle) {
    std::vector<std::string> events;

    SystemBootstrapper bootstrapper;
    bootstrapper
        .with_default_logger([&events]() {
            events.push_back("create_default_logger");
            return std::make_shared<TestLogger>("default");
        })
        .with_logger("network", [&events]() {
            events.push_back("create_network_logger");
            return std::make_shared<TestLogger>("network");
        })
        .on_initialize([&events]() {
            events.push_back("on_initialize");
        })
        .on_shutdown([&events]() {
            events.push_back("on_shutdown");
        });

    // Initialize
    auto result = bootstrapper.initialize();
    EXPECT_TRUE(result.is_ok());

    // Verify loggers are available
    auto default_logger = GlobalLoggerRegistry::instance().get_default_logger();
    auto network_logger = GlobalLoggerRegistry::instance().get_logger("network");

    EXPECT_NE(std::dynamic_pointer_cast<TestLogger>(default_logger), nullptr);
    EXPECT_NE(std::dynamic_pointer_cast<TestLogger>(network_logger), nullptr);

    // Shutdown
    bootstrapper.shutdown();

    // Verify event order
    EXPECT_EQ(events.size(), 4);
    EXPECT_EQ(events[0], "create_default_logger");
    EXPECT_EQ(events[1], "create_network_logger");
    EXPECT_EQ(events[2], "on_initialize");
    EXPECT_EQ(events[3], "on_shutdown");
}

TEST_F(SystemBootstrapperTest, Integration_MultipleBootstrappers) {
    SystemBootstrapper bootstrapper1;
    bootstrapper1.with_default_logger([]() {
        return std::make_shared<TestLogger>("logger1");
    });

    bootstrapper1.initialize();

    // Get reference to registered logger
    auto logger1 = GlobalLoggerRegistry::instance().get_default_logger();
    auto test1 = std::dynamic_pointer_cast<TestLogger>(logger1);
    EXPECT_NE(test1, nullptr);
    EXPECT_EQ(test1->name(), "logger1");

    // Shutdown first bootstrapper
    bootstrapper1.shutdown();

    // Second bootstrapper
    SystemBootstrapper bootstrapper2;
    bootstrapper2.with_default_logger([]() {
        return std::make_shared<TestLogger>("logger2");
    });

    bootstrapper2.initialize();

    auto logger2 = GlobalLoggerRegistry::instance().get_default_logger();
    auto test2 = std::dynamic_pointer_cast<TestLogger>(logger2);
    EXPECT_NE(test2, nullptr);
    EXPECT_EQ(test2->name(), "logger2");
}
