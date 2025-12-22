// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file logger_interface.h
 * @brief Standard logger interface for all systems.
 *
 * This header defines the standard logging interface to be used across
 * all systems for consistent logging behavior.
 *
 * @note Issue #177: Extended with C++20 source_location support.
 * @note Issue #217: Removed deprecated file/line/function API in v3.0.0.
 *       Use the source_location-based log() method for all logging.
 */

#pragma once

#include <algorithm>
#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include "../patterns/result.h"
#include "../utils/source_location.h"

namespace kcenon::common {
namespace interfaces {

/**
 * @enum log_level
 * @brief Standard log levels
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
 * @brief Standard log entry structure
 *
 * @note Issue #177: Extended with source_location support.
 *       The file, line, and function fields are now populated from
 *       source_location when using the create() factory method.
 */
struct log_entry {
    log_level level;
    std::string message;
    std::string file;
    int line;
    std::string function;
    std::chrono::system_clock::time_point timestamp;
    source_location location;  ///< C++20 source_location (Issue #177)

    /**
     * @brief Default constructor
     * @param lvl Log level (default: info)
     * @param msg Log message (default: empty)
     */
    log_entry(log_level lvl = log_level::info,
              const std::string& msg = "")
        : level(lvl)
        , message(msg)
        , line(0)
        , timestamp(std::chrono::system_clock::now())
        , location() {}

    /**
     * @brief Factory method to create a log_entry with source_location
     *
     * This is the preferred way to create log entries as it automatically
     * captures the source location at the call site and populates the
     * file, line, and function fields for backward compatibility.
     *
     * @param lvl Log level
     * @param msg Log message (string_view for efficiency)
     * @param loc Source location (automatically captured at call site)
     * @return Fully initialized log_entry
     *
     * @code
     * auto entry = log_entry::create(log_level::info, "Operation completed");
     * // entry.file, entry.line, entry.function are automatically populated
     * @endcode
     */
    static log_entry create(
        log_level lvl,
        std::string_view msg,
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
 * @brief Standard interface for logging implementations
 *
 * This interface defines the contract for any logging implementation,
 * allowing modules to work with different logging backends without
 * direct dependencies.
 *
 * @note Issue #177: Extended with C++20 source_location support.
 * @note Issue #217: The deprecated file/line/function overload was
 *       removed in v3.0.0. Use the source_location-based log() method.
 */
class ILogger {
public:
    virtual ~ILogger() = default;

    /**
     * @brief Log a message with specified level
     * @param level Log level
     * @param message Log message
     * @return VoidResult indicating success or error
     */
    virtual VoidResult log(log_level level, const std::string& message) = 0;

    /**
     * @brief Log a message with source location information (C++20)
     *
     * This is the preferred method for logging with source location.
     * The default implementation delegates to the simple log(level, message)
     * method for implementations that don't need source location.
     *
     * @param level Log level
     * @param message Log message (string_view for efficiency)
     * @param loc Source location (automatically captured at call site)
     * @return VoidResult indicating success or error
     *
     * @note Issue #177: Method with source_location support.
     *       Implementations should override this method to directly use
     *       source_location for improved type safety and efficiency.
     *
     * @code
     * logger->log(log_level::info, "Operation completed");
     * // Source location is automatically captured
     * @endcode
     */
    virtual VoidResult log(log_level level,
                           std::string_view message,
                           const source_location& loc = source_location::current()) {
        // Default implementation delegates to simple log method
        // Derived classes should override to use source location information
        (void)loc;  // Unused in default implementation
        return log(level, std::string(message));
    }

    /**
     * @brief Log a structured entry
     * @param entry Log entry containing all information
     * @return VoidResult indicating success or error
     */
    virtual VoidResult log(const log_entry& entry) = 0;

    /**
     * @brief Check if logging is enabled for the specified level
     * @param level Log level to check
     * @return true if logging is enabled for this level
     */
    virtual bool is_enabled(log_level level) const = 0;

    /**
     * @brief Set the minimum log level
     * @param level Minimum level for messages to be logged
     * @return VoidResult indicating success or error
     */
    virtual VoidResult set_level(log_level level) = 0;

    /**
     * @brief Get the current minimum log level
     * @return Current minimum log level
     */
    virtual log_level get_level() const = 0;

    /**
     * @brief Flush any buffered log messages
     * @return VoidResult indicating success or error
     */
    virtual VoidResult flush() = 0;
};

/**
 * @brief Factory function type for creating logger instances
 */
using LoggerFactory = std::function<std::shared_ptr<ILogger>()>;

/**
 * @interface ILoggerProvider
 * @brief Interface for modules that provide logger implementations
 */
class ILoggerProvider {
public:
    virtual ~ILoggerProvider() = default;

    /**
     * @brief Get the default logger instance
     * @return Shared pointer to the logger
     */
    virtual std::shared_ptr<ILogger> get_logger() = 0;

    /**
     * @brief Create a new logger with specific name
     * @param name Logger name
     * @return Shared pointer to the new logger
     */
    virtual std::shared_ptr<ILogger> create_logger(const std::string& name) = 0;
};

/**
 * @struct logger_config
 * @brief Configuration for logger instances
 *
 * Phase 2: Extended configuration structure for advanced logger features
 */
struct logger_config {
    log_level min_level = log_level::info;
    std::string pattern = "[%Y-%m-%d %H:%M:%S.%e] [%l] %v";
    bool async_mode = false;
    size_t queue_size = 8192;
    bool color_enabled = false;

    logger_config() = default;

    logger_config(log_level level, const std::string& fmt = "")
        : min_level(level)
        , pattern(fmt.empty() ? pattern : fmt) {}
};

/**
 * @interface ILoggerRegistry
 * @brief Phase 2: Global logger registry interface
 *
 * Provides thread-safe access to named logger instances
 */
class ILoggerRegistry {
public:
    virtual ~ILoggerRegistry() = default;

    /**
     * @brief Register a logger with a name
     * @param name Logger name
     * @param logger Logger instance
     * @return VoidResult indicating success or error
     */
    virtual VoidResult register_logger(const std::string& name, std::shared_ptr<ILogger> logger) = 0;

    /**
     * @brief Get a logger by name
     * @param name Logger name
     * @return Logger instance or nullptr if not found
     */
    virtual std::shared_ptr<ILogger> get_logger(const std::string& name) = 0;

    /**
     * @brief Remove a logger by name
     * @param name Logger name
     * @return VoidResult indicating success or error
     */
    virtual VoidResult unregister_logger(const std::string& name) = 0;

    /**
     * @brief Get the default logger
     * @return Default logger instance or nullptr
     */
    virtual std::shared_ptr<ILogger> get_default_logger() = 0;

    /**
     * @brief Set the default logger
     * @param logger Logger instance
     * @return VoidResult indicating success or error
     */
    virtual VoidResult set_default_logger(std::shared_ptr<ILogger> logger) = 0;
};

/**
 * @brief Convert log level to string
 */
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

/**
 * @brief Parse log level from string (case-insensitive)
 */
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
    return log_level::info; // default
}

} // namespace interfaces
} // namespace kcenon::common