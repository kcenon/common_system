// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file exception_conversion.h
 * @brief Exception to Result conversion utilities.
 *
 * Provides utilities for converting exceptions to Result<T> objects,
 * enabling seamless integration between exception-based and Result-based
 * error handling.
 */

#pragma once

#include "result_funcs.h"
#include "error_codes_compat.h"

#include <stdexcept>
#include <system_error>

namespace kcenon::common {

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
