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

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include "../patterns/result.h"

namespace common {
namespace interfaces {

/**
 * @enum metric_type
 * @brief Types of metrics supported by the monitoring system
 */
enum class metric_type {
    gauge,      // Instant value that can go up or down
    counter,    // Monotonic increasing value
    histogram,  // Distribution of values across buckets
    summary     // Statistical summary (min, max, mean, percentiles)
};

/**
 * @brief Convert metric type to string
 */
inline std::string to_string(metric_type type) {
    switch(type) {
        case metric_type::gauge: return "GAUGE";
        case metric_type::counter: return "COUNTER";
        case metric_type::histogram: return "HISTOGRAM";
        case metric_type::summary: return "SUMMARY";
        default: return "UNKNOWN";
    }
}

/**
 * @brief Convert string to metric type
 */
inline Result<metric_type> metric_type_from_string(const std::string& str) {
    std::string upper = str;
    std::transform(upper.begin(), upper.end(), upper.begin(),
                   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });

    if (upper == "GAUGE") return ok(metric_type::gauge);
    if (upper == "COUNTER") return ok(metric_type::counter);
    if (upper == "HISTOGRAM") return ok(metric_type::histogram);
    if (upper == "SUMMARY") return ok(metric_type::summary);

    return Result<metric_type>(error_info{1, "Invalid metric type: " + str, "monitoring_interface"});
}

/**
 * @struct metric_value
 * @brief Standard metric value structure with type information
 */
struct metric_value {
    std::string name;
    double value;
    metric_type type = metric_type::gauge;
    std::chrono::system_clock::time_point timestamp;
    std::unordered_map<std::string, std::string> tags;

    metric_value(const std::string& n = "", double v = 0.0, metric_type t = metric_type::gauge)
        : name(n)
        , value(v)
        , type(t)
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
 * @struct thread_pool_metrics
 * @brief Specialized metrics for thread pool monitoring
 */
struct thread_pool_metrics {
    metric_value jobs_completed{"jobs_completed", 0, metric_type::counter};
    metric_value jobs_pending{"jobs_pending", 0, metric_type::gauge};
    metric_value worker_threads{"worker_threads", 0, metric_type::gauge};
    metric_value idle_threads{"idle_threads", 0, metric_type::gauge};
    metric_value average_latency_ns{"average_latency_ns", 0, metric_type::gauge};
    metric_value total_execution_time_ns{"total_execution_time_ns", 0, metric_type::counter};

    std::chrono::system_clock::time_point timestamp = std::chrono::system_clock::now();

    // Pool identification for multi-pool scenarios
    std::string pool_name;
    std::uint32_t pool_instance_id{0};

    thread_pool_metrics() = default;

    explicit thread_pool_metrics(const std::string& name, std::uint32_t instance_id = 0)
        : pool_name(name), pool_instance_id(instance_id) {}

    /**
     * @brief Convert to vector of metric_value
     */
    std::vector<metric_value> to_metrics() const {
        return {
            jobs_completed,
            jobs_pending,
            worker_threads,
            idle_threads,
            average_latency_ns,
            total_execution_time_ns
        };
    }
};

/**
 * @struct worker_metrics
 * @brief Specialized metrics for worker thread monitoring
 */
struct worker_metrics {
    metric_value jobs_processed{"jobs_processed", 0, metric_type::counter};
    metric_value total_processing_time_ns{"total_processing_time_ns", 0, metric_type::counter};
    metric_value idle_time_ns{"idle_time_ns", 0, metric_type::counter};
    metric_value context_switches{"context_switches", 0, metric_type::counter};

    std::chrono::system_clock::time_point timestamp = std::chrono::system_clock::now();
    std::size_t worker_id{0};

    worker_metrics() = default;

    explicit worker_metrics(std::size_t id) : worker_id(id) {}

    /**
     * @brief Convert to vector of metric_value
     */
    std::vector<metric_value> to_metrics() const {
        return {
            jobs_processed,
            total_processing_time_ns,
            idle_time_ns,
            context_switches
        };
    }
};

/**
 * @struct system_metrics
 * @brief Specialized metrics for system-level monitoring
 */
struct system_metrics {
    metric_value cpu_usage_percent{"cpu_usage_percent", 0, metric_type::gauge};
    metric_value memory_usage_bytes{"memory_usage_bytes", 0, metric_type::gauge};
    metric_value active_threads{"active_threads", 0, metric_type::gauge};
    metric_value total_allocations{"total_allocations", 0, metric_type::counter};

    std::chrono::system_clock::time_point timestamp = std::chrono::system_clock::now();

    /**
     * @brief Convert to vector of metric_value
     */
    std::vector<metric_value> to_metrics() const {
        return {
            cpu_usage_percent,
            memory_usage_bytes,
            active_threads,
            total_allocations
        };
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