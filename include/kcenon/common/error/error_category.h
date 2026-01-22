// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file error_category.h
 * @brief Decentralized error category system for improved system isolation.
 *
 * This file provides the infrastructure for decentralized error handling:
 * - error_category: Abstract base class for system-specific error categories
 * - common_error_category: Implementation for common/shared error codes
 * - typed_error_code: Type-safe error code that carries its category
 *
 * Design Philosophy:
 * This design follows std::error_category pattern from <system_error> but
 * provides additional features specific to the kcenon ecosystem:
 * - Integration with Result<T> pattern
 * - Module-based categorization for better debugging
 * - Support for detailed error messages
 *
 * Migration Path:
 * - Phase 1 (this file): Infrastructure classes
 * - Phase 2: Migrate common errors to common_error_category
 * - Phase 3: Create system-specific categories in dependent systems
 * - Phase 4: Deprecate centralized error_codes.h
 *
 * Thread Safety:
 * - error_category implementations should be stateless singletons
 * - error_code objects are safe for concurrent reads
 * - For shared mutable access, users must provide synchronization
 *
 * @see https://github.com/kcenon/common_system/issues/300
 */

#pragma once

#include <functional>
#include <string>
#include <string_view>

namespace kcenon::common {

/**
 * @class error_category
 * @brief Abstract base class for error code categories.
 *
 * Each system can define its own error category by inheriting from this class.
 * Categories provide:
 * - A unique name for identification
 * - Human-readable messages for error codes
 * - Equivalence comparison between different categories
 *
 * Example implementation:
 * @code
 * class network_error_category : public error_category {
 * public:
 *     static const network_error_category& instance() {
 *         static network_error_category instance;
 *         return instance;
 *     }
 *
 *     std::string_view name() const noexcept override { return "network"; }
 *
 *     std::string message(int code) const override {
 *         switch (code) {
 *             case 1: return "Connection failed";
 *             case 2: return "Timeout";
 *             default: return "Unknown network error";
 *         }
 *     }
 * };
 * @endcode
 */
class error_category {
public:
    /**
     * @brief Virtual destructor for proper cleanup of derived classes.
     */
    virtual ~error_category() = default;

    /**
     * @brief Returns the unique name of this error category.
     *
     * The name should be a short, descriptive identifier for the category.
     * Examples: "common", "network", "database", "logger"
     *
     * @return Category name as a string view (must be valid for lifetime of category)
     */
    virtual std::string_view name() const noexcept = 0;

    /**
     * @brief Returns a human-readable message for the given error code.
     *
     * @param code The error code value
     * @return Human-readable error message
     */
    virtual std::string message(int code) const = 0;

    /**
     * @brief Checks if an error code in this category is equivalent to another.
     *
     * The default implementation checks for exact category and code match.
     * Derived classes can override this to provide semantic equivalence
     * (e.g., "timeout" errors from different systems may be equivalent).
     *
     * @param code Error code in this category
     * @param other_category Category of the other error code
     * @param other_code The other error code value
     * @return true if the error codes are semantically equivalent
     */
    virtual bool equivalent(int code,
                          const error_category& other_category,
                          int other_code) const noexcept {
        return (this == &other_category) && (code == other_code);
    }

    /**
     * @brief Equality comparison between categories.
     *
     * Categories are compared by identity (address), not by name.
     * This ensures that two different categories with the same name
     * are not considered equal.
     *
     * @param other Category to compare with
     * @return true if same category instance
     */
    bool operator==(const error_category& other) const noexcept {
        return this == &other;
    }

    /**
     * @brief Inequality comparison between categories.
     */
    bool operator!=(const error_category& other) const noexcept {
        return !(*this == other);
    }

    /**
     * @brief Less-than comparison for use in ordered containers.
     */
    bool operator<(const error_category& other) const noexcept {
        return std::less<const error_category*>()(this, &other);
    }

protected:
    /**
     * @brief Protected default constructor.
     *
     * Categories should be accessed through their singleton instance(),
     * not constructed directly.
     */
    error_category() = default;

    // Non-copyable, non-movable (singleton pattern)
    error_category(const error_category&) = delete;
    error_category& operator=(const error_category&) = delete;
    error_category(error_category&&) = delete;
    error_category& operator=(error_category&&) = delete;
};

// Forward declaration
class typed_error_code;

/**
 * @class common_error_category
 * @brief Error category for common/shared error codes.
 *
 * This category contains error codes that are truly common across all systems:
 * - Success/failure indicators
 * - Generic errors (invalid_argument, not_found, etc.)
 * - Resource errors (out_of_memory, timeout)
 *
 * System-specific errors should NOT be added here. Instead, each system
 * should define its own error category.
 */
class common_error_category : public error_category {
public:
    /**
     * @brief Common error codes.
     *
     * These are error codes that apply universally across all systems.
     * System-specific error codes should be defined in their respective
     * error categories.
     */
    enum codes : int {
        success = 0,
        unknown_error = -1,
        invalid_argument = -2,
        not_found = -3,
        permission_denied = -4,
        timeout = -5,
        cancelled = -6,
        not_initialized = -7,
        already_exists = -8,
        out_of_memory = -9,
        io_error = -10,
        operation_not_supported = -11,
        internal_error = -99
    };

    /**
     * @brief Returns the singleton instance of common_error_category.
     *
     * Thread-safe due to C++11 static local variable initialization.
     *
     * @return Reference to the singleton instance
     */
    static const common_error_category& instance() noexcept {
        static common_error_category inst;
        return inst;
    }

    /**
     * @brief Returns the category name.
     * @return "common"
     */
    std::string_view name() const noexcept override {
        return "common";
    }

    /**
     * @brief Returns a human-readable message for the error code.
     * @param code Error code value
     * @return Error message
     */
    std::string message(int code) const override {
        switch (code) {
            case success: return "Success";
            case unknown_error: return "Unknown error";
            case invalid_argument: return "Invalid argument";
            case not_found: return "Not found";
            case permission_denied: return "Permission denied";
            case timeout: return "Operation timed out";
            case cancelled: return "Operation was cancelled";
            case not_initialized: return "Not initialized";
            case already_exists: return "Already exists";
            case out_of_memory: return "Out of memory";
            case io_error: return "I/O error";
            case operation_not_supported: return "Operation not supported";
            case internal_error: return "Internal error";
            default: return "Unknown common error (code: " + std::to_string(code) + ")";
        }
    }

private:
    common_error_category() = default;
};

/**
 * @class typed_error_code
 * @brief A type-safe error code that carries its category.
 *
 * typed_error_code encapsulates both an error code value and a reference to its
 * category. This allows:
 * - Clear identification of error origin (via category name)
 * - Human-readable error messages
 * - Comparison between error codes from different systems
 *
 * @note This class is named typed_error_code to avoid conflicts with the
 *       existing error_code alias (error_code = error_info) in result/core.h.
 *       A future migration will deprecate the alias and rename this class
 *       to error_code.
 *
 * Example usage:
 * @code
 * // Create a typed error code
 * typed_error_code ec(common_error_category::timeout, common_error_category::instance());
 *
 * // Check for error
 * if (ec) {
 *     std::cout << "Error: " << ec.message() << std::endl;
 *     std::cout << "Category: " << ec.category().name() << std::endl;
 * }
 *
 * // Create using make_typed_error_code helper
 * auto ec2 = make_typed_error_code(common_error_category::not_found);
 * @endcode
 */
class typed_error_code {
public:
    /**
     * @brief Default constructor creates a success error code.
     */
    typed_error_code() noexcept
        : code_(0)
        , category_(&common_error_category::instance()) {}

    /**
     * @brief Constructs an error code with the given value and category.
     *
     * @param code Error code value
     * @param category Reference to the error category
     */
    typed_error_code(int code, const error_category& category) noexcept
        : code_(code)
        , category_(&category) {}

    /**
     * @brief Constructs from common_error_category::codes enum.
     *
     * Convenience constructor for common error codes.
     *
     * @param code Common error code enum value
     */
    typed_error_code(common_error_category::codes code) noexcept
        : code_(static_cast<int>(code))
        , category_(&common_error_category::instance()) {}

    /**
     * @brief Returns the error code value.
     */
    int value() const noexcept { return code_; }

    /**
     * @brief Returns the error category.
     */
    const error_category& category() const noexcept { return *category_; }

    /**
     * @brief Returns a human-readable error message.
     *
     * Delegates to the category's message() method.
     */
    std::string message() const { return category_->message(code_); }

    /**
     * @brief Returns the category name.
     *
     * Convenience method equivalent to category().name().
     */
    std::string_view category_name() const noexcept { return category_->name(); }

    /**
     * @brief Explicit bool conversion - true if error, false if success.
     *
     * @return true if error code represents an error (non-zero value)
     */
    explicit operator bool() const noexcept { return code_ != 0; }

    /**
     * @brief Clears the error code to success state.
     */
    void clear() noexcept {
        code_ = 0;
        category_ = &common_error_category::instance();
    }

    /**
     * @brief Assigns a new error code value and category.
     */
    void assign(int code, const error_category& category) noexcept {
        code_ = code;
        category_ = &category;
    }

    /**
     * @brief Equality comparison.
     *
     * Two error codes are equal if they have the same category and value.
     */
    bool operator==(const typed_error_code& other) const noexcept {
        return code_ == other.code_ && *category_ == *other.category_;
    }

    /**
     * @brief Inequality comparison.
     */
    bool operator!=(const typed_error_code& other) const noexcept {
        return !(*this == other);
    }

    /**
     * @brief Less-than comparison for use in ordered containers.
     */
    bool operator<(const typed_error_code& other) const noexcept {
        if (*category_ < *other.category_) return true;
        if (*other.category_ < *category_) return false;
        return code_ < other.code_;
    }

private:
    int code_;
    const error_category* category_;
};

// ============================================================================
// Helper Functions
// ============================================================================

/**
 * @brief Creates a typed_error_code from common_error_category::codes enum.
 *
 * @param code Common error code enum value
 * @return typed_error_code with common_error_category
 */
inline typed_error_code make_typed_error_code(common_error_category::codes code) noexcept {
    return typed_error_code(static_cast<int>(code), common_error_category::instance());
}

/**
 * @brief Creates a typed_error_code from a code value and category.
 *
 * @tparam Category The error category type (must have instance() method)
 * @param code Error code value
 * @return typed_error_code with the specified category
 */
template<typename Category>
typed_error_code make_typed_error_code(int code) noexcept {
    return typed_error_code(code, Category::instance());
}

/**
 * @brief Checks if a typed_error_code represents success (no error).
 *
 * @param ec The error code to check
 * @return true if the error code represents success
 */
inline bool is_success(const typed_error_code& ec) noexcept {
    return ec.value() == 0;
}

/**
 * @brief Checks if a typed_error_code represents an error.
 *
 * @param ec The error code to check
 * @return true if the error code represents an error
 */
inline bool is_error(const typed_error_code& ec) noexcept {
    return ec.value() != 0;
}

} // namespace kcenon::common
