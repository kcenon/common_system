// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file result_helpers.h
 * @brief Type-safe helper functions replacing macros for Result<T> pattern
 *
 * This header provides template-based utilities that replace the macro-based
 * helpers in result.h, offering better type safety, debuggability, and
 * compile-time error checking.
 *
 * Improvements over macros:
 * - Full type safety with template deduction
 * - Better error messages from compiler
 * - Debugger-friendly (no macro expansion)
 * - No name collision risks
 * - Works with IDEs and code analysis tools
 */

#pragma once

#include "result.h"
#include <utility>
#include <type_traits>

namespace kcenon::common {
namespace helpers {

/**
 * @brief Return error if Result is in error state (type-safe alternative to RETURN_IF_ERROR)
 * @param result The Result to check
 * @return The error_info if result is error, otherwise continues execution
 *
 * Usage:
 *   auto result = some_operation();
 *   if (auto err = return_if_error(result)) {
 *       return *err;
 *   }
 *   // Continue with result.value()
 */
template<typename T>
[[nodiscard]] auto return_if_error(const Result<T>& result)
    -> std::optional<error_info>
{
    if (result.is_err()) {
        return result.error();
    }
    return std::nullopt;
}

/**
 * @brief Try to unwrap a Result, returning early if error
 * @param expr The Result expression to unwrap
 * @return Reference to the value if successful
 *
 * Usage:
 * @code
 *   Result<int> get_value();
 *
 *   Result<std::string> process() {
 *       auto value = TRY_UNWRAP(get_value());
 *       return ok(std::to_string(value));
 *   }
 * @endcode
 */
#define TRY_UNWRAP(expr) \
    ({ \
        auto&& _result = (expr); \
        if (_result.is_err()) { \
            return _result.error(); \
        } \
        std::move(_result).value(); \
    })

/**
 * @brief Safely extract value from Result or return error
 * @param result The Result to extract from
 * @return Value if ok, propagates error if err
 *
 * This is a safer alternative that works at function boundaries.
 */
template<typename T>
[[nodiscard]] auto try_extract(Result<T>&& result)
    -> std::conditional_t<std::is_void_v<T>, Result<std::monostate>, Result<T>>
{
    return std::forward<Result<T>>(result);
}

/**
 * @brief Check condition and return error if false (type-safe alternative to RETURN_ERROR_IF)
 * @param condition The condition to check
 * @param error The error_info to return if condition is true
 * @return Optional error_info
 *
 * Usage:
 *   if (auto err = error_if(!ptr, error_info{-1, "Null pointer", "Module"})) {
 *       return *err;
 *   }
 */
[[nodiscard]] inline auto error_if(bool condition, const error_info& error)
    -> std::optional<error_info>
{
    if (condition) {
        return error;
    }
    return std::nullopt;
}

/**
 * @brief Create error_info with code, message, and module (type-safe)
 * @param code Error code
 * @param message Error message
 * @param module Module name
 * @return error_info object
 */
[[nodiscard]] inline auto make_error(int code,
                                     const std::string& message,
                                     const std::string& module = "") noexcept
    -> error_info
{
    return error_info{code, message, module};
}

/**
 * @brief Create error_info with code, message, module, and details
 * @param code Error code
 * @param message Error message
 * @param module Module name
 * @param details Additional details
 * @return error_info object
 */
[[nodiscard]] inline auto make_error_with_details(
    int code,
    const std::string& message,
    const std::string& module,
    const std::string& details) noexcept
    -> error_info
{
    return error_info{code, message, module, details};
}

/**
 * @brief Chain multiple Result-returning operations
 * @param funcs Variadic list of functions returning Result<T>
 * @return Result of the last successful operation, or first error
 *
 * Usage:
 *   auto result = chain(
 *       []() { return operation1(); },
 *       [](auto& r1) { return operation2(r1); },
 *       [](auto& r2) { return operation3(r2); }
 *   );
 */
template<typename... Funcs>
[[nodiscard]] auto chain(Funcs&&... funcs) {
    using FirstResult = std::invoke_result_t<std::tuple_element_t<0, std::tuple<Funcs...>>>;

    FirstResult result = std::get<0>(std::forward_as_tuple(funcs...))();

    if constexpr (sizeof...(funcs) > 1) {
        ([&]<typename Func>(Func&& func) {
            if (result.is_ok()) {
                result = func(result.value());
            }
        }(std::forward<Funcs>(funcs)), ...);
    }

    return result;
}

/**
 * @brief Execute function and convert exceptions to Result
 * @param func Function to execute
 * @param module Module name for error reporting
 * @return Result containing function result or error
 *
 * This is a more efficient version of try_catch from result.h
 */
template<typename Func>
[[nodiscard]] auto safe_execute(Func&& func, const std::string& module = "")
    -> Result<std::invoke_result_t<Func>>
{
    using ReturnType = std::invoke_result_t<Func>;

    try {
        if constexpr (std::is_void_v<ReturnType>) {
            func();
            return Result<std::monostate>(std::monostate{});
        } else {
            return Result<ReturnType>(func());
        }
    } catch (const std::exception& e) {
        return Result<ReturnType>(error_info{-99, e.what(), module});
    } catch (...) {
        return Result<ReturnType>(error_info{-99, "Unknown error", module});
    }
}

/**
 * @brief Unwrap Result with custom error handler
 * @param result The Result to unwrap
 * @param error_handler Function called if result is error
 * @return Value if ok, or default-constructed T if error
 *
 * Usage:
 *   int value = unwrap_or_handle(result, [](const auto& err) {
 *       log_error("Operation failed: {}", err.message);
 *   });
 */
template<typename T, typename ErrorHandler>
[[nodiscard]] auto unwrap_or_handle(Result<T>&& result, ErrorHandler&& error_handler) -> T
{
    if (result.is_err()) {
        error_handler(result.error());
        return T{};
    }
    return std::move(result).value();
}

/**
 * @brief Combine multiple Results into a single Result containing a tuple
 * @param results Variadic list of Result objects
 * @return Result<tuple> containing all values, or first error
 *
 * Usage:
 *   auto combined = combine_results(
 *       get_int(),
 *       get_string(),
 *       get_double()
 *   );
 *   if (combined) {
 *       auto [i, s, d] = combined.value();
 *   }
 */
template<typename... Results>
[[nodiscard]] auto combine_results(Results&&... results)
    -> Result<std::tuple<typename std::decay_t<Results>::value_type...>>
{
    // Check if any result is error
    bool has_error = (... || results.is_err());
    if (has_error) {
        // Return first error found
        error_info first_error;
        ((results.is_err() && (first_error = results.error(), true)) || ...);
        return Result<std::tuple<typename std::decay_t<Results>::value_type...>>(first_error);
    }

    // All ok, combine values
    return Result<std::tuple<typename std::decay_t<Results>::value_type...>>(
        std::make_tuple(std::forward<Results>(results).value()...)
    );
}

} // namespace helpers
} // namespace kcenon::common
