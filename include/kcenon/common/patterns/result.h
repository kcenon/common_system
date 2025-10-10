// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file result.h
 * @brief Result<T> type for exception-free error handling.
 *
 * Provides a `Result<T>` type similar to Rust's Result or C++23's
 * `std::expected`, enabling explicit error propagation at module boundaries
 * without using exceptions.
 *
 * Thread Safety:
 * - Result<T> objects are NOT thread-safe for concurrent modification.
 * - Multiple threads may safely read the same Result<T> if no thread modifies it.
 * - Helper functions are thread-safe as they don't modify global state.
 * - If sharing Result<T> across threads, users must provide synchronization.
 * - Best practice: Use Result<T> as return values; avoid shared mutable access.
 */

#pragma once

#include <variant>
#include <optional>
#include <string>
#include <type_traits>
#include <stdexcept>

namespace common {

/**
 * @struct error_info
 * @brief Standard error information used by Result<T>.
 */
struct error_info {
    int code;
    std::string message;
    std::string module;
    std::optional<std::string> details;

    error_info() : code(0) {}

    /** @brief Construct with message only. */
    error_info(const std::string& msg)
        : code(-1), message(msg), module("") {}

    /** @brief Construct with code, message and optional module. */
    error_info(int c, const std::string& msg, const std::string& mod = "")
        : code(c), message(msg), module(mod) {}

    /** @brief Construct with code, message, module and details. */
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
 * @brief Alias for backward compatibility
 *
 * Some code may use error_code instead of error_info.
 * This alias ensures compatibility.
 */
using error_code = error_info;

// Forward declarations
template<typename T> class Result;
template<typename T> class Optional;

/**
 * @class Result
 * @brief Result type for error handling with member function support
 *
 * A Result<T> can contain either a value of type T or an error_info.
 * This provides a type-safe way to handle errors without exceptions.
 *
 * Thread Safety Note:
 * - Based on std::variant, which is NOT thread-safe for concurrent access.
 * - Safe to pass by value or const reference across threads.
 * - Concurrent reads of the same Result are safe if guaranteed no writes.
 * - For shared mutable access, wrap in std::mutex or similar.
 */
template<typename T>
class Result {
private:
    std::variant<T, error_info> value_;

public:
    // Constructors
    // Default constructor creates an error state
    Result() : value_(error_info{-6, "Uninitialized result", ""}) {}

    Result(const T& value) : value_(value) {}
    Result(T&& value) : value_(std::move(value)) {}
    Result(const error_info& error) : value_(error) {}
    Result(error_info&& error) : value_(std::move(error)) {}

    // Copy and move
    Result(const Result&) = default;
    Result(Result&&) = default;
    Result& operator=(const Result&) = default;
    Result& operator=(Result&&) = default;

    // Static factory methods (Rust-style API)
    /**
     * @brief Create a successful result with value (static factory)
     * @param value The value to wrap in Result (forwarded)
     * @return Result<T> containing the value
     *
     * Uses perfect forwarding to avoid ambiguity and support both
     * lvalues and rvalues efficiently.
     */
    template<typename U = T>
    static Result<T> ok(U&& value) {
        return Result<T>(std::forward<U>(value));
    }

    /**
     * @brief Create an error result from error_info (static factory)
     * @param error The error information
     * @return Result<T> containing the error
     */
    static Result<T> err(const error_info& error) {
        return Result<T>(error);
    }

    /**
     * @brief Create an error result from error_info (static factory, move)
     * @param error The error information
     * @return Result<T> containing the error
     */
    static Result<T> err(error_info&& error) {
        return Result<T>(std::move(error));
    }

    /**
     * @brief Create an error result with code and message (static factory)
     * @param code Error code
     * @param message Error message
     * @param module Optional module name
     * @return Result<T> containing the error
     */
    static Result<T> err(int code, const std::string& message, const std::string& module = "") {
        return Result<T>(error_info{code, message, module});
    }

    /**
     * @brief Check if result contains a successful value
     */
    bool is_ok() const {
        return std::holds_alternative<T>(value_);
    }

    /**
     * @brief Check if result contains an error
     */
    bool is_err() const {
        return std::holds_alternative<error_info>(value_);
    }

    /**
     * @brief Get value from result (throws if error)
     * @throws std::runtime_error if result contains error
     */
    const T& unwrap() const {
        if (is_err()) {
            const auto& err = std::get<error_info>(value_);
            throw std::runtime_error("Called unwrap on error: " + err.message);
        }
        return std::get<T>(value_);
    }

    /**
     * @brief Get mutable value from result (throws if error)
     * @throws std::runtime_error if result contains error
     */
    T& unwrap() {
        if (is_err()) {
            const auto& err = std::get<error_info>(value_);
            throw std::runtime_error("Called unwrap on error: " + err.message);
        }
        return std::get<T>(value_);
    }

    /**
     * @brief Get value or return default
     */
    T unwrap_or(T default_value) const {
        if (is_ok()) {
            return std::get<T>(value_);
        }
        return default_value;
    }

    /**
     * @brief Get value reference (const)
     */
    const T& value() const {
        return std::get<T>(value_);
    }

    /**
     * @brief Get value reference (mutable)
     */
    T& value() {
        return std::get<T>(value_);
    }

    /**
     * @brief Get error reference
     */
    const error_info& error() const {
        return std::get<error_info>(value_);
    }

    /**
     * @brief Map a function over a successful result
     */
    template<typename F>
    auto map(F&& func) const -> Result<decltype(func(std::declval<T>()))> {
        using ReturnType = decltype(func(std::declval<T>()));

        if (is_ok()) {
            return Result<ReturnType>(func(std::get<T>(value_)));
        } else {
            return Result<ReturnType>(std::get<error_info>(value_));
        }
    }

    /**
     * @brief Map a function that returns a Result (flatMap/bind)
     */
    template<typename F>
    auto and_then(F&& func) const -> decltype(func(std::declval<T>())) {
        using ReturnType = decltype(func(std::declval<T>()));

        if (is_ok()) {
            return func(std::get<T>(value_));
        } else {
            return ReturnType(std::get<error_info>(value_));
        }
    }

    /**
     * @brief Provide alternative value if error
     */
    template<typename F>
    Result<T> or_else(F&& func) const {
        if (is_ok()) {
            return *this;
        } else {
            return func(std::get<error_info>(value_));
        }
    }

    // For compatibility with variant-based APIs
    const std::variant<T, error_info>& as_variant() const { return value_; }
};

/**
 * @brief Specialized Result for void operations
 */
using VoidResult = Result<std::monostate>;

/**
 * @class Optional
 * @brief Optional type similar to std::optional with Rust-like API
 */
template<typename T>
class Optional {
private:
    std::optional<T> value_;

public:
    Optional() : value_(std::nullopt) {}
    Optional(const T& value) : value_(value) {}
    Optional(T&& value) : value_(std::move(value)) {}
    Optional(std::nullopt_t) : value_(std::nullopt) {}

    bool has_value() const { return value_.has_value(); }
    bool is_some() const { return value_.has_value(); }
    bool is_none() const { return !value_.has_value(); }

    const T& value() const { return value_.value(); }
    T& value() { return value_.value(); }

    const T& unwrap() const {
        if (!has_value()) {
            throw std::runtime_error("Called unwrap on None");
        }
        return value_.value();
    }

    T unwrap_or(T default_value) const {
        return value_.value_or(default_value);
    }

    template<typename F>
    auto map(F&& func) const -> Optional<decltype(func(std::declval<T>()))> {
        using ReturnType = decltype(func(std::declval<T>()));

        if (has_value()) {
            return Optional<ReturnType>(func(value_.value()));
        } else {
            return Optional<ReturnType>(std::nullopt);
        }
    }
};

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

/**
 * @brief Create a successful result (lowercase - for compatibility)
 */
template<typename T>
inline Result<T> ok(T value) {
    return Result<T>(std::move(value));
}

/**
 * @brief Create a successful result (uppercase - Rust style)
 */
template<typename T>
inline Result<T> Ok(T value) {
    return Result<T>(std::move(value));
}

/**
 * @brief Create a successful void result
 */
inline VoidResult ok() {
    return VoidResult(std::monostate{});
}

/**
 * @brief Create an error result (lowercase)
 */
template<typename T>
inline Result<T> error(int code, const std::string& message,
                       const std::string& module = "") {
    return Result<T>(error_info{code, message, module});
}

/**
 * @brief Create an error result (uppercase - Rust style)
 */
template<typename T>
inline Result<T> Err(const std::string& message) {
    return Result<T>(error_info{message});
}

/**
 * @brief Create an error result with code
 */
template<typename T>
inline Result<T> Err(int code, const std::string& message,
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

// Optional factory functions

/**
 * @brief Create an Optional with value
 */
template<typename T>
inline Optional<T> Some(T value) {
    return Optional<T>(std::move(value));
}

/**
 * @brief Create an empty Optional
 */
template<typename T>
inline Optional<T> None() {
    return Optional<T>(std::nullopt);
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
        return ok<T>(func());
    } catch (const std::exception& e) {
        return error<T>(-99, e.what(), module);
    } catch (...) {
        return error<T>(-99, "Unknown error", module);
    }
}

// Specialization for void return type
template<typename F>
VoidResult try_catch_void(F&& func, const std::string& module = "") {
    try {
        func();
        return ok();
    } catch (const std::exception& e) {
        return error<std::monostate>(-99, e.what(), module);
    } catch (...) {
        return error<std::monostate>(-99, "Unknown error", module);
    }
}

} // namespace common

// Convenience macros for Result<T> pattern usage

/**
 * @brief Return early if expression is an error
 *
 * Usage:
 *   RETURN_IF_ERROR(some_operation());
 *   // Continue only if successful
 */
#define RETURN_IF_ERROR(expr) \
    do { \
        auto _result_temp = (expr); \
        if (common::is_error(_result_temp)) { \
            return common::get_error(_result_temp); \
        } \
    } while(false)

/**
 * @brief Assign value or return error
 *
 * Usage:
 *   ASSIGN_OR_RETURN(auto value, get_value());
 *   // Use 'value' here
 */
#define ASSIGN_OR_RETURN(decl, expr) \
    auto _result_##decl = (expr); \
    if (common::is_error(_result_##decl)) { \
        return common::get_error(_result_##decl); \
    } \
    decl = common::get_value(std::move(_result_##decl))

/**
 * @brief Return error if condition is false
 *
 * Usage:
 *   RETURN_ERROR_IF(!ptr, error_codes::INVALID_ARGUMENT, "Null pointer", "MyModule");
 */
#define RETURN_ERROR_IF(condition, code, message, module) \
    do { \
        if (condition) { \
            return common::error_info{code, message, module}; \
        } \
    } while(false)

/**
 * @brief Return error with details if condition is false
 *
 * Usage:
 *   RETURN_ERROR_IF_WITH_DETAILS(!valid, -1, "Invalid", "Module", "Details");
 */
#define RETURN_ERROR_IF_WITH_DETAILS(condition, code, message, module, details) \
    do { \
        if (condition) { \
            return common::error_info{code, message, module, details}; \
        } \
    } while(false)
