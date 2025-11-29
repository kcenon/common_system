// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file unified_bootstrapper_test.cpp
 * @brief Unit tests for unified_bootstrapper implementation (TICKET-104)
 */

#include <gtest/gtest.h>
#include <kcenon/common/di/unified_bootstrapper.h>

#include <atomic>
#include <thread>
#include <vector>

using namespace kcenon::common;
using namespace kcenon::common::di;

// Test service interface
class ITestService {
public:
    virtual ~ITestService() = default;
    virtual int get_value() const = 0;
};

// Test service implementation
class TestServiceImpl : public ITestService {
public:
    int get_value() const override { return 42; }
};

// ============================================================================
// Test Fixtures
// ============================================================================

class UnifiedBootstrapperTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Ensure clean state before each test
        if (unified_bootstrapper::is_initialized()) {
            unified_bootstrapper::shutdown();
        }
    }

    void TearDown() override {
        // Ensure clean state after each test
        if (unified_bootstrapper::is_initialized()) {
            unified_bootstrapper::shutdown();
        }
    }
};

// ============================================================================
// Initialization Tests
// ============================================================================

TEST_F(UnifiedBootstrapperTest, Initialize_DefaultOptions_Succeeds) {
    auto result = unified_bootstrapper::initialize();

    EXPECT_TRUE(result.is_ok());
    EXPECT_TRUE(unified_bootstrapper::is_initialized());
}

TEST_F(UnifiedBootstrapperTest, Initialize_WithOptions_Succeeds) {
    bootstrapper_options opts;
    opts.enable_logging = true;
    opts.enable_monitoring = true;
    opts.enable_database = false;
    opts.enable_network = false;
    opts.shutdown_timeout = std::chrono::milliseconds{5000};

    auto result = unified_bootstrapper::initialize(opts);

    EXPECT_TRUE(result.is_ok());
    EXPECT_TRUE(unified_bootstrapper::is_initialized());
}

TEST_F(UnifiedBootstrapperTest, Initialize_Idempotent) {
    auto result1 = unified_bootstrapper::initialize();
    auto result2 = unified_bootstrapper::initialize();

    EXPECT_TRUE(result1.is_ok());
    EXPECT_TRUE(result2.is_ok());
    EXPECT_TRUE(unified_bootstrapper::is_initialized());
}

TEST_F(UnifiedBootstrapperTest, Initialize_WithDisabledSignalHandlers) {
    bootstrapper_options opts;
    opts.register_signal_handlers = false;

    auto result = unified_bootstrapper::initialize(opts);

    EXPECT_TRUE(result.is_ok());
    EXPECT_TRUE(unified_bootstrapper::is_initialized());
}

// ============================================================================
// Shutdown Tests
// ============================================================================

TEST_F(UnifiedBootstrapperTest, Shutdown_AfterInitialize_Succeeds) {
    unified_bootstrapper::initialize();

    auto result = unified_bootstrapper::shutdown();

    EXPECT_TRUE(result.is_ok());
    EXPECT_FALSE(unified_bootstrapper::is_initialized());
}

TEST_F(UnifiedBootstrapperTest, Shutdown_WithoutInitialize_Fails) {
    auto result = unified_bootstrapper::shutdown();

    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error().code, error_codes::NOT_INITIALIZED);
}

TEST_F(UnifiedBootstrapperTest, Shutdown_WithTimeout_Succeeds) {
    unified_bootstrapper::initialize();

    auto result = unified_bootstrapper::shutdown(std::chrono::seconds(1));

    EXPECT_TRUE(result.is_ok());
    EXPECT_FALSE(unified_bootstrapper::is_initialized());
}

TEST_F(UnifiedBootstrapperTest, Shutdown_ClearsServices) {
    unified_bootstrapper::initialize();

    auto& services = unified_bootstrapper::services();
    services.register_type<ITestService, TestServiceImpl>();

    EXPECT_TRUE(services.is_registered<ITestService>());

    unified_bootstrapper::shutdown();
    unified_bootstrapper::initialize();

    // Services should be cleared after shutdown
    EXPECT_FALSE(unified_bootstrapper::services().is_registered<ITestService>());
}

// ============================================================================
// Services Access Tests
// ============================================================================

TEST_F(UnifiedBootstrapperTest, Services_AfterInitialize_ReturnsContainer) {
    unified_bootstrapper::initialize();

    auto& services = unified_bootstrapper::services();

    // Should be able to register and resolve services
    services.register_type<ITestService, TestServiceImpl>();
    auto result = services.resolve<ITestService>();

    EXPECT_TRUE(result.is_ok());
    EXPECT_EQ(result.value()->get_value(), 42);
}

TEST_F(UnifiedBootstrapperTest, Services_WithoutInitialize_Throws) {
    EXPECT_THROW(unified_bootstrapper::services(), std::runtime_error);
}

// ============================================================================
// Shutdown Hook Tests
// ============================================================================

TEST_F(UnifiedBootstrapperTest, RegisterShutdownHook_Succeeds) {
    unified_bootstrapper::initialize();

    bool hook_called = false;
    auto result = unified_bootstrapper::register_shutdown_hook(
        "test_hook",
        [&hook_called](std::chrono::milliseconds) {
            hook_called = true;
        }
    );

    EXPECT_TRUE(result.is_ok());

    unified_bootstrapper::shutdown();

    EXPECT_TRUE(hook_called);
}

TEST_F(UnifiedBootstrapperTest, RegisterShutdownHook_Duplicate_Fails) {
    unified_bootstrapper::initialize();

    unified_bootstrapper::register_shutdown_hook(
        "test_hook",
        [](std::chrono::milliseconds) {}
    );

    auto result = unified_bootstrapper::register_shutdown_hook(
        "test_hook",
        [](std::chrono::milliseconds) {}
    );

    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error().code, error_codes::ALREADY_EXISTS);
}

TEST_F(UnifiedBootstrapperTest, RegisterShutdownHook_WithoutInitialize_Fails) {
    auto result = unified_bootstrapper::register_shutdown_hook(
        "test_hook",
        [](std::chrono::milliseconds) {}
    );

    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error().code, error_codes::NOT_INITIALIZED);
}

TEST_F(UnifiedBootstrapperTest, UnregisterShutdownHook_Succeeds) {
    unified_bootstrapper::initialize();

    unified_bootstrapper::register_shutdown_hook(
        "test_hook",
        [](std::chrono::milliseconds) {}
    );

    auto result = unified_bootstrapper::unregister_shutdown_hook("test_hook");

    EXPECT_TRUE(result.is_ok());
}

TEST_F(UnifiedBootstrapperTest, UnregisterShutdownHook_NotFound_Fails) {
    unified_bootstrapper::initialize();

    auto result = unified_bootstrapper::unregister_shutdown_hook("nonexistent");

    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error().code, error_codes::NOT_FOUND);
}

TEST_F(UnifiedBootstrapperTest, ShutdownHooks_CalledInReverseOrder) {
    unified_bootstrapper::initialize();

    std::vector<std::string> order;

    unified_bootstrapper::register_shutdown_hook(
        "hook1",
        [&order](std::chrono::milliseconds) {
            order.push_back("hook1");
        }
    );

    unified_bootstrapper::register_shutdown_hook(
        "hook2",
        [&order](std::chrono::milliseconds) {
            order.push_back("hook2");
        }
    );

    unified_bootstrapper::register_shutdown_hook(
        "hook3",
        [&order](std::chrono::milliseconds) {
            order.push_back("hook3");
        }
    );

    unified_bootstrapper::shutdown();

    // Hooks should be called in reverse order (LIFO)
    // Note: There's also a default hook, so we check relative order
    ASSERT_GE(order.size(), 3u);
    auto hook3_pos = std::find(order.begin(), order.end(), "hook3");
    auto hook2_pos = std::find(order.begin(), order.end(), "hook2");
    auto hook1_pos = std::find(order.begin(), order.end(), "hook1");

    EXPECT_LT(hook3_pos, hook2_pos);
    EXPECT_LT(hook2_pos, hook1_pos);
}

TEST_F(UnifiedBootstrapperTest, ShutdownHook_ReceivesTimeout) {
    unified_bootstrapper::initialize();

    std::chrono::milliseconds received_timeout{0};
    unified_bootstrapper::register_shutdown_hook(
        "timeout_check",
        [&received_timeout](std::chrono::milliseconds timeout) {
            received_timeout = timeout;
        }
    );

    unified_bootstrapper::shutdown(std::chrono::seconds(5));

    // Should receive remaining timeout (approximately, due to default hook execution time)
    EXPECT_GT(received_timeout.count(), 0);
    EXPECT_LE(received_timeout.count(), 5000);
}

TEST_F(UnifiedBootstrapperTest, ShutdownHook_ExceptionIgnored) {
    unified_bootstrapper::initialize();

    bool second_hook_called = false;

    unified_bootstrapper::register_shutdown_hook(
        "throwing_hook",
        [](std::chrono::milliseconds) {
            throw std::runtime_error("Hook exception");
        }
    );

    unified_bootstrapper::register_shutdown_hook(
        "second_hook",
        [&second_hook_called](std::chrono::milliseconds) {
            second_hook_called = true;
        }
    );

    // Should not throw and should complete shutdown
    EXPECT_NO_THROW(unified_bootstrapper::shutdown());

    // Other hooks should still be called (in reverse order)
    EXPECT_TRUE(second_hook_called);
}

// ============================================================================
// Shutdown Request Tests
// ============================================================================

TEST_F(UnifiedBootstrapperTest, IsShutdownRequested_InitiallyFalse) {
    unified_bootstrapper::initialize();

    EXPECT_FALSE(unified_bootstrapper::is_shutdown_requested());
}

TEST_F(UnifiedBootstrapperTest, RequestShutdown_SetsFlag) {
    unified_bootstrapper::initialize();

    unified_bootstrapper::request_shutdown(false);

    EXPECT_TRUE(unified_bootstrapper::is_shutdown_requested());
}

TEST_F(UnifiedBootstrapperTest, RequestShutdown_WithTrigger_CallsShutdown) {
    unified_bootstrapper::initialize();

    unified_bootstrapper::request_shutdown(true);

    EXPECT_FALSE(unified_bootstrapper::is_initialized());
}

// ============================================================================
// Options Tests
// ============================================================================

TEST_F(UnifiedBootstrapperTest, GetOptions_ReturnsCurrentOptions) {
    bootstrapper_options opts;
    opts.enable_logging = true;
    opts.enable_monitoring = false;
    opts.enable_database = true;
    opts.enable_network = false;
    opts.config_path = "/path/to/config.yaml";
    opts.shutdown_timeout = std::chrono::milliseconds{10000};

    unified_bootstrapper::initialize(opts);

    auto retrieved = unified_bootstrapper::get_options();

    EXPECT_EQ(retrieved.enable_logging, true);
    EXPECT_EQ(retrieved.enable_monitoring, false);
    EXPECT_EQ(retrieved.enable_database, true);
    EXPECT_EQ(retrieved.enable_network, false);
    EXPECT_EQ(retrieved.config_path, "/path/to/config.yaml");
    EXPECT_EQ(retrieved.shutdown_timeout.count(), 10000);
}

TEST_F(UnifiedBootstrapperTest, GetOptions_WithoutInitialize_ReturnsDefaults) {
    auto opts = unified_bootstrapper::get_options();

    // Should return default-constructed options
    EXPECT_EQ(opts.enable_logging, true);
    EXPECT_EQ(opts.enable_monitoring, true);
    EXPECT_EQ(opts.enable_database, false);
    EXPECT_EQ(opts.enable_network, false);
}

// ============================================================================
// Reinitialization Tests
// ============================================================================

TEST_F(UnifiedBootstrapperTest, Reinitialize_AfterShutdown_Succeeds) {
    // First initialization
    unified_bootstrapper::initialize();
    unified_bootstrapper::services().register_type<ITestService, TestServiceImpl>();

    // Shutdown
    unified_bootstrapper::shutdown();

    // Second initialization
    auto result = unified_bootstrapper::initialize();

    EXPECT_TRUE(result.is_ok());
    EXPECT_TRUE(unified_bootstrapper::is_initialized());

    // Previous registrations should be gone
    EXPECT_FALSE(unified_bootstrapper::services().is_registered<ITestService>());
}

// ============================================================================
// Thread Safety Tests
// ============================================================================

TEST_F(UnifiedBootstrapperTest, IsInitialized_ThreadSafe) {
    unified_bootstrapper::initialize();

    constexpr int NUM_THREADS = 10;
    constexpr int ITERATIONS = 100;
    std::vector<std::thread> threads;
    std::atomic<int> check_count{0};

    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back([&check_count]() {
            for (int j = 0; j < ITERATIONS; ++j) {
                if (unified_bootstrapper::is_initialized()) {
                    check_count++;
                }
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_EQ(check_count, NUM_THREADS * ITERATIONS);
}

TEST_F(UnifiedBootstrapperTest, IsShutdownRequested_ThreadSafe) {
    unified_bootstrapper::initialize();

    constexpr int NUM_THREADS = 10;
    std::vector<std::thread> threads;
    std::atomic<bool> all_checked{false};

    // One thread requests shutdown
    threads.emplace_back([]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        unified_bootstrapper::request_shutdown(false);
    });

    // Other threads check the flag
    for (int i = 0; i < NUM_THREADS - 1; ++i) {
        threads.emplace_back([&all_checked]() {
            while (!unified_bootstrapper::is_shutdown_requested()) {
                std::this_thread::yield();
            }
            all_checked = true;
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_TRUE(unified_bootstrapper::is_shutdown_requested());
}
