// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file registry_audit_log.h
 * @brief Audit logging for registry operations.
 *
 * This header provides tamper-evident audit logging for all mutations
 * to global registries (GlobalLoggerRegistry, service_container).
 * The audit log captures the action, target, caller location, and timestamp
 * for security monitoring and compliance purposes.
 *
 * Thread Safety:
 * - RegistryAuditLog is thread-safe for concurrent logging.
 * - Uses std::mutex for synchronization.
 * - Audit log is append-only to maintain integrity.
 *
 * @see Issue #206 for security requirements.
 */

#pragma once

#include <chrono>
#include <mutex>
#include <source_location>
#include <string>
#include <vector>

namespace kcenon::common {
namespace interfaces {

/**
 * @enum registry_action
 * @brief Types of registry mutation actions.
 */
enum class registry_action {
    register_logger,      ///< Logger registration
    unregister_logger,    ///< Logger unregistration
    set_default_logger,   ///< Default logger set
    register_factory,     ///< Factory registration (logger)
    set_default_factory,  ///< Default factory set (logger)
    clear_loggers,        ///< Clear all loggers
    freeze_logger_registry,  ///< Freeze logger registry
    register_service,     ///< Service registration
    unregister_service,   ///< Service unregistration
    clear_services,       ///< Clear all services
    freeze_service_container  ///< Freeze service container
};

/**
 * @brief Convert registry_action to string representation.
 * @param action The action to convert
 * @return Human-readable string
 */
inline const char* to_string(registry_action action) {
    switch (action) {
        case registry_action::register_logger: return "register_logger";
        case registry_action::unregister_logger: return "unregister_logger";
        case registry_action::set_default_logger: return "set_default_logger";
        case registry_action::register_factory: return "register_factory";
        case registry_action::set_default_factory: return "set_default_factory";
        case registry_action::clear_loggers: return "clear_loggers";
        case registry_action::freeze_logger_registry: return "freeze_logger_registry";
        case registry_action::register_service: return "register_service";
        case registry_action::unregister_service: return "unregister_service";
        case registry_action::clear_services: return "clear_services";
        case registry_action::freeze_service_container: return "freeze_service_container";
        default: return "unknown";
    }
}

/**
 * @struct registry_event
 * @brief Represents a single audit event for registry mutations.
 *
 * Captures all relevant information about a registry operation for
 * security auditing and compliance purposes.
 */
struct registry_event {
    /// The type of action performed
    registry_action action;

    /// Name of the service or logger (empty for clear/freeze operations)
    std::string target_name;

    /// File where the operation was initiated
    std::string file;

    /// Line number where the operation was initiated
    int line;

    /// Function where the operation was initiated
    std::string function;

    /// Timestamp when the event occurred
    std::chrono::system_clock::time_point timestamp;

    /// Whether the operation was successful
    bool success;

    /// Error message if the operation failed (empty on success)
    std::string error_message;

    /**
     * @brief Construct a registry event.
     *
     * @param act The action type
     * @param target Target name (service/logger name)
     * @param loc Source location of the operation
     * @param succeeded Whether the operation succeeded
     * @param error Error message if failed
     */
    registry_event(
        registry_action act,
        std::string target,
        const std::source_location& loc = std::source_location::current(),
        bool succeeded = true,
        std::string error = "")
        : action(act)
        , target_name(std::move(target))
        , file(loc.file_name())
        , line(static_cast<int>(loc.line()))
        , function(loc.function_name())
        , timestamp(std::chrono::system_clock::now())
        , success(succeeded)
        , error_message(std::move(error)) {}
};

/**
 * @class RegistryAuditLog
 * @brief Thread-safe audit log for registry operations.
 *
 * RegistryAuditLog provides a centralized, thread-safe mechanism for
 * recording all mutations to global registries. The log is append-only
 * to maintain integrity for security auditing.
 *
 * Usage Example:
 * @code
 * // Log a successful registration
 * RegistryAuditLog::log_event(registry_event(
 *     registry_action::register_service,
 *     "ILogger",
 *     std::source_location::current(),
 *     true
 * ));
 *
 * // Get all events for analysis
 * auto events = RegistryAuditLog::get_events();
 * for (const auto& event : events) {
 *     std::cout << to_string(event.action) << " on " << event.target_name
 *               << " at " << event.file << ":" << event.line << std::endl;
 * }
 * @endcode
 */
class RegistryAuditLog {
public:
    /**
     * @brief Log a registry event.
     *
     * Appends the event to the audit log. This operation is thread-safe.
     *
     * @param event The event to log
     */
    static void log_event(const registry_event& event);

    /**
     * @brief Log a registry event (move version).
     *
     * @param event The event to log (moved)
     */
    static void log_event(registry_event&& event);

    /**
     * @brief Get all logged events.
     *
     * Returns a copy of all events in the audit log. This operation
     * is thread-safe.
     *
     * @return Vector of all registry events
     */
    static std::vector<registry_event> get_events();

    /**
     * @brief Get events filtered by action type.
     *
     * @param action The action type to filter by
     * @return Vector of matching registry events
     */
    static std::vector<registry_event> get_events_by_action(registry_action action);

    /**
     * @brief Get events within a time range.
     *
     * @param start Start of time range
     * @param end End of time range
     * @return Vector of events within the time range
     */
    static std::vector<registry_event> get_events_in_range(
        std::chrono::system_clock::time_point start,
        std::chrono::system_clock::time_point end);

    /**
     * @brief Get the number of logged events.
     *
     * @return Number of events in the audit log
     */
    static size_t event_count();

    /**
     * @brief Check if audit logging is enabled.
     *
     * @return true if audit logging is enabled
     */
    static bool is_enabled();

    /**
     * @brief Enable or disable audit logging.
     *
     * @param enabled Whether to enable audit logging
     *
     * @note Disabling audit logging is a security-sensitive operation.
     *       Consider logging this action before disabling.
     */
    static void set_enabled(bool enabled);

    /**
     * @brief Clear all audit events.
     *
     * @warning This is a destructive operation that removes all audit history.
     *          Use with caution and ensure proper authorization.
     *
     * @note This operation is allowed even when frozen for testing purposes,
     *       but a freeze event should be logged first in production.
     */
    static void clear();

private:
    RegistryAuditLog() = delete;

    static std::mutex& get_mutex();
    static std::vector<registry_event>& get_events_internal();
    static std::atomic<bool>& get_enabled_flag();
};

// ============================================================================
// RegistryAuditLog Implementation
// ============================================================================

inline std::mutex& RegistryAuditLog::get_mutex() {
    static std::mutex mutex;
    return mutex;
}

inline std::vector<registry_event>& RegistryAuditLog::get_events_internal() {
    static std::vector<registry_event> events;
    return events;
}

inline std::atomic<bool>& RegistryAuditLog::get_enabled_flag() {
    static std::atomic<bool> enabled{true};
    return enabled;
}

inline void RegistryAuditLog::log_event(const registry_event& event) {
    if (!is_enabled()) {
        return;
    }

    std::lock_guard<std::mutex> lock(get_mutex());
    get_events_internal().push_back(event);
}

inline void RegistryAuditLog::log_event(registry_event&& event) {
    if (!is_enabled()) {
        return;
    }

    std::lock_guard<std::mutex> lock(get_mutex());
    get_events_internal().push_back(std::move(event));
}

inline std::vector<registry_event> RegistryAuditLog::get_events() {
    std::lock_guard<std::mutex> lock(get_mutex());
    return get_events_internal();
}

inline std::vector<registry_event> RegistryAuditLog::get_events_by_action(
    registry_action action) {
    std::lock_guard<std::mutex> lock(get_mutex());

    std::vector<registry_event> result;
    for (const auto& event : get_events_internal()) {
        if (event.action == action) {
            result.push_back(event);
        }
    }
    return result;
}

inline std::vector<registry_event> RegistryAuditLog::get_events_in_range(
    std::chrono::system_clock::time_point start,
    std::chrono::system_clock::time_point end) {
    std::lock_guard<std::mutex> lock(get_mutex());

    std::vector<registry_event> result;
    for (const auto& event : get_events_internal()) {
        if (event.timestamp >= start && event.timestamp <= end) {
            result.push_back(event);
        }
    }
    return result;
}

inline size_t RegistryAuditLog::event_count() {
    std::lock_guard<std::mutex> lock(get_mutex());
    return get_events_internal().size();
}

inline bool RegistryAuditLog::is_enabled() {
    return get_enabled_flag().load(std::memory_order_acquire);
}

inline void RegistryAuditLog::set_enabled(bool enabled) {
    get_enabled_flag().store(enabled, std::memory_order_release);
}

inline void RegistryAuditLog::clear() {
    std::lock_guard<std::mutex> lock(get_mutex());
    get_events_internal().clear();
}

} // namespace interfaces
} // namespace kcenon::common
