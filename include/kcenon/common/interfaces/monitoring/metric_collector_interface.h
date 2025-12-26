// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file metric_collector_interface.h
 * @brief Unified metric collection interface for cross-module metric reporting.
 *
 * This header defines the IMetricCollector interface that enables clean metric
 * reporting across the ecosystem without requiring direct monitoring_system
 * dependencies. It complements the existing IMonitor interface:
 * - IMonitor: Pull-based (read status, get snapshots)
 * - IMetricCollector: Push-based (emit metrics in real-time)
 *
 * @see monitoring_interface.h for IMonitor details
 */

#pragma once

#include <chrono>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

namespace kcenon::common {
namespace interfaces {

/**
 * @brief Metric labels for dimensional data
 *
 * Labels allow attaching key-value pairs to metrics for filtering
 * and grouping in monitoring systems (e.g., Prometheus, StatsD).
 *
 * Example:
 * @code
 * metric_labels labels{{"service", "api"}, {"endpoint", "/users"}};
 * collector->increment("http_requests_total", 1.0, labels);
 * @endcode
 */
using metric_labels = std::unordered_map<std::string, std::string>;

/**
 * @interface IMetricCollector
 * @brief Abstract interface for collecting metrics across modules
 *
 * This interface defines the contract for metric collection implementations,
 * allowing modules to emit metrics without direct dependencies on specific
 * monitoring backends (e.g., Prometheus, StatsD, OpenTelemetry).
 *
 * @note Implementations should be thread-safe for concurrent metric emission.
 *
 * Example usage:
 * @code
 * // Counter - monotonically increasing value
 * collector->increment("http_requests_total", 1.0, {{"method", "GET"}});
 *
 * // Gauge - value that can go up or down
 * collector->gauge("active_connections", 42.0);
 *
 * // Histogram - distribution of values
 * collector->histogram("request_size_bytes", 1024.0);
 *
 * // Timing - duration measurement
 * collector->timing("request_duration", std::chrono::milliseconds{150});
 * @endcode
 */
class IMetricCollector {
public:
    virtual ~IMetricCollector() = default;

    /**
     * @brief Increment a counter metric
     *
     * Counters are monotonically increasing values, typically used for
     * counting events (requests, errors, completed jobs, etc.).
     *
     * @param name Metric name (should follow naming conventions, e.g., snake_case)
     * @param value Increment value (default: 1.0, must be non-negative)
     * @param labels Optional dimensional labels for filtering/grouping
     */
    virtual void increment(std::string_view name,
                          double value = 1.0,
                          const metric_labels& labels = {}) = 0;

    /**
     * @brief Set a gauge metric to an absolute value
     *
     * Gauges represent instantaneous values that can go up or down,
     * such as temperature, memory usage, or active connections.
     *
     * @param name Metric name
     * @param value Current value (can be negative)
     * @param labels Optional dimensional labels
     */
    virtual void gauge(std::string_view name,
                      double value,
                      const metric_labels& labels = {}) = 0;

    /**
     * @brief Record a histogram observation
     *
     * Histograms track the distribution of values across configurable
     * buckets, useful for measuring sizes, counts, or any discrete values.
     *
     * @param name Metric name
     * @param value Observed value
     * @param labels Optional dimensional labels
     */
    virtual void histogram(std::string_view name,
                          double value,
                          const metric_labels& labels = {}) = 0;

    /**
     * @brief Record a timing measurement
     *
     * Timing metrics are specialized histograms for duration measurements.
     * Implementations may convert to appropriate units (ms, s, etc.).
     *
     * @param name Metric name
     * @param duration Duration measurement
     * @param labels Optional dimensional labels
     */
    virtual void timing(std::string_view name,
                       std::chrono::nanoseconds duration,
                       const metric_labels& labels = {}) = 0;

    /**
     * @brief Get the implementation name for logging/debugging
     * @return Implementation identifier string
     */
    [[nodiscard]] virtual std::string get_implementation_name() const {
        return "IMetricCollector";
    }
};

/**
 * @class scoped_timer
 * @brief RAII helper for automatic timing measurements
 *
 * Measures elapsed time from construction to destruction and reports
 * it to the metric collector. Useful for timing function execution,
 * request handling, or any scoped operations.
 *
 * Example usage:
 * @code
 * void process_request() {
 *     scoped_timer timer(*collector, "request_processing_time",
 *                        {{"handler", "user_api"}});
 *     // ... do work ...
 * } // Timer automatically reports duration on scope exit
 * @endcode
 *
 * @note This class is non-copyable and non-movable to prevent
 *       accidental double-reporting.
 */
class scoped_timer {
public:
    /**
     * @brief Construct a scoped timer
     * @param collector Reference to the metric collector
     * @param name Metric name for the timing measurement
     * @param labels Optional dimensional labels
     */
    scoped_timer(IMetricCollector& collector,
                 std::string_view name,
                 metric_labels labels = {})
        : collector_(collector)
        , name_(name)
        , labels_(std::move(labels))
        , start_(std::chrono::steady_clock::now()) {}

    /**
     * @brief Destructor reports elapsed time to the collector
     */
    ~scoped_timer() {
        auto elapsed = std::chrono::steady_clock::now() - start_;
        collector_.timing(
            name_,
            std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed),
            labels_);
    }

    // Non-copyable and non-movable
    scoped_timer(const scoped_timer&) = delete;
    scoped_timer& operator=(const scoped_timer&) = delete;
    scoped_timer(scoped_timer&&) = delete;
    scoped_timer& operator=(scoped_timer&&) = delete;

    /**
     * @brief Get elapsed time since timer started
     * @return Duration since construction
     */
    [[nodiscard]] std::chrono::nanoseconds elapsed() const {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now() - start_);
    }

private:
    IMetricCollector& collector_;
    std::string name_;
    metric_labels labels_;
    std::chrono::steady_clock::time_point start_;
};

/**
 * @class null_metric_collector
 * @brief No-op implementation for when metrics are disabled
 *
 * This implementation silently discards all metrics, useful for:
 * - Testing without metric dependencies
 * - Disabling metrics in production for performance
 * - Default implementation when no collector is configured
 *
 * Example usage:
 * @code
 * // Use null collector when metrics are disabled
 * std::shared_ptr<IMetricCollector> collector =
 *     metrics_enabled ? create_prometheus_collector()
 *                     : std::make_shared<null_metric_collector>();
 * @endcode
 */
class null_metric_collector : public IMetricCollector {
public:
    void increment(std::string_view /*name*/,
                  double /*value*/ = 1.0,
                  const metric_labels& /*labels*/ = {}) override {}

    void gauge(std::string_view /*name*/,
              double /*value*/,
              const metric_labels& /*labels*/ = {}) override {}

    void histogram(std::string_view /*name*/,
                  double /*value*/,
                  const metric_labels& /*labels*/ = {}) override {}

    void timing(std::string_view /*name*/,
               std::chrono::nanoseconds /*duration*/,
               const metric_labels& /*labels*/ = {}) override {}

    [[nodiscard]] std::string get_implementation_name() const override {
        return "null_metric_collector";
    }
};

/**
 * @brief Factory function type for creating metric collector instances
 */
using MetricCollectorFactory = std::function<std::shared_ptr<IMetricCollector>()>;

/**
 * @interface IMetricCollectorProvider
 * @brief Interface for modules that provide metric collector implementations
 *
 * This interface allows modules to register and retrieve metric collectors
 * through dependency injection.
 */
class IMetricCollectorProvider {
public:
    virtual ~IMetricCollectorProvider() = default;

    /**
     * @brief Get the default metric collector instance
     * @return Shared pointer to the metric collector
     */
    virtual std::shared_ptr<IMetricCollector> get_metric_collector() = 0;

    /**
     * @brief Create a new metric collector with specific prefix
     * @param prefix Metric name prefix for namespacing
     * @return Shared pointer to the new metric collector
     */
    virtual std::shared_ptr<IMetricCollector> create_metric_collector(
        const std::string& prefix) = 0;
};

}  // namespace interfaces
}  // namespace kcenon::common
