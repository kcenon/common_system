// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file result.cppm
 * @brief C++20 module partition aggregator for Result<T> pattern.
 *
 * This module partition aggregates all Result-related partitions:
 * - :result.core - Result<T>, Optional<T>, error_info
 * - :result.utilities - Factory functions, exception conversion
 *
 * Usage:
 * @code
 * import kcenon.common:result;
 *
 * auto result = kcenon::common::ok(42);
 * if (result.is_ok()) {
 *     std::cout << result.value() << std::endl;
 * }
 * @endcode
 *
 * Part of the kcenon.common module.
 */

export module kcenon.common:result;

export import :result.core;
export import :result.utilities;
