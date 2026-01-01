// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file fwd.h
 * @brief Forward declarations for Result pattern types.
 *
 * @deprecated This header is deprecated. Use result/core.h instead.
 * This header will be removed in a future version.
 *
 * This header provides forward declarations to minimize compilation
 * dependencies and enable type references without full definitions.
 */

#pragma once

#ifdef _MSC_VER
#pragma message("warning: fwd.h is deprecated. Use result/core.h instead.")
#else
#pragma message("fwd.h is deprecated. Use result/core.h instead.")
#endif

#include "core.h"

// Backward compatibility - all types are now defined in core.h
namespace kcenon::common {
// Types re-exported from core.h for backward compatibility
} // namespace kcenon::common

#if 0  // Original content preserved for reference

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

#endif  // Original content preserved for reference
