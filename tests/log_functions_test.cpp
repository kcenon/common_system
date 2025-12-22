// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file log_functions_test.cpp
 * @brief Unit tests for unified logging functions and macros (Issue #175)
 *
 * These tests verify:
 * - Inline logging function behavior
 * - source_location automatic capture
 * - LOG_* macro functionality
 * - Conditional logging (LOG_IF)
 * - Named logger support
 * - Compile-time log level filtering
 *
 * @note Issue #177: Extended with source_location-specific tests.
 * @note Issue #180: Removed deprecated THREAD_LOG_* legacy macro tests.
 * @note Issue #217: Removed deprecated file/line/function API in v3.0.0.
 */

#include <gtest/gtest.h>
#include <kcenon/common/logging/log_functions.h>
#include <kcenon/common/logging/log_macros.h>
#include <kcenon/common/interfaces/global_logger_registry.h>
#include <kcenon/common/utils/source_location.h>

#include <atomic>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

using namespace kcenon::common;
using namespace kcenon::common::interfaces;
using namespace kcenon::common::logging;

// ============================================================================
// Test Logger Implementation
// ============================================================================

/**
 * @brief Test logger that captures log entries for verification.
 *
 * @note Issue #177: Updated to support source_location-based logging.
 * @note Issue #217: Removed deprecated file/line/function method in v3.0.0.
 */
class CaptureLogger : public ILogger {
public:
    struct LogEntry {
        log_level level;
        std::string message;
        std::string file;
        int line;
        std::string function;
        source_location location;  // Issue #177: Store source_location
    };

    CaptureLogger() = default;
    explicit CaptureLogger(log_level min_level) : level_(min_level) {}

    VoidResult log(log_level level, const std::string& message) override {
        std::lock_guard<std::mutex> lock(mutex_);
        entries_.push_back({level, message, "", 0, "", {}});
        return VoidResult::ok({});
    }

    /**
     * @brief Log with source_location (Issue #177 - preferred method)
     *
     * This override directly captures the source_location object,
     * avoiding the need for string copies of file/function names.
     */
    VoidResult log(log_level level,
                   std::string_view message,
                   const source_location& loc = source_location::current()) override {
        std::lock_guard<std::mutex> lock(mutex_);
        entries_.push_back({
            level,
            std::string(message),
            std::string(loc.file_name()),
            static_cast<int>(loc.line()),
            std::string(loc.function_name()),
            loc
        });
        return VoidResult::ok({});
    }

    VoidResult log(const log_entry& entry) override {
        std::lock_guard<std::mutex> lock(mutex_);
        entries_.push_back({
            entry.level,
            entry.message,
            entry.file,
            entry.line,
            entry.function,
            entry.location
        });
        return VoidResult::ok({});
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
    size_t entry_count() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return entries_.size();
    }

    LogEntry get_entry(size_t index) const {
        std::lock_guard<std::mutex> lock(mutex_);
        if (index >= entries_.size()) {
            return {};
        }
        return entries_[index];
    }

    LogEntry last_entry() const {
        std::lock_guard<std::mutex> lock(mutex_);
        if (entries_.empty()) {
            return {};
        }
        return entries_.back();
    }

    bool was_flushed() const { return flushed_; }

    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        entries_.clear();
        flushed_ = false;
    }

private:
    log_level level_ = log_level::trace;
    bool flushed_ = false;
    mutable std::mutex mutex_;
    std::vector<LogEntry> entries_;
};

// ============================================================================
// Test Fixtures
// ============================================================================

class LogFunctionsTest : public ::testing::Test {
protected:
    void SetUp() override {
        GlobalLoggerRegistry::instance().clear();
        test_logger_ = std::make_shared<CaptureLogger>();
        GlobalLoggerRegistry::instance().set_default_logger(test_logger_);
    }

    void TearDown() override {
        GlobalLoggerRegistry::instance().clear();
    }

    std::shared_ptr<CaptureLogger> test_logger_;
};

// ============================================================================
// Basic Logging Function Tests
// ============================================================================

TEST_F(LogFunctionsTest, Log_BasicMessage) {
    auto result = logging::log(log_level::info, "Test message");

    EXPECT_TRUE(result.is_ok());
    EXPECT_EQ(test_logger_->entry_count(), 1);

    auto entry = test_logger_->last_entry();
    EXPECT_EQ(entry.level, log_level::info);
    EXPECT_EQ(entry.message, "Test message");
}

TEST_F(LogFunctionsTest, Log_CapturesSourceLocation) {
    auto result = logging::log_info("Source location test"); // Line number matters

    EXPECT_TRUE(result.is_ok());
    EXPECT_EQ(test_logger_->entry_count(), 1);

    auto entry = test_logger_->last_entry();
    EXPECT_FALSE(entry.file.empty());
    EXPECT_GT(entry.line, 0);
    EXPECT_FALSE(entry.function.empty());

    // Verify file contains this test file name
    EXPECT_NE(entry.file.find("log_functions_test.cpp"), std::string::npos);
}

TEST_F(LogFunctionsTest, LogTrace_CorrectLevel) {
    logging::log_trace("Trace message");

    auto entry = test_logger_->last_entry();
    EXPECT_EQ(entry.level, log_level::trace);
    EXPECT_EQ(entry.message, "Trace message");
}

TEST_F(LogFunctionsTest, LogDebug_CorrectLevel) {
    logging::log_debug("Debug message");

    auto entry = test_logger_->last_entry();
    EXPECT_EQ(entry.level, log_level::debug);
    EXPECT_EQ(entry.message, "Debug message");
}

TEST_F(LogFunctionsTest, LogInfo_CorrectLevel) {
    logging::log_info("Info message");

    auto entry = test_logger_->last_entry();
    EXPECT_EQ(entry.level, log_level::info);
    EXPECT_EQ(entry.message, "Info message");
}

TEST_F(LogFunctionsTest, LogWarning_CorrectLevel) {
    logging::log_warning("Warning message");

    auto entry = test_logger_->last_entry();
    EXPECT_EQ(entry.level, log_level::warning);
    EXPECT_EQ(entry.message, "Warning message");
}

TEST_F(LogFunctionsTest, LogError_CorrectLevel) {
    logging::log_error("Error message");

    auto entry = test_logger_->last_entry();
    EXPECT_EQ(entry.level, log_level::error);
    EXPECT_EQ(entry.message, "Error message");
}

TEST_F(LogFunctionsTest, LogCritical_CorrectLevel) {
    logging::log_critical("Critical message");

    auto entry = test_logger_->last_entry();
    EXPECT_EQ(entry.level, log_level::critical);
    EXPECT_EQ(entry.message, "Critical message");
}

// ============================================================================
// Named Logger Tests
// ============================================================================

TEST_F(LogFunctionsTest, Log_ToNamedLogger) {
    auto network_logger = std::make_shared<CaptureLogger>();
    GlobalLoggerRegistry::instance().register_logger("network", network_logger);

    logging::log_info_to("network", "Network message");

    // Default logger should be empty
    EXPECT_EQ(test_logger_->entry_count(), 0);

    // Named logger should have the message
    EXPECT_EQ(network_logger->entry_count(), 1);
    auto entry = network_logger->last_entry();
    EXPECT_EQ(entry.level, log_level::info);
    EXPECT_EQ(entry.message, "Network message");
}

TEST_F(LogFunctionsTest, Log_ToSpecificLoggerInstance) {
    auto custom_logger = std::make_shared<CaptureLogger>();

    logging::log(log_level::warning, "Custom message", custom_logger);

    // Default logger should be empty
    EXPECT_EQ(test_logger_->entry_count(), 0);

    // Custom logger should have the message
    EXPECT_EQ(custom_logger->entry_count(), 1);
    auto entry = custom_logger->last_entry();
    EXPECT_EQ(entry.level, log_level::warning);
    EXPECT_EQ(entry.message, "Custom message");
}

// ============================================================================
// Level Filtering Tests
// ============================================================================

TEST_F(LogFunctionsTest, Log_RespectsLogLevel) {
    test_logger_->set_level(log_level::warning);

    logging::log_trace("Trace");
    logging::log_debug("Debug");
    logging::log_info("Info");
    logging::log_warning("Warning");
    logging::log_error("Error");

    // Only warning and above should be logged
    EXPECT_EQ(test_logger_->entry_count(), 2);
    EXPECT_EQ(test_logger_->get_entry(0).level, log_level::warning);
    EXPECT_EQ(test_logger_->get_entry(1).level, log_level::error);
}

TEST_F(LogFunctionsTest, IsEnabled_CorrectBehavior) {
    test_logger_->set_level(log_level::info);

    EXPECT_FALSE(logging::is_enabled(log_level::trace));
    EXPECT_FALSE(logging::is_enabled(log_level::debug));
    EXPECT_TRUE(logging::is_enabled(log_level::info));
    EXPECT_TRUE(logging::is_enabled(log_level::warning));
    EXPECT_TRUE(logging::is_enabled(log_level::error));
    EXPECT_TRUE(logging::is_enabled(log_level::critical));
}

TEST_F(LogFunctionsTest, IsEnabled_ForNamedLogger) {
    auto network_logger = std::make_shared<CaptureLogger>(log_level::error);
    GlobalLoggerRegistry::instance().register_logger("network", network_logger);

    EXPECT_FALSE(logging::is_enabled(log_level::warning, "network"));
    EXPECT_TRUE(logging::is_enabled(log_level::error, "network"));
}

// ============================================================================
// Flush Tests
// ============================================================================

TEST_F(LogFunctionsTest, Flush_DefaultLogger) {
    EXPECT_FALSE(test_logger_->was_flushed());

    logging::flush();

    EXPECT_TRUE(test_logger_->was_flushed());
}

TEST_F(LogFunctionsTest, Flush_NamedLogger) {
    auto network_logger = std::make_shared<CaptureLogger>();
    GlobalLoggerRegistry::instance().register_logger("network", network_logger);

    EXPECT_FALSE(network_logger->was_flushed());

    logging::flush("network");

    EXPECT_TRUE(network_logger->was_flushed());
    EXPECT_FALSE(test_logger_->was_flushed()); // Default not flushed
}

// ============================================================================
// Macro Tests
// ============================================================================

TEST_F(LogFunctionsTest, Macro_LOG_INFO) {
    LOG_INFO("Macro test message");

    EXPECT_EQ(test_logger_->entry_count(), 1);
    auto entry = test_logger_->last_entry();
    EXPECT_EQ(entry.level, log_level::info);
    EXPECT_EQ(entry.message, "Macro test message");
}

TEST_F(LogFunctionsTest, Macro_AllLevels) {
    LOG_TRACE("Trace");
    LOG_DEBUG("Debug");
    LOG_INFO("Info");
    LOG_WARNING("Warning");
    LOG_ERROR("Error");
    LOG_CRITICAL("Critical");

    EXPECT_EQ(test_logger_->entry_count(), 6);
    EXPECT_EQ(test_logger_->get_entry(0).level, log_level::trace);
    EXPECT_EQ(test_logger_->get_entry(1).level, log_level::debug);
    EXPECT_EQ(test_logger_->get_entry(2).level, log_level::info);
    EXPECT_EQ(test_logger_->get_entry(3).level, log_level::warning);
    EXPECT_EQ(test_logger_->get_entry(4).level, log_level::error);
    EXPECT_EQ(test_logger_->get_entry(5).level, log_level::critical);
}

TEST_F(LogFunctionsTest, Macro_LOG_TO_NamedLogger) {
    auto api_logger = std::make_shared<CaptureLogger>();
    GlobalLoggerRegistry::instance().register_logger("api", api_logger);

    LOG_INFO_TO("api", "API message");
    LOG_ERROR_TO("api", "API error");

    EXPECT_EQ(test_logger_->entry_count(), 0);
    EXPECT_EQ(api_logger->entry_count(), 2);
    EXPECT_EQ(api_logger->get_entry(0).message, "API message");
    EXPECT_EQ(api_logger->get_entry(1).message, "API error");
}

TEST_F(LogFunctionsTest, Macro_LOG_IF_Enabled) {
    test_logger_->set_level(log_level::info);

    LOG_IF(log_level::info, "Should log");
    LOG_IF(log_level::debug, "Should not log");

    EXPECT_EQ(test_logger_->entry_count(), 1);
    EXPECT_EQ(test_logger_->last_entry().message, "Should log");
}

TEST_F(LogFunctionsTest, Macro_LOG_FLUSH) {
    EXPECT_FALSE(test_logger_->was_flushed());

    LOG_FLUSH();

    EXPECT_TRUE(test_logger_->was_flushed());
}

TEST_F(LogFunctionsTest, Macro_LOG_IS_ENABLED) {
    test_logger_->set_level(log_level::warning);

    EXPECT_FALSE(LOG_IS_ENABLED(log_level::debug));
    EXPECT_TRUE(LOG_IS_ENABLED(log_level::warning));
    EXPECT_TRUE(LOG_IS_ENABLED(log_level::error));
}

// ============================================================================
// NullLogger Fallback Tests
// ============================================================================

TEST_F(LogFunctionsTest, Log_ToUnregisteredLogger_UsesNullLogger) {
    // This should not throw, NullLogger handles it silently
    auto result = logging::log_info_to("nonexistent_logger", "Message");
    EXPECT_TRUE(result.is_ok());

    // Default logger should be empty
    EXPECT_EQ(test_logger_->entry_count(), 0);
}

TEST_F(LogFunctionsTest, Log_WhenNoDefaultLogger) {
    GlobalLoggerRegistry::instance().clear();

    // Should not throw, NullLogger fallback
    auto result = logging::log_info("Message without logger");
    EXPECT_TRUE(result.is_ok());
}

// ============================================================================
// Thread Safety Tests
// ============================================================================

TEST_F(LogFunctionsTest, ConcurrentLogging) {
    constexpr int num_threads = 8;
    constexpr int logs_per_thread = 100;

    std::vector<std::thread> threads;
    std::atomic<int> completed{0};

    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([t, &completed]() {
            for (int i = 0; i < logs_per_thread; ++i) {
                std::string msg = "Thread " + std::to_string(t) + " msg " + std::to_string(i);
                logging::log_info(msg);
            }
            completed++;
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    EXPECT_EQ(completed.load(), num_threads);
    EXPECT_EQ(test_logger_->entry_count(),
              static_cast<size_t>(num_threads * logs_per_thread));
}

TEST_F(LogFunctionsTest, ConcurrentLogging_MultipleLoggers) {
    constexpr int num_threads = 4;
    constexpr int logs_per_thread = 50;

    auto logger_a = std::make_shared<CaptureLogger>();
    auto logger_b = std::make_shared<CaptureLogger>();
    GlobalLoggerRegistry::instance().register_logger("a", logger_a);
    GlobalLoggerRegistry::instance().register_logger("b", logger_b);

    std::vector<std::thread> threads;

    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([t]() {
            for (int i = 0; i < logs_per_thread; ++i) {
                if (t % 2 == 0) {
                    logging::log_info_to("a", "To A");
                } else {
                    logging::log_info_to("b", "To B");
                }
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    size_t total = logger_a->entry_count() + logger_b->entry_count();
    EXPECT_EQ(total, static_cast<size_t>(num_threads * logs_per_thread));
}

// ============================================================================
// String View Compatibility Tests
// ============================================================================

TEST_F(LogFunctionsTest, Log_StringView) {
    std::string_view sv = "String view message";
    logging::log_info(sv);

    auto entry = test_logger_->last_entry();
    EXPECT_EQ(entry.message, "String view message");
}

TEST_F(LogFunctionsTest, Log_ConstCharPtr) {
    const char* cstr = "C-string message";
    logging::log_info(cstr);

    auto entry = test_logger_->last_entry();
    EXPECT_EQ(entry.message, "C-string message");
}

TEST_F(LogFunctionsTest, Log_StdString) {
    std::string str = "std::string message";
    logging::log_info(str);

    auto entry = test_logger_->last_entry();
    EXPECT_EQ(entry.message, "std::string message");
}

// ============================================================================
// Issue #177: source_location Tests
// ============================================================================

/**
 * @brief Test log_entry::create() factory method
 */
TEST_F(LogFunctionsTest, LogEntry_Create_FactoryMethod) {
    auto entry = log_entry::create(log_level::info, "Factory created entry");

    EXPECT_EQ(entry.level, log_level::info);
    EXPECT_EQ(entry.message, "Factory created entry");

    // Verify source location was captured
    EXPECT_FALSE(std::string(entry.file).empty());
    EXPECT_GT(entry.line, 0);
    EXPECT_FALSE(std::string(entry.function).empty());

    // Verify file contains this test file name
    EXPECT_NE(std::string(entry.file).find("log_functions_test.cpp"), std::string::npos);

    // Verify location field matches file/line/function
    EXPECT_EQ(std::string(entry.location.file_name()), entry.file);
    EXPECT_EQ(entry.location.line(), entry.line);
    EXPECT_EQ(std::string(entry.location.function_name()), entry.function);
}

/**
 * @brief Test log_entry::create() with different log levels
 */
TEST_F(LogFunctionsTest, LogEntry_Create_AllLevels) {
    auto trace_entry = log_entry::create(log_level::trace, "Trace");
    auto debug_entry = log_entry::create(log_level::debug, "Debug");
    auto info_entry = log_entry::create(log_level::info, "Info");
    auto warning_entry = log_entry::create(log_level::warning, "Warning");
    auto error_entry = log_entry::create(log_level::error, "Error");
    auto critical_entry = log_entry::create(log_level::critical, "Critical");

    EXPECT_EQ(trace_entry.level, log_level::trace);
    EXPECT_EQ(debug_entry.level, log_level::debug);
    EXPECT_EQ(info_entry.level, log_level::info);
    EXPECT_EQ(warning_entry.level, log_level::warning);
    EXPECT_EQ(error_entry.level, log_level::error);
    EXPECT_EQ(critical_entry.level, log_level::critical);

    // All should have valid source location
    EXPECT_GT(trace_entry.line, 0);
    EXPECT_GT(debug_entry.line, 0);
    EXPECT_GT(info_entry.line, 0);
    EXPECT_GT(warning_entry.line, 0);
    EXPECT_GT(error_entry.line, 0);
    EXPECT_GT(critical_entry.line, 0);
}

/**
 * @brief Test ILogger::log() with source_location directly
 */
TEST_F(LogFunctionsTest, ILogger_Log_SourceLocation) {
    auto loc = source_location::current();
    test_logger_->log(log_level::info, "Direct source_location", loc);

    auto entry = test_logger_->last_entry();
    EXPECT_EQ(entry.level, log_level::info);
    EXPECT_EQ(entry.message, "Direct source_location");

    // Verify source location was passed correctly
    EXPECT_EQ(entry.line, loc.line());
    EXPECT_EQ(entry.file, std::string(loc.file_name()));
    EXPECT_EQ(entry.function, std::string(loc.function_name()));
}

/**
 * @brief Test log_entry location field preservation
 */
TEST_F(LogFunctionsTest, LogEntry_LocationField_Preserved) {
    auto entry = log_entry::create(log_level::debug, "Location test");
    test_logger_->log(entry);

    auto logged_entry = test_logger_->last_entry();

    // Verify the location field was preserved
    EXPECT_EQ(logged_entry.location.line(), entry.location.line());
    EXPECT_STREQ(logged_entry.location.file_name(), entry.location.file_name());
    EXPECT_STREQ(logged_entry.location.function_name(), entry.location.function_name());
}

/**
 * @brief Test source_location::current() captures correct location
 */
TEST_F(LogFunctionsTest, SourceLocation_Current_CorrectCapture) {
    auto loc = source_location::current();  // Line A

    // Verify file name
    std::string file_name(loc.file_name());
    EXPECT_NE(file_name.find("log_functions_test.cpp"), std::string::npos);

    // Verify function name contains test function name
    std::string func_name(loc.function_name());
    EXPECT_FALSE(func_name.empty());

    // Verify line number is positive
    EXPECT_GT(loc.line(), 0);
}

/**
 * @brief Test that logging functions use source_location-based interface
 */
TEST_F(LogFunctionsTest, LoggingFunctions_UseSourceLocation) {
    // Clear any previous entries
    test_logger_->clear();

    // Log via the logging functions
    logging::log_info("Test via log_info");

    EXPECT_EQ(test_logger_->entry_count(), 1);
    auto entry = test_logger_->last_entry();

    // Verify source location was captured (not empty)
    EXPECT_FALSE(entry.file.empty());
    EXPECT_GT(entry.line, 0);
    EXPECT_FALSE(entry.function.empty());
}

/**
 * @brief Test default log_entry constructor backward compatibility
 */
TEST_F(LogFunctionsTest, LogEntry_DefaultConstructor_BackwardCompatible) {
    // Old-style construction
    log_entry old_entry(log_level::warning, "Legacy entry");

    EXPECT_EQ(old_entry.level, log_level::warning);
    EXPECT_EQ(old_entry.message, "Legacy entry");
    EXPECT_EQ(old_entry.line, 0);  // Default
    EXPECT_TRUE(old_entry.file.empty());
    EXPECT_TRUE(old_entry.function.empty());

    // Log it and verify
    test_logger_->log(old_entry);
    auto logged = test_logger_->last_entry();
    EXPECT_EQ(logged.message, "Legacy entry");
}
