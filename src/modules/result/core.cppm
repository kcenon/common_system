// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file result/core.cppm
 * @brief C++20 module partition for Result<T> core types.
 *
 * This module partition exports core Result pattern types:
 * - error_info struct for error representation
 * - Result<T> class for exception-free error handling
 * - Optional<T> class with Rust-like API
 *
 * Part of the kcenon.common module.
 */

module;

#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>

// Feature detection for source_location
#if __has_include(<source_location>)
    #include <source_location>
    #define KCENON_MODULE_HAS_SOURCE_LOCATION 1
#else
    #define KCENON_MODULE_HAS_SOURCE_LOCATION 0
#endif

export module kcenon.common:result.core;

export namespace kcenon::common {

// ============================================================================
// Source Location (local definition for this partition)
// ============================================================================

#if KCENON_MODULE_HAS_SOURCE_LOCATION
using source_location = std::source_location;
#else
struct source_location {
public:
    constexpr source_location(
        const char* file = __builtin_FILE(),
        const char* function = __builtin_FUNCTION(),
        int line = __builtin_LINE()
    ) noexcept
        : file_(file), function_(function), line_(line), column_(0) {}

    constexpr const char* file_name() const noexcept { return file_; }
    constexpr const char* function_name() const noexcept { return function_; }
    constexpr int line() const noexcept { return line_; }
    constexpr int column() const noexcept { return column_; }

    static constexpr source_location current(
        const char* file = __builtin_FILE(),
        const char* function = __builtin_FUNCTION(),
        int line = __builtin_LINE()
    ) noexcept {
        return source_location(file, function, line);
    }

private:
    const char* file_;
    const char* function_;
    int line_;
    int column_;
};
#endif

// ============================================================================
// Forward Declarations
// ============================================================================

struct error_info;
template<typename T> class Result;
template<typename T> class Optional;

/**
 * @brief Result state enum for tracking initialization.
 */
enum class result_state {
    uninitialized,  ///< Result has not been initialized with a value or error
    ok,             ///< Result contains a valid value
    error           ///< Result contains an error
};

/**
 * @brief Specialized Result for void operations.
 */
using VoidResult = Result<std::monostate>;

// ============================================================================
// Error Info
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
 * @brief Alias for backward compatibility.
 */
using error_code = error_info;

// ============================================================================
// Result<T> Class
// ============================================================================

/**
 * @class Result
 * @brief Result type for error handling with member function support.
 *
 * A Result<T> can be in one of two states:
 * 1. Ok - Contains a valid value of type T
 * 2. Error - Contains an error_info describing the failure
 *
 * @tparam T The type of the successful value
 */
template<typename T>
class Result {
public:
    using value_type = T;
    using error_type = error_info;

private:
    std::optional<T> value_;
    std::optional<error_info> error_;

public:
    Result(const T& value) : value_(value), error_(std::nullopt) {}
    Result(T&& value) : value_(std::move(value)), error_(std::nullopt) {}
    Result(const error_info& error) : value_(std::nullopt), error_(error) {}
    Result(error_info&& error) : value_(std::nullopt), error_(std::move(error)) {}

    Result() = delete;

    Result(const Result&) = default;
    Result(Result&&) = default;
    Result& operator=(const Result&) = default;
    Result& operator=(Result&&) = default;

    // Static factory methods
    template<typename U = T>
    static Result<T> ok(U&& value) {
        return Result<T>(std::forward<U>(value));
    }

    static Result<T> err(const error_info& error) {
        return Result<T>(error);
    }

    static Result<T> err(error_info&& error) {
        return Result<T>(std::move(error));
    }

    static Result<T> err(int code, const std::string& message, const std::string& module = "") {
        return Result<T>(error_info{code, message, module});
    }

    static Result<T> uninitialized() {
        return Result<T>(error_info{-6, "Result not initialized", "common::Result"});
    }

    bool is_ok() const {
        return value_.has_value();
    }

    bool is_err() const {
        return error_.has_value();
    }

#if KCENON_MODULE_HAS_SOURCE_LOCATION
    const T& unwrap(source_location loc = source_location::current()) const {
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

    T& unwrap(source_location loc = source_location::current()) {
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

    T& unwrap() {
        if (is_err()) {
            const auto& err = error_.value();
            throw std::runtime_error("Called unwrap on error: " + err.message);
        }
        return value_.value();
    }
#endif

    T unwrap_or(T default_value) const {
        if (is_ok()) {
            return value_.value();
        }
        return default_value;
    }

    T value_or(T default_value) const {
        return unwrap_or(std::move(default_value));
    }

    const T& value() const {
        return value_.value();
    }

    T& value() {
        return value_.value();
    }

    const error_info& error() const {
        return error_.value();
    }

    template<typename F>
    auto map(F&& func) const -> Result<decltype(func(std::declval<T>()))> {
        using ReturnType = decltype(func(std::declval<T>()));

        if (is_ok()) {
            return Result<ReturnType>(func(value_.value()));
        } else if (is_err()) {
            return Result<ReturnType>(error_.value());
        } else {
            return Result<ReturnType>::uninitialized();
        }
    }

    template<typename F>
    auto and_then(F&& func) const -> decltype(func(std::declval<T>())) {
        using ReturnType = decltype(func(std::declval<T>()));

        if (is_ok()) {
            return func(value_.value());
        } else if (is_err()) {
            return ReturnType(error_.value());
        } else {
            return ReturnType::uninitialized();
        }
    }

    template<typename F>
    Result<T> or_else(F&& func) const {
        if (is_ok()) {
            return *this;
        } else if (is_err()) {
            return func(error_.value());
        } else {
            return *this;
        }
    }
};

// ============================================================================
// Optional<T> Class
// ============================================================================

/**
 * @class Optional
 * @brief Optional type similar to std::optional with Rust-like API.
 *
 * @tparam T The type of the contained value
 */
template<typename T>
class Optional {
public:
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

#if KCENON_MODULE_HAS_SOURCE_LOCATION
    const T& unwrap(source_location loc = source_location::current()) const {
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
 * @brief Create an Optional with value.
 */
template<typename T>
inline Optional<T> Some(T value) {
    return Optional<T>(std::move(value));
}

/**
 * @brief Create an empty Optional.
 */
template<typename T>
inline Optional<T> None() {
    return Optional<T>(std::nullopt);
}

// ============================================================================
// VoidResult Factory Functions
// ============================================================================

/**
 * @brief Factory function to create successful VoidResult.
 * @return VoidResult containing std::monostate (success indicator)
 */
inline VoidResult ok() {
    return VoidResult(std::monostate{});
}

/**
 * @brief Factory function to create error VoidResult.
 * @param error The error information
 * @return VoidResult containing the error
 */
inline VoidResult err(const error_info& error) {
    return VoidResult(error);
}

/**
 * @brief Factory function to create error VoidResult.
 * @param code Error code
 * @param message Error message
 * @param module Module name (optional)
 * @return VoidResult containing the error
 */
inline VoidResult err(int code, const std::string& message, const std::string& module = "") {
    return VoidResult(error_info{code, message, module});
}

} // namespace kcenon::common
