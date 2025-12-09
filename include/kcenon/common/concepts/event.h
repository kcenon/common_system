// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file event.h
 * @brief C++20 concepts for event bus types.
 *
 * This header provides concepts for validating event types, handlers,
 * and filters used in the event bus system. These concepts enable
 * compile-time validation with clear, actionable error messages.
 *
 * Requirements:
 * - C++20 compiler with concepts support
 * - GCC 10+, Clang 10+, MSVC 2022+
 *
 * Thread Safety:
 * - Concepts are evaluated at compile-time only.
 * - No runtime thread-safety considerations apply.
 *
 * @see event_bus.h for simple_event_bus implementation
 * @see https://en.cppreference.com/w/cpp/language/constraints
 */

#pragma once

#include <concepts>
#include <type_traits>
#include <functional>
#include <string>
#include <chrono>

namespace kcenon::common {
namespace concepts {

/**
 * @concept EventType
 * @brief A type that can be used as an event in the event bus.
 *
 * Events must be class types that are copy-constructible to ensure
 * they can be safely passed to multiple handlers.
 *
 * Example usage:
 * @code
 * template<EventType E>
 * void publish(const E& event) {
 *     event_bus.publish(event);
 * }
 * @endcode
 */
template<typename T>
concept EventType = std::is_class_v<T> &&
    std::is_copy_constructible_v<T>;

/**
 * @concept EventHandler
 * @brief A callable that can handle events of a specific type.
 *
 * Event handlers receive events by const reference and return void.
 * They should not throw exceptions to avoid disrupting event dispatch.
 *
 * Example usage:
 * @code
 * template<EventType E, EventHandler<E> H>
 * uint64_t subscribe(H&& handler) {
 *     return event_bus.subscribe<E>(std::forward<H>(handler));
 * }
 * @endcode
 */
template<typename H, typename E>
concept EventHandler = std::invocable<H, const E&> &&
    std::is_void_v<std::invoke_result_t<H, const E&>>;

/**
 * @concept EventFilter
 * @brief A callable that filters events based on criteria.
 *
 * Event filters receive events by const reference and return a boolean
 * indicating whether the event should be processed by the handler.
 *
 * Example usage:
 * @code
 * template<EventType E, EventFilter<E> F, EventHandler<E> H>
 * uint64_t subscribe_filtered(H&& handler, F&& filter) {
 *     return event_bus.subscribe_filtered<E>(
 *         std::forward<H>(handler),
 *         std::forward<F>(filter)
 *     );
 * }
 * @endcode
 */
template<typename F, typename E>
concept EventFilter = std::invocable<F, const E&> &&
    std::convertible_to<std::invoke_result_t<F, const E&>, bool>;

/**
 * @concept TimestampedEvent
 * @brief An event type that includes a timestamp.
 *
 * Events satisfying this concept provide timing information for
 * ordering, debugging, and audit purposes.
 *
 * Example usage:
 * @code
 * template<TimestampedEvent E>
 * void log_event(const E& event) {
 *     auto time = event.timestamp;
 *     // Log with timestamp
 * }
 * @endcode
 */
template<typename T>
concept TimestampedEvent = EventType<T> && requires(const T t) {
    { t.timestamp } -> std::convertible_to<std::chrono::steady_clock::time_point>;
};

/**
 * @concept NamedEvent
 * @brief An event type that includes a module or source name.
 *
 * Events satisfying this concept identify their source module
 * for debugging and routing purposes.
 *
 * Example usage:
 * @code
 * template<NamedEvent E>
 * void route_event(const E& event) {
 *     auto source = event.module_name;
 *     // Route based on source
 * }
 * @endcode
 */
template<typename T>
concept NamedEvent = EventType<T> && requires(const T t) {
    { t.module_name } -> std::convertible_to<std::string>;
};

/**
 * @concept ErrorEvent
 * @brief An event type representing an error.
 *
 * Error events include error message and error code for
 * diagnostic purposes.
 *
 * Example usage:
 * @code
 * template<ErrorEvent E>
 * void handle_error(const E& event) {
 *     std::cerr << "Error " << event.error_code << ": "
 *               << event.error_message << std::endl;
 * }
 * @endcode
 */
template<typename T>
concept ErrorEvent = EventType<T> && requires(const T t) {
    { t.error_message } -> std::convertible_to<std::string>;
    { t.error_code } -> std::convertible_to<int>;
};

/**
 * @concept MetricEvent
 * @brief An event type representing a metric measurement.
 *
 * Metric events include a name, value, and optional unit for
 * monitoring and observability purposes.
 *
 * Example usage:
 * @code
 * template<MetricEvent E>
 * void record_metric(const E& event) {
 *     metrics_system.record(event.name, event.value, event.unit);
 * }
 * @endcode
 */
template<typename T>
concept MetricEvent = EventType<T> && requires(const T t) {
    { t.name } -> std::convertible_to<std::string>;
    { t.value } -> std::convertible_to<double>;
    { t.unit } -> std::convertible_to<std::string>;
};

/**
 * @concept ModuleLifecycleEvent
 * @brief An event type representing module lifecycle changes.
 *
 * These events include module name and timestamp, used for
 * tracking module startup and shutdown.
 *
 * Example usage:
 * @code
 * template<ModuleLifecycleEvent E>
 * void track_lifecycle(const E& event) {
 *     lifecycle_tracker.record(event.module_name, event.timestamp);
 * }
 * @endcode
 */
template<typename T>
concept ModuleLifecycleEvent = NamedEvent<T> && TimestampedEvent<T>;

/**
 * @concept FullErrorEvent
 * @brief A complete error event with module, message, code, and timestamp.
 *
 * This concept combines all error-related requirements for
 * comprehensive error reporting events.
 *
 * Example usage:
 * @code
 * template<FullErrorEvent E>
 * void log_full_error(const E& event) {
 *     logger.error("[{}] {} (code: {})",
 *         event.module_name, event.error_message, event.error_code);
 * }
 * @endcode
 */
template<typename T>
concept FullErrorEvent = ErrorEvent<T> && NamedEvent<T> && TimestampedEvent<T>;

/**
 * @concept FullMetricEvent
 * @brief A complete metric event with timestamp.
 *
 * This concept combines metric requirements with timing information
 * for time-series metric collection.
 *
 * Example usage:
 * @code
 * template<FullMetricEvent E>
 * void publish_metric(const E& event) {
 *     time_series.add(event.name, event.value, event.timestamp);
 * }
 * @endcode
 */
template<typename T>
concept FullMetricEvent = MetricEvent<T> && TimestampedEvent<T>;

/**
 * @concept EventBusLike
 * @brief A type that satisfies the event bus interface requirements.
 *
 * Types satisfying this concept can publish events and manage subscriptions.
 *
 * Example usage:
 * @code
 * template<EventBusLike B, EventType E>
 * void broadcast(B& bus, const E& event) {
 *     bus.publish(event);
 * }
 * @endcode
 */
template<typename T>
concept EventBusLike = requires(T t, uint64_t id) {
    { t.start() } -> std::same_as<void>;
    { t.stop() } -> std::same_as<void>;
    { t.is_running() } -> std::convertible_to<bool>;
    { t.unsubscribe(id) } -> std::same_as<void>;
};

} // namespace concepts
} // namespace kcenon::common
