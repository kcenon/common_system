// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file result_macros.h
 * @brief Convenience macros for Result<T> pattern usage.
 *
 * These macros provide shorthand for common Result patterns like
 * early return on error and conditional error generation.
 *
 * @note For type-safe alternatives to these macros, see result_helpers.h
 * in the patterns directory.
 */

#pragma once

#include "result_funcs.h"

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
