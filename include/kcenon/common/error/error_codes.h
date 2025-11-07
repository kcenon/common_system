// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file error_codes.h
 * @brief Centralized error code registry for all systems
 *
 * This file defines error code ranges and specific codes for all systems.
 * Error codes are organized by category to prevent conflicts.
 *
 * Error Code Ranges:
 * - 0: Success
 * - -1 to -99: Common errors
 * - -100 to -199: thread_system errors
 * - -200 to -299: logger_system errors
 * - -300 to -399: monitoring_system errors
 * - -400 to -499: container_system errors
 * - -500 to -599: database_system errors
 * - -600 to -699: network_system errors
 * - -1000+: Reserved for future systems
 */

#pragma once

#include <string>
#include <string_view>

namespace kcenon::common {
namespace error {

/**
 * @enum category
 * @brief Error code category ranges for compile-time validation
 */
enum class category : int {
    success = 0,
    common = -1,
    thread_system = -100,
    logger_system = -200,
    monitoring_system = -300,
    container_system = -400,
    database_system = -500,
    network_system = -600,
};

/**
 * @namespace codes
 * @brief Specific error codes organized by category
 */
namespace codes {

// ============================================================================
// Common Error Codes (-1 to -99)
// ============================================================================
namespace common_errors {
    constexpr int success = 0;
    constexpr int invalid_argument = -1;
    constexpr int not_found = -2;
    constexpr int permission_denied = -3;
    constexpr int timeout = -4;
    constexpr int cancelled = -5;
    constexpr int not_initialized = -6;
    constexpr int already_exists = -7;
    constexpr int out_of_memory = -8;
    constexpr int io_error = -9;
    constexpr int network_error = -10;
    constexpr int internal_error = -99;
} // namespace common_errors

// ============================================================================
// thread_system Error Codes (-100 to -199)
// ============================================================================
namespace thread_system {
    constexpr int base = static_cast<int>(category::thread_system);

    // Pool errors (-100 to -119)
    constexpr int pool_full = base - 0;
    constexpr int pool_shutdown = base - 1;
    constexpr int pool_not_started = base - 2;
    constexpr int invalid_pool_size = base - 3;

    // Worker errors (-120 to -139)
    constexpr int worker_failed = base - 20;
    constexpr int worker_not_found = base - 21;
    constexpr int worker_busy = base - 22;

    // Job errors (-140 to -159)
    constexpr int job_rejected = base - 40;
    constexpr int job_timeout = base - 41;
    constexpr int job_cancelled = base - 42;
    constexpr int invalid_job = base - 43;

    // Queue errors (-160 to -179)
    constexpr int queue_full = base - 60;
    constexpr int queue_empty = base - 61;
    constexpr int queue_stopped = base - 62;
} // namespace thread_system

// ============================================================================
// logger_system Error Codes (-200 to -299)
// ============================================================================
namespace logger_system {
    constexpr int base = static_cast<int>(category::logger_system);

    // File errors (-200 to -219)
    constexpr int file_open_failed = base - 0;
    constexpr int file_write_failed = base - 1;
    constexpr int file_close_failed = base - 2;
    constexpr int file_rotation_failed = base - 3;
    constexpr int file_permission_denied = base - 4;

    // Writer errors (-220 to -239)
    constexpr int writer_not_initialized = base - 20;
    constexpr int writer_stopped = base - 21;
    constexpr int writer_full = base - 22;
    constexpr int async_writer_failed = base - 23;

    // Format errors (-240 to -259)
    constexpr int invalid_format = base - 40;
    constexpr int format_error = base - 41;

    // Filter errors (-260 to -279)
    constexpr int invalid_filter = base - 60;
    constexpr int filter_rejected = base - 61;
} // namespace logger_system

// ============================================================================
// monitoring_system Error Codes (-300 to -399)
// ============================================================================
namespace monitoring_system {
    constexpr int base = static_cast<int>(category::monitoring_system);

    // Metric errors (-300 to -319)
    constexpr int metric_not_found = base - 0;
    constexpr int invalid_metric_type = base - 1;
    constexpr int metric_collection_failed = base - 2;

    // Storage errors (-320 to -339)
    constexpr int storage_full = base - 20;
    constexpr int storage_error = base - 21;

    // Event errors (-340 to -359)
    constexpr int event_publish_failed = base - 40;
    constexpr int event_subscribe_failed = base - 41;
    constexpr int invalid_event_type = base - 42;

    // Profiler errors (-360 to -379)
    constexpr int profiler_not_enabled = base - 60;
    constexpr int profiler_error = base - 61;
} // namespace monitoring_system

// ============================================================================
// container_system Error Codes (-400 to -499)
// ============================================================================
namespace container_system {
    constexpr int base = static_cast<int>(category::container_system);

    // Value errors (-400 to -419)
    constexpr int value_type_mismatch = base - 0;
    constexpr int invalid_value_type = base - 1;
    constexpr int value_conversion_failed = base - 2;

    // Serialization errors (-420 to -439)
    constexpr int serialization_failed = base - 20;
    constexpr int deserialization_failed = base - 21;
    constexpr int invalid_format = base - 22;

    // Memory pool errors (-440 to -459)
    constexpr int pool_exhausted = base - 40;
    constexpr int allocation_failed = base - 41;
    constexpr int invalid_allocation_size = base - 42;

    // Container errors (-460 to -479)
    constexpr int key_not_found = base - 60;
    constexpr int duplicate_key = base - 61;
    constexpr int container_full = base - 62;
} // namespace container_system

// ============================================================================
// database_system Error Codes (-500 to -599)
// ============================================================================
namespace database_system {
    constexpr int base = static_cast<int>(category::database_system);

    // Connection errors (-500 to -519)
    constexpr int connection_failed = base - 0;
    constexpr int connection_lost = base - 1;
    constexpr int connection_timeout = base - 2;
    constexpr int invalid_connection_string = base - 3;

    // Pool errors (-520 to -539)
    constexpr int pool_exhausted = base - 20;
    constexpr int pool_shutdown = base - 21;
    constexpr int pool_timeout = base - 22;

    // Query errors (-540 to -559)
    constexpr int query_failed = base - 40;
    constexpr int query_syntax_error = base - 41;
    constexpr int query_timeout = base - 42;

    // Transaction errors (-560 to -579)
    constexpr int transaction_failed = base - 60;
    constexpr int transaction_rolled_back = base - 61;
    constexpr int transaction_timeout = base - 62;
} // namespace database_system

// ============================================================================
// network_system Error Codes (-600 to -699)
// ============================================================================
namespace network_system {
    constexpr int base = static_cast<int>(category::network_system);

    // Connection errors (-600 to -619)
    constexpr int connection_failed = base - 0;
    constexpr int connection_refused = base - 1;
    constexpr int connection_timeout = base - 2;
    constexpr int connection_closed = base - 3;

    // Session errors (-620 to -639)
    constexpr int session_not_found = base - 20;
    constexpr int session_expired = base - 21;
    constexpr int invalid_session = base - 22;

    // Send/Receive errors (-640 to -659)
    constexpr int send_failed = base - 40;
    constexpr int receive_failed = base - 41;
    constexpr int message_too_large = base - 42;

    // Server errors (-660 to -679)
    constexpr int server_not_started = base - 60;
    constexpr int server_already_running = base - 61;
    constexpr int bind_failed = base - 62;
} // namespace network_system

} // namespace codes

// ============================================================================
// Compile-time Range Validation
// ============================================================================

/**
 * @brief Compile-time validation to prevent error code conflicts
 *
 * These static assertions ensure that error code ranges don't overlap
 * between systems. Each system has a dedicated range of 100 error codes.
 * If a new error code is added outside its designated range, compilation
 * will fail with a descriptive error message.
 */
namespace validation {

// Validate thread_system range (-100 to -199)
static_assert(codes::thread_system::base == -100,
              "thread_system base must be -100");
static_assert(codes::thread_system::pool_full >= -199 && codes::thread_system::pool_full <= -100,
              "thread_system error codes must be in range [-199, -100]");
static_assert(codes::thread_system::queue_stopped >= -199 && codes::thread_system::queue_stopped <= -100,
              "thread_system error codes must be in range [-199, -100]");

// Validate logger_system range (-200 to -299)
static_assert(codes::logger_system::base == -200,
              "logger_system base must be -200");
static_assert(codes::logger_system::file_open_failed >= -299 && codes::logger_system::file_open_failed <= -200,
              "logger_system error codes must be in range [-299, -200]");
static_assert(codes::logger_system::filter_rejected >= -299 && codes::logger_system::filter_rejected <= -200,
              "logger_system error codes must be in range [-299, -200]");

// Validate monitoring_system range (-300 to -399)
static_assert(codes::monitoring_system::base == -300,
              "monitoring_system base must be -300");
static_assert(codes::monitoring_system::metric_not_found >= -399 && codes::monitoring_system::metric_not_found <= -300,
              "monitoring_system error codes must be in range [-399, -300]");
static_assert(codes::monitoring_system::profiler_error >= -399 && codes::monitoring_system::profiler_error <= -300,
              "monitoring_system error codes must be in range [-399, -300]");

// Validate container_system range (-400 to -499)
static_assert(codes::container_system::base == -400,
              "container_system base must be -400");
static_assert(codes::container_system::value_type_mismatch >= -499 && codes::container_system::value_type_mismatch <= -400,
              "container_system error codes must be in range [-499, -400]");
static_assert(codes::container_system::container_full >= -499 && codes::container_system::container_full <= -400,
              "container_system error codes must be in range [-499, -400]");

// Validate database_system range (-500 to -599)
static_assert(codes::database_system::base == -500,
              "database_system base must be -500");
static_assert(codes::database_system::connection_failed >= -599 && codes::database_system::connection_failed <= -500,
              "database_system error codes must be in range [-599, -500]");
static_assert(codes::database_system::transaction_timeout >= -599 && codes::database_system::transaction_timeout <= -500,
              "database_system error codes must be in range [-599, -500]");

// Validate network_system range (-600 to -699)
static_assert(codes::network_system::base == -600,
              "network_system base must be -600");
static_assert(codes::network_system::connection_failed >= -699 && codes::network_system::connection_failed <= -600,
              "network_system error codes must be in range [-699, -600]");
static_assert(codes::network_system::bind_failed >= -699 && codes::network_system::bind_failed <= -600,
              "network_system error codes must be in range [-699, -600]");

// Validate common errors range (-1 to -99)
static_assert(codes::common_errors::success == 0, "Success code must be 0");
static_assert(codes::common_errors::invalid_argument >= -99 && codes::common_errors::invalid_argument <= -1,
              "common error codes must be in range [-99, -1]");
static_assert(codes::common_errors::internal_error >= -99 && codes::common_errors::internal_error <= -1,
              "common error codes must be in range [-99, -1]");

} // namespace validation

/**
 * @brief Get human-readable error message for error code
 * @param code Error code
 * @return Error message string
 */
inline std::string_view get_error_message(int code) {
    switch (code) {
        // Common errors
        case codes::common_errors::success: return "Success";
        case codes::common_errors::invalid_argument: return "Invalid argument";
        case codes::common_errors::not_found: return "Not found";
        case codes::common_errors::permission_denied: return "Permission denied";
        case codes::common_errors::timeout: return "Timeout";
        case codes::common_errors::cancelled: return "Cancelled";
        case codes::common_errors::not_initialized: return "Not initialized";
        case codes::common_errors::already_exists: return "Already exists";
        case codes::common_errors::out_of_memory: return "Out of memory";
        case codes::common_errors::io_error: return "I/O error";
        case codes::common_errors::network_error: return "Network error";
        case codes::common_errors::internal_error: return "Internal error";

        // thread_system errors
        case codes::thread_system::pool_full: return "Thread pool full";
        case codes::thread_system::pool_shutdown: return "Thread pool shutdown";
        case codes::thread_system::job_rejected: return "Job rejected";
        case codes::thread_system::job_timeout: return "Job timeout";

        // logger_system errors
        case codes::logger_system::file_open_failed: return "Failed to open log file";
        case codes::logger_system::file_write_failed: return "Failed to write to log file";
        case codes::logger_system::file_rotation_failed: return "Log file rotation failed";

        // monitoring_system errors
        case codes::monitoring_system::metric_not_found: return "Metric not found";
        case codes::monitoring_system::storage_full: return "Metric storage full";

        // container_system errors
        case codes::container_system::value_type_mismatch: return "Value type mismatch";
        case codes::container_system::serialization_failed: return "Serialization failed";
        case codes::container_system::pool_exhausted: return "Memory pool exhausted";

        // database_system errors
        case codes::database_system::connection_failed: return "Database connection failed";
        case codes::database_system::pool_exhausted: return "Connection pool exhausted";
        case codes::database_system::query_failed: return "Database query failed";

        // network_system errors
        case codes::network_system::connection_failed: return "Network connection failed";
        case codes::network_system::send_failed: return "Network send failed";
        case codes::network_system::server_not_started: return "Server not started";

        default: return "Unknown error";
    }
}

/**
 * @brief Get category name for error code
 * @param code Error code
 * @return Category name
 */
inline std::string_view get_category_name(int code) {
    if (code >= 0) return "Success";
    if (code > static_cast<int>(category::thread_system)) return "Common";
    if (code > static_cast<int>(category::logger_system)) return "ThreadSystem";
    if (code > static_cast<int>(category::monitoring_system)) return "LoggerSystem";
    if (code > static_cast<int>(category::container_system)) return "MonitoringSystem";
    if (code > static_cast<int>(category::database_system)) return "ContainerSystem";
    if (code > static_cast<int>(category::network_system)) return "DatabaseSystem";
    return "NetworkSystem";
}

} // namespace error
} // namespace kcenon::common
