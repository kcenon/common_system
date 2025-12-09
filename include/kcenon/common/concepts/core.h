// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file core.h
 * @brief Core C++20 concepts for Result/Optional types.
 *
 * This header provides fundamental concepts for types that support
 * error handling patterns similar to Rust's Result and Option types.
 * These concepts enable compile-time validation with clear error messages.
 *
 * Requirements:
 * - C++20 compiler with concepts support
 * - GCC 10+, Clang 10+, MSVC 2022+
 *
 * Thread Safety:
 * - Concepts are evaluated at compile-time only.
 * - No runtime thread-safety considerations apply.
 *
 * @see result.h for Result<T> implementation
 * @see https://en.cppreference.com/w/cpp/language/constraints
 */

#pragma once

#include <concepts>
#include <type_traits>
#include <functional>

namespace kcenon::common {
namespace concepts {

/**
 * @concept Resultable
 * @brief A type that can contain either a value or an error.
 *
 * Types satisfying this concept provide is_ok() and is_err() methods
 * to query the state of the result.
 *
 * Example usage:
 * @code
 * template<Resultable R>
 * void process(const R& result) {
 *     if (result.is_ok()) {
 *         // Handle success
 *     } else {
 *         // Handle error
 *     }
 * }
 * @endcode
 */
template<typename T>
concept Resultable = requires(const T t) {
    { t.is_ok() } -> std::convertible_to<bool>;
    { t.is_err() } -> std::convertible_to<bool>;
};

/**
 * @concept Unwrappable
 * @brief A type that supports value extraction (unwrapping).
 *
 * Types satisfying this concept can extract their contained value
 * either directly (with potential exception) or with a default fallback.
 *
 * Example usage:
 * @code
 * template<Unwrappable U>
 * auto get_or_default(const U& container, typename U::value_type default_val) {
 *     return container.unwrap_or(default_val);
 * }
 * @endcode
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
 *
 * Types satisfying this concept can transform their contained value
 * using a function, propagating errors automatically.
 *
 * Example usage:
 * @code
 * template<Mappable M>
 * auto transform(const M& container) {
 *     return container.map([](auto& val) { return val * 2; });
 * }
 * @endcode
 */
template<typename T>
concept Mappable = requires(const T t) {
    { t.map(std::declval<std::function<int(typename T::value_type)>>()) };
};

/**
 * @concept Chainable
 * @brief A type that supports monadic chaining (flatMap/and_then).
 *
 * Types satisfying this concept can chain operations that return
 * the same container type, enabling fluent error handling.
 *
 * Example usage:
 * @code
 * template<Chainable C>
 * auto chain_operations(const C& container) {
 *     return container.and_then([](auto& val) {
 *         return C::ok(val + 1);
 *     });
 * }
 * @endcode
 */
template<typename T>
concept Chainable = requires(const T t) {
    { t.and_then(std::declval<std::function<T(typename T::value_type)>>()) };
};

/**
 * @concept MonadicResult
 * @brief A complete Result-like type with all monadic operations.
 *
 * Combines Resultable, Unwrappable, Mappable, and Chainable concepts
 * for types that provide full monadic error handling capabilities.
 *
 * Example usage:
 * @code
 * template<MonadicResult R>
 * auto process_chain(const R& result) {
 *     return result
 *         .map([](auto& v) { return v * 2; })
 *         .and_then([](auto v) { return R::ok(v + 1); });
 * }
 * @endcode
 */
template<typename T>
concept MonadicResult = Resultable<T> && Mappable<T> && Chainable<T>;

/**
 * @concept OptionalLike
 * @brief A type that represents an optional value (present or absent).
 *
 * Types satisfying this concept provide has_value(), is_some(), and is_none()
 * methods for querying the presence of a value.
 *
 * Example usage:
 * @code
 * template<OptionalLike O>
 * void process_if_present(const O& opt) {
 *     if (opt.has_value()) {
 *         // Use opt.value()
 *     }
 * }
 * @endcode
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
 *
 * Types satisfying this concept provide error code, message, and module
 * information for diagnostic purposes.
 *
 * Example usage:
 * @code
 * template<ErrorInfo E>
 * void log_error(const E& error) {
 *     std::cerr << "[" << error.module << "] Error " << error.code
 *               << ": " << error.message << std::endl;
 * }
 * @endcode
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
 *
 * Types satisfying this concept can report their state and provide
 * access to either the contained value or error details.
 *
 * Example usage:
 * @code
 * template<ValueOrError V>
 * void handle(const V& result) {
 *     if (result.is_ok()) {
 *         auto& val = result.value();
 *         // Process value
 *     } else {
 *         auto& err = result.error();
 *         // Handle error
 *     }
 * }
 * @endcode
 */
template<typename T>
concept ValueOrError = Resultable<T> && requires(const T t) {
    { t.value() };
    { t.error() };
};

} // namespace concepts
} // namespace kcenon::common
