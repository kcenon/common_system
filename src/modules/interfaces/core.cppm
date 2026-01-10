// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file interfaces/core.cppm
 * @brief C++20 module partition aggregator for core interfaces.
 *
 * This module partition aggregates all interface partitions for backward
 * compatibility:
 * - :interfaces.logger - Logger interfaces (ILogger, log_level, log_entry)
 * - :interfaces.executor - Executor interfaces (IJob, IExecutor, IThreadPool)
 *
 * This allows existing code using `import :interfaces.core;` to continue
 * working while enabling more granular imports for modules that only need
 * specific interfaces.
 *
 * Part of the kcenon.common module.
 */

module;

#include <variant>

export module kcenon.common:interfaces.core;

// Import and re-export all interface partitions
export import :interfaces.logger;
export import :interfaces.executor;

// Import result.core for VoidResult factory function
import :result.core;

export namespace kcenon::common {

// Re-export types from result.core for use in interfaces namespace
// (Types are already exported from result.core, just need VoidResult factory here)

/**
 * @brief Factory function to create successful VoidResult.
 */
inline VoidResult ok() { return VoidResult(std::monostate{}); }

} // namespace kcenon::common
