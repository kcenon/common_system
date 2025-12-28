// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file monitoring.h
 * @brief C++20 concepts for monitoring and metric collection interfaces.
 *
 * This header provides concepts for validating metric collector types used in
 * observability operations. These concepts replace abstract class-based constraints
 * with compile-time validation and clearer error messages.
 *
 * Requirements:
 * - C++20 compiler with concepts support
 * - GCC 10+, Clang 10+, MSVC 2022+
 *
 * Thread Safety:
 * - Concepts are evaluated at compile-time only.
 * - No runtime thread-safety considerations apply.
 *
 * @see metric_collector_interface.h for IMetricCollector definition
 * @see https://en.cppreference.com/w/cpp/language/constraints
 */

#pragma once

#include <chrono>
#include <concepts>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>

namespace kcenon::common {

// Forward declarations
namespace interfaces {
using metric_labels = std::unordered_map<std::string, std::string>;
}  // namespace interfaces

namespace concepts {

/**
 * @concept CounterMetric
 * @brief A type that supports counter metric operations.
 *
 * Types satisfying this concept can increment counter metrics,
 * which are monotonically increasing values used for counting events.
 *
 * Example usage:
 * @code
 * template<CounterMetric M>
 * void count_request(M& collector) {
 *     collector.increment("http_requests_total", 1.0);
 * }
 * @endcode
 */
template <typename T>
concept CounterMetric = requires(T t,
                                 std::string_view name,
                                 double value,
                                 const interfaces::metric_labels& labels) {
    { t.increment(name, value, labels) } -> std::same_as<void>;
};

/**
 * @concept GaugeMetric
 * @brief A type that supports gauge metric operations.
 *
 * Types satisfying this concept can set gauge metrics,
 * which represent instantaneous values that can go up or down.
 *
 * Example usage:
 * @code
 * template<GaugeMetric M>
 * void report_memory(M& collector, double bytes) {
 *     collector.gauge("memory_usage_bytes", bytes);
 * }
 * @endcode
 */
template <typename T>
concept GaugeMetric = requires(T t,
                               std::string_view name,
                               double value,
                               const interfaces::metric_labels& labels) {
    { t.gauge(name, value, labels) } -> std::same_as<void>;
};

/**
 * @concept HistogramMetric
 * @brief A type that supports histogram metric operations.
 *
 * Types satisfying this concept can record histogram observations,
 * which track the distribution of values across configurable buckets.
 *
 * Example usage:
 * @code
 * template<HistogramMetric M>
 * void record_size(M& collector, double size) {
 *     collector.histogram("request_size_bytes", size);
 * }
 * @endcode
 */
template <typename T>
concept HistogramMetric = requires(T t,
                                   std::string_view name,
                                   double value,
                                   const interfaces::metric_labels& labels) {
    { t.histogram(name, value, labels) } -> std::same_as<void>;
};

/**
 * @concept TimingMetric
 * @brief A type that supports timing metric operations.
 *
 * Types satisfying this concept can record timing measurements,
 * which are specialized histograms for duration measurements.
 *
 * Example usage:
 * @code
 * template<TimingMetric M>
 * void record_latency(M& collector, std::chrono::nanoseconds duration) {
 *     collector.timing("request_duration", duration);
 * }
 * @endcode
 */
template <typename T>
concept TimingMetric = requires(T t,
                                std::string_view name,
                                std::chrono::nanoseconds duration,
                                const interfaces::metric_labels& labels) {
    { t.timing(name, duration, labels) } -> std::same_as<void>;
};

/**
 * @concept MetricCollectorLike
 * @brief A complete metric collector type satisfying IMetricCollector interface.
 *
 * Types satisfying this concept provide full metric collection functionality
 * including counters, gauges, histograms, and timing measurements.
 * This concept matches the IMetricCollector interface contract.
 *
 * Example usage:
 * @code
 * template<MetricCollectorLike M>
 * void emit_metrics(M& collector) {
 *     collector.increment("requests_total", 1.0);
 *     collector.gauge("active_connections", 42.0);
 *     collector.histogram("response_size", 1024.0);
 *     collector.timing("request_duration", std::chrono::milliseconds{150});
 * }
 * @endcode
 */
template <typename T>
concept MetricCollectorLike = CounterMetric<T> &&
                              GaugeMetric<T> &&
                              HistogramMetric<T> &&
                              TimingMetric<T>;

/**
 * @concept NamedImplementation
 * @brief A type that provides implementation name for debugging.
 *
 * Types satisfying this concept can report their implementation name,
 * useful for logging and debugging purposes.
 *
 * Example usage:
 * @code
 * template<NamedImplementation I>
 * void log_implementation(const I& impl) {
 *     std::cout << "Using: " << impl.get_implementation_name() << std::endl;
 * }
 * @endcode
 */
template <typename T>
concept NamedImplementation = requires(const T t) {
    { t.get_implementation_name() } -> std::convertible_to<std::string>;
};

/**
 * @concept MetricCollectorProviderLike
 * @brief A type that can provide metric collector instances.
 *
 * Types satisfying this concept can create and retrieve metric collectors,
 * enabling dependency injection for metrics.
 *
 * Example usage:
 * @code
 * template<MetricCollectorProviderLike P>
 * auto get_collector(P& provider, const std::string& prefix) {
 *     return provider.create_metric_collector(prefix);
 * }
 * @endcode
 */
template <typename T>
concept MetricCollectorProviderLike = requires(T t, const std::string& prefix) {
    { t.get_metric_collector() };
    { t.create_metric_collector(prefix) };
};

}  // namespace concepts
}  // namespace kcenon::common
