// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file concepts.cppm
 * @brief C++20 module partition for common_system concepts.
 *
 * This module partition exports all C++20 concepts:
 * - Core concepts: Result/Optional type constraints
 * - Callable concepts: Function and executor constraints
 * - Event concepts: Event bus type constraints
 * - Service concepts: DI container constraints
 * - Container concepts: Collection type constraints
 * - Logger concepts: Logging interface constraints
 * - Monitoring concepts: Metric collection constraints
 * - Transport concepts: HTTP/UDP client constraints
 *
 * Part of the kcenon.common module.
 */

module;

#include <chrono>
#include <concepts>
#include <functional>
#include <future>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

export module kcenon.common:concepts;

// Forward declarations for interface types
namespace kcenon::common::interfaces {
class IJob;
}

export namespace kcenon::common::concepts {

// ============================================================================
// Core Concepts (Result/Optional)
// ============================================================================

/**
 * @concept Resultable
 * @brief A type that can contain either a value or an error.
 */
template<typename T>
concept Resultable = requires(const T t) {
    { t.is_ok() } -> std::convertible_to<bool>;
    { t.is_err() } -> std::convertible_to<bool>;
};

/**
 * @concept Unwrappable
 * @brief A type that supports value extraction (unwrapping).
 */
template<typename T>
concept Unwrappable = requires(T t) {
    typename T::value_type;
    { t.unwrap() } -> std::same_as<typename std::add_lvalue_reference<
        typename std::add_const<typename T::value_type>::type>::type>;
    { t.unwrap_or(std::declval<typename T::value_type>()) }
        -> std::convertible_to<typename T::value_type>;
};

/**
 * @concept Mappable
 * @brief A type that supports monadic map operations.
 */
template<typename T>
concept Mappable = requires(const T t) {
    { t.map(std::declval<std::function<int(typename T::value_type)>>()) };
};

/**
 * @concept Chainable
 * @brief A type that supports monadic chaining (flatMap/and_then).
 */
template<typename T>
concept Chainable = requires(const T t) {
    { t.and_then(std::declval<std::function<T(typename T::value_type)>>()) };
};

/**
 * @concept MonadicResult
 * @brief A complete Result-like type with all monadic operations.
 */
template<typename T>
concept MonadicResult = Resultable<T> && Mappable<T> && Chainable<T>;

/**
 * @concept OptionalLike
 * @brief A type that represents an optional value (present or absent).
 */
template<typename T>
concept OptionalLike = requires(const T t) {
    { t.has_value() } -> std::convertible_to<bool>;
    { t.is_some() } -> std::convertible_to<bool>;
    { t.is_none() } -> std::convertible_to<bool>;
};

/**
 * @concept ErrorInfo
 * @brief A type that contains error information.
 */
template<typename T>
concept ErrorInfo = requires(const T t) {
    { t.code } -> std::convertible_to<int>;
    { t.message } -> std::convertible_to<std::string>;
    { t.module } -> std::convertible_to<std::string>;
};

/**
 * @concept ValueOrError
 * @brief A type that holds either a value or error information.
 */
template<typename T>
concept ValueOrError = Resultable<T> && requires(const T t) {
    { t.value() };
    { t.error() };
};

// ============================================================================
// Callable Concepts
// ============================================================================

/**
 * @concept Invocable
 * @brief A callable type that can be invoked with given arguments.
 */
template<typename F, typename... Args>
concept Invocable = std::invocable<F, Args...>;

/**
 * @concept VoidCallable
 * @brief A callable type that returns void when invoked.
 */
template<typename F, typename... Args>
concept VoidCallable = Invocable<F, Args...> &&
    std::is_void_v<std::invoke_result_t<F, Args...>>;

/**
 * @concept ReturnsResult
 * @brief A callable type that returns a value convertible to the specified type.
 */
template<typename F, typename R, typename... Args>
concept ReturnsResult = Invocable<F, Args...> &&
    std::convertible_to<std::invoke_result_t<F, Args...>, R>;

/**
 * @concept NoexceptCallable
 * @brief A callable type that is marked noexcept.
 */
template<typename F, typename... Args>
concept NoexceptCallable = Invocable<F, Args...> &&
    std::is_nothrow_invocable_v<F, Args...>;

/**
 * @concept Predicate
 * @brief A callable type that returns a boolean value.
 */
template<typename F, typename... Args>
concept Predicate = Invocable<F, Args...> &&
    std::convertible_to<std::invoke_result_t<F, Args...>, bool>;

/**
 * @concept UnaryFunction
 * @brief A callable type that takes a single argument.
 */
template<typename F, typename Arg>
concept UnaryFunction = Invocable<F, Arg>;

/**
 * @concept BinaryFunction
 * @brief A callable type that takes two arguments.
 */
template<typename F, typename Arg1, typename Arg2>
concept BinaryFunction = Invocable<F, Arg1, Arg2>;

/**
 * @concept JobLike
 * @brief A type that satisfies the Job interface requirements.
 */
template<typename T>
concept JobLike = requires(T t) {
    { t.execute() };
    { t.get_name() } -> std::convertible_to<std::string>;
    { t.get_priority() } -> std::convertible_to<int>;
};

/**
 * @concept ExecutorLike
 * @brief A type that satisfies the Executor interface requirements.
 */
template<typename T>
concept ExecutorLike = requires(T t, std::unique_ptr<interfaces::IJob> job, bool wait) {
    { t.worker_count() } -> std::convertible_to<size_t>;
    { t.is_running() } -> std::convertible_to<bool>;
    { t.pending_tasks() } -> std::convertible_to<size_t>;
    { t.shutdown(wait) } -> std::same_as<void>;
};

/**
 * @concept TaskFactory
 * @brief A callable that creates a job or task.
 */
template<typename F, typename T>
concept TaskFactory = Invocable<F> &&
    std::convertible_to<std::invoke_result_t<F>, std::unique_ptr<T>>;

/**
 * @concept AsyncCallable
 * @brief A callable suitable for async execution returning a future.
 */
template<typename F, typename R>
concept AsyncCallable = Invocable<F> &&
    std::same_as<std::invoke_result_t<F>, R>;

/**
 * @concept DelayedCallable
 * @brief A callable suitable for delayed execution.
 */
template<typename F>
concept DelayedCallable = VoidCallable<F> &&
    std::move_constructible<std::decay_t<F>>;

// ============================================================================
// Event Concepts
// ============================================================================

/**
 * @concept EventType
 * @brief A type that can be used as an event in the event bus.
 */
template<typename T>
concept EventType = std::is_class_v<T> &&
    std::is_copy_constructible_v<T>;

/**
 * @concept EventHandler
 * @brief A callable that can handle events of a specific type.
 */
template<typename H, typename E>
concept EventHandler = std::invocable<H, const E&> &&
    std::is_void_v<std::invoke_result_t<H, const E&>>;

/**
 * @concept EventFilter
 * @brief A callable that filters events based on criteria.
 */
template<typename F, typename E>
concept EventFilter = std::invocable<F, const E&> &&
    std::convertible_to<std::invoke_result_t<F, const E&>, bool>;

/**
 * @concept TimestampedEvent
 * @brief An event type that includes a timestamp.
 */
template<typename T>
concept TimestampedEvent = EventType<T> && requires(const T t) {
    { t.timestamp } -> std::convertible_to<std::chrono::steady_clock::time_point>;
};

/**
 * @concept NamedEvent
 * @brief An event type that includes a module or source name.
 */
template<typename T>
concept NamedEvent = EventType<T> && requires(const T t) {
    { t.module_name } -> std::convertible_to<std::string>;
};

/**
 * @concept ErrorEvent
 * @brief An event type representing an error.
 */
template<typename T>
concept ErrorEvent = EventType<T> && requires(const T t) {
    { t.error_message } -> std::convertible_to<std::string>;
    { t.error_code } -> std::convertible_to<int>;
};

/**
 * @concept MetricEvent
 * @brief An event type representing a metric measurement.
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
 */
template<typename T>
concept ModuleLifecycleEvent = NamedEvent<T> && TimestampedEvent<T>;

/**
 * @concept FullErrorEvent
 * @brief A complete error event with module, message, code, and timestamp.
 */
template<typename T>
concept FullErrorEvent = ErrorEvent<T> && NamedEvent<T> && TimestampedEvent<T>;

/**
 * @concept FullMetricEvent
 * @brief A complete metric event with timestamp.
 */
template<typename T>
concept FullMetricEvent = MetricEvent<T> && TimestampedEvent<T>;

/**
 * @concept EventBusLike
 * @brief A type that satisfies the event bus interface requirements.
 */
template<typename T>
concept EventBusLike = requires(T t, uint64_t id) {
    { t.start() } -> std::same_as<void>;
    { t.stop() } -> std::same_as<void>;
    { t.is_running() } -> std::convertible_to<bool>;
    { t.unsubscribe(id) } -> std::same_as<void>;
};

// ============================================================================
// Service Concepts (DI)
// ============================================================================

/**
 * @concept ServiceInterface
 * @brief A type that can serve as a service interface.
 */
template<typename T>
concept ServiceInterface = std::is_class_v<T> && std::has_virtual_destructor_v<T>;

/**
 * @concept ServiceImplementation
 * @brief A type that implements a service interface.
 */
template<typename Impl, typename Interface>
concept ServiceImplementation = std::derived_from<Impl, Interface> &&
    std::is_constructible_v<Impl>;

/**
 * @concept ServiceFactory
 * @brief A callable that creates a service instance.
 */
template<typename F, typename T>
concept ServiceFactory = Invocable<F> &&
    std::convertible_to<std::invoke_result_t<F>, std::shared_ptr<T>>;

/**
 * @concept Validatable
 * @brief A type that can validate itself.
 */
template<typename T>
concept Validatable = requires(const T t) {
    { t.validate() } -> std::convertible_to<bool>;
};

// ============================================================================
// Container Concepts
// ============================================================================

/**
 * @concept Container
 * @brief Basic container requirements.
 */
template<typename T>
concept Container = requires(T t) {
    typename T::value_type;
    typename T::size_type;
    { t.size() } -> std::convertible_to<typename T::size_type>;
    { t.empty() } -> std::convertible_to<bool>;
    { t.begin() };
    { t.end() };
};

/**
 * @concept SequenceContainer
 * @brief Sequential container requirements.
 */
template<typename T>
concept SequenceContainer = Container<T> && requires(T t, typename T::value_type v) {
    { t.push_back(v) };
    { t.front() } -> std::same_as<typename T::value_type&>;
    { t.back() } -> std::same_as<typename T::value_type&>;
};

/**
 * @concept ResizableContainer
 * @brief Container that can be resized.
 */
template<typename T>
concept ResizableContainer = Container<T> && requires(T t, typename T::size_type n) {
    { t.resize(n) };
    { t.reserve(n) };
    { t.capacity() } -> std::convertible_to<typename T::size_type>;
};

// ============================================================================
// Logger Concepts
// ============================================================================

/**
 * @concept BasicLogger
 * @brief Types with basic log functionality.
 */
template<typename T>
concept BasicLogger = requires(T t, const std::string& msg) {
    { t.log(msg) };
};

/**
 * @concept FlushableLogger
 * @brief Types supporting flush operation.
 */
template<typename T>
concept FlushableLogger = BasicLogger<T> && requires(T t) {
    { t.flush() };
};

// ============================================================================
// Monitoring Concepts
// ============================================================================

/**
 * @concept CounterMetric
 * @brief Types supporting increment operations.
 */
template<typename T>
concept CounterMetric = requires(T t, const std::string& name, int64_t val) {
    { t.increment(name) };
    { t.increment(name, val) };
};

/**
 * @concept GaugeMetric
 * @brief Types supporting gauge operations.
 */
template<typename T>
concept GaugeMetric = requires(T t, const std::string& name, double val) {
    { t.set_gauge(name, val) };
};

/**
 * @concept HistogramMetric
 * @brief Types supporting histogram observations.
 */
template<typename T>
concept HistogramMetric = requires(T t, const std::string& name, double val) {
    { t.observe(name, val) };
};

// ============================================================================
// Transport Concepts
// ============================================================================

/**
 * @concept Sendable
 * @brief A type that can send data.
 */
template<typename T>
concept Sendable = requires(T t, const std::string& data) {
    { t.send(data) };
};

/**
 * @concept Receivable
 * @brief A type that can receive data.
 */
template<typename T>
concept Receivable = requires(T t) {
    { t.receive() } -> std::convertible_to<std::string>;
};

/**
 * @concept TransportClient
 * @brief A generic transport client.
 */
template<typename T>
concept TransportClient = Sendable<T> && Receivable<T>;

} // namespace kcenon::common::concepts
