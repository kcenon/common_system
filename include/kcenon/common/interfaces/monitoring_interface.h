// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file monitoring_interface.h
 * @brief Standard monitoring interface for all systems.
 *
 * This header defines the standard monitoring interface to be used across
 * all systems for consistent metrics and health monitoring.
 */

#pragma once

#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include "../patterns/result.h"

namespace common {
namespace interfaces {

/**
 * @struct metric_value
 * @brief Standard metric value structure
 */
struct metric_value {
    std::string name;
    double value;
    std::chrono::system_clock::time_point timestamp;
    std::unordered_map<std::string, std::string> tags;

    metric_value(const std::string& n = "", double v = 0.0)
        : name(n)
        , value(v)
        , timestamp(std::chrono::system_clock::now()) {}
};

/**
 * @struct metrics_snapshot
 * @brief Complete snapshot of metrics at a point in time
 */
struct metrics_snapshot {
    std::vector<metric_value> metrics;
    std::chrono::system_clock::time_point capture_time;
    std::string source_id;

    metrics_snapshot()
        : capture_time(std::chrono::system_clock::now()) {}

    void add_metric(const std::string& name, double value) {
        metrics.emplace_back(name, value);
    }
};

/**
 * @enum health_status
 * @brief Standard health status levels
 */
enum class health_status {
    healthy = 0,
    degraded = 1,
    unhealthy = 2,
    unknown = 3
};

/**
 * @struct health_check_result
 * @brief Result of a health check operation
 */
struct health_check_result {
    health_status status = health_status::unknown;
    std::string message;
    std::chrono::system_clock::time_point timestamp;
    std::chrono::milliseconds check_duration{0};
    std::unordered_map<std::string, std::string> metadata;

    health_check_result()
        : timestamp(std::chrono::system_clock::now()) {}

    bool is_healthy() const {
        return status == health_status::healthy;
    }

    bool is_operational() const {
        return status == health_status::healthy ||
               status == health_status::degraded;
    }
};

/**
 * @interface IMonitor
 * @brief Standard interface for monitoring implementations
 *
 * This interface defines the contract for any monitoring implementation,
 * allowing modules to collect metrics and check health status.
 */
class IMonitor {
public:
    virtual ~IMonitor() = default;

    /**
     * @brief Record a metric value
     * @param name Metric name
     * @param value Metric value
     * @return VoidResult indicating success or error
     */
    virtual VoidResult record_metric(const std::string& name, double value) = 0;

    /**
     * @brief Record a metric with tags
     * @param name Metric name
     * @param value Metric value
     * @param tags Additional metadata tags
     * @return VoidResult indicating success or error
     */
    virtual VoidResult record_metric(
        const std::string& name,
        double value,
        const std::unordered_map<std::string, std::string>& tags) = 0;

    /**
     * @brief Get current metrics snapshot
     * @return Result containing metrics snapshot or error
     */
    virtual Result<metrics_snapshot> get_metrics() = 0;

    /**
     * @brief Perform health check
     * @return Result containing health check result or error
     */
    virtual Result<health_check_result> check_health() = 0;

    /**
     * @brief Reset all metrics
     * @return VoidResult indicating success or error
     */
    virtual VoidResult reset() = 0;
};

/**
 * @interface IMonitorable
 * @brief Interface for objects that can be monitored
 *
 * This interface allows objects to expose their internal state
 * for monitoring purposes.
 */
class IMonitorable {
public:
    virtual ~IMonitorable() = default;

    /**
     * @brief Get monitoring data
     * @return Result containing metrics snapshot or error
     */
    virtual Result<metrics_snapshot> get_monitoring_data() = 0;

    /**
     * @brief Check health status
     * @return Result containing health check result or error
     */
    virtual Result<health_check_result> health_check() = 0;

    /**
     * @brief Get component name for monitoring
     * @return Component identifier
     */
    virtual std::string get_component_name() const = 0;
};

/**
 * @brief Factory function type for creating monitor instances
 */
using MonitorFactory = std::function<std::shared_ptr<IMonitor>()>;

/**
 * @interface IMonitorProvider
 * @brief Interface for modules that provide monitor implementations
 */
class IMonitorProvider {
public:
    virtual ~IMonitorProvider() = default;

    /**
     * @brief Get the default monitor instance
     * @return Shared pointer to the monitor
     */
    virtual std::shared_ptr<IMonitor> get_monitor() = 0;

    /**
     * @brief Create a new monitor with specific name
     * @param name Monitor name
     * @return Shared pointer to the new monitor
     */
    virtual std::shared_ptr<IMonitor> create_monitor(const std::string& name) = 0;
};

/**
 * @brief Convert health status to string
 */
inline std::string to_string(health_status status) {
    switch(status) {
        case health_status::healthy: return "HEALTHY";
        case health_status::degraded: return "DEGRADED";
        case health_status::unhealthy: return "UNHEALTHY";
        case health_status::unknown: return "UNKNOWN";
        default: return "UNKNOWN";
    }
}

} // namespace interfaces
} // namespace common