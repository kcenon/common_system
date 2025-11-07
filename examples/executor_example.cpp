/**
 * @file executor_example.cpp
 * @brief Example demonstrating the IExecutor interface usage
 */

#include <kcenon/common/interfaces/executor_interface.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <atomic>
#include <queue>
#include <mutex>
#include <condition_variable>

using namespace kcenon::common;
using namespace kcenon::common::interfaces;
using namespace std::chrono_literals;

/**
 * Simple mock executor for demonstration
 */
class mock_executor : public IExecutor {
public:
    mock_executor(size_t num_workers = 4)
        : num_workers_(num_workers), running_(true) {
        // Start worker threads
        for (size_t i = 0; i < num_workers_; ++i) {
            workers_.emplace_back([this] { work_loop(); });
        }
    }

    ~mock_executor() {
        shutdown(true);
    }

    std::future<void> submit(std::function<void()> task) override {
        auto promise = std::make_shared<std::promise<void>>();
        auto future = promise->get_future();

        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            tasks_.emplace([task = std::move(task), promise]() {
                try {
                    task();
                    promise->set_value();
                } catch (...) {
                    promise->set_exception(std::current_exception());
                }
            });
            pending_count_++;
        }
        queue_cv_.notify_one();

        return future;
    }

    std::future<void> submit_delayed(std::function<void()> task,
                                    std::chrono::milliseconds delay) override {
        // Simple implementation: sleep then submit
        std::this_thread::sleep_for(delay);
        return submit(std::move(task));
    }

    // Phase 2: Job-based execution support
    common::Result<std::future<void>> execute(std::unique_ptr<IJob>&& job) override {
        if (!job) {
            return common::error_info(1, "Job is null", "mock_executor");
        }

        // Use shared_ptr to make lambda copy-constructible
        auto shared_job = std::shared_ptr<IJob>(std::move(job));

        auto task = [shared_job]() {
            auto result = shared_job->execute();
            if (common::is_error(result)) {
                // Log error but don't throw - already handled by Result
                auto& err = common::get_error(result);
                std::cerr << "Job execution failed: " << err.message << std::endl;
            }
        };

        return submit(std::move(task));
    }

    common::Result<std::future<void>> execute_delayed(
        std::unique_ptr<IJob>&& job,
        std::chrono::milliseconds delay) override {
        if (!job) {
            return common::error_info(1, "Job is null", "mock_executor");
        }

        // Use shared_ptr to make lambda copy-constructible
        auto shared_job = std::shared_ptr<IJob>(std::move(job));

        auto task = [shared_job]() {
            auto result = shared_job->execute();
            if (common::is_error(result)) {
                auto& err = common::get_error(result);
                std::cerr << "Job execution failed: " << err.message << std::endl;
            }
        };

        return submit_delayed(std::move(task), delay);
    }

    size_t worker_count() const override {
        return num_workers_;
    }

    bool is_running() const override {
        return running_;
    }

    size_t pending_tasks() const override {
        return pending_count_;
    }

    void shutdown(bool wait_for_completion) override {
        if (!running_) return;

        if (wait_for_completion) {
            // Wait for all tasks to complete
            std::unique_lock<std::mutex> lock(queue_mutex_);
            queue_cv_.wait(lock, [this] { return tasks_.empty(); });
        }

        running_ = false;
        queue_cv_.notify_all();

        for (auto& worker : workers_) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }

private:
    void work_loop() {
        while (running_) {
            std::function<void()> task;

            {
                std::unique_lock<std::mutex> lock(queue_mutex_);
                queue_cv_.wait(lock, [this] {
                    return !tasks_.empty() || !running_;
                });

                if (!running_ && tasks_.empty()) {
                    break;
                }

                if (!tasks_.empty()) {
                    task = std::move(tasks_.front());
                    tasks_.pop();
                    pending_count_--;
                }
            }

            if (task) {
                task();
            }
        }
    }

    size_t num_workers_;
    std::atomic<bool> running_;
    std::atomic<size_t> pending_count_{0};
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
};

/**
 * Example job implementation
 */
class calculation_job : public IJob {
public:
    calculation_job(int value, std::atomic<int>& result)
        : value_(value), result_(result) {}

    common::VoidResult execute() override {
        try {
            // Simulate some work
            std::this_thread::sleep_for(10ms);
            result_ += value_ * value_;
            return common::VoidResult(std::monostate{});
        } catch (const std::exception& e) {
            return common::VoidResult(
                common::error_info(1, e.what(), "calculation_job"));
        }
    }

    std::string get_name() const override {
        return "calculation_job_" + std::to_string(value_);
    }

    int get_priority() const override {
        return value_; // Higher values = higher priority
    }

private:
    int value_;
    std::atomic<int>& result_;
};

/**
 * Example function that uses IExecutor interface
 */
void process_data_batch(IExecutor& executor, const std::vector<int>& data) {
    std::atomic<int> sum{0};
    std::vector<std::future<void>> futures;

    std::cout << "Processing " << data.size() << " items using "
              << executor.worker_count() << " workers\n";

    // Submit tasks
    for (int value : data) {
        auto future = executor.submit([&sum, value] {
            // Simulate some work
            std::this_thread::sleep_for(10ms);
            sum += value * value;
        });
        futures.push_back(std::move(future));
    }

    // Wait for completion
    for (auto& future : futures) {
        future.wait();
    }

    std::cout << "Sum of squares: " << sum << "\n";
}

/**
 * Example of executor provider pattern
 */
class example_executor_provider : public IExecutorProvider {
public:
    std::shared_ptr<IExecutor> get_executor() override {
        if (!default_executor_) {
            default_executor_ = create_executor(4);
        }
        return default_executor_;
    }

    std::shared_ptr<IExecutor> create_executor(size_t worker_count) override {
        return std::make_shared<mock_executor>(worker_count);
    }

private:
    std::shared_ptr<IExecutor> default_executor_;
};

int main() {
    std::cout << "=== IExecutor Interface Examples ===\n\n";

    // Example 1: Basic usage
    std::cout << "1. Basic task submission:\n";
    mock_executor executor(2);

    auto future1 = executor.submit([] {
        std::cout << "   Task 1 executed\n";
    });

    auto future2 = executor.submit([] {
        std::cout << "   Task 2 executed\n";
    });

    future1.wait();
    future2.wait();

    // Example 2: Check executor status
    std::cout << "\n2. Executor status:\n";
    std::cout << "   Workers: " << executor.worker_count() << "\n";
    std::cout << "   Running: " << (executor.is_running() ? "yes" : "no") << "\n";
    std::cout << "   Pending: " << executor.pending_tasks() << "\n";

    // Example 3: Batch processing
    std::cout << "\n3. Batch processing:\n";
    std::vector<int> data = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    process_data_batch(executor, data);

    // Example 4: Using executor provider
    std::cout << "\n4. Using executor provider:\n";
    example_executor_provider provider;
    auto shared_executor = provider.get_executor();

    shared_executor->submit([] {
        std::cout << "   Task from shared executor\n";
    }).wait();

    // Example 5: Delayed execution
    std::cout << "\n5. Delayed execution:\n";
    std::cout << "   Scheduling delayed task...\n";
    auto start = std::chrono::steady_clock::now();

    auto delayed_future = executor.submit_delayed(
        [start] {
            auto elapsed = std::chrono::steady_clock::now() - start;
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed);
            std::cout << "   Delayed task executed after " << ms.count() << "ms\n";
        },
        500ms
    );

    delayed_future.wait();

    // Example 6: Error handling
    std::cout << "\n6. Error handling:\n";
    auto error_future = executor.submit([] {
        throw std::runtime_error("Task failed!");
    });

    try {
        error_future.get();
    } catch (const std::exception& e) {
        std::cout << "   Caught exception: " << e.what() << "\n";
    }

    // Example 7: Job-based execution (Phase 2)
    std::cout << "\n7. Job-based execution (Phase 2):\n";
    {
        mock_executor job_executor(2);
        std::atomic<int> job_sum{0};
        std::vector<std::future<void>> job_futures;

        std::cout << "   Submitting calculation jobs...\n";
        for (int i = 1; i <= 5; ++i) {
            auto job = std::make_unique<calculation_job>(i, job_sum);
            auto result = job_executor.execute(std::move(job));

            if (common::is_ok(result)) {
                job_futures.push_back(std::move(common::get_value(result)));
            } else {
                auto& err = common::get_error(result);
                std::cout << "   Failed to submit job: "
                         << err.message << "\n";
            }
        }

        // Wait for all jobs to complete
        for (auto& future : job_futures) {
            future.wait();
        }

        std::cout << "   Job-based sum of squares: " << job_sum << "\n";
    }

    // Example 8: Graceful shutdown
    std::cout << "\n8. Graceful shutdown:\n";

    // Submit some tasks
    for (int i = 0; i < 5; ++i) {
        executor.submit([i] {
            std::this_thread::sleep_for(50ms);
            std::cout << "   Final task " << i << " completed\n";
        });
    }

    std::cout << "   Pending tasks before shutdown: "
              << executor.pending_tasks() << "\n";
    std::cout << "   Shutting down (waiting for completion)...\n";

    executor.shutdown(true);
    std::cout << "   Shutdown complete\n";

    std::cout << "\n=== Examples completed ===\n";
    return 0;
}