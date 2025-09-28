// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file logger_interface.h
 * @brief Standard logger interface for all systems.
 *
 * This header defines the standard logging interface to be used across
 * all systems for consistent logging behavior.
 */

#pragma once

#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include "../patterns/result.h"

namespace common {
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
 */
struct log_entry {
    log_level level;
    std::string message;
    std::string file;
    int line;
    std::string function;
    std::chrono::system_clock::time_point timestamp;

    log_entry(log_level lvl = log_level::info,
              const std::string& msg = "")
        : level(lvl)
        , message(msg)
        , line(0)
        , timestamp(std::chrono::system_clock::now()) {}
};

/**
 * @interface ILogger
 * @brief Standard interface for logging implementations
 *
 * This interface defines the contract for any logging implementation,
 * allowing modules to work with different logging backends without
 * direct dependencies.
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
     * @brief Log a message with source location information
     * @param level Log level
     * @param message Log message
     * @param file Source file name
     * @param line Source line number
     * @param function Function name
     * @return VoidResult indicating success or error
     */
    virtual VoidResult log(log_level level,
                           const std::string& message,
                           const std::string& file,
                           int line,
                           const std::string& function) = 0;

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
 * @brief Parse log level from string
 */
inline log_level from_string(const std::string& str) {
    if (str == "TRACE") return log_level::trace;
    if (str == "DEBUG") return log_level::debug;
    if (str == "INFO") return log_level::info;
    if (str == "WARNING") return log_level::warning;
    if (str == "ERROR") return log_level::error;
    if (str == "CRITICAL") return log_level::critical;
    if (str == "OFF") return log_level::off;
    return log_level::info; // default
}

} // namespace interfaces
} // namespace common