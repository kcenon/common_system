// BSD 3-Clause License
//
// Copyright (c) 2021-2025, 🍀☀🌕🌥 🌊
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
// this
//    list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its
//    contributors may be used to endorse or promote products derived from
//    this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

/**
 * @file runtime_binding_integration_test.cpp
 * @brief Integration tests for the runtime binding pattern (Issue #178)
 *
 * This file contains comprehensive integration tests that verify the runtime
 * binding pattern works correctly across all systems. The tests cover:
 * - GlobalLoggerRegistry functionality
 * - SystemBootstrapper initialization and shutdown
 * - Cross-system communication via shared logger
 * - Log level conversion correctness
 * - Thread-safe concurrent access patterns
 */

#include "system_fixture.h"
#include "test_helpers.h"

#include <kcenon/common/bootstrap/system_bootstrapper.h>
#include <kcenon/common/interfaces/global_logger_registry.h>
#include <kcenon/common/logging/log_functions.h>
#include <kcenon/common/logging/log_macros.h>
#include <kcenon/common/patterns/result.h>

#include <atomic>
#include <chrono>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

using namespace integration_tests;
using namespace kcenon::common;
using namespace kcenon::common::interfaces;
using namespace kcenon::common::bootstrap;

// ============================================================================
// Thread-Safe Test Logger Implementation
// ============================================================================

/**
 * @brief Thread-safe logger implementation for integration testing.
 *
 * This logger captures all log messages in a thread-safe manner, allowing
 * verification of logging behavior across multiple threads and systems.
 */
class ThreadSafeTestLogger : public ILogger {
public:
  explicit ThreadSafeTestLogger(const std::string &name = "test")
      : name_(name), level_(log_level::trace) {}

  VoidResult log(log_level level, const std::string &message) override {
    std::lock_guard<std::mutex> lock(mutex_);
    entries_.push_back({level, message, "", 0, ""});
    return VoidResult::ok({});
  }

  VoidResult
  log(log_level level, std::string_view message,
      const source_location &loc = source_location::current()) override {
    std::lock_guard<std::mutex> lock(mutex_);
    entries_.push_back({level, std::string(message), loc.file_name(),
                        static_cast<int>(loc.line()), loc.function_name()});
    return VoidResult::ok({});
  }

// Suppress deprecation warning for implementing the deprecated interface method
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#elif defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4996)
#endif

  VoidResult log(log_level level, const std::string &message,
                 const std::string &file, int line,
                 const std::string &function) override {
    std::lock_guard<std::mutex> lock(mutex_);
    entries_.push_back({level, message, file, line, function});
    return VoidResult::ok({});
  }

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#pragma warning(pop)
#endif

  VoidResult log(const log_entry &entry) override {
    std::lock_guard<std::mutex> lock(mutex_);
    entries_.push_back(
        {entry.level, entry.message, entry.file, entry.line, entry.function});
    return VoidResult::ok({});
  }

  bool is_enabled(log_level level) const override { return level >= level_; }

  VoidResult set_level(log_level level) override {
    level_ = level;
    return VoidResult::ok({});
  }

  log_level get_level() const override { return level_; }

  VoidResult flush() override {
    flush_count_++;
    return VoidResult::ok({});
  }

  // Test accessors
  const std::string &name() const { return name_; }

  size_t log_count() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return entries_.size();
  }

  int flush_count() const { return flush_count_.load(); }

  struct LogEntry {
    log_level level;
    std::string message;
    std::string file;
    int line;
    std::string function;
  };

  std::vector<LogEntry> get_entries() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return entries_;
  }

  void clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    entries_.clear();
    flush_count_ = 0;
  }

  size_t count_messages_containing(const std::string &substring) const {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t count = 0;
    for (const auto &entry : entries_) {
      if (entry.message.find(substring) != std::string::npos) {
        ++count;
      }
    }
    return count;
  }

private:
  std::string name_;
  log_level level_;
  mutable std::mutex mutex_;
  std::vector<LogEntry> entries_;
  std::atomic<int> flush_count_{0};
};

// ============================================================================
// Mock System Components for Cross-System Testing
// ============================================================================

/**
 * @brief Simulates a component from thread_system that performs logging.
 */
class MockThreadSystem {
public:
  void do_work() {
    auto logger = GlobalLoggerRegistry::instance().get_default_logger();
    if (logger) {
      logger->log(log_level::info,
                  std::string_view{"ThreadSystem: executing task"});
    }
  }

  void spawn_workers(int count) {
    auto logger = GlobalLoggerRegistry::instance().get_default_logger();
    for (int i = 0; i < count; ++i) {
      if (logger) {
        std::string msg = "ThreadSystem: spawned worker " + std::to_string(i);
        logger->log(log_level::debug, std::string_view{msg});
      }
    }
  }
};

/**
 * @brief Simulates a component from network_system that performs logging.
 */
class MockNetworkSystem {
public:
  void handle_connection() {
    auto logger = GlobalLoggerRegistry::instance().get_default_logger();
    if (logger) {
      logger->log(log_level::info,
                  std::string_view{"NetworkSystem: connection established"});
    }
  }

  void send_data(const std::string &data) {
    auto logger = GlobalLoggerRegistry::instance().get_default_logger();
    if (logger) {
      std::string msg =
          "NetworkSystem: sending " + std::to_string(data.size()) + " bytes";
      logger->log(log_level::debug, std::string_view{msg});
    }
  }
};

/**
 * @brief Simulates a component from database_system that performs logging.
 */
class MockDatabaseSystem {
public:
  void execute_query(const std::string &query) {
    auto logger = GlobalLoggerRegistry::instance().get_default_logger();
    if (logger) {
      std::string msg = "DatabaseSystem: executing query: " + query;
      logger->log(log_level::info, std::string_view{msg});
    }
  }

  void log_error(const std::string &error) {
    auto logger = GlobalLoggerRegistry::instance().get_default_logger();
    if (logger) {
      std::string msg = "DatabaseSystem: " + error;
      logger->log(log_level::error, std::string_view{msg});
    }
  }
};

// ============================================================================
// Test Fixtures
// ============================================================================

/**
 * @brief Fixture for GlobalLoggerRegistry integration tests.
 *
 * Ensures the registry is cleared before and after each test to prevent
 * cross-test contamination.
 */
class GlobalLoggerRegistryIntegrationTest : public SystemFixture {
protected:
  void SetUp() override {
    SystemFixture::SetUp();
    GlobalLoggerRegistry::instance().clear();
  }

  void TearDown() override {
    GlobalLoggerRegistry::instance().clear();
    SystemFixture::TearDown();
  }
};

/**
 * @brief Fixture for SystemBootstrapper integration tests.
 */
class SystemBootstrapperIntegrationTest : public SystemFixture {
protected:
  void SetUp() override {
    SystemFixture::SetUp();
    GlobalLoggerRegistry::instance().clear();
  }

  void TearDown() override {
    GlobalLoggerRegistry::instance().clear();
    SystemFixture::TearDown();
  }
};

/**
 * @brief Fixture for cross-system integration tests.
 */
class CrossSystemIntegrationTest : public SystemFixture {
protected:
  void SetUp() override {
    SystemFixture::SetUp();
    GlobalLoggerRegistry::instance().clear();
  }

  void TearDown() override {
    GlobalLoggerRegistry::instance().clear();
    SystemFixture::TearDown();
  }
};

/**
 * @brief Fixture for log level conversion tests.
 */
class LevelConversionIntegrationTest : public SystemFixture {
protected:
  void SetUp() override {
    SystemFixture::SetUp();
    GlobalLoggerRegistry::instance().clear();
  }

  void TearDown() override {
    GlobalLoggerRegistry::instance().clear();
    SystemFixture::TearDown();
  }
};

// ============================================================================
// GlobalLoggerRegistry Integration Tests
// ============================================================================

/**
 * @brief Verifies that multiple systems share the same logger instance.
 *
 * This test simulates the scenario where different system components
 * (thread, network, database) all log to the same unified logger.
 */
TEST_F(GlobalLoggerRegistryIntegrationTest, MultipleSystemsShareLogger) {
  // Setup: Create and register a shared logger
  auto logger = std::make_shared<ThreadSafeTestLogger>();
  auto result = GlobalLoggerRegistry::instance().set_default_logger(logger);
  ASSERT_TRUE(result.is_ok());

  // Create mock system components
  MockThreadSystem thread_sys;
  MockNetworkSystem network_sys;
  MockDatabaseSystem database_sys;

  // Execute operations from each system
  thread_sys.do_work();
  network_sys.handle_connection();
  database_sys.execute_query("SELECT 1");

  // Verify all logs went to the same logger
  EXPECT_EQ(logger->log_count(), 3);

  // Verify messages from each system are present
  EXPECT_EQ(logger->count_messages_containing("ThreadSystem"), 1);
  EXPECT_EQ(logger->count_messages_containing("NetworkSystem"), 1);
  EXPECT_EQ(logger->count_messages_containing("DatabaseSystem"), 1);
}

/**
 * @brief Verifies thread-safe concurrent access to GlobalLoggerRegistry.
 *
 * This test launches multiple threads that concurrently log messages
 * to verify the registry handles concurrent access correctly.
 */
TEST_F(GlobalLoggerRegistryIntegrationTest, ThreadSafeAccess) {
  // Setup: Create and register a thread-safe logger
  auto logger = std::make_shared<ThreadSafeTestLogger>();
  auto result = GlobalLoggerRegistry::instance().set_default_logger(logger);
  ASSERT_TRUE(result.is_ok());

  const int num_threads = 100;
  const int logs_per_thread = 10;
  std::vector<std::thread> threads;

  // Launch multiple threads that all log concurrently
  for (int i = 0; i < num_threads; ++i) {
    threads.emplace_back([i, logs_per_thread] {
      for (int j = 0; j < logs_per_thread; ++j) {
        auto log = GlobalLoggerRegistry::instance().get_default_logger();
        if (log) {
          log->log(log_level::info,
                   "Thread " + std::to_string(i) + " log " + std::to_string(j));
        }
      }
    });
  }

  // Wait for all threads to complete
  for (auto &t : threads) {
    t.join();
  }

  // Verify all logs were captured
  EXPECT_EQ(logger->log_count(), num_threads * logs_per_thread);
}

/**
 * @brief Verifies concurrent registration and retrieval operations.
 *
 * This test exercises the thread safety of registration operations
 * by having multiple threads register and retrieve loggers concurrently.
 */
TEST_F(GlobalLoggerRegistryIntegrationTest,
       ConcurrentRegistrationAndRetrieval) {
  const int num_threads = 50;
  std::atomic<int> successful_registrations{0};
  std::atomic<int> successful_retrievals{0};
  std::vector<std::thread> threads;

  // Half the threads register loggers, half retrieve them
  for (int i = 0; i < num_threads; ++i) {
    if (i % 2 == 0) {
      // Registration thread
      threads.emplace_back([i, &successful_registrations] {
        auto logger = std::make_shared<ThreadSafeTestLogger>("logger_" +
                                                             std::to_string(i));
        auto result = GlobalLoggerRegistry::instance().register_logger(
            "logger_" + std::to_string(i), logger);
        if (result.is_ok()) {
          successful_registrations++;
        }
      });
    } else {
      // Retrieval thread - tries to get previously registered loggers
      threads.emplace_back([i, &successful_retrievals] {
        // Yield to allow registrations to proceed
        std::this_thread::yield();
        for (int j = 0; j < i; j += 2) {
          auto logger = GlobalLoggerRegistry::instance().get_logger(
              "logger_" + std::to_string(j));
          if (logger && logger != GlobalLoggerRegistry::null_logger()) {
            successful_retrievals++;
          }
        }
      });
    }
  }

  // Wait for all threads
  for (auto &t : threads) {
    t.join();
  }

  // All registration threads should succeed
  EXPECT_EQ(successful_registrations.load(), num_threads / 2);
  // At least some retrievals should succeed
  EXPECT_GT(successful_retrievals.load(), 0);
}

/**
 * @brief Verifies factory-based lazy initialization works correctly.
 */
TEST_F(GlobalLoggerRegistryIntegrationTest, FactoryBasedLazyInitialization) {
  std::atomic<int> factory_call_count{0};

  // Register a factory instead of an instance
  auto result = GlobalLoggerRegistry::instance().register_factory(
      "lazy_logger", [&factory_call_count]() -> std::shared_ptr<ILogger> {
        factory_call_count++;
        return std::make_shared<ThreadSafeTestLogger>("lazy");
      });
  ASSERT_TRUE(result.is_ok());

  // Factory should not be called yet
  EXPECT_EQ(factory_call_count.load(), 0);

  // First retrieval should trigger factory
  auto logger1 = GlobalLoggerRegistry::instance().get_logger("lazy_logger");
  EXPECT_NE(logger1, nullptr);
  EXPECT_NE(logger1, GlobalLoggerRegistry::null_logger());
  EXPECT_EQ(factory_call_count.load(), 1);

  // Subsequent retrievals should return cached instance
  auto logger2 = GlobalLoggerRegistry::instance().get_logger("lazy_logger");
  EXPECT_EQ(logger1, logger2);
  EXPECT_EQ(factory_call_count.load(), 1); // Still 1, not 2
}

/**
 * @brief Verifies NullLogger is returned for unregistered logger names.
 */
TEST_F(GlobalLoggerRegistryIntegrationTest, NullLoggerFallback) {
  // Request a logger that doesn't exist
  auto logger = GlobalLoggerRegistry::instance().get_logger("nonexistent");

  // Should return NullLogger, not nullptr
  EXPECT_NE(logger, nullptr);
  EXPECT_EQ(logger, GlobalLoggerRegistry::null_logger());

  // NullLogger should be safe to use
  auto result =
      logger->log(log_level::info, std::string_view{"This should not crash"});
  EXPECT_TRUE(result.is_ok());

  // NullLogger should report as disabled
  EXPECT_FALSE(logger->is_enabled(log_level::trace));
  EXPECT_FALSE(logger->is_enabled(log_level::critical));
}

// ============================================================================
// SystemBootstrapper Integration Tests
// ============================================================================

/**
 * @brief Verifies basic initialization and shutdown sequence.
 */
TEST_F(SystemBootstrapperIntegrationTest, InitializeAndShutdown) {
  auto logger = std::make_shared<ThreadSafeTestLogger>();

  SystemBootstrapper bootstrapper;
  bootstrapper.with_default_logger([logger]() { return logger; });

  // Initialize
  auto init_result = bootstrapper.initialize();
  ASSERT_TRUE(init_result.is_ok());
  EXPECT_TRUE(bootstrapper.is_initialized());

  // Verify logger is available through GlobalLoggerRegistry
  auto retrieved = GlobalLoggerRegistry::instance().get_default_logger();
  EXPECT_NE(retrieved, nullptr);
  EXPECT_NE(retrieved, GlobalLoggerRegistry::null_logger());

  // Shutdown
  bootstrapper.shutdown();
  EXPECT_FALSE(bootstrapper.is_initialized());
}

/**
 * @brief Verifies shutdown hooks execute in LIFO order.
 */
TEST_F(SystemBootstrapperIntegrationTest, ShutdownHooksExecuteInOrder) {
  std::vector<int> execution_order;
  std::mutex order_mutex;

  SystemBootstrapper bootstrapper;

  // Register shutdown hooks
  bootstrapper.on_shutdown([&execution_order, &order_mutex]() {
    std::lock_guard<std::mutex> lock(order_mutex);
    execution_order.push_back(1);
  });

  bootstrapper.on_shutdown([&execution_order, &order_mutex]() {
    std::lock_guard<std::mutex> lock(order_mutex);
    execution_order.push_back(2);
  });

  bootstrapper.on_shutdown([&execution_order, &order_mutex]() {
    std::lock_guard<std::mutex> lock(order_mutex);
    execution_order.push_back(3);
  });

  // Initialize and shutdown
  auto init_result = bootstrapper.initialize();
  ASSERT_TRUE(init_result.is_ok());

  bootstrapper.shutdown();

  // Verify LIFO order: 3, 2, 1
  ASSERT_EQ(execution_order.size(), 3);
  EXPECT_EQ(execution_order[0], 3);
  EXPECT_EQ(execution_order[1], 2);
  EXPECT_EQ(execution_order[2], 1);
}

/**
 * @brief Verifies initialization hooks execute in registration order.
 */
TEST_F(SystemBootstrapperIntegrationTest, InitializationHooksExecuteInOrder) {
  std::vector<int> execution_order;
  std::mutex order_mutex;

  SystemBootstrapper bootstrapper;

  // Register initialization hooks
  bootstrapper.on_initialize([&execution_order, &order_mutex]() {
    std::lock_guard<std::mutex> lock(order_mutex);
    execution_order.push_back(1);
  });

  bootstrapper.on_initialize([&execution_order, &order_mutex]() {
    std::lock_guard<std::mutex> lock(order_mutex);
    execution_order.push_back(2);
  });

  bootstrapper.on_initialize([&execution_order, &order_mutex]() {
    std::lock_guard<std::mutex> lock(order_mutex);
    execution_order.push_back(3);
  });

  // Initialize
  auto init_result = bootstrapper.initialize();
  ASSERT_TRUE(init_result.is_ok());

  // Verify FIFO order: 1, 2, 3
  ASSERT_EQ(execution_order.size(), 3);
  EXPECT_EQ(execution_order[0], 1);
  EXPECT_EQ(execution_order[1], 2);
  EXPECT_EQ(execution_order[2], 3);

  bootstrapper.shutdown();
}

/**
 * @brief Verifies multiple named loggers can be registered via bootstrapper.
 */
TEST_F(SystemBootstrapperIntegrationTest, MultipleNamedLoggers) {
  auto logger1 = std::make_shared<ThreadSafeTestLogger>("logger1");
  auto logger2 = std::make_shared<ThreadSafeTestLogger>("logger2");
  auto default_logger = std::make_shared<ThreadSafeTestLogger>("default");

  SystemBootstrapper bootstrapper;
  bootstrapper
      .with_default_logger([default_logger]() { return default_logger; })
      .with_logger("app", [logger1]() { return logger1; })
      .with_logger("audit", [logger2]() { return logger2; });

  auto init_result = bootstrapper.initialize();
  ASSERT_TRUE(init_result.is_ok());

  // Verify all loggers are accessible
  auto retrieved_default =
      GlobalLoggerRegistry::instance().get_default_logger();
  auto retrieved_app = GlobalLoggerRegistry::instance().get_logger("app");
  auto retrieved_audit = GlobalLoggerRegistry::instance().get_logger("audit");

  EXPECT_NE(retrieved_default, GlobalLoggerRegistry::null_logger());
  EXPECT_NE(retrieved_app, GlobalLoggerRegistry::null_logger());
  EXPECT_NE(retrieved_audit, GlobalLoggerRegistry::null_logger());

  // Log to each and verify
  retrieved_default->log(log_level::info, std::string_view{"default message"});
  retrieved_app->log(log_level::info, std::string_view{"app message"});
  retrieved_audit->log(log_level::info, std::string_view{"audit message"});

  EXPECT_EQ(default_logger->log_count(), 1);
  EXPECT_EQ(logger1->log_count(), 1);
  EXPECT_EQ(logger2->log_count(), 1);

  bootstrapper.shutdown();
}

/**
 * @brief Verifies double initialization is prevented.
 */
TEST_F(SystemBootstrapperIntegrationTest, DoubleInitializationPrevented) {
  SystemBootstrapper bootstrapper;

  auto result1 = bootstrapper.initialize();
  ASSERT_TRUE(result1.is_ok());
  EXPECT_TRUE(bootstrapper.is_initialized());

  // Second initialization should fail
  auto result2 = bootstrapper.initialize();
  EXPECT_TRUE(result2.is_err());

  bootstrapper.shutdown();
}

/**
 * @brief Verifies RAII shutdown on destruction.
 */
TEST_F(SystemBootstrapperIntegrationTest, RAIIShutdownOnDestruction) {
  std::atomic<bool> shutdown_called{false};

  {
    SystemBootstrapper bootstrapper;
    bootstrapper.on_shutdown([&shutdown_called]() { shutdown_called = true; });

    auto init_result = bootstrapper.initialize();
    ASSERT_TRUE(init_result.is_ok());

    // Bootstrapper goes out of scope here
  }

  // Shutdown hook should have been called
  EXPECT_TRUE(shutdown_called.load());
}

// ============================================================================
// Cross-System Integration Tests
// ============================================================================

/**
 * @brief Verifies logging from multiple simulated systems routes to unified
 * logger.
 */
TEST_F(CrossSystemIntegrationTest, LoggingFromMultipleSystems) {
  auto logger = std::make_shared<ThreadSafeTestLogger>();

  // Initialize using SystemBootstrapper
  SystemBootstrapper bootstrapper;
  bootstrapper.with_default_logger([logger]() { return logger; });

  auto init_result = bootstrapper.initialize();
  ASSERT_TRUE(init_result.is_ok());

  // Create mock implementations of different systems
  MockThreadSystem thread_sys;
  MockNetworkSystem network_sys;
  MockDatabaseSystem database_sys;

  // Each system logs using the common interface
  thread_sys.do_work();
  thread_sys.spawn_workers(3);
  network_sys.handle_connection();
  network_sys.send_data("test data");
  database_sys.execute_query("SELECT * FROM users");
  database_sys.log_error("Connection timeout");

  // Verify all logs are captured:
  // - thread_sys.do_work() = 1
  // - thread_sys.spawn_workers(3) = 3
  // - network_sys.handle_connection() = 1
  // - network_sys.send_data() = 1
  // - database_sys.execute_query() = 1
  // - database_sys.log_error() = 1
  // Total = 8
  EXPECT_EQ(logger->log_count(), 8);

  // Verify system-specific messages
  EXPECT_EQ(logger->count_messages_containing("ThreadSystem"), 4);   // 1 + 3
  EXPECT_EQ(logger->count_messages_containing("NetworkSystem"), 2);  // 1 + 1
  EXPECT_EQ(logger->count_messages_containing("DatabaseSystem"), 2); // 1 + 1

  bootstrapper.shutdown();
}

/**
 * @brief Verifies concurrent cross-system logging is thread-safe.
 */
TEST_F(CrossSystemIntegrationTest, ConcurrentCrossSystemLogging) {
  auto logger = std::make_shared<ThreadSafeTestLogger>();

  SystemBootstrapper bootstrapper;
  bootstrapper.with_default_logger([logger]() { return logger; });

  auto init_result = bootstrapper.initialize();
  ASSERT_TRUE(init_result.is_ok());

  const int iterations = 100;
  std::vector<std::thread> threads;

  // Thread system worker
  threads.emplace_back([iterations]() {
    MockThreadSystem sys;
    for (int i = 0; i < iterations; ++i) {
      sys.do_work();
    }
  });

  // Network system worker
  threads.emplace_back([iterations]() {
    MockNetworkSystem sys;
    for (int i = 0; i < iterations; ++i) {
      sys.handle_connection();
    }
  });

  // Database system worker
  threads.emplace_back([iterations]() {
    MockDatabaseSystem sys;
    for (int i = 0; i < iterations; ++i) {
      sys.execute_query("SELECT 1");
    }
  });

  // Wait for all threads
  for (auto &t : threads) {
    t.join();
  }

  // Verify all logs were captured (3 threads * iterations)
  EXPECT_EQ(logger->log_count(), 3 * iterations);

  bootstrapper.shutdown();
}

/**
 * @brief Verifies named loggers allow per-system log separation.
 */
TEST_F(CrossSystemIntegrationTest, PerSystemNamedLoggers) {
  auto thread_logger = std::make_shared<ThreadSafeTestLogger>("thread");
  auto network_logger = std::make_shared<ThreadSafeTestLogger>("network");
  auto db_logger = std::make_shared<ThreadSafeTestLogger>("database");

  SystemBootstrapper bootstrapper;
  bootstrapper
      .with_logger("thread_system", [thread_logger]() { return thread_logger; })
      .with_logger("network_system",
                   [network_logger]() { return network_logger; })
      .with_logger("database_system", [db_logger]() { return db_logger; });

  auto init_result = bootstrapper.initialize();
  ASSERT_TRUE(init_result.is_ok());

  // Log to each system-specific logger
  auto ts_logger = GlobalLoggerRegistry::instance().get_logger("thread_system");
  auto ns_logger =
      GlobalLoggerRegistry::instance().get_logger("network_system");
  auto ds_logger =
      GlobalLoggerRegistry::instance().get_logger("database_system");

  ts_logger->log(log_level::info, std::string_view{"Thread operation"});
  ts_logger->log(log_level::debug, std::string_view{"Thread debug"});
  ns_logger->log(log_level::info, std::string_view{"Network operation"});
  ds_logger->log(log_level::error, std::string_view{"Database error"});

  // Verify per-system logging
  EXPECT_EQ(thread_logger->log_count(), 2);
  EXPECT_EQ(network_logger->log_count(), 1);
  EXPECT_EQ(db_logger->log_count(), 1);

  bootstrapper.shutdown();
}

// ============================================================================
// Level Conversion Integration Tests
// ============================================================================

/**
 * @brief Verifies all log levels convert correctly through the logging
 * pipeline.
 */
TEST_F(LevelConversionIntegrationTest, AllLevelsConvertCorrectly) {
  auto logger = std::make_shared<ThreadSafeTestLogger>();
  logger->set_level(log_level::trace); // Enable all levels

  auto result = GlobalLoggerRegistry::instance().set_default_logger(logger);
  ASSERT_TRUE(result.is_ok());

  // Test all log levels
  std::vector<log_level> test_levels = {log_level::trace, log_level::debug,
                                        log_level::info,  log_level::warning,
                                        log_level::error, log_level::critical};

  for (auto level : test_levels) {
    logger->log(level, "Message at " + to_string(level));
  }

  // Verify all messages were logged
  EXPECT_EQ(logger->log_count(), test_levels.size());

  // Verify each level was correctly captured
  auto entries = logger->get_entries();
  for (size_t i = 0; i < test_levels.size(); ++i) {
    EXPECT_EQ(entries[i].level, test_levels[i]);
  }
}

/**
 * @brief Verifies level filtering works correctly.
 */
TEST_F(LevelConversionIntegrationTest, LevelFilteringWorks) {
  auto logger = std::make_shared<ThreadSafeTestLogger>();
  logger->set_level(log_level::warning); // Only warning and above

  auto result = GlobalLoggerRegistry::instance().set_default_logger(logger);
  ASSERT_TRUE(result.is_ok());

  // These should be filtered out
  EXPECT_FALSE(logger->is_enabled(log_level::trace));
  EXPECT_FALSE(logger->is_enabled(log_level::debug));
  EXPECT_FALSE(logger->is_enabled(log_level::info));

  // These should be allowed
  EXPECT_TRUE(logger->is_enabled(log_level::warning));
  EXPECT_TRUE(logger->is_enabled(log_level::error));
  EXPECT_TRUE(logger->is_enabled(log_level::critical));
}

/**
 * @brief Verifies level string conversion roundtrip.
 */
TEST_F(LevelConversionIntegrationTest, LevelStringRoundtrip) {
  std::vector<log_level> levels = {log_level::trace, log_level::debug,
                                   log_level::info,  log_level::warning,
                                   log_level::error, log_level::critical};

  for (auto level : levels) {
    // Convert to string
    std::string level_str = to_string(level);
    EXPECT_FALSE(level_str.empty());

    // Convert back to enum
    log_level parsed = from_string(level_str);
    EXPECT_EQ(parsed, level);
  }
}

/**
 * @brief Verifies case-insensitive level parsing.
 */
TEST_F(LevelConversionIntegrationTest, CaseInsensitiveLevelParsing) {
  EXPECT_EQ(from_string("INFO"), log_level::info);
  EXPECT_EQ(from_string("info"), log_level::info);
  EXPECT_EQ(from_string("Info"), log_level::info);
  EXPECT_EQ(from_string("WARNING"), log_level::warning);
  EXPECT_EQ(from_string("warning"), log_level::warning);
  EXPECT_EQ(from_string("Warning"), log_level::warning);
}

// ============================================================================
// Memory Safety and Stress Tests
// ============================================================================

/**
 * @brief Stress test for registry under high concurrent load.
 */
TEST_F(GlobalLoggerRegistryIntegrationTest, StressTestHighConcurrency) {
  const int num_threads = 50;
  const int operations_per_thread = 1000;
  std::atomic<int> total_operations{0};
  std::vector<std::thread> threads;

  // Pre-register a default logger
  auto logger = std::make_shared<ThreadSafeTestLogger>();
  GlobalLoggerRegistry::instance().set_default_logger(logger);

  for (int i = 0; i < num_threads; ++i) {
    threads.emplace_back([i, operations_per_thread, &total_operations]() {
      for (int j = 0; j < operations_per_thread; ++j) {
        // Mix of operations
        if (j % 3 == 0) {
          // Log operation
          auto log = GlobalLoggerRegistry::instance().get_default_logger();
          if (log) {
            log->log(log_level::debug, std::string_view{"stress test message"});
          }
        } else if (j % 3 == 1) {
          // Register named logger
          auto new_logger = std::make_shared<ThreadSafeTestLogger>();
          GlobalLoggerRegistry::instance().register_logger(
              "stress_" + std::to_string(i) + "_" + std::to_string(j),
              new_logger);
        } else {
          // Retrieve logger
          GlobalLoggerRegistry::instance().get_default_logger();
        }
        total_operations++;
      }
    });
  }

  // Wait for all threads
  for (auto &t : threads) {
    t.join();
  }

  // Verify all operations completed
  EXPECT_EQ(total_operations.load(), num_threads * operations_per_thread);

  // Verify registry is still functional
  auto final_logger = GlobalLoggerRegistry::instance().get_default_logger();
  EXPECT_NE(final_logger, nullptr);
}

/**
 * @brief Verifies cleanup works correctly after heavy usage.
 */
TEST_F(GlobalLoggerRegistryIntegrationTest, CleanupAfterHeavyUsage) {
  // Register many loggers
  const int num_loggers = 100;
  for (int i = 0; i < num_loggers; ++i) {
    auto logger = std::make_shared<ThreadSafeTestLogger>();
    GlobalLoggerRegistry::instance().register_logger(
        "logger_" + std::to_string(i), logger);
  }

  EXPECT_EQ(GlobalLoggerRegistry::instance().size(), num_loggers);

  // Clear all
  GlobalLoggerRegistry::instance().clear();

  EXPECT_EQ(GlobalLoggerRegistry::instance().size(), 0);

  // Verify registry is still functional
  auto null_logger = GlobalLoggerRegistry::instance().get_logger("nonexistent");
  EXPECT_EQ(null_logger, GlobalLoggerRegistry::null_logger());
}
