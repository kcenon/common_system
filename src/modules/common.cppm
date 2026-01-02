// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file common.cppm
 * @brief Primary C++20 module for common_system.
 *
 * This is the main module interface for the common_system library.
 * It aggregates all module partitions to provide a single import point.
 *
 * Usage:
 * @code
 * import kcenon.common;
 *
 * using namespace kcenon::common;
 *
 * auto result = ok(42);
 * if (result.is_ok()) {
 *     std::cout << result.value() << std::endl;
 * }
 * @endcode
 *
 * Module Structure:
 * - kcenon.common:utils - Utility classes (CircularBuffer, ObjectPool)
 * - kcenon.common:error - Error codes and categories
 * - kcenon.common:result - Result<T> pattern implementation
 * - kcenon.common:concepts - C++20 concepts for type validation
 * - kcenon.common:interfaces - Core interfaces (IExecutor, ILogger, etc.)
 * - kcenon.common:config - Configuration utilities
 * - kcenon.common:di - Dependency injection
 * - kcenon.common:patterns - Design patterns (EventBus)
 * - kcenon.common:logging - Logging utilities
 */

export module kcenon.common;

// Tier 1: Core (No Dependencies)
export import :utils;
export import :error;

// Tier 2: Foundation
export import :result;
export import :concepts;

// Tier 3: Infrastructure
export import :interfaces;
export import :config;
export import :di;

// Tier 4: Application
export import :patterns;
export import :logging;

export namespace kcenon::common {

/**
 * @brief Version information for common_system module.
 */
struct module_version {
    static constexpr int major = 0;
    static constexpr int minor = 2;
    static constexpr int patch = 0;
    static constexpr int tweak = 0;
    static constexpr const char* string = "0.2.0.0";
    static constexpr const char* module_name = "kcenon.common";
};

} // namespace kcenon::common
