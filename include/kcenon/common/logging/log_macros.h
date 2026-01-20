// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file log_macros.h
 * @brief Unified logging macros for convenient logging across all subsystems.
 *
 * This header provides preprocessor macros that wrap the logging functions
 * from log_functions.h. These macros offer a convenient, concise way to
 * log messages while automatically capturing source location information.
 *
 * The macros support:
 * - Standard LOG_* macros for each log level
 * - Conditional logging based on log level
 *
 * Thread Safety:
 * - All macros are thread-safe as they delegate to thread-safe functions.
 *
 * Usage:
 * @code
 * // Basic logging
 * LOG_INFO("Application started");
 * LOG_DEBUG("Processing item: " + std::to_string(id));
 *
 * // Conditional logging (avoids message construction when disabled)
 * LOG_IF(log_level::debug, expensive_to_string(data));
 * @endcode
 *
 * @note Prefer using LOG_* macros over direct function calls for consistency.
 *
 * @see Issue #175 for implementation requirements.
 * @see log_functions.h for the underlying function implementations.
 */

#pragma once

#include "log_functions.h"

// =============================================================================
// Variadic Macro Dispatch Helpers
// =============================================================================

/**
 * @brief Helper macros for variadic argument dispatch.
 *
 * These macros enable LOG_* macros to accept either 1 or 2 arguments:
 * - LOG_DEBUG(msg)              -> logs to default logger
 * - LOG_DEBUG(logger_name, msg) -> logs to named logger
 *
 * Implementation uses argument counting and token pasting for dispatch.
 */

// Argument counter: returns 2 for 2+ args, 1 for 1 arg
#define KCENON_LOG_ARG_COUNT(...) \
    KCENON_LOG_ARG_COUNT_IMPL(__VA_ARGS__, 2, 1)
#define KCENON_LOG_ARG_COUNT_IMPL(_1, _2, N, ...) N

// Token paste helpers for macro dispatch
#define KCENON_LOG_PASTE(a, b) KCENON_LOG_PASTE_IMPL(a, b)
#define KCENON_LOG_PASTE_IMPL(a, b) a##b

// Dispatch macro: calls LEVEL_1 or LEVEL_2 based on argument count
#define KCENON_LOG_DISPATCH(LEVEL, ...) \
    KCENON_LOG_PASTE(LEVEL##_, KCENON_LOG_ARG_COUNT(__VA_ARGS__))(__VA_ARGS__)

// =============================================================================
// Primary LOG_* Macros (Unified Variadic Interface)
// =============================================================================

/**
 * @def LOG_TRACE(...)
 * @brief Log a trace-level message.
 *
 * Supports two forms:
 * - LOG_TRACE(msg)              - Log to default logger
 * - LOG_TRACE(logger_name, msg) - Log to named logger
 *
 * @param msg The message to log (string or string_view compatible)
 * @param logger_name (optional) The name of the logger
 */
#define LOG_TRACE(...) KCENON_LOG_DISPATCH(LOG_TRACE, __VA_ARGS__)
#define LOG_TRACE_1(msg) \
    ::kcenon::common::logging::log_trace(msg)
#define LOG_TRACE_2(logger_name, msg) \
    ::kcenon::common::logging::log_trace_to(logger_name, msg)

/**
 * @def LOG_DEBUG(...)
 * @brief Log a debug-level message.
 *
 * Supports two forms:
 * - LOG_DEBUG(msg)              - Log to default logger
 * - LOG_DEBUG(logger_name, msg) - Log to named logger
 *
 * @param msg The message to log (string or string_view compatible)
 * @param logger_name (optional) The name of the logger
 */
#define LOG_DEBUG(...) KCENON_LOG_DISPATCH(LOG_DEBUG, __VA_ARGS__)
#define LOG_DEBUG_1(msg) \
    ::kcenon::common::logging::log_debug(msg)
#define LOG_DEBUG_2(logger_name, msg) \
    ::kcenon::common::logging::log_debug_to(logger_name, msg)

/**
 * @def LOG_INFO(...)
 * @brief Log an info-level message.
 *
 * Supports two forms:
 * - LOG_INFO(msg)              - Log to default logger
 * - LOG_INFO(logger_name, msg) - Log to named logger
 *
 * @param msg The message to log (string or string_view compatible)
 * @param logger_name (optional) The name of the logger
 */
#define LOG_INFO(...) KCENON_LOG_DISPATCH(LOG_INFO, __VA_ARGS__)
#define LOG_INFO_1(msg) \
    ::kcenon::common::logging::log_info(msg)
#define LOG_INFO_2(logger_name, msg) \
    ::kcenon::common::logging::log_info_to(logger_name, msg)

/**
 * @def LOG_WARNING(...)
 * @brief Log a warning-level message.
 *
 * Supports two forms:
 * - LOG_WARNING(msg)              - Log to default logger
 * - LOG_WARNING(logger_name, msg) - Log to named logger
 *
 * @param msg The message to log (string or string_view compatible)
 * @param logger_name (optional) The name of the logger
 */
#define LOG_WARNING(...) KCENON_LOG_DISPATCH(LOG_WARNING, __VA_ARGS__)
#define LOG_WARNING_1(msg) \
    ::kcenon::common::logging::log_warning(msg)
#define LOG_WARNING_2(logger_name, msg) \
    ::kcenon::common::logging::log_warning_to(logger_name, msg)

/**
 * @def LOG_ERROR(...)
 * @brief Log an error-level message.
 *
 * Supports two forms:
 * - LOG_ERROR(msg)              - Log to default logger
 * - LOG_ERROR(logger_name, msg) - Log to named logger
 *
 * @param msg The message to log (string or string_view compatible)
 * @param logger_name (optional) The name of the logger
 */
#define LOG_ERROR(...) KCENON_LOG_DISPATCH(LOG_ERROR, __VA_ARGS__)
#define LOG_ERROR_1(msg) \
    ::kcenon::common::logging::log_error(msg)
#define LOG_ERROR_2(logger_name, msg) \
    ::kcenon::common::logging::log_error_to(logger_name, msg)

/**
 * @def LOG_CRITICAL(...)
 * @brief Log a critical-level message.
 *
 * Supports two forms:
 * - LOG_CRITICAL(msg)              - Log to default logger
 * - LOG_CRITICAL(logger_name, msg) - Log to named logger
 *
 * @param msg The message to log (string or string_view compatible)
 * @param logger_name (optional) The name of the logger
 */
#define LOG_CRITICAL(...) KCENON_LOG_DISPATCH(LOG_CRITICAL, __VA_ARGS__)
#define LOG_CRITICAL_1(msg) \
    ::kcenon::common::logging::log_critical(msg)
#define LOG_CRITICAL_2(logger_name, msg) \
    ::kcenon::common::logging::log_critical_to(logger_name, msg)

// =============================================================================
// Named Logger Macros (Backward Compatibility)
// =============================================================================

/**
 * @def LOG_TRACE_TO(logger_name, msg)
 * @brief Log a trace-level message to a named logger.
 * @deprecated Use LOG_TRACE(logger_name, msg) instead.
 * @param logger_name The name of the logger (string)
 * @param msg The message to log
 */
#define LOG_TRACE_TO(logger_name, msg) \
    ::kcenon::common::logging::log_trace_to(logger_name, msg)

/**
 * @def LOG_DEBUG_TO(logger_name, msg)
 * @brief Log a debug-level message to a named logger.
 * @deprecated Use LOG_DEBUG(logger_name, msg) instead.
 * @param logger_name The name of the logger (string)
 * @param msg The message to log
 */
#define LOG_DEBUG_TO(logger_name, msg) \
    ::kcenon::common::logging::log_debug_to(logger_name, msg)

/**
 * @def LOG_INFO_TO(logger_name, msg)
 * @brief Log an info-level message to a named logger.
 * @deprecated Use LOG_INFO(logger_name, msg) instead.
 * @param logger_name The name of the logger (string)
 * @param msg The message to log
 */
#define LOG_INFO_TO(logger_name, msg) \
    ::kcenon::common::logging::log_info_to(logger_name, msg)

/**
 * @def LOG_WARNING_TO(logger_name, msg)
 * @brief Log a warning-level message to a named logger.
 * @deprecated Use LOG_WARNING(logger_name, msg) instead.
 * @param logger_name The name of the logger (string)
 * @param msg The message to log
 */
#define LOG_WARNING_TO(logger_name, msg) \
    ::kcenon::common::logging::log_warning_to(logger_name, msg)

/**
 * @def LOG_ERROR_TO(logger_name, msg)
 * @brief Log an error-level message to a named logger.
 * @deprecated Use LOG_ERROR(logger_name, msg) instead.
 * @param logger_name The name of the logger (string)
 * @param msg The message to log
 */
#define LOG_ERROR_TO(logger_name, msg) \
    ::kcenon::common::logging::log_error_to(logger_name, msg)

/**
 * @def LOG_CRITICAL_TO(logger_name, msg)
 * @brief Log a critical-level message to a named logger.
 * @deprecated Use LOG_CRITICAL(logger_name, msg) instead.
 * @param logger_name The name of the logger (string)
 * @param msg The message to log
 */
#define LOG_CRITICAL_TO(logger_name, msg) \
    ::kcenon::common::logging::log_critical_to(logger_name, msg)

// =============================================================================
// Conditional Logging Macros
// =============================================================================

/**
 * @def LOG_IF(level, msg)
 * @brief Log a message only if the specified level is enabled.
 *
 * This macro checks if the log level is enabled before evaluating
 * the message expression, which is useful for avoiding expensive
 * message construction when logging is disabled.
 *
 * @param level Log level (e.g., log_level::debug)
 * @param msg The message to log
 *
 * Usage:
 * @code
 * LOG_IF(log_level::debug, "Expensive data: " + expensive_to_string(data));
 * @endcode
 */
#define LOG_IF(level, msg) \
    do { \
        if (::kcenon::common::logging::is_enabled(level)) { \
            ::kcenon::common::logging::log(level, msg); \
        } \
    } while (0)

/**
 * @def LOG_IF_TO(level, logger_name, msg)
 * @brief Log a message to a named logger only if the level is enabled.
 *
 * @param level Log level (e.g., log_level::debug)
 * @param logger_name The name of the logger
 * @param msg The message to log
 */
#define LOG_IF_TO(level, logger_name, msg) \
    do { \
        if (::kcenon::common::logging::is_enabled(level, logger_name)) { \
            ::kcenon::common::logging::log(level, msg, logger_name); \
        } \
    } while (0)

// =============================================================================
// Utility Macros
// =============================================================================

/**
 * @def LOG_FLUSH()
 * @brief Flush the default logger's buffer.
 */
#define LOG_FLUSH() \
    ::kcenon::common::logging::flush()

/**
 * @def LOG_FLUSH_TO(logger_name)
 * @brief Flush a named logger's buffer.
 * @param logger_name The name of the logger
 */
#define LOG_FLUSH_TO(logger_name) \
    ::kcenon::common::logging::flush(logger_name)

/**
 * @def LOG_IS_ENABLED(level)
 * @brief Check if a log level is enabled for the default logger.
 * @param level Log level to check
 * @return true if the level is enabled
 */
#define LOG_IS_ENABLED(level) \
    ::kcenon::common::logging::is_enabled(level)

/**
 * @def LOG_IS_ENABLED_FOR(level, logger_name)
 * @brief Check if a log level is enabled for a named logger.
 * @param level Log level to check
 * @param logger_name The name of the logger
 * @return true if the level is enabled
 */
#define LOG_IS_ENABLED_FOR(level, logger_name) \
    ::kcenon::common::logging::is_enabled(level, logger_name)

// =============================================================================
// Compile-time Log Level Control (Optional)
// =============================================================================

/**
 * @def KCENON_MIN_LOG_LEVEL
 * @brief Minimum log level at compile time.
 *
 * Define this macro before including log_macros.h to disable
 * lower log levels at compile time. This removes the logging
 * calls entirely from the compiled code.
 *
 * Values:
 * - 0: trace (default, all levels enabled)
 * - 1: debug
 * - 2: info
 * - 3: warning
 * - 4: error
 * - 5: critical
 * - 6: off (all logging disabled)
 *
 * Usage:
 * @code
 * #define KCENON_MIN_LOG_LEVEL 2  // Disable trace and debug in release
 * #include <kcenon/common/logging/log_macros.h>
 * @endcode
 */
#ifndef KCENON_MIN_LOG_LEVEL
    #define KCENON_MIN_LOG_LEVEL 0
#endif

// Redefine macros if compile-time level filtering is enabled
// Each level is disabled when KCENON_MIN_LOG_LEVEL exceeds its threshold
// Note: variadic LOG_* macros are replaced with no-op, _TO variants kept for backward compat
#if KCENON_MIN_LOG_LEVEL > 0  // threshold: trace = 0
    #undef LOG_TRACE
    #undef LOG_TRACE_TO
    #define LOG_TRACE(...) ((void)0)
    #define LOG_TRACE_TO(logger_name, msg) ((void)0)
#endif

#if KCENON_MIN_LOG_LEVEL > 1  // threshold: debug = 1
    #undef LOG_DEBUG
    #undef LOG_DEBUG_TO
    #define LOG_DEBUG(...) ((void)0)
    #define LOG_DEBUG_TO(logger_name, msg) ((void)0)
#endif

#if KCENON_MIN_LOG_LEVEL > 2  // threshold: info = 2
    #undef LOG_INFO
    #undef LOG_INFO_TO
    #define LOG_INFO(...) ((void)0)
    #define LOG_INFO_TO(logger_name, msg) ((void)0)
#endif

#if KCENON_MIN_LOG_LEVEL > 3  // threshold: warning = 3
    #undef LOG_WARNING
    #undef LOG_WARNING_TO
    #define LOG_WARNING(...) ((void)0)
    #define LOG_WARNING_TO(logger_name, msg) ((void)0)
#endif

#if KCENON_MIN_LOG_LEVEL > 4  // threshold: error = 4
    #undef LOG_ERROR
    #undef LOG_ERROR_TO
    #define LOG_ERROR(...) ((void)0)
    #define LOG_ERROR_TO(logger_name, msg) ((void)0)
#endif

#if KCENON_MIN_LOG_LEVEL > 5  // threshold: critical = 5
    #undef LOG_CRITICAL
    #undef LOG_CRITICAL_TO
    #define LOG_CRITICAL(...) ((void)0)
    #define LOG_CRITICAL_TO(logger_name, msg) ((void)0)
#endif
