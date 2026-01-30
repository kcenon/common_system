// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file stats_interface.h
 * @brief Standard statistics interface for all systems.
 *
 * Provides a unified way for components to expose statistics and metrics
 * that can be collected by monitoring systems.
 *
 * @note Issue #316: Create IStats common interface for statistics
 */

#pragma once

#include "stats_snapshot.h"
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>

namespace kcenon::common::interfaces {

/**
 * @brief Type-safe value type for statistics.
 *
 * Supports common metric types: counters, gauges, rates, and boolean flags.
 */
using stats_value = std::variant<
    std::int64_t,    ///< Integer counters (request counts, error counts)
    double,          ///< Floating-point gauges (rates, percentages, latencies)
    std::string,     ///< String values (state names, labels)
    bool             ///< Boolean flags (enabled/disabled, healthy/unhealthy)
>;

/**
 * @interface IStats
 * @brief Interface for components that expose statistics.
 *
 * All systems that want to provide metrics for monitoring should
 * implement this interface. This enables the monitoring_system to
 * generically collect stats from any component.
 *
 * Usage Example:
 * @code
 * class my_component : public IStats {
 * public:
 *     auto get_stats() const -> std::unordered_map<std::string, stats_value> override {
 *         return {
 *             {"request_count", request_count_},
 *             {"error_rate", calculate_error_rate()},
 *             {"is_healthy", is_healthy_}
 *         };
 *     }
 *
 *     auto to_json() const -> std::string override {
 *         auto snapshot = get_snapshot();
 *         return snapshot.to_json();
 *     }
 *
 *     auto name() const -> std::string_view override {
 *         return "my_component";
 *     }
 * };
 * @endcode
 *
 * Thread Safety:
 * - Implementations should ensure get_stats() is thread-safe.
 * - The interface itself does not mandate internal synchronization,
 *   allowing implementations to choose appropriate locking strategies.
 */
class IStats {
public:
    virtual ~IStats() = default;

    /**
     * @brief Get current statistics as key-value pairs.
     *
     * Returns a snapshot of current metrics. Keys should be descriptive
     * and stable across calls (use snake_case naming convention).
     *
     * Common metric names:
     * - Counters: "request_count", "error_count", "total_operations"
     * - Rates: "error_rate", "success_rate", "failure_rate"
     * - Gauges: "active_connections", "queue_size", "memory_usage_mb"
     * - State: "current_state", "is_healthy", "is_enabled"
     *
     * @return Map of metric names to values
     */
    [[nodiscard]] virtual auto get_stats() const -> std::unordered_map<std::string, stats_value> = 0;

    /**
     * @brief Get statistics as JSON string.
     *
     * Serializes current statistics to JSON format for logging,
     * monitoring, or API responses.
     *
     * @return JSON string representation (e.g., {"request_count": 100, "error_rate": 0.05})
     */
    [[nodiscard]] virtual auto to_json() const -> std::string = 0;

    /**
     * @brief Get component name for identification.
     *
     * Returns a unique identifier for this component. Used by
     * monitoring systems to distinguish stats from different sources.
     *
     * @return Component name (e.g., "circuit_breaker", "http_client", "database_pool")
     */
    [[nodiscard]] virtual auto name() const -> std::string_view = 0;

    /**
     * @brief Get a complete statistics snapshot with metadata.
     *
     * Convenience method that bundles stats with component name and timestamp.
     * Useful for monitoring systems that need full context.
     *
     * @return Complete snapshot with component name, timestamp, and values
     */
    [[nodiscard]] virtual auto get_snapshot() const -> stats_snapshot
    {
        return stats_snapshot{
            .component_name = std::string(name()),
            .timestamp = std::chrono::system_clock::now(),
            .values = get_stats()
        };
    }
};

} // namespace kcenon::common::interfaces
