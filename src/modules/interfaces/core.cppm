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

export module kcenon.common:interfaces.core;

// Import and re-export all interface partitions
export import :interfaces.logger;
export import :interfaces.executor;

// Re-export ok() from result.core for backward compatibility
export import :result.core;
