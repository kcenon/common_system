// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file interfaces.cppm
 * @brief C++20 module partition aggregator for interfaces.
 *
 * This module partition aggregates all interface partitions:
 * - :interfaces.core - Core interfaces (IJob, IExecutor, IThreadPool, ILogger)
 *
 * Part of the kcenon.common module.
 */

export module kcenon.common:interfaces;

export import :interfaces.core;
