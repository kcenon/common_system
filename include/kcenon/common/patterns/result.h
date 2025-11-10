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
#include <system_error>
#include <sstream>

// Import error codes from centralized location
#include <kcenon/common/error/error_codes.h>

// Import C++17-compatible source_location
#include <kcenon/common/utils/source_location.h>

namespace kcenon::common {

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
 * @brief Result state enum for tracking initialization
 */
enum class result_state {
    uninitialized,  // Result has not been initialized with a value or error
    ok,             // Result contains a valid value
    error           // Result contains an error
};

/**
 * @class Result
 * @brief Result type for error handling with member function support
 *
 * A Result<T> can be in one of two states:
 * 1. Ok - Contains a valid value of type T
 * 2. Error - Contains an error_info describing the failure
 *
 * This provides a type-safe way to handle errors without exceptions.
 *
 * IMPORTANT: Default-constructed Results are initialized to error state
 * with not_initialized code (-6). For explicit construction, use factory
 * methods Result<T>::ok() and Result<T>::err().
 *
 * BREAKING CHANGE (from v1.x): Previously, default construction created
 * an uninitialized state. Now it creates an error state for safety.
 *
 * Thread Safety Note:
 * - Based on std::optional, which is NOT thread-safe for concurrent access.
 * - Safe to pass by value or const reference across threads.
 * - Concurrent reads of the same Result are safe if guaranteed no writes.
 * - For shared mutable access, wrap in std::mutex or similar.
 */
template<typename T>
class Result {
private:
    // Use std::optional<T> for value, which naturally supports uninitialized state
    // Store error separately to avoid variant issues
    std::optional<T> value_;
    std::optional<error_info> error_;

public:
    // Constructors
    /**
     * @brief Default constructor - initializes to error state
     *
     * BREAKING CHANGE: Previously created uninitialized state, now creates
     * error state with not_initialized code to prevent accidental access.
     * This makes Result exception-safe by default.
     *
     * Migration: Code relying on default construction should explicitly use
     * Result<T>::ok() or Result<T>::err() factory methods instead.
     */
    Result() : value_(std::nullopt),
               error_(error_info{-6, "Result not initialized", "common::Result"}) {}

    Result(const T& value) : value_(value), error_(std::nullopt) {}
    Result(T&& value) : value_(std::move(value)), error_(std::nullopt) {}
    Result(const error_info& error) : value_(std::nullopt), error_(error) {}
    Result(error_info&& error) : value_(std::nullopt), error_(std::move(error)) {}

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
     * @brief Check if result is in uninitialized state
     * @deprecated Since default constructor now initializes to error state,
     *             this method always returns false. Kept for backward compatibility.
     * @return Always false (Results are always initialized now)
     */
    [[deprecated("Result is always initialized now; check is_err() instead")]]
    bool is_uninitialized() const {
        return !value_.has_value() && !error_.has_value();
    }

    /**
     * @brief Check if result contains a successful value
     */
    bool is_ok() const {
        return value_.has_value();
    }

    /**
     * @brief Check if result contains an error
     */
    bool is_err() const {
        return error_.has_value();
    }

    /**
     * @brief Get value from result (throws if error or uninitialized)
     * @param loc Source location of the unwrap() call (automatically captured, if supported)
     * @throws std::runtime_error if result contains error or is uninitialized
     */
#if COMMON_HAS_SOURCE_LOCATION
    const T& unwrap(
        source_location loc = source_location::current()
    ) const {
        if (is_uninitialized()) {
            std::ostringstream oss;
            oss << "Called unwrap on uninitialized Result\n"
                << "  Location: " << loc.file_name() << ":" << loc.line() << ":" << loc.column() << "\n"
                << "  Function: " << loc.function_name();
            throw std::runtime_error(oss.str());
        }
        if (is_err()) {
            const auto& err = error_.value();
            std::ostringstream oss;
            oss << "Called unwrap on error: " << err.message << "\n"
                << "  Error code: " << err.code << "\n"
                << "  Module: " << (err.module.empty() ? "unknown" : err.module) << "\n"
                << "  Location: " << loc.file_name() << ":" << loc.line() << ":" << loc.column() << "\n"
                << "  Function: " << loc.function_name();
            if (err.details.has_value()) {
                oss << "\n  Details: " << err.details.value();
            }
            throw std::runtime_error(oss.str());
        }
        return value_.value();
    }
#else
    const T& unwrap() const {
        if (is_uninitialized()) {
            throw std::runtime_error("Called unwrap on uninitialized Result");
        }
        if (is_err()) {
            const auto& err = error_.value();
            throw std::runtime_error("Called unwrap on error: " + err.message);
        }
        return value_.value();
    }
#endif

    /**
     * @brief Get mutable value from result (throws if error or uninitialized)
     * @param loc Source location of the unwrap() call (automatically captured, if supported)
     * @throws std::runtime_error if result contains error or is uninitialized
     */
#if COMMON_HAS_SOURCE_LOCATION
    T& unwrap(
        source_location loc = source_location::current()
    ) {
        if (is_uninitialized()) {
            std::ostringstream oss;
            oss << "Called unwrap on uninitialized Result\n"
                << "  Location: " << loc.file_name() << ":" << loc.line() << ":" << loc.column() << "\n"
                << "  Function: " << loc.function_name();
            throw std::runtime_error(oss.str());
        }
        if (is_err()) {
            const auto& err = error_.value();
            std::ostringstream oss;
            oss << "Called unwrap on error: " << err.message << "\n"
                << "  Error code: " << err.code << "\n"
                << "  Module: " << (err.module.empty() ? "unknown" : err.module) << "\n"
                << "  Location: " << loc.file_name() << ":" << loc.line() << ":" << loc.column() << "\n"
                << "  Function: " << loc.function_name();
            if (err.details.has_value()) {
                oss << "\n  Details: " << err.details.value();
            }
            throw std::runtime_error(oss.str());
        }
        return value_.value();
    }
#else
    T& unwrap() {
        if (is_uninitialized()) {
            throw std::runtime_error("Called unwrap on uninitialized Result");
        }
        if (is_err()) {
            const auto& err = error_.value();
            throw std::runtime_error("Called unwrap on error: " + err.message);
        }
        return value_.value();
    }
#endif

    /**
     * @brief Get value or return default
     */
    T unwrap_or(T default_value) const {
        if (is_ok()) {
            return value_.value();
        }
        return default_value;
    }

    /**
     * @brief Get value or return default (C++23 std::expected compatible)
     *
     * Alias for unwrap_or() that matches std::expected::value_or() API.
     */
    T value_or(T default_value) const {
        return unwrap_or(std::move(default_value));
    }

    /**
     * @brief Get value reference (const)
     */
    const T& value() const {
        return value_.value();
    }

    /**
     * @brief Get value reference (mutable)
     */
    T& value() {
        return value_.value();
    }

    /**
     * @brief Get error reference
     */
    const error_info& error() const {
        return error_.value();
    }

    /**
     * @brief Map a function over a successful result
     */
    template<typename F>
    auto map(F&& func) const -> Result<decltype(func(std::declval<T>()))> {
        using ReturnType = decltype(func(std::declval<T>()));

        if (is_ok()) {
            return Result<ReturnType>(func(value_.value()));
        } else if (is_err()) {
            return Result<ReturnType>(error_.value());
        } else {
            // Uninitialized state
            return Result<ReturnType>();
        }
    }

    /**
     * @brief Map a function that returns a Result (flatMap/bind)
     */
    template<typename F>
    auto and_then(F&& func) const -> decltype(func(std::declval<T>())) {
        using ReturnType = decltype(func(std::declval<T>()));

        if (is_ok()) {
            return func(value_.value());
        } else if (is_err()) {
            return ReturnType(error_.value());
        } else {
            // Uninitialized state
            return ReturnType();
        }
    }

    /**
     * @brief Provide alternative value if error
     */
    template<typename F>
    Result<T> or_else(F&& func) const {
        if (is_ok()) {
            return *this;
        } else if (is_err()) {
            return func(error_.value());
        } else {
            // Uninitialized state
            return *this;
        }
    }
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

    /**
     * @brief Get value from optional (throws if None)
     * @param loc Source location of the unwrap() call (automatically captured, if supported)
     * @throws std::runtime_error if optional is None with detailed location info
     */
#if COMMON_HAS_SOURCE_LOCATION
    const T& unwrap(
        source_location loc = source_location::current()
    ) const {
        if (!has_value()) {
            std::ostringstream oss;
            oss << "Called unwrap on None\n"
                << "  Location: " << loc.file_name() << ":" << loc.line() << ":" << loc.column() << "\n"
                << "  Function: " << loc.function_name();
            throw std::runtime_error(oss.str());
        }
        return value_.value();
    }
#else
    const T& unwrap() const {
        if (!has_value()) {
            throw std::runtime_error("Called unwrap on None");
        }
        return value_.value();
    }
#endif

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
 * @brief Create an error result (Rust style)
 * @note Safe to keep as "Err" doesn't conflict with error:: namespace
 */
template<typename T>
inline Result<T> Err(const std::string& message) {
    return Result<T>(error_info{message});
}

/**
 * @brief Create an error result with code (Rust style)
 */
template<typename T>
inline Result<T> Err(int code, const std::string& message,
                     const std::string& module = "") {
    return Result<T>(error_info{code, message, module});
}

/**
 * @brief Create an error result with all parameters
 * @note Renamed from error() to make_error() to avoid conflict with error:: namespace
 */
template<typename T>
inline Result<T> make_error(int code, const std::string& message,
                            const std::string& module = "") {
    return Result<T>(error_info{code, message, module});
}

/**
 * @brief Create an error result with details
 * @note Renamed from error() to make_error() to avoid conflict with error:: namespace
 */
template<typename T>
inline Result<T> make_error(int code, const std::string& message,
                            const std::string& module,
                            const std::string& details) {
    return Result<T>(error_info{code, message, module, details});
}

/**
 * @brief Create an error result from existing error_info
 * @note Renamed from error() to make_error() to avoid conflict with error:: namespace
 *
 * BREAKING CHANGE: The old error() function has been removed completely due to
 * C++ not allowing the same identifier for both a function and a namespace.
 * All code must be updated to use make_error() instead of error().
 */
template<typename T>
inline Result<T> make_error(const error_info& err) {
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

// Import common error codes from centralized error_codes.h
// Provide backward compatibility aliases for existing code
namespace error_codes {
    using namespace error::codes::common_errors;

    // Uppercase aliases for backward compatibility
    constexpr int SUCCESS = error::codes::common_errors::success;
    constexpr int INVALID_ARGUMENT = error::codes::common_errors::invalid_argument;
    constexpr int NOT_FOUND = error::codes::common_errors::not_found;
    constexpr int PERMISSION_DENIED = error::codes::common_errors::permission_denied;
    constexpr int TIMEOUT = error::codes::common_errors::timeout;
    constexpr int CANCELLED = error::codes::common_errors::cancelled;
    constexpr int NOT_INITIALIZED = error::codes::common_errors::not_initialized;
    constexpr int ALREADY_EXISTS = error::codes::common_errors::already_exists;
    constexpr int OUT_OF_MEMORY = error::codes::common_errors::out_of_memory;
    constexpr int IO_ERROR = error::codes::common_errors::io_error;
    constexpr int NETWORK_ERROR = error::codes::common_errors::network_error;
    constexpr int INTERNAL_ERROR = error::codes::common_errors::internal_error;

    // Module-specific base ranges
    constexpr int THREAD_ERROR_BASE = static_cast<int>(error::category::thread_system);
    constexpr int LOGGER_ERROR_BASE = static_cast<int>(error::category::logger_system);
    constexpr int MONITORING_ERROR_BASE = static_cast<int>(error::category::monitoring_system);
    constexpr int CONTAINER_ERROR_BASE = static_cast<int>(error::category::container_system);
    constexpr int DATABASE_ERROR_BASE = static_cast<int>(error::category::database_system);
    constexpr int NETWORK_ERROR_BASE = static_cast<int>(error::category::network_system);
}

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

// Backward compatibility - deprecated, will be removed in future versions
#ifndef COMMON_DISABLE_LEGACY_MACROS
#define RETURN_IF_ERROR(expr) COMMON_RETURN_IF_ERROR(expr)
#define ASSIGN_OR_RETURN(decl, expr) COMMON_ASSIGN_OR_RETURN(decl, expr)
#define RETURN_ERROR_IF(condition, code, message, module) COMMON_RETURN_ERROR_IF(condition, code, message, module)
#define RETURN_ERROR_IF_WITH_DETAILS(condition, code, message, module, details) COMMON_RETURN_ERROR_IF_WITH_DETAILS(condition, code, message, module, details)
#endif // COMMON_DISABLE_LEGACY_MACROS
