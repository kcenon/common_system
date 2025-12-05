// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file log_functions.h
 * @brief Unified logging functions with source_location support.
 *
 * This header provides inline logging functions that automatically capture
 * source location information (file, line, function) at the call site.
 * These functions integrate with GlobalLoggerRegistry to provide a
 * consistent logging interface across all subsystems.
 *
 * Thread Safety:
 * - All logging functions are thread-safe.
 * - Source location capture happens at compile-time.
 * - Logger retrieval uses GlobalLoggerRegistry which is thread-safe.
 *
 * Usage:
 * @code
 * // Basic logging
 * kcenon::common::logging::log_info("Application started");
 *
 * // Different log levels
 * kcenon::common::logging::log_trace("Detailed trace info");
 * kcenon::common::logging::log_debug("Debug message");
 * kcenon::common::logging::log_warning("Warning condition");
 * kcenon::common::logging::log_error("Error occurred");
 * kcenon::common::logging::log_critical("Critical failure");
 *
 * // Using named logger
 * auto network_logger = get_logger("network");
 * kcenon::common::logging::log(log_level::info, "Connected", network_logger);
 * @endcode
 *
 * @see Issue #175 for implementation requirements.
 * @see global_logger_registry.h for logger management.
 * @see source_location.h for source location support.
 */

#pragma once

#include "../interfaces/global_logger_registry.h"
#include "../utils/source_location.h"

#include <memory>
#include <string>
#include <string_view>

namespace kcenon::common::logging {

/**
 * @brief Log a message with the specified level and source location.
 *
 * Primary logging function that logs to the default logger with
 * automatic source location capture.
 *
 * @param level Log level for the message
 * @param message Message to log
 * @param loc Source location (automatically captured at call site)
 * @return VoidResult indicating success or error
 */
inline VoidResult log(interfaces::log_level level,
                      std::string_view message,
                      const source_location& loc = source_location::current()) {
    auto logger = interfaces::get_logger();
    if (!logger->is_enabled(level)) {
        return VoidResult::ok({});
    }
    return logger->log(level,
                       std::string(message),
                       loc.file_name(),
                       loc.line(),
                       loc.function_name());
}

/**
 * @brief Log a message to a specific logger with source location.
 *
 * Allows logging to a named or custom logger while still capturing
 * source location information.
 *
 * @param level Log level for the message
 * @param message Message to log
 * @param logger Logger instance to use
 * @param loc Source location (automatically captured at call site)
 * @return VoidResult indicating success or error
 */
inline VoidResult log(interfaces::log_level level,
                      std::string_view message,
                      const std::shared_ptr<interfaces::ILogger>& logger,
                      const source_location& loc = source_location::current()) {
    if (!logger || !logger->is_enabled(level)) {
        return VoidResult::ok({});
    }
    return logger->log(level,
                       std::string(message),
                       loc.file_name(),
                       loc.line(),
                       loc.function_name());
}

/**
 * @brief Log a message to a named logger with source location.
 *
 * Retrieves a logger by name from GlobalLoggerRegistry and logs to it.
 *
 * @param level Log level for the message
 * @param message Message to log
 * @param logger_name Name of the logger in the registry
 * @param loc Source location (automatically captured at call site)
 * @return VoidResult indicating success or error
 */
inline VoidResult log(interfaces::log_level level,
                      std::string_view message,
                      const std::string& logger_name,
                      const source_location& loc = source_location::current()) {
    auto logger = interfaces::get_logger(logger_name);
    if (!logger->is_enabled(level)) {
        return VoidResult::ok({});
    }
    return logger->log(level,
                       std::string(message),
                       loc.file_name(),
                       loc.line(),
                       loc.function_name());
}

// =============================================================================
// Level-specific logging functions
// =============================================================================

/**
 * @brief Log a trace-level message.
 *
 * Trace messages are the most verbose and typically used for
 * detailed debugging information.
 *
 * @param message Message to log
 * @param loc Source location (automatically captured)
 * @return VoidResult indicating success or error
 */
inline VoidResult log_trace(std::string_view message,
                            const source_location& loc = source_location::current()) {
    return log(interfaces::log_level::trace, message, loc);
}

/**
 * @brief Log a debug-level message.
 *
 * Debug messages are used for development-time debugging information.
 *
 * @param message Message to log
 * @param loc Source location (automatically captured)
 * @return VoidResult indicating success or error
 */
inline VoidResult log_debug(std::string_view message,
                            const source_location& loc = source_location::current()) {
    return log(interfaces::log_level::debug, message, loc);
}

/**
 * @brief Log an info-level message.
 *
 * Info messages convey general operational information.
 *
 * @param message Message to log
 * @param loc Source location (automatically captured)
 * @return VoidResult indicating success or error
 */
inline VoidResult log_info(std::string_view message,
                           const source_location& loc = source_location::current()) {
    return log(interfaces::log_level::info, message, loc);
}

/**
 * @brief Log a warning-level message.
 *
 * Warning messages indicate potentially problematic situations
 * that don't prevent the application from functioning.
 *
 * @param message Message to log
 * @param loc Source location (automatically captured)
 * @return VoidResult indicating success or error
 */
inline VoidResult log_warning(std::string_view message,
                              const source_location& loc = source_location::current()) {
    return log(interfaces::log_level::warning, message, loc);
}

/**
 * @brief Log an error-level message.
 *
 * Error messages indicate failures that may require attention
 * but don't necessarily terminate the application.
 *
 * @param message Message to log
 * @param loc Source location (automatically captured)
 * @return VoidResult indicating success or error
 */
inline VoidResult log_error(std::string_view message,
                            const source_location& loc = source_location::current()) {
    return log(interfaces::log_level::error, message, loc);
}

/**
 * @brief Log a critical-level message.
 *
 * Critical messages indicate severe failures that may cause
 * the application to terminate or enter an unstable state.
 *
 * @param message Message to log
 * @param loc Source location (automatically captured)
 * @return VoidResult indicating success or error
 */
inline VoidResult log_critical(std::string_view message,
                               const source_location& loc = source_location::current()) {
    return log(interfaces::log_level::critical, message, loc);
}

// =============================================================================
// Level-specific logging functions with named logger
// =============================================================================

/**
 * @brief Log a trace-level message to a named logger.
 *
 * @param message Message to log
 * @param logger_name Name of the logger
 * @param loc Source location (automatically captured)
 * @return VoidResult indicating success or error
 */
inline VoidResult log_trace(std::string_view message,
                            const std::string& logger_name,
                            const source_location& loc = source_location::current()) {
    return log(interfaces::log_level::trace, message, logger_name, loc);
}

/**
 * @brief Log a debug-level message to a named logger.
 *
 * @param message Message to log
 * @param logger_name Name of the logger
 * @param loc Source location (automatically captured)
 * @return VoidResult indicating success or error
 */
inline VoidResult log_debug(std::string_view message,
                            const std::string& logger_name,
                            const source_location& loc = source_location::current()) {
    return log(interfaces::log_level::debug, message, logger_name, loc);
}

/**
 * @brief Log an info-level message to a named logger.
 *
 * @param message Message to log
 * @param logger_name Name of the logger
 * @param loc Source location (automatically captured)
 * @return VoidResult indicating success or error
 */
inline VoidResult log_info(std::string_view message,
                           const std::string& logger_name,
                           const source_location& loc = source_location::current()) {
    return log(interfaces::log_level::info, message, logger_name, loc);
}

/**
 * @brief Log a warning-level message to a named logger.
 *
 * @param message Message to log
 * @param logger_name Name of the logger
 * @param loc Source location (automatically captured)
 * @return VoidResult indicating success or error
 */
inline VoidResult log_warning(std::string_view message,
                              const std::string& logger_name,
                              const source_location& loc = source_location::current()) {
    return log(interfaces::log_level::warning, message, logger_name, loc);
}

/**
 * @brief Log an error-level message to a named logger.
 *
 * @param message Message to log
 * @param logger_name Name of the logger
 * @param loc Source location (automatically captured)
 * @return VoidResult indicating success or error
 */
inline VoidResult log_error(std::string_view message,
                            const std::string& logger_name,
                            const source_location& loc = source_location::current()) {
    return log(interfaces::log_level::error, message, logger_name, loc);
}

/**
 * @brief Log a critical-level message to a named logger.
 *
 * @param message Message to log
 * @param logger_name Name of the logger
 * @param loc Source location (automatically captured)
 * @return VoidResult indicating success or error
 */
inline VoidResult log_critical(std::string_view message,
                               const std::string& logger_name,
                               const source_location& loc = source_location::current()) {
    return log(interfaces::log_level::critical, message, logger_name, loc);
}

// =============================================================================
// Utility functions
// =============================================================================

/**
 * @brief Check if a log level is enabled for the default logger.
 *
 * Use this to avoid expensive message formatting when the log level
 * is disabled.
 *
 * @param level Log level to check
 * @return true if the level is enabled
 */
inline bool is_enabled(interfaces::log_level level) {
    return interfaces::get_logger()->is_enabled(level);
}

/**
 * @brief Check if a log level is enabled for a named logger.
 *
 * @param level Log level to check
 * @param logger_name Name of the logger
 * @return true if the level is enabled
 */
inline bool is_enabled(interfaces::log_level level, const std::string& logger_name) {
    return interfaces::get_logger(logger_name)->is_enabled(level);
}

/**
 * @brief Flush all buffered log messages for the default logger.
 *
 * @return VoidResult indicating success or error
 */
inline VoidResult flush() {
    return interfaces::get_logger()->flush();
}

/**
 * @brief Flush all buffered log messages for a named logger.
 *
 * @param logger_name Name of the logger
 * @return VoidResult indicating success or error
 */
inline VoidResult flush(const std::string& logger_name) {
    return interfaces::get_logger(logger_name)->flush();
}

} // namespace kcenon::common::logging
