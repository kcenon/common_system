/**
 * @file result.h
 * @brief Standard Result<T> type for consistent error handling
 *
 * Provides a Result type similar to Rust's Result or C++23's std::expected
 * for handling errors without exceptions at module boundaries.
 */

#pragma once

#include <variant>
#include <optional>
#include <string>
#include <type_traits>

namespace common {

/**
 * @struct error_info
 * @brief Standard error information structure
 */
struct error_info {
    int code;
    std::string message;
    std::string module;
    std::optional<std::string> details;

    error_info() : code(0) {}

    error_info(int c, const std::string& msg, const std::string& mod = "")
        : code(c), message(msg), module(mod) {}

    error_info(int c, const std::string& msg, const std::string& mod,
               const std::string& det)
        : code(c), message(msg), module(mod), details(det) {}

    bool operator==(const error_info& other) const {
        return code == other.code && message == other.message &&
               module == other.module && details == other.details;
    }

    bool operator!=(const error_info& other) const {
        return !(*this == other);
    }
};

/**
 * @brief Result type for error handling
 *
 * A Result<T> can contain either a value of type T or an error_info.
 * This provides a type-safe way to handle errors without exceptions.
 */
template<typename T>
using Result = std::variant<T, error_info>;

/**
 * @brief Specialized Result for void operations
 */
using VoidResult = Result<std::monostate>;

// Helper functions for working with Results

/**
 * @brief Check if result contains a successful value
 */
template<typename T>
inline bool is_ok(const Result<T>& result) {
    return std::holds_alternative<T>(result);
}

/**
 * @brief Check if result contains an error
 */
template<typename T>
inline bool is_error(const Result<T>& result) {
    return std::holds_alternative<error_info>(result);
}

/**
 * @brief Get value from result
 * @throws std::bad_variant_access if result contains error
 */
template<typename T>
inline const T& get_value(const Result<T>& result) {
    return std::get<T>(result);
}

/**
 * @brief Get mutable value from result
 * @throws std::bad_variant_access if result contains error
 */
template<typename T>
inline T& get_value(Result<T>& result) {
    return std::get<T>(result);
}

/**
 * @brief Get error from result
 * @throws std::bad_variant_access if result contains value
 */
template<typename T>
inline const error_info& get_error(const Result<T>& result) {
    return std::get<error_info>(result);
}

/**
 * @brief Get value or return default
 */
template<typename T>
inline T value_or(const Result<T>& result, T default_value) {
    if (is_ok(result)) {
        return get_value(result);
    }
    return default_value;
}

/**
 * @brief Get value pointer if ok, nullptr if error
 */
template<typename T>
inline const T* get_if_ok(const Result<T>& result) {
    return std::get_if<T>(&result);
}

/**
 * @brief Get error pointer if error, nullptr if ok
 */
template<typename T>
inline const error_info* get_if_error(const Result<T>& result) {
    return std::get_if<error_info>(&result);
}

// Factory functions for creating Results

/**
 * @brief Create a successful result
 */
template<typename T>
inline Result<T> ok(T value) {
    return Result<T>(std::move(value));
}

/**
 * @brief Create a successful void result
 */
inline VoidResult ok() {
    return VoidResult(std::monostate{});
}

/**
 * @brief Create an error result
 */
template<typename T>
inline Result<T> error(int code, const std::string& message,
                       const std::string& module = "") {
    return Result<T>(error_info{code, message, module});
}

/**
 * @brief Create an error result with details
 */
template<typename T>
inline Result<T> error(int code, const std::string& message,
                       const std::string& module,
                       const std::string& details) {
    return Result<T>(error_info{code, message, module, details});
}

/**
 * @brief Create an error result from existing error_info
 */
template<typename T>
inline Result<T> error(const error_info& err) {
    return Result<T>(err);
}

// Monadic operations (similar to std::expected when available)

/**
 * @brief Map a function over a successful result
 */
template<typename T, typename F>
auto map(const Result<T>& result, F&& func)
    -> Result<decltype(func(std::declval<T>()))> {
    using ReturnType = decltype(func(std::declval<T>()));

    if (is_ok(result)) {
        return ok<ReturnType>(func(get_value(result)));
    } else {
        return error<ReturnType>(get_error(result));
    }
}

/**
 * @brief Map a function that returns a Result
 */
template<typename T, typename F>
auto and_then(const Result<T>& result, F&& func)
    -> decltype(func(std::declval<T>())) {
    using ReturnType = decltype(func(std::declval<T>()));

    if (is_ok(result)) {
        return func(get_value(result));
    } else {
        return ReturnType(get_error(result));
    }
}

/**
 * @brief Provide alternative value if error
 */
template<typename T, typename F>
Result<T> or_else(const Result<T>& result, F&& func) {
    if (is_ok(result)) {
        return result;
    } else {
        return func(get_error(result));
    }
}

// Common error codes
namespace error_codes {
    constexpr int SUCCESS = 0;
    constexpr int INVALID_ARGUMENT = -1;
    constexpr int NOT_FOUND = -2;
    constexpr int PERMISSION_DENIED = -3;
    constexpr int TIMEOUT = -4;
    constexpr int CANCELLED = -5;
    constexpr int NOT_INITIALIZED = -6;
    constexpr int ALREADY_EXISTS = -7;
    constexpr int OUT_OF_MEMORY = -8;
    constexpr int IO_ERROR = -9;
    constexpr int NETWORK_ERROR = -10;
    constexpr int INTERNAL_ERROR = -99;

    // Module-specific ranges
    constexpr int THREAD_ERROR_BASE = -100;
    constexpr int LOGGER_ERROR_BASE = -200;
    constexpr int MONITORING_ERROR_BASE = -300;
    constexpr int CONTAINER_ERROR_BASE = -400;
    constexpr int DATABASE_ERROR_BASE = -500;
    constexpr int NETWORK_ERROR_BASE = -600;
}

/**
 * @brief Convert exception to Result
 *
 * Helper to wrap exception-throwing code into Result
 */
template<typename T, typename F>
Result<T> try_catch(F&& func, const std::string& module = "") {
    try {
        if constexpr (std::is_void_v<T>) {
            func();
            return ok();
        } else {
            return ok<T>(func());
        }
    } catch (const std::exception& e) {
        return error<T>(error_codes::INTERNAL_ERROR, e.what(), module);
    } catch (...) {
        return error<T>(error_codes::INTERNAL_ERROR, "Unknown error", module);
    }
}

} // namespace common