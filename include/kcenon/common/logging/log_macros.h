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
 * - Legacy THREAD_LOG_* compatibility (redirects to LOG_*)
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
 *
 * // Legacy macros (deprecated, prefer LOG_* versions)
 * THREAD_LOG_INFO("Still works for compatibility");
 * @endcode
 *
 * @note Prefer using LOG_* macros over direct function calls for consistency.
 * @note Legacy THREAD_LOG_* macros are provided for backward compatibility
 *       but are marked as deprecated.
 *
 * @see Issue #175 for implementation requirements.
 * @see log_functions.h for the underlying function implementations.
 */

#pragma once

#include "log_functions.h"

// =============================================================================
// Primary LOG_* Macros
// =============================================================================

/**
 * @def LOG_TRACE(msg)
 * @brief Log a trace-level message.
 * @param msg The message to log (string or string_view compatible)
 */
#define LOG_TRACE(msg) \
    ::kcenon::common::logging::log_trace(msg)

/**
 * @def LOG_DEBUG(msg)
 * @brief Log a debug-level message.
 * @param msg The message to log (string or string_view compatible)
 */
#define LOG_DEBUG(msg) \
    ::kcenon::common::logging::log_debug(msg)

/**
 * @def LOG_INFO(msg)
 * @brief Log an info-level message.
 * @param msg The message to log (string or string_view compatible)
 */
#define LOG_INFO(msg) \
    ::kcenon::common::logging::log_info(msg)

/**
 * @def LOG_WARNING(msg)
 * @brief Log a warning-level message.
 * @param msg The message to log (string or string_view compatible)
 */
#define LOG_WARNING(msg) \
    ::kcenon::common::logging::log_warning(msg)

/**
 * @def LOG_ERROR(msg)
 * @brief Log an error-level message.
 * @param msg The message to log (string or string_view compatible)
 */
#define LOG_ERROR(msg) \
    ::kcenon::common::logging::log_error(msg)

/**
 * @def LOG_CRITICAL(msg)
 * @brief Log a critical-level message.
 * @param msg The message to log (string or string_view compatible)
 */
#define LOG_CRITICAL(msg) \
    ::kcenon::common::logging::log_critical(msg)

// =============================================================================
// Named Logger Macros
// =============================================================================

/**
 * @def LOG_TRACE_TO(logger_name, msg)
 * @brief Log a trace-level message to a named logger.
 * @param logger_name The name of the logger (string)
 * @param msg The message to log
 */
#define LOG_TRACE_TO(logger_name, msg) \
    ::kcenon::common::logging::log_trace(msg, logger_name)

/**
 * @def LOG_DEBUG_TO(logger_name, msg)
 * @brief Log a debug-level message to a named logger.
 * @param logger_name The name of the logger (string)
 * @param msg The message to log
 */
#define LOG_DEBUG_TO(logger_name, msg) \
    ::kcenon::common::logging::log_debug(msg, logger_name)

/**
 * @def LOG_INFO_TO(logger_name, msg)
 * @brief Log an info-level message to a named logger.
 * @param logger_name The name of the logger (string)
 * @param msg The message to log
 */
#define LOG_INFO_TO(logger_name, msg) \
    ::kcenon::common::logging::log_info(msg, logger_name)

/**
 * @def LOG_WARNING_TO(logger_name, msg)
 * @brief Log a warning-level message to a named logger.
 * @param logger_name The name of the logger (string)
 * @param msg The message to log
 */
#define LOG_WARNING_TO(logger_name, msg) \
    ::kcenon::common::logging::log_warning(msg, logger_name)

/**
 * @def LOG_ERROR_TO(logger_name, msg)
 * @brief Log an error-level message to a named logger.
 * @param logger_name The name of the logger (string)
 * @param msg The message to log
 */
#define LOG_ERROR_TO(logger_name, msg) \
    ::kcenon::common::logging::log_error(msg, logger_name)

/**
 * @def LOG_CRITICAL_TO(logger_name, msg)
 * @brief Log a critical-level message to a named logger.
 * @param logger_name The name of the logger (string)
 * @param msg The message to log
 */
#define LOG_CRITICAL_TO(logger_name, msg) \
    ::kcenon::common::logging::log_critical(msg, logger_name)

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
// Legacy Compatibility Macros (Deprecated)
// =============================================================================

/**
 * @def THREAD_LOG_TRACE(msg)
 * @brief Legacy trace logging macro.
 * @deprecated Use LOG_TRACE instead.
 */
#define THREAD_LOG_TRACE(msg) LOG_TRACE(msg)

/**
 * @def THREAD_LOG_DEBUG(msg)
 * @brief Legacy debug logging macro.
 * @deprecated Use LOG_DEBUG instead.
 */
#define THREAD_LOG_DEBUG(msg) LOG_DEBUG(msg)

/**
 * @def THREAD_LOG_INFO(msg)
 * @brief Legacy info logging macro.
 * @deprecated Use LOG_INFO instead.
 */
#define THREAD_LOG_INFO(msg) LOG_INFO(msg)

/**
 * @def THREAD_LOG_WARNING(msg)
 * @brief Legacy warning logging macro.
 * @deprecated Use LOG_WARNING instead.
 */
#define THREAD_LOG_WARNING(msg) LOG_WARNING(msg)

/**
 * @def THREAD_LOG_ERROR(msg)
 * @brief Legacy error logging macro.
 * @deprecated Use LOG_ERROR instead.
 */
#define THREAD_LOG_ERROR(msg) LOG_ERROR(msg)

/**
 * @def THREAD_LOG_CRITICAL(msg)
 * @brief Legacy critical logging macro.
 * @deprecated Use LOG_CRITICAL instead.
 */
#define THREAD_LOG_CRITICAL(msg) LOG_CRITICAL(msg)

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
#if KCENON_MIN_LOG_LEVEL > 0
    #undef LOG_TRACE
    #define LOG_TRACE(msg) ((void)0)
    #undef LOG_TRACE_TO
    #define LOG_TRACE_TO(logger_name, msg) ((void)0)
    #undef THREAD_LOG_TRACE
    #define THREAD_LOG_TRACE(msg) ((void)0)
#endif

#if KCENON_MIN_LOG_LEVEL > 1
    #undef LOG_DEBUG
    #define LOG_DEBUG(msg) ((void)0)
    #undef LOG_DEBUG_TO
    #define LOG_DEBUG_TO(logger_name, msg) ((void)0)
    #undef THREAD_LOG_DEBUG
    #define THREAD_LOG_DEBUG(msg) ((void)0)
#endif

#if KCENON_MIN_LOG_LEVEL > 2
    #undef LOG_INFO
    #define LOG_INFO(msg) ((void)0)
    #undef LOG_INFO_TO
    #define LOG_INFO_TO(logger_name, msg) ((void)0)
    #undef THREAD_LOG_INFO
    #define THREAD_LOG_INFO(msg) ((void)0)
#endif

#if KCENON_MIN_LOG_LEVEL > 3
    #undef LOG_WARNING
    #define LOG_WARNING(msg) ((void)0)
    #undef LOG_WARNING_TO
    #define LOG_WARNING_TO(logger_name, msg) ((void)0)
    #undef THREAD_LOG_WARNING
    #define THREAD_LOG_WARNING(msg) ((void)0)
#endif

#if KCENON_MIN_LOG_LEVEL > 4
    #undef LOG_ERROR
    #define LOG_ERROR(msg) ((void)0)
    #undef LOG_ERROR_TO
    #define LOG_ERROR_TO(logger_name, msg) ((void)0)
    #undef THREAD_LOG_ERROR
    #define THREAD_LOG_ERROR(msg) ((void)0)
#endif

#if KCENON_MIN_LOG_LEVEL > 5
    #undef LOG_CRITICAL
    #define LOG_CRITICAL(msg) ((void)0)
    #undef LOG_CRITICAL_TO
    #define LOG_CRITICAL_TO(logger_name, msg) ((void)0)
    #undef THREAD_LOG_CRITICAL
    #define THREAD_LOG_CRITICAL(msg) ((void)0)
#endif
