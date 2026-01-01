// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file utilities.h
 * @brief Consolidated utility functions and macros for Result<T> pattern.
 *
 * This header consolidates Result pattern utilities:
 * - Factory functions (ok, make_error)
 * - Helper functions for working with Results
 * - Exception to Result conversion utilities
 * - Convenience macros for common patterns
 *
 * This consolidation reduces header count from 3 to 1, preparing for
 * C++20 module migration while following Kent Beck's "Fewest Elements" principle.
 */

#pragma once

#include "core.h"
#include "compat.h"

#include <stdexcept>
#include <system_error>
#include <utility>
#include <variant>

namespace kcenon::common {

// ============================================================================
// Factory and Helper Functions (from result_funcs.h)
// ============================================================================

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

// ============================================================================
// Exception Conversion (from exception_conversion.h)
// ============================================================================

/**
 * @class exception_mapper
 * @brief Maps standard exception types to appropriate error codes
 *
 * Provides automatic error code assignment based on exception type,
 * enabling more precise error handling without manual code specification.
 *
 * Note: This class is kept for API compatibility but is no longer used internally.
 * The try_catch functions now use multiple catch blocks for better performance.
 */
class exception_mapper {
public:
    /**
     * @brief Map unknown exception (catch-all)
     * @param module Module name
     * @return error_info for unknown exception
     */
    static error_info map_unknown_exception(const std::string& module = "") {
        return error_info{error_codes::INTERNAL_ERROR, "Unknown exception caught", module,
                         "Non-standard exception (not derived from std::exception)"};
    }

    /**
     * @brief Map generic exception
     * @param e Exception object
     * @param module Module name
     * @return error_info
     */
    static error_info map_generic_exception(const std::exception& e, const std::string& module = "") {
        return error_info{error_codes::INTERNAL_ERROR, e.what(), module, "std::exception"};
    }
};

/**
 * @brief Convert exception to Result with automatic error code mapping
 *
 * Enhanced version that automatically assigns appropriate error codes
 * based on exception type, providing better error diagnostics.
 * Uses multiple catch blocks instead of RTTI for better performance.
 *
 * @tparam T Return type
 * @tparam F Callable type
 * @param func Callable to execute
 * @param module Module name for error context
 * @return Result<T> with value or mapped error
 *
 * Example:
 * @code
 * auto result = try_catch<int>([]() {
 *     return parse_integer("invalid");  // throws std::invalid_argument
 * }, "parser");
 * // result contains error with code INVALID_ARGUMENT
 * @endcode
 */
template<typename T, typename F>
Result<T> try_catch(F&& func, const std::string& module = "") {
    try {
        return ok<T>(func());
    }
    // Memory allocation failure
    catch (const std::bad_alloc& e) {
        return Result<T>(error_info{error_codes::OUT_OF_MEMORY, e.what(), module, "std::bad_alloc"});
    }
    // Invalid argument (check before logic_error)
    catch (const std::invalid_argument& e) {
        return Result<T>(error_info{error_codes::INVALID_ARGUMENT, e.what(), module, "std::invalid_argument"});
    }
    // Out of range (check before logic_error)
    catch (const std::out_of_range& e) {
        return Result<T>(error_info{error_codes::INVALID_ARGUMENT, e.what(), module, "std::out_of_range"});
    }
    // System errors (check before runtime_error as it inherits from it)
    catch (const std::system_error& e) {
        return Result<T>(error_info{e.code().value(), e.what(), module,
                        std::string("std::system_error: ") + e.code().category().name()});
    }
    // Logic errors
    catch (const std::logic_error& e) {
        return Result<T>(error_info{error_codes::INTERNAL_ERROR, e.what(), module, "std::logic_error"});
    }
    // Runtime errors
    catch (const std::runtime_error& e) {
        return Result<T>(error_info{error_codes::INTERNAL_ERROR, e.what(), module, "std::runtime_error"});
    }
    // Generic std::exception
    catch (const std::exception& e) {
        return Result<T>(exception_mapper::map_generic_exception(e, module));
    }
    // Unknown exception
    catch (...) {
        return Result<T>(exception_mapper::map_unknown_exception(module));
    }
}

/**
 * @brief Convert exception to VoidResult with automatic error code mapping
 *
 * Specialization for void return type.
 * Uses multiple catch blocks instead of RTTI for better performance.
 *
 * @tparam F Callable type
 * @param func Callable to execute (returns void)
 * @param module Module name for error context
 * @return VoidResult with success or mapped error
 */
template<typename F>
VoidResult try_catch_void(F&& func, const std::string& module = "") {
    try {
        func();
        return ok();
    }
    // Memory allocation failure
    catch (const std::bad_alloc& e) {
        return VoidResult(error_info{error_codes::OUT_OF_MEMORY, e.what(), module, "std::bad_alloc"});
    }
    // Invalid argument (check before logic_error)
    catch (const std::invalid_argument& e) {
        return VoidResult(error_info{error_codes::INVALID_ARGUMENT, e.what(), module, "std::invalid_argument"});
    }
    // Out of range (check before logic_error)
    catch (const std::out_of_range& e) {
        return VoidResult(error_info{error_codes::INVALID_ARGUMENT, e.what(), module, "std::out_of_range"});
    }
    // System errors (check before runtime_error as it inherits from it)
    catch (const std::system_error& e) {
        return VoidResult(error_info{e.code().value(), e.what(), module,
                        std::string("std::system_error: ") + e.code().category().name()});
    }
    // Logic errors
    catch (const std::logic_error& e) {
        return VoidResult(error_info{error_codes::INTERNAL_ERROR, e.what(), module, "std::logic_error"});
    }
    // Runtime errors
    catch (const std::runtime_error& e) {
        return VoidResult(error_info{error_codes::INTERNAL_ERROR, e.what(), module, "std::runtime_error"});
    }
    // Generic std::exception
    catch (const std::exception& e) {
        return VoidResult(exception_mapper::map_generic_exception(e, module));
    }
    // Unknown exception
    catch (...) {
        return VoidResult(exception_mapper::map_unknown_exception(module));
    }
}

} // namespace kcenon::common

// ============================================================================
// Convenience Macros (from result_macros.h)
// ============================================================================

// Convenience macros for Result<T> pattern usage

/**
 * @brief Return early if expression is an error
 *
 * Usage:
 *   COMMON_RETURN_IF_ERROR(some_operation());
 *   // Continue only if successful
 */
#define COMMON_RETURN_IF_ERROR(expr) \
    do { \
        auto _result_temp = (expr); \
        if (kcenon::common::is_error(_result_temp)) { \
            return kcenon::common::get_error(_result_temp); \
        } \
    } while(false)

/**
 * @brief Assign value or return error
 *
 * Usage:
 *   COMMON_ASSIGN_OR_RETURN(auto value, get_value());
 *   // Use 'value' here
 */
#define COMMON_ASSIGN_OR_RETURN(decl, expr) \
    auto _result_##decl = (expr); \
    if (kcenon::common::is_error(_result_##decl)) { \
        return kcenon::common::get_error(_result_##decl); \
    } \
    decl = kcenon::common::get_value(std::move(_result_##decl))

/**
 * @brief Return error if condition is false
 *
 * Usage:
 *   COMMON_RETURN_ERROR_IF(!ptr, error_codes::INVALID_ARGUMENT, "Null pointer", "MyModule");
 */
#define COMMON_RETURN_ERROR_IF(condition, code, message, module) \
    do { \
        if (condition) { \
            return kcenon::common::error_info{code, message, module}; \
        } \
    } while(false)

/**
 * @brief Return error with details if condition is false
 *
 * Usage:
 *   COMMON_RETURN_ERROR_IF_WITH_DETAILS(!valid, -1, "Invalid", "Module", "Details");
 */
#define COMMON_RETURN_ERROR_IF_WITH_DETAILS(condition, code, message, module, details) \
    do { \
        if (condition) { \
            return kcenon::common::error_info{code, message, module, details}; \
        } \
    } while(false)
