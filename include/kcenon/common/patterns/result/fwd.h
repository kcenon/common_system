// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file fwd.h
 * @brief Forward declarations for Result pattern types.
 *
 * This header provides forward declarations to minimize compilation
 * dependencies and enable type references without full definitions.
 */

#pragma once

#include <variant>

namespace kcenon::common {

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

} // namespace kcenon::common
