// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file callable.h
 * @brief C++20 concepts for callable types and executor interfaces.
 *
 * This header provides concepts for validating callable types used in
 * task execution, job scheduling, and async operations. These concepts
 * replace SFINAE-based constraints with clearer compile-time errors.
 *
 * Requirements:
 * - C++20 compiler with concepts support
 * - GCC 10+, Clang 10+, MSVC 2022+
 *
 * Thread Safety:
 * - Concepts are evaluated at compile-time only.
 * - No runtime thread-safety considerations apply.
 *
 * @see executor_interface.h for IExecutor implementation
 * @see https://en.cppreference.com/w/cpp/concepts/invocable
 */

#pragma once

#include <concepts>
#include <type_traits>
#include <functional>
#include <string>
#include <future>
#include <memory>
#include <chrono>

#include "../patterns/result/core.h"

namespace kcenon::common {

// Forward declarations
namespace interfaces {
class IJob;
}

namespace concepts {

/**
 * @concept Invocable
 * @brief A callable type that can be invoked with given arguments.
 *
 * This concept wraps std::invocable for consistency within the
 * kcenon::common::concepts namespace.
 *
 * Example usage:
 * @code
 * template<Invocable<int, int> F>
 * auto apply(F&& func, int a, int b) {
 *     return func(a, b);
 * }
 * @endcode
 */
template<typename F, typename... Args>
concept Invocable = std::invocable<F, Args...>;

/**
 * @concept VoidCallable
 * @brief A callable type that returns void when invoked.
 *
 * Use this concept for fire-and-forget tasks or callbacks that
 * don't produce a return value.
 *
 * Example usage:
 * @code
 * template<VoidCallable F>
 * void execute_async(F&& func) {
 *     std::thread([f = std::forward<F>(func)]() { f(); }).detach();
 * }
 * @endcode
 */
template<typename F, typename... Args>
concept VoidCallable = Invocable<F, Args...> &&
    std::is_void_v<std::invoke_result_t<F, Args...>>;

/**
 * @concept ReturnsResult
 * @brief A callable type that returns a value convertible to the specified type.
 *
 * Example usage:
 * @code
 * template<ReturnsResult<int> F>
 * int compute(F&& func) {
 *     return func();
 * }
 * @endcode
 */
template<typename F, typename R, typename... Args>
concept ReturnsResult = Invocable<F, Args...> &&
    std::convertible_to<std::invoke_result_t<F, Args...>, R>;

/**
 * @concept NoexceptCallable
 * @brief A callable type that is marked noexcept.
 *
 * Use this concept for performance-critical code paths where
 * exception handling overhead should be avoided.
 *
 * Example usage:
 * @code
 * template<NoexceptCallable F>
 * void safe_execute(F&& func) noexcept {
 *     func();
 * }
 * @endcode
 */
template<typename F, typename... Args>
concept NoexceptCallable = Invocable<F, Args...> &&
    std::is_nothrow_invocable_v<F, Args...>;

/**
 * @concept Predicate
 * @brief A callable type that returns a boolean value.
 *
 * Use this concept for filter functions, condition checks,
 * and boolean predicates.
 *
 * Example usage:
 * @code
 * template<typename T, Predicate<const T&> P>
 * bool any_of(const std::vector<T>& vec, P&& pred) {
 *     for (const auto& item : vec) {
 *         if (pred(item)) return true;
 *     }
 *     return false;
 * }
 * @endcode
 */
template<typename F, typename... Args>
concept Predicate = Invocable<F, Args...> &&
    std::convertible_to<std::invoke_result_t<F, Args...>, bool>;

/**
 * @concept UnaryFunction
 * @brief A callable type that takes a single argument.
 *
 * Example usage:
 * @code
 * template<typename T, UnaryFunction<T> F>
 * auto transform_all(std::vector<T>& vec, F&& func) {
 *     for (auto& item : vec) {
 *         item = func(item);
 *     }
 * }
 * @endcode
 */
template<typename F, typename Arg>
concept UnaryFunction = Invocable<F, Arg>;

/**
 * @concept BinaryFunction
 * @brief A callable type that takes two arguments.
 *
 * Example usage:
 * @code
 * template<typename T, BinaryFunction<T, T> F>
 * T reduce(const std::vector<T>& vec, T init, F&& func) {
 *     for (const auto& item : vec) {
 *         init = func(init, item);
 *     }
 *     return init;
 * }
 * @endcode
 */
template<typename F, typename Arg1, typename Arg2>
concept BinaryFunction = Invocable<F, Arg1, Arg2>;

/**
 * @concept JobLike
 * @brief A type that satisfies the Job interface requirements.
 *
 * Types satisfying this concept can be executed by an executor,
 * providing name and priority information for scheduling.
 *
 * Example usage:
 * @code
 * template<JobLike J>
 * void schedule(std::unique_ptr<J> job) {
 *     std::cout << "Scheduling job: " << job->get_name()
 *               << " with priority " << job->get_priority() << std::endl;
 *     job->execute();
 * }
 * @endcode
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
 *
 * Types satisfying this concept can manage and execute jobs,
 * providing status information and shutdown capabilities.
 *
 * Example usage:
 * @code
 * template<ExecutorLike E>
 * void run_with_executor(E& executor, std::unique_ptr<interfaces::IJob> job) {
 *     if (executor.is_running()) {
 *         executor.execute(std::move(job));
 *     }
 * }
 * @endcode
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
 *
 * Use this concept for factory functions that produce executable tasks.
 *
 * Example usage:
 * @code
 * template<TaskFactory<interfaces::IJob> F>
 * void schedule_from_factory(F&& factory) {
 *     auto job = factory();
 *     executor.execute(std::move(job));
 * }
 * @endcode
 */
template<typename F, typename T>
concept TaskFactory = Invocable<F> &&
    std::convertible_to<std::invoke_result_t<F>, std::unique_ptr<T>>;

/**
 * @concept AsyncCallable
 * @brief A callable suitable for async execution returning a future.
 *
 * Example usage:
 * @code
 * template<AsyncCallable<int> F>
 * std::future<int> run_async(F&& func) {
 *     return std::async(std::launch::async, std::forward<F>(func));
 * }
 * @endcode
 */
template<typename F, typename R>
concept AsyncCallable = Invocable<F> &&
    std::same_as<std::invoke_result_t<F>, R>;

/**
 * @concept DelayedCallable
 * @brief A callable suitable for delayed execution.
 *
 * Combines VoidCallable with move-constructibility for
 * storage in delayed execution queues.
 *
 * Example usage:
 * @code
 * template<DelayedCallable F>
 * void schedule_delayed(F&& func, std::chrono::milliseconds delay) {
 *     // Store func and schedule for later execution
 * }
 * @endcode
 */
template<typename F>
concept DelayedCallable = VoidCallable<F> &&
    std::move_constructible<std::decay_t<F>>;

} // namespace concepts
} // namespace kcenon::common
