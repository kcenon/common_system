// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file thread_pool_interface.h
 * @brief Thread pool interface extending executor functionality.
 *
 * This header defines the thread pool interface that extends IExecutor
 * with thread pool-specific operations like resizing, queue management,
 * and lifecycle control.
 */

#pragma once

#include "executor_interface.h"
#include "../patterns/result.h"
#include <cstddef>

namespace kcenon::common {
namespace interfaces {

/**
 * @interface IThreadPool
 * @brief Extended interface for thread pool implementations
 *
 * This interface extends IExecutor with thread pool-specific functionality
 * such as dynamic resizing, queue capacity management, and fine-grained
 * lifecycle control.
 */
class IThreadPool : public IExecutor {
public:
    virtual ~IThreadPool() = default;

    // ===== Thread pool sizing =====

    /**
     * @brief Resize the thread pool
     * @param new_size New number of worker threads
     * @return VoidResult indicating success or failure
     *
     * Resizing may fail if:
     * - new_size is 0
     * - System resources are insufficient
     * - Pool is shutting down
     */
    virtual VoidResult resize(size_t new_size) = 0;

    /**
     * @brief Get the minimum number of worker threads
     * @return Minimum worker count
     */
    virtual size_t min_workers() const { return 1; }

    /**
     * @brief Get the maximum number of worker threads
     * @return Maximum worker count (0 = unlimited)
     */
    virtual size_t max_workers() const { return 0; }

    // ===== Queue management =====

    /**
     * @brief Set the maximum queue capacity
     * @param capacity Maximum number of pending tasks (0 = unlimited)
     * @return VoidResult indicating success or failure
     */
    virtual VoidResult set_queue_capacity(size_t capacity) = 0;

    /**
     * @brief Get the current queue capacity
     * @return Maximum queue capacity (0 = unlimited)
     */
    virtual size_t get_queue_capacity() const = 0;

    /**
     * @brief Check if the queue is full
     * @return true if queue is at capacity, false otherwise
     */
    virtual bool is_queue_full() const = 0;

    /**
     * @brief Clear all pending tasks
     * @return Number of tasks removed
     *
     * This operation removes all pending tasks from the queue.
     * Currently executing tasks are not affected.
     */
    virtual size_t clear_pending_tasks() = 0;

    // ===== Lifecycle control =====

    /**
     * @brief Start the thread pool
     * @return VoidResult indicating success or failure
     *
     * Starting an already running pool is a no-op.
     */
    virtual VoidResult start() = 0;

    /**
     * @brief Stop the thread pool
     * @param wait_for_completion Wait for all pending tasks
     * @return VoidResult indicating success or failure
     *
     * This is an alias for shutdown() for consistency.
     */
    virtual VoidResult stop(bool wait_for_completion = true) = 0;

    /**
     * @brief Pause task processing
     * @return VoidResult indicating success or failure
     *
     * When paused:
     * - New tasks can still be enqueued
     * - Worker threads will not dequeue new tasks
     * - Currently executing tasks continue to completion
     */
    virtual VoidResult pause() = 0;

    /**
     * @brief Resume task processing
     * @return VoidResult indicating success or failure
     */
    virtual VoidResult resume() = 0;

    /**
     * @brief Check if the pool is paused
     * @return true if paused, false otherwise
     */
    virtual bool is_paused() const = 0;

    // ===== Statistics =====

    /**
     * @brief Get the number of active (executing) tasks
     * @return Number of tasks currently being executed
     */
    virtual size_t active_tasks() const = 0;

    /**
     * @brief Get the number of idle workers
     * @return Number of worker threads waiting for tasks
     */
    virtual size_t idle_workers() const = 0;

    /**
     * @brief Get total number of completed tasks
     * @return Cumulative count of completed tasks
     */
    virtual size_t completed_tasks() const { return 0; }

    /**
     * @brief Get total number of failed tasks
     * @return Cumulative count of failed tasks
     */
    virtual size_t failed_tasks() const { return 0; }
};

/**
 * @brief Factory function type for creating thread pool instances
 */
using ThreadPoolFactory = std::function<std::shared_ptr<IThreadPool>(size_t worker_count)>;

/**
 * @interface IThreadPoolProvider
 * @brief Interface for modules that provide thread pool implementations
 */
class IThreadPoolProvider {
public:
    virtual ~IThreadPoolProvider() = default;

    /**
     * @brief Get the default thread pool instance
     * @return Shared pointer to the thread pool
     */
    virtual std::shared_ptr<IThreadPool> get_thread_pool() = 0;

    /**
     * @brief Create a new thread pool with specific configuration
     * @param worker_count Number of worker threads
     * @param queue_capacity Maximum queue size (0 = unlimited)
     * @return Result containing the thread pool or error
     */
    virtual Result<std::shared_ptr<IThreadPool>> create_thread_pool(
        size_t worker_count,
        size_t queue_capacity = 0) = 0;
};

} // namespace interfaces
} // namespace kcenon::common
