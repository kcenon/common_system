// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file result_funcs.h
 * @brief Factory and helper functions for Result<T>.
 *
 * @deprecated This header is deprecated. Use result/utilities.h instead.
 * This header will be removed in a future version.
 *
 * This header provides free functions for creating and working with Result<T>
 * objects, including factory functions (ok, make_error) and utility functions.
 */

#pragma once

#ifdef _MSC_VER
#pragma message("warning: result_funcs.h is deprecated. Use result/utilities.h instead.")
#else
#pragma message("result_funcs.h is deprecated. Use result/utilities.h instead.")
#endif

#include "utilities.h"

// Backward compatibility - factory functions are now defined in utilities.h

#if 0  // Original content preserved for reference

#include "result_core.h"

#include <variant>
#include <utility>

namespace kcenon::common {

// Forward declaration for VoidResult
using VoidResult = Result<std::monostate>;

// Helper functions for working with Results

/**
 * @brief Check if result contains a successful value
 */
template<typename T>
inline bool is_ok(const Result<T>& result) {
    return result.is_ok();
}

/**
 * @brief Check if result contains an error
 */
template<typename T>
inline bool is_error(const Result<T>& result) {
    return result.is_err();
}

/**
 * @brief Get value from result
 * @throws std::bad_variant_access if result contains error
 */
template<typename T>
inline const T& get_value(const Result<T>& result) {
    return result.value();
}

/**
 * @brief Get mutable value from result
 * @throws std::bad_variant_access if result contains error
 */
template<typename T>
inline T& get_value(Result<T>& result) {
    return result.value();
}

/**
 * @brief Get error from result
 * @throws std::bad_variant_access if result contains value
 */
template<typename T>
inline const error_info& get_error(const Result<T>& result) {
    return result.error();
}

/**
 * @brief Get value or return default
 */
template<typename T>
inline T value_or(const Result<T>& result, T default_value) {
    return result.unwrap_or(default_value);
}

/**
 * @brief Get value pointer if ok, nullptr if error
 */
template<typename T>
inline const T* get_if_ok(const Result<T>& result) {
    if (result.is_ok()) {
        return &result.value();
    }
    return nullptr;
}

/**
 * @brief Get error pointer if error, nullptr if ok
 */
template<typename T>
inline const error_info* get_if_error(const Result<T>& result) {
    if (result.is_err()) {
        return &result.error();
    }
    return nullptr;
}

// Factory functions for creating Results
// See docs/BEST_PRACTICES.md for detailed usage patterns

/**
 * @brief Create a successful result
 * @param value The value to wrap
 * @return Result<T> containing the value
 *
 * @note RECOMMENDED: This is the preferred way to create successful results.
 *
 * Example:
 * @code
 * auto result = ok(42);
 * auto str_result = ok(std::string("hello"));
 * @endcode
 */
template<typename T>
inline Result<T> ok(T value) {
    return Result<T>(std::move(value));
}

/**
 * @brief Create a successful void result
 *
 * @note RECOMMENDED: Use for functions that don't return a value.
 *
 * Example:
 * @code
 * VoidResult save_data() {
 *     // ... save logic ...
 *     return ok();
 * }
 * @endcode
 */
inline VoidResult ok() {
    return VoidResult(std::monostate{});
}

/**
 * @brief Create an error result with code and message
 * @param code Error code
 * @param message Error message
 * @param module Optional module name
 * @return Result<T> containing the error
 *
 * @note RECOMMENDED: This is the preferred way to create error results.
 *
 * Example:
 * @code
 * return make_error<int>(error_codes::INVALID_ARGUMENT, "Value must be positive");
 * return make_error<int>(error_codes::NOT_FOUND, "Resource not found", "database");
 * @endcode
 */
template<typename T>
inline Result<T> make_error(int code, const std::string& message,
                            const std::string& module = "") {
    return Result<T>(error_info{code, message, module});
}

/**
 * @brief Create an error result with details
 */
template<typename T>
inline Result<T> make_error(int code, const std::string& message,
                            const std::string& module,
                            const std::string& details) {
    return Result<T>(error_info{code, message, module, details});
}

/**
 * @brief Create an error result from existing error_info
 */
template<typename T>
inline Result<T> make_error(const error_info& err) {
    return Result<T>(err);
}

// Monadic operations (for free functions)

/**
 * @brief Map a function over a successful result
 */
template<typename T, typename F>
auto map(const Result<T>& result, F&& func)
    -> Result<decltype(func(std::declval<T>()))> {
    return result.map(std::forward<F>(func));
}

/**
 * @brief Map a function that returns a Result
 */
template<typename T, typename F>
auto and_then(const Result<T>& result, F&& func)
    -> decltype(func(std::declval<T>())) {
    return result.and_then(std::forward<F>(func));
}

/**
 * @brief Provide alternative value if error
 */
template<typename T, typename F>
Result<T> or_else(const Result<T>& result, F&& func) {
    return result.or_else(std::forward<F>(func));
}

} // namespace kcenon::common

#endif  // Original content preserved for reference
