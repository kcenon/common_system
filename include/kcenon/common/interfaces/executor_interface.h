/**
 * @file executor_interface.h
 * @brief Common executor interface for task submission and management
 *
 * This header-only interface provides a unified abstraction for different
 * threading implementations, enabling loose coupling between modules.
 */

#ifndef COMMON_INTERFACES_EXECUTOR_INTERFACE_H
#define COMMON_INTERFACES_EXECUTOR_INTERFACE_H

#include <chrono>
#include <functional>
#include <future>
#include <memory>

namespace common {
namespace interfaces {

/**
 * @interface IExecutor
 * @brief Abstract interface for task execution systems
 *
 * This interface defines the contract for any task executor implementation,
 * allowing modules to work with different threading backends without
 * direct dependencies.
 */
class IExecutor {
public:
    virtual ~IExecutor() = default;

    /**
     * @brief Submit a task for immediate execution
     * @param task The function to execute
     * @return Future representing the task result
     */
    virtual std::future<void> submit(std::function<void()> task) = 0;

    /**
     * @brief Submit a task for delayed execution
     * @param task The function to execute
     * @param delay The delay before execution
     * @return Future representing the task result
     */
    virtual std::future<void> submit_delayed(
        std::function<void()> task,
        std::chrono::milliseconds delay) = 0;

    /**
     * @brief Get the number of worker threads
     * @return Number of available workers
     */
    virtual size_t worker_count() const = 0;

    /**
     * @brief Check if the executor is running
     * @return true if running, false otherwise
     */
    virtual bool is_running() const = 0;

    /**
     * @brief Get the number of pending tasks
     * @return Number of tasks waiting to be executed
     */
    virtual size_t pending_tasks() const = 0;

    /**
     * @brief Shutdown the executor gracefully
     * @param wait_for_completion Wait for all pending tasks to complete
     */
    virtual void shutdown(bool wait_for_completion = true) = 0;
};

/**
 * @brief Factory function type for creating executor instances
 */
using ExecutorFactory = std::function<std::shared_ptr<IExecutor>()>;

/**
 * @interface IExecutorProvider
 * @brief Interface for modules that provide executor implementations
 */
class IExecutorProvider {
public:
    virtual ~IExecutorProvider() = default;

    /**
     * @brief Get the default executor instance
     * @return Shared pointer to the executor
     */
    virtual std::shared_ptr<IExecutor> get_executor() = 0;

    /**
     * @brief Create a new executor with specific configuration
     * @param worker_count Number of worker threads
     * @return Shared pointer to the new executor
     */
    virtual std::shared_ptr<IExecutor> create_executor(size_t worker_count) = 0;
};

} // namespace interfaces
} // namespace common

#endif // COMMON_INTERFACES_EXECUTOR_INTERFACE_H