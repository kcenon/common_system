// BSD 3-Clause License
//
// Copyright (c) 2021-2025, 🍀☀🌕🌥 🌊
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its
//    contributors may be used to endorse or promote products derived from
//    this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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

    // Job-based execution support
    Result<std::future<void>> execute(std::unique_ptr<IJob>&& job) override {
        if (!job) {
            return error_info(1, "Job is null", "mock_executor");
        }

        auto promise = std::make_shared<std::promise<void>>();
        auto future = promise->get_future();

        // Use shared_ptr to make lambda copy-constructible
        auto shared_job = std::shared_ptr<IJob>(std::move(job));

        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            tasks_.emplace([shared_job, promise]() {
                try {
                    auto result = shared_job->execute();
                    if (is_error(result)) {
                        auto& err = get_error(result);
                        promise->set_exception(
                            std::make_exception_ptr(
                                std::runtime_error(err.message)));
                    } else {
                        promise->set_value();
                    }
                } catch (...) {
                    promise->set_exception(std::current_exception());
                }
            });
            pending_count_++;
        }
        queue_cv_.notify_one();

        return ok(std::move(future));
    }

    Result<std::future<void>> execute_delayed(
        std::unique_ptr<IJob>&& job,
        std::chrono::milliseconds delay) override {
        if (!job) {
            return error_info(1, "Job is null", "mock_executor");
        }

        auto promise = std::make_shared<std::promise<void>>();
        auto future = promise->get_future();

        // Use shared_ptr to make lambda copy-constructible
        auto shared_job = std::shared_ptr<IJob>(std::move(job));

        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            tasks_.emplace([shared_job, promise, delay]() {
                std::this_thread::sleep_for(delay);
                try {
                    auto result = shared_job->execute();
                    if (is_error(result)) {
                        auto& err = get_error(result);
                        promise->set_exception(
                            std::make_exception_ptr(
                                std::runtime_error(err.message)));
                    } else {
                        promise->set_value();
                    }
                } catch (...) {
                    promise->set_exception(std::current_exception());
                }
            });
            pending_count_++;
        }
        queue_cv_.notify_one();

        return ok(std::move(future));
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
 * Simple function job wrapper
 */
class function_job : public IJob {
public:
    explicit function_job(std::function<void()> func, std::string name = "function_job")
        : func_(std::move(func)), name_(std::move(name)) {}

    VoidResult execute() override {
        try {
            func_();
            return VoidResult(std::monostate{});
        } catch (const std::exception& e) {
            return VoidResult(error_info(1, e.what(), "function_job"));
        }
    }

    std::string get_name() const override { return name_; }

private:
    std::function<void()> func_;
    std::string name_;
};

/**
 * Example job implementation
 */
class calculation_job : public IJob {
public:
    calculation_job(int value, std::atomic<int>& result)
        : value_(value), result_(result) {}

    VoidResult execute() override {
        try {
            // Simulate some work
            std::this_thread::sleep_for(10ms);
            result_ += value_ * value_;
            return VoidResult(std::monostate{});
        } catch (const std::exception& e) {
            return VoidResult(
                error_info(1, e.what(), "calculation_job"));
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

    // Submit tasks using job-based API
    for (int value : data) {
        auto job = std::make_unique<function_job>([&sum, value] {
            // Simulate some work
            std::this_thread::sleep_for(10ms);
            sum += value * value;
        });

        auto result = executor.execute(std::move(job));
        if (is_ok(result)) {
            futures.push_back(std::move(get_value(result)));
        }
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
    std::cout << "1. Basic task execution:\n";
    mock_executor executor(2);

    auto job1 = std::make_unique<function_job>([] {
        std::cout << "   Task 1 executed\n";
    });
    auto result1 = executor.execute(std::move(job1));
    if (is_ok(result1)) {
        get_value(std::move(result1)).wait();
    }

    auto job2 = std::make_unique<function_job>([] {
        std::cout << "   Task 2 executed\n";
    });
    auto result2 = executor.execute(std::move(job2));
    if (is_ok(result2)) {
        get_value(std::move(result2)).wait();
    }

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

    auto provider_job = std::make_unique<function_job>([] {
        std::cout << "   Task from shared executor\n";
    });
    auto provider_result = shared_executor->execute(std::move(provider_job));
    if (is_ok(provider_result)) {
        get_value(std::move(provider_result)).wait();
    }

    // Example 5: Delayed execution
    std::cout << "\n5. Delayed execution:\n";
    std::cout << "   Scheduling delayed task...\n";
    auto start = std::chrono::steady_clock::now();

    auto delayed_job = std::make_unique<function_job>([start] {
        auto elapsed = std::chrono::steady_clock::now() - start;
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed);
        std::cout << "   Delayed task executed after " << ms.count() << "ms\n";
    });

    auto delayed_result = executor.execute_delayed(std::move(delayed_job), 500ms);
    if (is_ok(delayed_result)) {
        get_value(std::move(delayed_result)).wait();
    }

    // Example 6: Error handling
    std::cout << "\n6. Error handling:\n";
    auto error_job = std::make_unique<function_job>([] {
        throw std::runtime_error("Task failed!");
    });

    auto error_result = executor.execute(std::move(error_job));
    if (is_ok(error_result)) {
        try {
            auto error_future = std::move(get_value(error_result));
            error_future.get();
        } catch (const std::exception& e) {
            std::cout << "   Caught exception: " << e.what() << "\n";
        }
    }

    // Example 7: Custom job execution
    std::cout << "\n7. Custom job execution:\n";
    {
        mock_executor job_executor(2);
        std::atomic<int> job_sum{0};
        std::vector<std::future<void>> job_futures;

        std::cout << "   Executing calculation jobs...\n";
        for (int i = 1; i <= 5; ++i) {
            auto job = std::make_unique<calculation_job>(i, job_sum);
            auto result = job_executor.execute(std::move(job));

            if (is_ok(result)) {
                job_futures.push_back(std::move(get_value(result)));
            } else {
                auto& err = get_error(result);
                std::cout << "   Failed to execute job: "
                         << err.message << "\n";
            }
        }

        // Wait for all jobs to complete
        for (auto& future : job_futures) {
            future.wait();
        }

        std::cout << "   Custom job sum of squares: " << job_sum << "\n";
    }

    // Example 8: Graceful shutdown
    std::cout << "\n8. Graceful shutdown:\n";

    // Execute some tasks
    for (int i = 0; i < 5; ++i) {
        auto final_job = std::make_unique<function_job>([i] {
            std::this_thread::sleep_for(50ms);
            std::cout << "   Final task " << i << " completed\n";
        });
        executor.execute(std::move(final_job));
    }

    std::cout << "   Pending tasks before shutdown: "
              << executor.pending_tasks() << "\n";
    std::cout << "   Shutting down (waiting for completion)...\n";

    executor.shutdown(true);
    std::cout << "   Shutdown complete\n";

    std::cout << "\n=== Examples completed ===\n";
    return 0;
}