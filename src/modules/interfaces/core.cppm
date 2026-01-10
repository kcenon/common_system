// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file interfaces/core.cppm
 * @brief C++20 module partition for core interfaces.
 *
 * This module partition exports core interface types:
 * - IJob: Abstract job interface for task execution
 * - IExecutor: Abstract interface for task execution systems
 * - IThreadPool: Extended interface for thread pool implementations
 * - ILogger: Standard interface for logging implementations
 * - IDatabase: Database interface (forward declaration)
 *
 * Part of the kcenon.common module.
 */

module;

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <functional>
#include <future>
#include <memory>
#include <string>
#include <string_view>
#include <variant>

export module kcenon.common:interfaces.core;

// Import result.core partition to reuse error_info, Result<T>, VoidResult, source_location
// This avoids symbol duplication issues in MSVC module builds
import :result.core;

export namespace kcenon::common {

// Re-export types from result.core for use in interfaces namespace
// (Types are already exported from result.core, just need VoidResult factory here)

/**
 * @brief Factory function to create successful VoidResult.
 */
inline VoidResult ok() { return VoidResult(std::monostate{}); }

} // namespace kcenon::common

// Internal type aliases for this partition (non-exported to avoid symbol duplication)
// MSVC module build requires using declarations to be outside export namespace
namespace kcenon::common::interfaces {
using kcenon::common::source_location;
using kcenon::common::error_info;
using kcenon::common::Result;
using kcenon::common::VoidResult;
} // namespace kcenon::common::interfaces

export namespace kcenon::common::interfaces {

// ============================================================================
// IJob Interface
// ============================================================================

/**
 * @interface IJob
 * @brief Abstract job interface for task execution.
 */
class IJob {
public:
    virtual ~IJob() = default;

    /**
     * @brief Execute the job.
     * @return VoidResult indicating success or failure
     */
    virtual VoidResult execute() = 0;

    /**
     * @brief Get the name of the job.
     * @return Job name
     */
    virtual std::string get_name() const { return "unnamed_job"; }

    /**
     * @brief Get the priority of the job.
     * @return Job priority (default: 0)
     */
    virtual int get_priority() const { return 0; }
};

// ============================================================================
// IExecutor Interface
// ============================================================================

/**
 * @interface IExecutor
 * @brief Abstract interface for task execution systems.
 */
class IExecutor {
public:
    virtual ~IExecutor() = default;

    /**
     * @brief Execute a job with Result-based error handling.
     */
    virtual Result<std::future<void>> execute(std::unique_ptr<IJob>&& job) = 0;

    /**
     * @brief Execute a job with delay.
     */
    virtual Result<std::future<void>> execute_delayed(
        std::unique_ptr<IJob>&& job,
        std::chrono::milliseconds delay) = 0;

    /**
     * @brief Get the number of worker threads.
     */
    virtual size_t worker_count() const = 0;

    /**
     * @brief Check if the executor is running.
     */
    virtual bool is_running() const = 0;

    /**
     * @brief Get the number of pending tasks.
     */
    virtual size_t pending_tasks() const = 0;

    /**
     * @brief Shutdown the executor gracefully.
     */
    virtual void shutdown(bool wait_for_completion = true) = 0;
};

using ExecutorFactory = std::function<std::shared_ptr<IExecutor>()>;

/**
 * @interface IExecutorProvider
 * @brief Provider for obtaining executor implementations.
 */
class IExecutorProvider {
public:
    virtual ~IExecutorProvider() = default;
    virtual std::shared_ptr<IExecutor> get_executor() = 0;
    virtual std::shared_ptr<IExecutor> create_executor(size_t worker_count) = 0;
};

// ============================================================================
// IThreadPool Interface
// ============================================================================

/**
 * @interface IThreadPool
 * @brief Extended interface for thread pool implementations.
 */
class IThreadPool : public IExecutor {
public:
    virtual ~IThreadPool() = default;

    virtual VoidResult resize(size_t new_size) = 0;
    virtual size_t min_workers() const { return 1; }
    virtual size_t max_workers() const { return 0; }
    virtual VoidResult set_queue_capacity(size_t capacity) = 0;
    virtual size_t get_queue_capacity() const = 0;
    virtual bool is_queue_full() const = 0;
    virtual size_t clear_pending_tasks() = 0;
    virtual VoidResult start() = 0;
    virtual VoidResult stop(bool wait_for_completion = true) = 0;
    virtual VoidResult pause() = 0;
    virtual VoidResult resume() = 0;
    virtual bool is_paused() const = 0;
    virtual size_t active_tasks() const = 0;
    virtual size_t idle_workers() const = 0;
    virtual size_t completed_tasks() const { return 0; }
    virtual size_t failed_tasks() const { return 0; }
};

using ThreadPoolFactory = std::function<std::shared_ptr<IThreadPool>(size_t worker_count)>;

class IThreadPoolProvider {
public:
    virtual ~IThreadPoolProvider() = default;
    virtual std::shared_ptr<IThreadPool> get_thread_pool() = 0;
    virtual Result<std::shared_ptr<IThreadPool>> create_thread_pool(
        size_t worker_count, size_t queue_capacity = 0) = 0;
};

// ============================================================================
// Logger Interface
// ============================================================================

/**
 * @enum log_level
 * @brief Standard log levels.
 */
enum class log_level {
    trace = 0,
    debug = 1,
    info = 2,
    warning = 3,
    error = 4,
    critical = 5,
    off = 6
};

/**
 * @struct log_entry
 * @brief Standard log entry structure.
 */
struct log_entry {
    log_level level;
    std::string message;
    std::string file;
    int line;
    std::string function;
    std::chrono::system_clock::time_point timestamp;
    source_location location;

    log_entry(log_level lvl = log_level::info, const std::string& msg = "")
        : level(lvl), message(msg), line(0), timestamp(std::chrono::system_clock::now()), location() {}

    static log_entry create(log_level lvl, std::string_view msg,
                           const source_location& loc = source_location::current()) {
        log_entry entry;
        entry.level = lvl;
        entry.message = std::string(msg);
        entry.file = loc.file_name();
        entry.line = loc.line();
        entry.function = loc.function_name();
        entry.timestamp = std::chrono::system_clock::now();
        entry.location = loc;
        return entry;
    }
};

/**
 * @interface ILogger
 * @brief Standard interface for logging implementations.
 */
class ILogger {
public:
    virtual ~ILogger() = default;

    virtual VoidResult log(log_level level, const std::string& message) = 0;

    virtual VoidResult log(log_level level, std::string_view message,
                          const source_location& loc = source_location::current()) {
        (void)loc;
        return log(level, std::string(message));
    }

    virtual VoidResult log(const log_entry& entry) = 0;
    virtual bool is_enabled(log_level level) const = 0;
    virtual VoidResult set_level(log_level level) = 0;
    virtual log_level get_level() const = 0;
    virtual VoidResult flush() = 0;
};

using LoggerFactory = std::function<std::shared_ptr<ILogger>()>;

class ILoggerProvider {
public:
    virtual ~ILoggerProvider() = default;
    virtual std::shared_ptr<ILogger> get_logger() = 0;
    virtual std::shared_ptr<ILogger> create_logger(const std::string& name) = 0;
};

struct logger_config {
    log_level min_level = log_level::info;
    std::string pattern = "[%Y-%m-%d %H:%M:%S.%e] [%l] %v";
    bool async_mode = false;
    size_t queue_size = 8192;
    bool color_enabled = false;

    logger_config() = default;
    logger_config(log_level level, const std::string& fmt = "")
        : min_level(level), pattern(fmt.empty() ? pattern : fmt) {}
};

class ILoggerRegistry {
public:
    virtual ~ILoggerRegistry() = default;
    virtual VoidResult register_logger(const std::string& name, std::shared_ptr<ILogger> logger) = 0;
    virtual std::shared_ptr<ILogger> get_logger(const std::string& name) = 0;
    virtual VoidResult unregister_logger(const std::string& name) = 0;
    virtual std::shared_ptr<ILogger> get_default_logger() = 0;
    virtual VoidResult set_default_logger(std::shared_ptr<ILogger> logger) = 0;
};

inline std::string to_string(log_level level) {
    switch(level) {
        case log_level::trace: return "TRACE";
        case log_level::debug: return "DEBUG";
        case log_level::info: return "INFO";
        case log_level::warning: return "WARNING";
        case log_level::error: return "ERROR";
        case log_level::critical: return "CRITICAL";
        case log_level::off: return "OFF";
        default: return "UNKNOWN";
    }
}

inline log_level from_string(const std::string& str) {
    std::string upper = str;
    std::transform(upper.begin(), upper.end(), upper.begin(),
                   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });

    if (upper == "TRACE") return log_level::trace;
    if (upper == "DEBUG") return log_level::debug;
    if (upper == "INFO") return log_level::info;
    if (upper == "WARNING" || upper == "WARN") return log_level::warning;
    if (upper == "ERROR") return log_level::error;
    if (upper == "CRITICAL" || upper == "FATAL") return log_level::critical;
    if (upper == "OFF") return log_level::off;
    return log_level::info;
}

// ============================================================================
// Database Interface (forward declaration only)
// ============================================================================

class IDatabase {
public:
    virtual ~IDatabase() = default;
};

} // namespace kcenon::common::interfaces
