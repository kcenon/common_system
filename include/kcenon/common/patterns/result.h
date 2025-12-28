// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file result.h
 * @brief Umbrella header for Result<T> type and related utilities.
 *
 * This is the main include file for the Result pattern. Including this
 * header provides access to all Result-related types and functions.
 *
 * For selective includes to reduce compilation times, you can include
 * individual headers from the result/ subdirectory:
 *
 * @code
 * // Minimal include for just the Result class
 * #include <kcenon/common/patterns/result/result_core.h>
 *
 * // Include factory functions
 * #include <kcenon/common/patterns/result/result_funcs.h>
 *
 * // Include exception conversion utilities
 * #include <kcenon/common/patterns/result/exception_conversion.h>
 * @endcode
 *
 * Thread Safety:
 * - Result<T> objects are NOT thread-safe for concurrent modification.
 * - Multiple threads may safely read the same Result<T> if no thread modifies it.
 * - Helper functions are thread-safe as they don't modify global state.
 * - If sharing Result<T> across threads, users must provide synchronization.
 * - Best practice: Use Result<T> as return values; avoid shared mutable access.
 */

#pragma once

// Forward declarations and common types
#include "result/fwd.h"

// Core types
#include "result/error_info.h"
#include "result/result_core.h"
#include "result/optional.h"

// Factory and helper functions
#include "result/result_funcs.h"

// Error codes compatibility layer
#include "result/error_codes_compat.h"

// Exception conversion utilities
#include "result/exception_conversion.h"

// Convenience macros
#include "result/result_macros.h"
