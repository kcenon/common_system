// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file result_core.h
 * @brief Core Result<T> class definition.
 *
 * @deprecated This header is deprecated. Use result/core.h instead.
 * This header will be removed in a future version.
 *
 * Provides the Result<T> type for exception-free error handling.
 * This is the main class that holds either a value or an error.
 *
 * Thread Safety:
 * - Result<T> objects are NOT thread-safe for concurrent modification.
 * - Multiple threads may safely read the same Result<T> if no thread modifies it.
 * - If sharing Result<T> across threads, users must provide synchronization.
 * - Best practice: Use Result<T> as return values; avoid shared mutable access.
 */

#pragma once

#ifdef _MSC_VER
#pragma message("warning: result_core.h is deprecated. Use result/core.h instead.")
#else
#pragma message("result_core.h is deprecated. Use result/core.h instead.")
#endif

#include "core.h"

// Backward compatibility - Result<T> is now defined in core.h

#if 0  // Original content preserved for reference

#include "error_info.h"

#include <optional>
#include <stdexcept>
#include <sstream>
#include <utility>
#include <variant>

// Import C++17-compatible source_location
#include <kcenon/common/utils/source_location.h>

namespace kcenon::common {

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

} // namespace kcenon::common

#endif  // Original content preserved for reference
