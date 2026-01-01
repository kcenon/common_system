// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file core.h
 * @brief Consolidated core types for Result<T> pattern.
 *
 * This header consolidates the core Result pattern types:
 * - Forward declarations and common types
 * - error_info struct for error representation
 * - Result<T> class for exception-free error handling
 * - Optional<T> class with Rust-like API
 *
 * This consolidation reduces header count from 4 to 1, preparing for
 * C++20 module migration while following Kent Beck's "Fewest Elements" principle.
 *
 * Thread Safety:
 * - Result<T> objects are NOT thread-safe for concurrent modification.
 * - Multiple threads may safely read the same Result<T> if no thread modifies it.
 * - If sharing Result<T> across threads, users must provide synchronization.
 * - Best practice: Use Result<T> as return values; avoid shared mutable access.
 */

#pragma once

#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>

// Import C++17-compatible source_location
#include <kcenon/common/utils/source_location.h>

namespace kcenon::common {

// ============================================================================
// Forward Declarations (from fwd.h)
// ============================================================================

// Forward declarations
struct error_info;
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
 * @brief Specialized Result for void operations
 *
 * Forward declaration of VoidResult typedef.
 * Full definition in void_result.h.
 */
using VoidResult = Result<std::monostate>;

// ============================================================================
// Error Info (from error_info.h)
// ============================================================================

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

    /**
     * @brief Construct from strongly-typed enum error codes.
     *
     * Enables database_system/network_system enums to be passed directly
     * without manual static_cast noise.
     */
    template<typename Enum,
             typename std::enable_if_t<std::is_enum_v<Enum>, int> = 0>
    error_info(Enum c, std::string msg, std::string mod = "",
               std::optional<std::string> det = std::nullopt)
        : code(static_cast<int>(c)),
          message(std::move(msg)),
          module(std::move(mod)),
          details(std::move(det)) {}

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

// ============================================================================
// Result<T> Class (from result_core.h)
// ============================================================================

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
 * IMPORTANT: Default-constructed Results are deleted to enforce explicit
 * initialization. For explicit construction, use factory methods
 * Result<T>::ok() and Result<T>::err().
 *
 * Thread Safety Note:
 * - Based on std::optional, which is NOT thread-safe for concurrent access.
 * - Safe to pass by value or const reference across threads.
 * - Concurrent reads of the same Result are safe if guaranteed no writes.
 * - For shared mutable access, wrap in std::mutex or similar.
 */
template<typename T>
class Result {
public:
    /// @brief Type alias for the contained value type (for concept compatibility)
    using value_type = T;

    /// @brief Type alias for the error type
    using error_type = error_info;

private:
    // Use std::optional<T> for value, which naturally supports uninitialized state
    // Store error separately to avoid variant issues
    std::optional<T> value_;
    std::optional<error_info> error_;

public:
    // Constructors - public but encourage use of factory methods
    Result(const T& value) : value_(value), error_(std::nullopt) {}
    Result(T&& value) : value_(std::move(value)), error_(std::nullopt) {}
    Result(const error_info& error) : value_(std::nullopt), error_(error) {}
    Result(error_info&& error) : value_(std::nullopt), error_(std::move(error)) {}
    /**
     * @brief Default constructor is deleted to enforce explicit initialization
     *
     * This forces developers to explicitly use factory methods.
     *
     * Migration: Use Result<T>::ok() for successful results or
     * Result<T>::err() for error results. For cases requiring a
     * default error state, use Result<T>::uninitialized() factory method.
     */
    Result() = delete;

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
     * @brief Create an explicitly uninitialized result (use with caution)
     * @return Result<T> containing an error state indicating uninitialized
     *
     * This factory method is provided for cases where an uninitialized state
     * is explicitly required (e.g., delayed initialization, placeholder values).
     * Use sparingly and prefer explicit initialization with ok() or err().
     */
    static Result<T> uninitialized() {
        return Result<T>(error_info{-6, "Result not initialized", "common::Result"});
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
     * @brief Get value from result (throws if error)
     * @throws std::runtime_error if result contains error
     * @note When source_location is available, error messages include file/line info
     */
#if KCENON_HAS_SOURCE_LOCATION
    const T& unwrap(
        source_location loc = source_location::current()
    ) const {
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
        if (is_err()) {
            const auto& err = error_.value();
            throw std::runtime_error("Called unwrap on error: " + err.message);
        }
        return value_.value();
    }
#endif

    /**
     * @brief Get mutable value from result (throws if error)
     * @throws std::runtime_error if result contains error
     * @note When source_location is available, error messages include file/line info
     */
#if KCENON_HAS_SOURCE_LOCATION
    T& unwrap(
        source_location loc = source_location::current()
    ) {
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
            // Uninitialized state - use uninitialized factory
            return Result<ReturnType>::uninitialized();
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
            // Uninitialized state - use uninitialized factory
            return ReturnType::uninitialized();
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

// ============================================================================
// Optional<T> Class (from optional.h)
// ============================================================================

/**
 * @class Optional
 * @brief Optional type similar to std::optional with Rust-like API
 */
template<typename T>
class Optional {
public:
    /// @brief Type alias for the contained value type (for concept compatibility)
    using value_type = T;

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
     * @throws std::runtime_error if optional is None with detailed location info
     * @note When source_location is available, error messages include file/line info
     */
#if KCENON_HAS_SOURCE_LOCATION
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

} // namespace kcenon::common
