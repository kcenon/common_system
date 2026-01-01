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
 * The Result pattern has been consolidated into 3 logical headers:
 *
 * @code
 * // Core types: Result<T>, Optional<T>, error_info
 * #include <kcenon/common/patterns/result/core.h>
 *
 * // Utilities: factory functions, exception conversion, macros
 * #include <kcenon/common/patterns/result/utilities.h>
 *
 * // Compatibility: legacy error code aliases
 * #include <kcenon/common/patterns/result/compat.h>
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

// New consolidated headers (C++20 module-ready)
#include "result/core.h"
#include "result/utilities.h"
#include "result/compat.h"
