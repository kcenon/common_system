// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file result/utilities.cppm
 * @brief C++20 module partition for Result<T> utility functions.
 *
 * This module partition exports Result pattern utilities:
 * - Factory functions (ok, make_error)
 * - Exception to Result conversion utilities
 * - Error code compatibility aliases
 *
 * Part of the kcenon.common module.
 */

module;

#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>
#include <variant>

export module kcenon.common:result.utilities;

import :result.core;
import :error;

export namespace kcenon::common {

// ============================================================================
// Error Code Compatibility Aliases
// ============================================================================

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
    constexpr int REGISTRY_FROZEN = error::codes::common_errors::registry_frozen;
    constexpr int INTERNAL_ERROR = error::codes::common_errors::internal_error;

    // Module-specific base ranges
    constexpr int THREAD_ERROR_BASE = static_cast<int>(error::category::thread_system);
    constexpr int LOGGER_ERROR_BASE = static_cast<int>(error::category::logger_system);
    constexpr int MONITORING_ERROR_BASE = static_cast<int>(error::category::monitoring_system);
    constexpr int CONTAINER_ERROR_BASE = static_cast<int>(error::category::container_system);
    constexpr int DATABASE_ERROR_BASE = static_cast<int>(error::category::database_system);
    constexpr int NETWORK_ERROR_BASE = static_cast<int>(error::category::network_system);
} // namespace error_codes

// ============================================================================
// Factory Functions
// ============================================================================

/**
 * @brief Create a successful result with a value.
 * @param value The value to wrap
 * @return Result<T> containing the value
 *
 * @note For VoidResult (successful void result), use the ok() function
 *       exported from :result.core partition.
 */
template<typename T>
inline Result<T> ok(T value) {
    return Result<T>(std::move(value));
}

// Note: VoidResult ok() is defined in :result.core partition to avoid circular
// dependencies and ensure availability in modules that only import :result.core.

/**
 * @brief Create an error result with code and message.
 * @param code Error code
 * @param message Error message
 * @param module Optional module name
 * @return Result<T> containing the error
 */
template<typename T>
inline Result<T> make_error(int code, const std::string& message,
                            const std::string& module = "") {
    return Result<T>(error_info{code, message, module});
}

/**
 * @brief Create an error result with details.
 */
template<typename T>
inline Result<T> make_error(int code, const std::string& message,
                            const std::string& module,
                            const std::string& details) {
    return Result<T>(error_info{code, message, module, details});
}

/**
 * @brief Create an error result from existing error_info.
 */
template<typename T>
inline Result<T> make_error(const error_info& err) {
    return Result<T>(err);
}

// ============================================================================
// Exception Conversion
// ============================================================================

/**
 * @class exception_mapper
 * @brief Maps standard exception types to appropriate error codes.
 */
class exception_mapper {
public:
    static error_info map_unknown_exception(const std::string& module = "") {
        return error_info{error_codes::INTERNAL_ERROR, "Unknown exception caught", module,
                         "Non-standard exception (not derived from std::exception)"};
    }

    static error_info map_generic_exception(const std::exception& e, const std::string& module = "") {
        return error_info{error_codes::INTERNAL_ERROR, e.what(), module, "std::exception"};
    }
};

/**
 * @brief Convert exception to Result with automatic error code mapping.
 *
 * @tparam T Return type
 * @tparam F Callable type
 * @param func Callable to execute
 * @param module Module name for error context
 * @return Result<T> with value or mapped error
 */
template<typename T, typename F>
Result<T> try_catch(F&& func, const std::string& module = "") {
    try {
        return ok<T>(func());
    }
    catch (const std::bad_alloc& e) {
        return Result<T>(error_info{error_codes::OUT_OF_MEMORY, e.what(), module, "std::bad_alloc"});
    }
    catch (const std::invalid_argument& e) {
        return Result<T>(error_info{error_codes::INVALID_ARGUMENT, e.what(), module, "std::invalid_argument"});
    }
    catch (const std::out_of_range& e) {
        return Result<T>(error_info{error_codes::INVALID_ARGUMENT, e.what(), module, "std::out_of_range"});
    }
    catch (const std::system_error& e) {
        return Result<T>(error_info{e.code().value(), e.what(), module,
                        std::string("std::system_error: ") + e.code().category().name()});
    }
    catch (const std::logic_error& e) {
        return Result<T>(error_info{error_codes::INTERNAL_ERROR, e.what(), module, "std::logic_error"});
    }
    catch (const std::runtime_error& e) {
        return Result<T>(error_info{error_codes::INTERNAL_ERROR, e.what(), module, "std::runtime_error"});
    }
    catch (const std::exception& e) {
        return Result<T>(exception_mapper::map_generic_exception(e, module));
    }
    catch (...) {
        return Result<T>(exception_mapper::map_unknown_exception(module));
    }
}

/**
 * @brief Convert exception to VoidResult with automatic error code mapping.
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
    catch (const std::bad_alloc& e) {
        return VoidResult(error_info{error_codes::OUT_OF_MEMORY, e.what(), module, "std::bad_alloc"});
    }
    catch (const std::invalid_argument& e) {
        return VoidResult(error_info{error_codes::INVALID_ARGUMENT, e.what(), module, "std::invalid_argument"});
    }
    catch (const std::out_of_range& e) {
        return VoidResult(error_info{error_codes::INVALID_ARGUMENT, e.what(), module, "std::out_of_range"});
    }
    catch (const std::system_error& e) {
        return VoidResult(error_info{e.code().value(), e.what(), module,
                        std::string("std::system_error: ") + e.code().category().name()});
    }
    catch (const std::logic_error& e) {
        return VoidResult(error_info{error_codes::INTERNAL_ERROR, e.what(), module, "std::logic_error"});
    }
    catch (const std::runtime_error& e) {
        return VoidResult(error_info{error_codes::INTERNAL_ERROR, e.what(), module, "std::runtime_error"});
    }
    catch (const std::exception& e) {
        return VoidResult(exception_mapper::map_generic_exception(e, module));
    }
    catch (...) {
        return VoidResult(exception_mapper::map_unknown_exception(module));
    }
}

// ============================================================================
// Deprecated Free Functions (for backward compatibility)
// ============================================================================

template<typename T>
[[deprecated("Use result.is_ok() instead. Will be removed in v2.0.0.")]]
inline bool is_ok(const Result<T>& result) {
    return result.is_ok();
}

template<typename T>
[[deprecated("Use result.is_err() instead. Will be removed in v2.0.0.")]]
inline bool is_error(const Result<T>& result) {
    return result.is_err();
}

template<typename T>
[[deprecated("Use result.value() instead. Will be removed in v2.0.0.")]]
inline const T& get_value(const Result<T>& result) {
    return result.value();
}

template<typename T>
[[deprecated("Use result.value() instead. Will be removed in v2.0.0.")]]
inline T& get_value(Result<T>& result) {
    return result.value();
}

template<typename T>
[[deprecated("Use result.error() instead. Will be removed in v2.0.0.")]]
inline const error_info& get_error(const Result<T>& result) {
    return result.error();
}

template<typename T>
[[deprecated("Use result.value_or(default) instead. Will be removed in v2.0.0.")]]
inline T value_or(const Result<T>& result, T default_value) {
    return result.unwrap_or(default_value);
}

template<typename T, typename F>
[[deprecated("Use result.map(func) instead. Will be removed in v2.0.0.")]]
auto map(const Result<T>& result, F&& func)
    -> Result<decltype(func(std::declval<T>()))> {
    return result.map(std::forward<F>(func));
}

template<typename T, typename F>
[[deprecated("Use result.and_then(func) instead. Will be removed in v2.0.0.")]]
auto and_then(const Result<T>& result, F&& func)
    -> decltype(func(std::declval<T>())) {
    return result.and_then(std::forward<F>(func));
}

template<typename T, typename F>
[[deprecated("Use result.or_else(func) instead. Will be removed in v2.0.0.")]]
Result<T> or_else(const Result<T>& result, F&& func) {
    return result.or_else(std::forward<F>(func));
}

} // namespace kcenon::common
