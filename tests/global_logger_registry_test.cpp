// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file global_logger_registry_test.cpp
 * @brief Unit tests for GlobalLoggerRegistry implementation (Issue #174)
 *
 * These tests verify:
 * - Single-threaded registration and retrieval
 * - Multi-threaded concurrent access scenarios
 * - Factory-based deferred creation
 * - NullLogger fallback behavior
 * - Edge cases and error handling
 */

#include <gtest/gtest.h>
#include <kcenon/common/interfaces/global_logger_registry.h>

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

using namespace kcenon::common;
using namespace kcenon::common::interfaces;

// ============================================================================
// Test Logger Implementation
// ============================================================================

/**
 * @brief Test logger implementation that tracks log messages.
 *
 * @note Issue #177: Updated to support source_location-based logging.
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

    /**
     * @brief Log with source_location (Issue #177 - preferred method)
     */
    VoidResult log(log_level level,
                   std::string_view message,
                   const source_location& /*loc*/ = source_location::current()) override {
        return log(level, std::string(message));
    }

// Suppress deprecation warning for implementing the deprecated interface method
#if defined(__GNUC__) || defined(__clang__)
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#elif defined(_MSC_VER)
    #pragma warning(push)
    #pragma warning(disable: 4996)
#endif

    VoidResult log(log_level level,
                   const std::string& message,
                   const std::string& /*file*/,
                   int /*line*/,
                   const std::string& /*function*/) override {
        return log(level, message);
    }

#if defined(__GNUC__) || defined(__clang__)
    #pragma GCC diagnostic pop
#elif defined(_MSC_VER)
    #pragma warning(pop)
#endif

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

// Factory creation counter for testing
static std::atomic<int> g_factory_call_count{0};

// ============================================================================
// Test Fixtures
// ============================================================================

class GlobalLoggerRegistryTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Clear the global registry before each test
        GlobalLoggerRegistry::instance().clear();
        g_factory_call_count = 0;
    }

    void TearDown() override {
        // Clean up after each test
        GlobalLoggerRegistry::instance().clear();
    }
};

// ============================================================================
// Singleton Tests
// ============================================================================

TEST_F(GlobalLoggerRegistryTest, Instance_ReturnsSameInstance) {
    auto& instance1 = GlobalLoggerRegistry::instance();
    auto& instance2 = GlobalLoggerRegistry::instance();

    EXPECT_EQ(&instance1, &instance2);
}

TEST_F(GlobalLoggerRegistryTest, GetRegistry_ReturnsSameInstance) {
    auto& registry = get_registry();
    auto& instance = GlobalLoggerRegistry::instance();

    EXPECT_EQ(&registry, &instance);
}

// ============================================================================
// Default Logger Tests
// ============================================================================

TEST_F(GlobalLoggerRegistryTest, GetDefaultLogger_ReturnsNullLoggerWhenNotSet) {
    auto logger = GlobalLoggerRegistry::instance().get_default_logger();

    EXPECT_NE(logger, nullptr);
    // NullLogger should have level 'off'
    EXPECT_EQ(logger->get_level(), log_level::off);
    // NullLogger should report as disabled for all levels
    EXPECT_FALSE(logger->is_enabled(log_level::trace));
    EXPECT_FALSE(logger->is_enabled(log_level::critical));
}

TEST_F(GlobalLoggerRegistryTest, SetDefaultLogger_Success) {
    auto test_logger = std::make_shared<TestLogger>("default");
    auto result = GlobalLoggerRegistry::instance().set_default_logger(test_logger);

    EXPECT_TRUE(result.is_ok());
    EXPECT_TRUE(GlobalLoggerRegistry::instance().has_default_logger());

    auto retrieved = GlobalLoggerRegistry::instance().get_default_logger();
    EXPECT_EQ(retrieved, test_logger);
}

TEST_F(GlobalLoggerRegistryTest, SetDefaultLogger_NullReturnsError) {
    auto result = GlobalLoggerRegistry::instance().set_default_logger(nullptr);

    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error().code, error_codes::INVALID_ARGUMENT);
}

TEST_F(GlobalLoggerRegistryTest, GetLogger_ConvenienceFunction_ReturnsDefaultLogger) {
    auto test_logger = std::make_shared<TestLogger>("default");
    GlobalLoggerRegistry::instance().set_default_logger(test_logger);

    auto retrieved = get_logger();
    EXPECT_EQ(retrieved, test_logger);
}

// ============================================================================
// Named Logger Tests
// ============================================================================

TEST_F(GlobalLoggerRegistryTest, RegisterLogger_Success) {
    auto test_logger = std::make_shared<TestLogger>("network");
    auto result = GlobalLoggerRegistry::instance().register_logger("network", test_logger);

    EXPECT_TRUE(result.is_ok());
    EXPECT_TRUE(GlobalLoggerRegistry::instance().has_logger("network"));
    EXPECT_EQ(GlobalLoggerRegistry::instance().size(), 1);
}

TEST_F(GlobalLoggerRegistryTest, RegisterLogger_EmptyNameReturnsError) {
    auto test_logger = std::make_shared<TestLogger>();
    auto result = GlobalLoggerRegistry::instance().register_logger("", test_logger);

    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error().code, error_codes::INVALID_ARGUMENT);
}

TEST_F(GlobalLoggerRegistryTest, RegisterLogger_NullLoggerReturnsError) {
    auto result = GlobalLoggerRegistry::instance().register_logger("test", nullptr);

    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error().code, error_codes::INVALID_ARGUMENT);
}

TEST_F(GlobalLoggerRegistryTest, RegisterLogger_ReplacesExisting) {
    auto logger1 = std::make_shared<TestLogger>("first");
    auto logger2 = std::make_shared<TestLogger>("second");

    GlobalLoggerRegistry::instance().register_logger("test", logger1);
    GlobalLoggerRegistry::instance().register_logger("test", logger2);

    auto retrieved = GlobalLoggerRegistry::instance().get_logger("test");
    auto test_logger = std::dynamic_pointer_cast<TestLogger>(retrieved);

    EXPECT_NE(test_logger, nullptr);
    EXPECT_EQ(test_logger->name(), "second");
}

TEST_F(GlobalLoggerRegistryTest, GetLogger_ReturnsRegisteredLogger) {
    auto test_logger = std::make_shared<TestLogger>("database");
    GlobalLoggerRegistry::instance().register_logger("database", test_logger);

    auto retrieved = GlobalLoggerRegistry::instance().get_logger("database");
    EXPECT_EQ(retrieved, test_logger);
}

TEST_F(GlobalLoggerRegistryTest, GetLogger_ReturnsNullLoggerForUnregistered) {
    auto logger = GlobalLoggerRegistry::instance().get_logger("nonexistent");

    EXPECT_NE(logger, nullptr);
    EXPECT_EQ(logger->get_level(), log_level::off);
}

TEST_F(GlobalLoggerRegistryTest, GetLogger_ConvenienceFunction_ReturnsNamedLogger) {
    auto test_logger = std::make_shared<TestLogger>("api");
    GlobalLoggerRegistry::instance().register_logger("api", test_logger);

    auto retrieved = get_logger("api");
    EXPECT_EQ(retrieved, test_logger);
}

TEST_F(GlobalLoggerRegistryTest, UnregisterLogger_Success) {
    auto test_logger = std::make_shared<TestLogger>("temp");
    GlobalLoggerRegistry::instance().register_logger("temp", test_logger);

    EXPECT_TRUE(GlobalLoggerRegistry::instance().has_logger("temp"));

    auto result = GlobalLoggerRegistry::instance().unregister_logger("temp");
    EXPECT_TRUE(result.is_ok());
    EXPECT_FALSE(GlobalLoggerRegistry::instance().has_logger("temp"));
}

TEST_F(GlobalLoggerRegistryTest, UnregisterLogger_NonexistentSucceeds) {
    auto result = GlobalLoggerRegistry::instance().unregister_logger("nonexistent");
    EXPECT_TRUE(result.is_ok());
}

// ============================================================================
// Factory Tests
// ============================================================================

TEST_F(GlobalLoggerRegistryTest, RegisterFactory_LazyInitialization) {
    auto result = GlobalLoggerRegistry::instance().register_factory("lazy", []() {
        g_factory_call_count++;
        return std::make_shared<TestLogger>("lazy-created");
    });

    EXPECT_TRUE(result.is_ok());
    EXPECT_TRUE(GlobalLoggerRegistry::instance().has_logger("lazy"));
    EXPECT_EQ(g_factory_call_count.load(), 0); // Factory not called yet

    // First retrieval should call factory
    auto logger1 = GlobalLoggerRegistry::instance().get_logger("lazy");
    EXPECT_EQ(g_factory_call_count.load(), 1);

    // Subsequent retrievals should not call factory
    auto logger2 = GlobalLoggerRegistry::instance().get_logger("lazy");
    EXPECT_EQ(g_factory_call_count.load(), 1);

    // Same instance should be returned
    EXPECT_EQ(logger1, logger2);
}

TEST_F(GlobalLoggerRegistryTest, RegisterFactory_EmptyNameReturnsError) {
    auto result = GlobalLoggerRegistry::instance().register_factory("", []() {
        return std::make_shared<TestLogger>();
    });

    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error().code, error_codes::INVALID_ARGUMENT);
}

TEST_F(GlobalLoggerRegistryTest, RegisterFactory_NullFactoryReturnsError) {
    auto result = GlobalLoggerRegistry::instance().register_factory("test", nullptr);

    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error().code, error_codes::INVALID_ARGUMENT);
}

TEST_F(GlobalLoggerRegistryTest, RegisterFactory_FailsIfLoggerExists) {
    auto existing = std::make_shared<TestLogger>("existing");
    GlobalLoggerRegistry::instance().register_logger("test", existing);

    auto result = GlobalLoggerRegistry::instance().register_factory("test", []() {
        return std::make_shared<TestLogger>("from-factory");
    });

    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error().code, error_codes::ALREADY_EXISTS);
}

TEST_F(GlobalLoggerRegistryTest, RegisterLogger_RemovesFactory) {
    GlobalLoggerRegistry::instance().register_factory("test", []() {
        g_factory_call_count++;
        return std::make_shared<TestLogger>("from-factory");
    });

    // Register a concrete logger with same name
    auto concrete = std::make_shared<TestLogger>("concrete");
    GlobalLoggerRegistry::instance().register_logger("test", concrete);

    // Factory should not be called
    auto logger = GlobalLoggerRegistry::instance().get_logger("test");
    EXPECT_EQ(g_factory_call_count.load(), 0);

    auto test_logger = std::dynamic_pointer_cast<TestLogger>(logger);
    EXPECT_EQ(test_logger->name(), "concrete");
}

TEST_F(GlobalLoggerRegistryTest, SetDefaultFactory_LazyInitialization) {
    auto result = GlobalLoggerRegistry::instance().set_default_factory([]() {
        g_factory_call_count++;
        return std::make_shared<TestLogger>("default-lazy");
    });

    EXPECT_TRUE(result.is_ok());
    EXPECT_TRUE(GlobalLoggerRegistry::instance().has_default_logger());
    EXPECT_EQ(g_factory_call_count.load(), 0);

    // First retrieval should call factory
    auto logger1 = GlobalLoggerRegistry::instance().get_default_logger();
    EXPECT_EQ(g_factory_call_count.load(), 1);

    // Subsequent retrievals should not call factory
    auto logger2 = GlobalLoggerRegistry::instance().get_default_logger();
    EXPECT_EQ(g_factory_call_count.load(), 1);
}

TEST_F(GlobalLoggerRegistryTest, SetDefaultFactory_NullFactoryReturnsError) {
    auto result = GlobalLoggerRegistry::instance().set_default_factory(nullptr);

    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error().code, error_codes::INVALID_ARGUMENT);
}

TEST_F(GlobalLoggerRegistryTest, SetDefaultFactory_FailsIfLoggerExists) {
    auto existing = std::make_shared<TestLogger>("existing-default");
    GlobalLoggerRegistry::instance().set_default_logger(existing);

    auto result = GlobalLoggerRegistry::instance().set_default_factory([]() {
        return std::make_shared<TestLogger>("from-factory");
    });

    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error().code, error_codes::ALREADY_EXISTS);
}

// ============================================================================
// NullLogger Tests
// ============================================================================

TEST_F(GlobalLoggerRegistryTest, NullLogger_AllOperationsSucceed) {
    auto null_logger = GlobalLoggerRegistry::null_logger();

    // All operations should succeed without throwing
    // Use string_view overload (Issue #177 - preferred)
    EXPECT_TRUE(null_logger->log(log_level::info, std::string_view("test")).is_ok());

// Suppress deprecation warning for testing the deprecated method
#if defined(__GNUC__) || defined(__clang__)
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#elif defined(_MSC_VER)
    #pragma warning(push)
    #pragma warning(disable: 4996)
#endif
    // Test legacy file/line/function overload
    EXPECT_TRUE(null_logger->log(log_level::error, std::string("test"),
                                  std::string("file.cpp"), 42, std::string("func")).is_ok());
#if defined(__GNUC__) || defined(__clang__)
    #pragma GCC diagnostic pop
#elif defined(_MSC_VER)
    #pragma warning(pop)
#endif

    log_entry entry;
    entry.level = log_level::warning;
    entry.message = "test entry";
    EXPECT_TRUE(null_logger->log(entry).is_ok());

    EXPECT_TRUE(null_logger->set_level(log_level::debug).is_ok());
    EXPECT_TRUE(null_logger->flush().is_ok());
}

TEST_F(GlobalLoggerRegistryTest, NullLogger_ReturnsSameInstance) {
    auto null1 = GlobalLoggerRegistry::null_logger();
    auto null2 = GlobalLoggerRegistry::null_logger();

    EXPECT_EQ(null1, null2);
}

TEST_F(GlobalLoggerRegistryTest, NullLogger_AlwaysDisabled) {
    auto null_logger = GlobalLoggerRegistry::null_logger();

    EXPECT_FALSE(null_logger->is_enabled(log_level::trace));
    EXPECT_FALSE(null_logger->is_enabled(log_level::debug));
    EXPECT_FALSE(null_logger->is_enabled(log_level::info));
    EXPECT_FALSE(null_logger->is_enabled(log_level::warning));
    EXPECT_FALSE(null_logger->is_enabled(log_level::error));
    EXPECT_FALSE(null_logger->is_enabled(log_level::critical));
}

// ============================================================================
// Clear and Size Tests
// ============================================================================

TEST_F(GlobalLoggerRegistryTest, Clear_RemovesAllLoggersAndFactories) {
    auto logger1 = std::make_shared<TestLogger>("l1");
    auto logger2 = std::make_shared<TestLogger>("l2");
    auto default_logger = std::make_shared<TestLogger>("default");

    GlobalLoggerRegistry::instance().register_logger("l1", logger1);
    GlobalLoggerRegistry::instance().register_logger("l2", logger2);
    GlobalLoggerRegistry::instance().set_default_logger(default_logger);
    GlobalLoggerRegistry::instance().register_factory("f1", []() {
        return std::make_shared<TestLogger>("f1");
    });

    EXPECT_GT(GlobalLoggerRegistry::instance().size(), 0);
    EXPECT_TRUE(GlobalLoggerRegistry::instance().has_default_logger());

    GlobalLoggerRegistry::instance().clear();

    EXPECT_EQ(GlobalLoggerRegistry::instance().size(), 0);
    EXPECT_FALSE(GlobalLoggerRegistry::instance().has_default_logger());
    EXPECT_FALSE(GlobalLoggerRegistry::instance().has_logger("l1"));
    EXPECT_FALSE(GlobalLoggerRegistry::instance().has_logger("l2"));
    EXPECT_FALSE(GlobalLoggerRegistry::instance().has_logger("f1"));
}

TEST_F(GlobalLoggerRegistryTest, Size_ReturnsCorrectCount) {
    EXPECT_EQ(GlobalLoggerRegistry::instance().size(), 0);

    GlobalLoggerRegistry::instance().register_logger("a", std::make_shared<TestLogger>());
    EXPECT_EQ(GlobalLoggerRegistry::instance().size(), 1);

    GlobalLoggerRegistry::instance().register_logger("b", std::make_shared<TestLogger>());
    EXPECT_EQ(GlobalLoggerRegistry::instance().size(), 2);

    GlobalLoggerRegistry::instance().register_factory("c", []() {
        return std::make_shared<TestLogger>();
    });
    EXPECT_EQ(GlobalLoggerRegistry::instance().size(), 3);

    // Default logger is not counted in size
    GlobalLoggerRegistry::instance().set_default_logger(std::make_shared<TestLogger>());
    EXPECT_EQ(GlobalLoggerRegistry::instance().size(), 3);
}

// ============================================================================
// Thread Safety Tests
// ============================================================================

TEST_F(GlobalLoggerRegistryTest, ConcurrentRegistration) {
    constexpr int num_threads = 10;
    constexpr int loggers_per_thread = 100;

    std::vector<std::thread> threads;
    std::atomic<int> success_count{0};

    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([t, &success_count]() {
            for (int i = 0; i < loggers_per_thread; ++i) {
                std::string name = "logger_" + std::to_string(t) + "_" + std::to_string(i);
                auto logger = std::make_shared<TestLogger>(name);
                auto result = GlobalLoggerRegistry::instance().register_logger(name, logger);
                if (result.is_ok()) {
                    success_count++;
                }
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    EXPECT_EQ(success_count.load(), num_threads * loggers_per_thread);
    EXPECT_EQ(GlobalLoggerRegistry::instance().size(),
              static_cast<size_t>(num_threads * loggers_per_thread));
}

TEST_F(GlobalLoggerRegistryTest, ConcurrentRetrieval) {
    // Pre-register loggers
    constexpr int num_loggers = 10;
    for (int i = 0; i < num_loggers; ++i) {
        std::string name = "logger_" + std::to_string(i);
        GlobalLoggerRegistry::instance().register_logger(
            name, std::make_shared<TestLogger>(name));
    }

    constexpr int num_threads = 10;
    constexpr int retrievals_per_thread = 1000;

    std::vector<std::thread> threads;
    std::atomic<int> null_logger_count{0};

    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&null_logger_count]() {
            for (int i = 0; i < retrievals_per_thread; ++i) {
                std::string name = "logger_" + std::to_string(i % num_loggers);
                auto logger = GlobalLoggerRegistry::instance().get_logger(name);
                if (logger->get_level() == log_level::off) {
                    null_logger_count++;
                }
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    // All retrievals should return actual loggers, not NullLogger
    EXPECT_EQ(null_logger_count.load(), 0);
}

TEST_F(GlobalLoggerRegistryTest, ConcurrentFactoryCreation) {
    // Register a factory
    std::atomic<int> factory_calls{0};
    GlobalLoggerRegistry::instance().register_factory("concurrent", [&factory_calls]() {
        factory_calls++;
        // Simulate slow factory
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        return std::make_shared<TestLogger>("from-factory");
    });

    constexpr int num_threads = 10;
    std::vector<std::thread> threads;
    std::vector<std::shared_ptr<ILogger>> results(num_threads);

    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([t, &results]() {
            results[t] = GlobalLoggerRegistry::instance().get_logger("concurrent");
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    // Factory should only be called once despite concurrent access
    EXPECT_EQ(factory_calls.load(), 1);

    // All threads should get the same logger instance
    for (int i = 1; i < num_threads; ++i) {
        EXPECT_EQ(results[0], results[i]);
    }
}

TEST_F(GlobalLoggerRegistryTest, ConcurrentDefaultLoggerAccess) {
    std::atomic<int> factory_calls{0};
    GlobalLoggerRegistry::instance().set_default_factory([&factory_calls]() {
        factory_calls++;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        return std::make_shared<TestLogger>("default-from-factory");
    });

    constexpr int num_threads = 10;
    std::vector<std::thread> threads;
    std::vector<std::shared_ptr<ILogger>> results(num_threads);

    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([t, &results]() {
            results[t] = GlobalLoggerRegistry::instance().get_default_logger();
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    // Factory should only be called once
    EXPECT_EQ(factory_calls.load(), 1);

    // All threads should get the same logger instance
    for (int i = 1; i < num_threads; ++i) {
        EXPECT_EQ(results[0], results[i]);
    }
}

TEST_F(GlobalLoggerRegistryTest, ConcurrentMixedOperations) {
    constexpr int num_threads = 8;
    constexpr int operations_per_thread = 500;

    std::vector<std::thread> threads;

    // Writer threads (register/unregister)
    for (int t = 0; t < num_threads / 2; ++t) {
        threads.emplace_back([t]() {
            for (int i = 0; i < operations_per_thread; ++i) {
                std::string name = "mixed_" + std::to_string((t * operations_per_thread + i) % 50);
                if (i % 2 == 0) {
                    GlobalLoggerRegistry::instance().register_logger(
                        name, std::make_shared<TestLogger>(name));
                } else {
                    GlobalLoggerRegistry::instance().unregister_logger(name);
                }
            }
        });
    }

    // Reader threads (get_logger)
    for (int t = num_threads / 2; t < num_threads; ++t) {
        threads.emplace_back([]() {
            for (int i = 0; i < operations_per_thread; ++i) {
                std::string name = "mixed_" + std::to_string(i % 50);
                auto logger = GlobalLoggerRegistry::instance().get_logger(name);
                // Just ensure we can call a method without crashing
                (void)logger->get_level();
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    // Test passed if no crashes or deadlocks occurred
    SUCCEED();
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST_F(GlobalLoggerRegistryTest, LoggingThroughRegistry) {
    auto test_logger = std::make_shared<TestLogger>("integration");
    GlobalLoggerRegistry::instance().register_logger("integration", test_logger);

    auto logger = get_logger("integration");
    // Use string_view overload (Issue #177)
    logger->log(log_level::info, std::string_view("Test message"));

    EXPECT_EQ(test_logger->message_count(), 1);
    auto [level, message] = test_logger->last_message();
    EXPECT_EQ(level, log_level::info);
    EXPECT_EQ(message, "Test message");
}

TEST_F(GlobalLoggerRegistryTest, MultipleNamedLoggers) {
    auto network_logger = std::make_shared<TestLogger>("network");
    auto database_logger = std::make_shared<TestLogger>("database");
    auto api_logger = std::make_shared<TestLogger>("api");

    GlobalLoggerRegistry::instance().register_logger("network", network_logger);
    GlobalLoggerRegistry::instance().register_logger("database", database_logger);
    GlobalLoggerRegistry::instance().register_logger("api", api_logger);

    // Use string_view overload (Issue #177)
    get_logger("network")->log(log_level::info, std::string_view("Network message"));
    get_logger("database")->log(log_level::warning, std::string_view("Database message"));
    get_logger("api")->log(log_level::error, std::string_view("API message"));

    EXPECT_EQ(network_logger->message_count(), 1);
    EXPECT_EQ(database_logger->message_count(), 1);
    EXPECT_EQ(api_logger->message_count(), 1);

    EXPECT_EQ(network_logger->last_message().first, log_level::info);
    EXPECT_EQ(database_logger->last_message().first, log_level::warning);
    EXPECT_EQ(api_logger->last_message().first, log_level::error);
}
