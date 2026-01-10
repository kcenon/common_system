// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file interfaces/logger.cppm
 * @brief C++20 module partition for logger interfaces.
 *
 * This module partition exports logger-related interfaces:
 * - log_level: Standard log levels
 * - log_entry: Standard log entry structure
 * - ILogger: Standard interface for logging implementations
 * - ILoggerProvider: Provider for obtaining logger implementations
 * - ILoggerRegistry: Registry for managing multiple loggers
 *
 * Part of the kcenon.common module.
 */

module;

#include <algorithm>
#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <string_view>

export module kcenon.common:interfaces.logger;

// Import result.core partition to reuse error_info, Result<T>, VoidResult, source_location
import :result.core;

// Internal type aliases for this partition (non-exported to avoid symbol duplication)
namespace kcenon::common::interfaces {
using kcenon::common::source_location;
using kcenon::common::error_info;
using kcenon::common::Result;
using kcenon::common::VoidResult;
} // namespace kcenon::common::interfaces

export namespace kcenon::common::interfaces {

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

} // namespace kcenon::common::interfaces
