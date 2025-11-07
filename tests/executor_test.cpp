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
 * Mock executor for testing
 */
class MockExecutor : public IExecutor {
public:
    MockExecutor(size_t num_workers = 1) : num_workers_(num_workers) {}

    std::future<void> submit(std::function<void()> task) override {
        auto promise = std::make_shared<std::promise<void>>();
        auto future = promise->get_future();

        std::thread([task = std::move(task), promise]() {
            try {
                task();
                promise->set_value();
            } catch (...) {
                promise->set_exception(std::current_exception());
            }
        }).detach();

        submitted_count_++;
        return future;
    }

    std::future<void> submit_delayed(std::function<void()> task,
                                    std::chrono::milliseconds delay) override {
        auto promise = std::make_shared<std::promise<void>>();
        auto future = promise->get_future();

        std::thread([task = std::move(task), promise, delay]() {
            std::this_thread::sleep_for(delay);
            try {
                task();
                promise->set_value();
            } catch (...) {
                promise->set_exception(std::current_exception());
            }
        }).detach();

        return future;
    }

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

TEST_F(ExecutorTest, SubmitTask) {
    std::atomic<bool> executed{false};

    auto future = executor_->submit([&executed]() {
        executed = true;
    });

    future.wait();
    EXPECT_TRUE(executed);
}

TEST_F(ExecutorTest, SubmitMultipleTasks) {
    const size_t task_count = 10;
    std::atomic<size_t> counter{0};
    std::vector<std::future<void>> futures;

    for (size_t i = 0; i < task_count; ++i) {
        futures.push_back(executor_->submit([&counter]() {
            counter++;
        }));
    }

    for (auto& future : futures) {
        future.wait();
    }

    EXPECT_EQ(counter, task_count);
}

TEST_F(ExecutorTest, SubmitDelayed) {
    std::atomic<bool> executed{false};
    auto start = std::chrono::steady_clock::now();

    auto future = executor_->submit_delayed([&executed]() {
        executed = true;
    }, 100ms);

    future.wait();
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
    auto future = executor_->submit([]() {
        throw std::runtime_error("Test exception");
    });

    EXPECT_THROW(future.get(), std::runtime_error);
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
        executor_->submit([]() {
            std::this_thread::sleep_for(1ms);
        });
    }

    EXPECT_EQ(executor_->get_submitted_count(), task_count);
}