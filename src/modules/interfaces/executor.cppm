// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file interfaces/executor.cppm
 * @brief C++20 module partition for executor interfaces.
 *
 * This module partition exports executor-related interfaces:
 * - IJob: Abstract job interface for task execution
 * - IExecutor: Abstract interface for task execution systems
 * - IThreadPool: Extended interface for thread pool implementations
 * - IDatabase: Database interface (forward declaration)
 *
 * Part of the kcenon.common module.
 */

module;

#include <chrono>
#include <cstddef>
#include <functional>
#include <future>
#include <memory>
#include <string>

export module kcenon.common:interfaces.executor;

// Import result.core partition to reuse error_info, Result<T>, VoidResult
import :result.core;

// Internal type aliases for this partition (non-exported to avoid symbol duplication)
namespace kcenon::common::interfaces {
using kcenon::common::error_info;
using kcenon::common::Result;
using kcenon::common::VoidResult;
} // namespace kcenon::common::interfaces

export namespace kcenon::common::interfaces {

// ============================================================================
// IJob Interface
// ============================================================================

/**
 * @interface IJob
 * @brief Abstract job interface for task execution.
 */
class IJob {
public:
    virtual ~IJob() = default;

    /**
     * @brief Execute the job.
     * @return VoidResult indicating success or failure
     */
    virtual VoidResult execute() = 0;

    /**
     * @brief Get the name of the job.
     * @return Job name
     */
    virtual std::string get_name() const { return "unnamed_job"; }

    /**
     * @brief Get the priority of the job.
     * @return Job priority (default: 0)
     */
    virtual int get_priority() const { return 0; }
};

// ============================================================================
// IExecutor Interface
// ============================================================================

/**
 * @interface IExecutor
 * @brief Abstract interface for task execution systems.
 */
class IExecutor {
public:
    virtual ~IExecutor() = default;

    /**
     * @brief Execute a job with Result-based error handling.
     */
    virtual Result<std::future<void>> execute(std::unique_ptr<IJob>&& job) = 0;

    /**
     * @brief Execute a job with delay.
     */
    virtual Result<std::future<void>> execute_delayed(
        std::unique_ptr<IJob>&& job,
        std::chrono::milliseconds delay) = 0;

    /**
     * @brief Get the number of worker threads.
     */
    virtual size_t worker_count() const = 0;

    /**
     * @brief Check if the executor is running.
     */
    virtual bool is_running() const = 0;

    /**
     * @brief Get the number of pending tasks.
     */
    virtual size_t pending_tasks() const = 0;

    /**
     * @brief Shutdown the executor gracefully.
     */
    virtual void shutdown(bool wait_for_completion = true) = 0;
};

using ExecutorFactory = std::function<std::shared_ptr<IExecutor>()>;

/**
 * @interface IExecutorProvider
 * @brief Provider for obtaining executor implementations.
 */
class IExecutorProvider {
public:
    virtual ~IExecutorProvider() = default;
    virtual std::shared_ptr<IExecutor> get_executor() = 0;
    virtual std::shared_ptr<IExecutor> create_executor(size_t worker_count) = 0;
};

// ============================================================================
// IThreadPool Interface
// ============================================================================

/**
 * @interface IThreadPool
 * @brief Extended interface for thread pool implementations.
 */
class IThreadPool : public IExecutor {
public:
    virtual ~IThreadPool() = default;

    virtual VoidResult resize(size_t new_size) = 0;
    virtual size_t min_workers() const { return 1; }
    virtual size_t max_workers() const { return 0; }
    virtual VoidResult set_queue_capacity(size_t capacity) = 0;
    virtual size_t get_queue_capacity() const = 0;
    virtual bool is_queue_full() const = 0;
    virtual size_t clear_pending_tasks() = 0;
    virtual VoidResult start() = 0;
    virtual VoidResult stop(bool wait_for_completion = true) = 0;
    virtual VoidResult pause() = 0;
    virtual VoidResult resume() = 0;
    virtual bool is_paused() const = 0;
    virtual size_t active_tasks() const = 0;
    virtual size_t idle_workers() const = 0;
    virtual size_t completed_tasks() const { return 0; }
    virtual size_t failed_tasks() const { return 0; }
};

using ThreadPoolFactory = std::function<std::shared_ptr<IThreadPool>(size_t worker_count)>;

class IThreadPoolProvider {
public:
    virtual ~IThreadPoolProvider() = default;
    virtual std::shared_ptr<IThreadPool> get_thread_pool() = 0;
    virtual Result<std::shared_ptr<IThreadPool>> create_thread_pool(
        size_t worker_count, size_t queue_capacity = 0) = 0;
};

// ============================================================================
// Database Interface (forward declaration only)
// ============================================================================

class IDatabase {
public:
    virtual ~IDatabase() = default;
};

} // namespace kcenon::common::interfaces
