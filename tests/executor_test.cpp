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
 * @file executor_test.cpp
 * @brief Unit tests for IExecutor interface
 */

#include <gtest/gtest.h>
#include <kcenon/common/interfaces/executor_interface.h>
#include <atomic>
#include <chrono>
#include <thread>

using namespace kcenon::common;
using namespace kcenon::common::interfaces;
using namespace std::chrono_literals;

/**
 * Simple job wrapper for testing
 */
class FunctionJob : public IJob {
public:
    explicit FunctionJob(std::function<void()> func, std::string name = "test_job")
        : func_(std::move(func)), name_(std::move(name)) {}

    VoidResult execute() override {
        try {
            func_();
            return ok();
        } catch (const std::exception& e) {
            return VoidResult(error_info(1, e.what(), "JobExecutionError"));
        }
    }

    std::string get_name() const override { return name_; }

private:
    std::function<void()> func_;
    std::string name_;
};

/**
 * Mock executor for testing
 */
class MockExecutor : public IExecutor {
public:
    MockExecutor(size_t num_workers = 1) : num_workers_(num_workers) {}

    Result<std::future<void>> execute(std::unique_ptr<IJob>&& job) override {
        if (!job) {
            return make_error<std::future<void>>(1, "Null job provided", "ExecutorError");
        }

        auto promise = std::make_shared<std::promise<void>>();
        auto future = promise->get_future();

        std::thread([job = std::move(job), promise]() {
            try {
                auto result = job->execute();
                if (is_ok(result)) {
                    promise->set_value();
                } else {
                    promise->set_exception(
                        std::make_exception_ptr(
                            std::runtime_error(get_error(result).message)));
                }
            } catch (...) {
                promise->set_exception(std::current_exception());
            }
        }).detach();

        submitted_count_++;
        return ok(std::move(future));
    }

    Result<std::future<void>> execute_delayed(
        std::unique_ptr<IJob>&& job,
        std::chrono::milliseconds delay) override {
        if (!job) {
            return make_error<std::future<void>>(1, "Null job provided", "ExecutorError");
        }

        auto promise = std::make_shared<std::promise<void>>();
        auto future = promise->get_future();

        std::thread([job = std::move(job), promise, delay]() {
            std::this_thread::sleep_for(delay);
            try {
                auto result = job->execute();
                if (is_ok(result)) {
                    promise->set_value();
                } else {
                    promise->set_exception(
                        std::make_exception_ptr(
                            std::runtime_error(get_error(result).message)));
                }
            } catch (...) {
                promise->set_exception(std::current_exception());
            }
        }).detach();

        return ok(std::move(future));
    }

    size_t worker_count() const override {
        return num_workers_;
    }

    bool is_running() const override {
        return running_;
    }

    size_t pending_tasks() const override {
        return 0; // Simplified for testing
    }

    void shutdown(bool wait_for_completion = true) override {
        running_ = false;
    }

    // Test helpers
    size_t get_submitted_count() const { return submitted_count_; }

private:
    size_t num_workers_;
    std::atomic<bool> running_{true};
    std::atomic<size_t> submitted_count_{0};
};

class ExecutorTest : public ::testing::Test {
protected:
    void SetUp() override {
        executor_ = std::make_unique<MockExecutor>(4);
    }

    void TearDown() override {
        if (executor_) {
            executor_->shutdown(true);
        }
    }

    std::unique_ptr<MockExecutor> executor_;
};

TEST_F(ExecutorTest, ExecuteTask) {
    std::atomic<bool> executed{false};

    auto job = std::make_unique<FunctionJob>([&executed]() {
        executed = true;
    });

    auto result = executor_->execute(std::move(job));
    ASSERT_TRUE(is_ok(result));

    get_value(std::move(result)).wait();
    EXPECT_TRUE(executed);
}

TEST_F(ExecutorTest, ExecuteMultipleTasks) {
    const size_t task_count = 10;
    std::atomic<size_t> counter{0};
    std::vector<std::future<void>> futures;

    for (size_t i = 0; i < task_count; ++i) {
        auto job = std::make_unique<FunctionJob>([&counter]() {
            counter++;
        });

        auto result = executor_->execute(std::move(job));
        ASSERT_TRUE(is_ok(result));
        futures.push_back(std::move(get_value(result)));
    }

    for (auto& future : futures) {
        future.wait();
    }

    EXPECT_EQ(counter, task_count);
}

TEST_F(ExecutorTest, ExecuteDelayed) {
    std::atomic<bool> executed{false};
    auto start = std::chrono::steady_clock::now();

    auto job = std::make_unique<FunctionJob>([&executed]() {
        executed = true;
    });

    auto result = executor_->execute_delayed(std::move(job), 100ms);
    ASSERT_TRUE(is_ok(result));

    get_value(std::move(result)).wait();
    auto elapsed = std::chrono::steady_clock::now() - start;

    EXPECT_TRUE(executed);
    EXPECT_GE(elapsed, 100ms);
}

TEST_F(ExecutorTest, WorkerCount) {
    EXPECT_EQ(executor_->worker_count(), 4);
}

TEST_F(ExecutorTest, IsRunning) {
    EXPECT_TRUE(executor_->is_running());

    executor_->shutdown(false);
    EXPECT_FALSE(executor_->is_running());
}

TEST_F(ExecutorTest, ExceptionHandling) {
    auto job = std::make_unique<FunctionJob>([]() {
        throw std::runtime_error("Test exception");
    });

    auto result = executor_->execute(std::move(job));
    ASSERT_TRUE(is_ok(result));

    EXPECT_THROW(get_value(std::move(result)).get(), std::runtime_error);
}

TEST_F(ExecutorTest, ExecutorProvider) {
    class TestProvider : public IExecutorProvider {
    public:
        std::shared_ptr<IExecutor> get_executor() override {
            if (!executor_) {
                executor_ = create_executor(2);
            }
            return executor_;
        }

        std::shared_ptr<IExecutor> create_executor(size_t worker_count) override {
            return std::make_shared<MockExecutor>(worker_count);
        }

    private:
        std::shared_ptr<IExecutor> executor_;
    };

    TestProvider provider;
    auto executor1 = provider.get_executor();
    auto executor2 = provider.get_executor();

    // Should return the same instance
    EXPECT_EQ(executor1.get(), executor2.get());

    // Create new executor
    auto executor3 = provider.create_executor(8);
    EXPECT_NE(executor1.get(), executor3.get());
    EXPECT_EQ(executor3->worker_count(), 8);
}

TEST_F(ExecutorTest, SubmittedCount) {
    const size_t task_count = 5;

    for (size_t i = 0; i < task_count; ++i) {
        auto job = std::make_unique<FunctionJob>([]() {
            std::this_thread::sleep_for(1ms);
        });
        auto result = executor_->execute(std::move(job));
        ASSERT_TRUE(is_ok(result));
    }

    EXPECT_EQ(executor_->get_submitted_count(), task_count);
}