// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file optional.h
 * @brief Optional<T> type with Rust-like API.
 *
 * Provides an Optional<T> type similar to std::optional with additional
 * Rust-inspired methods like is_some(), is_none(), and unwrap().
 */

#pragma once

#include <optional>
#include <stdexcept>
#include <sstream>
#include <utility>

// Import C++17-compatible source_location
#include <kcenon/common/utils/source_location.h>

namespace kcenon::common {

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
