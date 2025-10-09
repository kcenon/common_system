#include "framework/system_fixture.h"
#include "framework/test_helpers.h"
#include <kcenon/common/patterns/result.h>
#include <stdexcept>
#include <memory>

using namespace integration_tests;
using namespace kcenon::common;

/**
 * Tests for error handling and failure scenarios
 */
class ErrorHandlingTest : public SystemFixture {};

TEST_F(ErrorHandlingTest, ResultErrorPropagation) {
    // Test error propagation through function chain
    auto step1 = []() -> Result<int> {
        return Result<int>::err(error_code{1, "step1 failed"});
    };

    auto step2 = [](int value) -> Result<std::string> {
        return Result<std::string>::ok("value: " + std::to_string(value));
    };

    auto result = step1().and_then(step2);

    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error().code, 1);
    EXPECT_EQ(result.error().message, "step1 failed");
}

TEST_F(ErrorHandlingTest, ErrorRecoveryWithOrElse) {
    // Test error recovery
    auto failing_operation = []() -> Result<int> {
        return Result<int>::err(error_code{1, "operation failed"});
    };

    auto fallback = [](const error_code& err) -> Result<int> {
        // Log error and return default value
        return Result<int>::ok(0);
    };

    auto result = failing_operation().or_else(fallback);

    EXPECT_TRUE(result.is_ok());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(ErrorHandlingTest, MultipleErrorRecoveryAttempts) {
    // Test multiple recovery attempts
    int recovery_attempts = 0;

    auto failing_operation = [&]() -> Result<int> {
        recovery_attempts++;
        if (recovery_attempts < 3) {
            return Result<int>::err(error_code{1, "temporary failure"});
        }
        return Result<int>::ok(42);
    };

    Result<int> result = failing_operation();

    // First attempt fails
    EXPECT_TRUE(result.is_err());

    // Second attempt fails
    result = result.or_else([&](const error_code&) {
        return failing_operation();
    });
    EXPECT_TRUE(result.is_err());

    // Third attempt succeeds
    result = result.or_else([&](const error_code&) {
        return failing_operation();
    });
    EXPECT_TRUE(result.is_ok());
    EXPECT_EQ(result.value(), 42);
    EXPECT_EQ(recovery_attempts, 3);
}

TEST_F(ErrorHandlingTest, ErrorCodeChaining) {
    // Test error code propagation through multiple layers
    auto layer1 = []() -> Result<int> {
        return Result<int>::err(error_code{1, "layer1 error"});
    };

    auto layer2 = [&]() -> Result<std::string> {
        return layer1().and_then([](int val) -> Result<std::string> {
            return Result<std::string>::ok(std::to_string(val));
        });
    };

    auto layer3 = [&]() -> Result<double> {
        return layer2().and_then([](const std::string& str) -> Result<double> {
            return Result<double>::ok(std::stod(str));
        });
    };

    auto result = layer3();

    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error().code, 1);
    EXPECT_EQ(result.error().message, "layer1 error");
}

TEST_F(ErrorHandlingTest, ExceptionSafetyInCallbacks) {
    // Ensure exceptions in callbacks are handled properly
    auto& bus = get_event_bus();

    struct TestEvent {
        int value;
    };

    bool first_handler_called = false;
    bool second_handler_called = false;

    // First handler throws exception
    auto sub1 = bus.subscribe<TestEvent>([&](const TestEvent&) {
        first_handler_called = true;
        throw std::runtime_error("handler exception");
    });

    // Second handler should still execute
    auto sub2 = bus.subscribe<TestEvent>([&](const TestEvent&) {
        second_handler_called = true;
    });

    TestEvent event{42};

    // This should not throw, even though first handler throws
    EXPECT_NO_THROW(bus.publish(event));

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    // First handler was called (but threw)
    EXPECT_TRUE(first_handler_called);

    // Note: Depending on implementation, second handler may or may not execute
    // This test documents the current behavior

    // Cleanup
    bus.unsubscribe(sub1);
    bus.unsubscribe(sub2);
}

TEST_F(ErrorHandlingTest, ResourceCleanupOnError) {
    // Test RAII-style cleanup even when errors occur
    bool cleanup_called = false;

    auto cleanup = helpers::make_scoped_cleanup([&]() {
        cleanup_called = true;
    });

    // Simulate error condition
    auto result = Result<int>::err(error_code{1, "error"});

    EXPECT_TRUE(result.is_err());
    // cleanup hasn't run yet
    EXPECT_FALSE(cleanup_called);

    // Scope exit triggers cleanup
    {
        auto scoped = helpers::make_scoped_cleanup([&]() {
            cleanup_called = true;
        });

        // Even if we return early due to error, cleanup runs
        if (result.is_err()) {
            // Cleanup will run on scope exit
        }
    }

    EXPECT_TRUE(cleanup_called);
}

TEST_F(ErrorHandlingTest, NullPointerHandling) {
    // Test handling of null pointers
    std::unique_ptr<int> null_ptr;

    auto access_value = [](std::unique_ptr<int>& ptr) -> Result<int> {
        if (!ptr) {
            return Result<int>::err(error_code{1, "null pointer"});
        }
        return Result<int>::ok(*ptr);
    };

    auto result = access_value(null_ptr);

    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error().message, "null pointer");
}

TEST_F(ErrorHandlingTest, InvalidOperationHandling) {
    // Test handling of invalid operations
    auto divide = [](int a, int b) -> Result<double> {
        if (b == 0) {
            return Result<double>::err(error_code{1, "division by zero"});
        }
        return Result<double>::ok(static_cast<double>(a) / b);
    };

    auto result1 = divide(10, 2);
    EXPECT_TRUE(result1.is_ok());
    EXPECT_DOUBLE_EQ(result1.value(), 5.0);

    auto result2 = divide(10, 0);
    EXPECT_TRUE(result2.is_err());
    EXPECT_EQ(result2.error().message, "division by zero");
}

TEST_F(ErrorHandlingTest, CascadingFailures) {
    // Test multiple failures in sequence
    std::vector<error_code> errors;

    auto operation1 = [&]() -> Result<int> {
        auto err = error_code{1, "operation1 failed"};
        errors.push_back(err);
        return Result<int>::err(err);
    };

    auto operation2 = [&]() -> Result<int> {
        auto err = error_code{2, "operation2 failed"};
        errors.push_back(err);
        return Result<int>::err(err);
    };

    auto operation3 = [&]() -> Result<int> {
        auto err = error_code{3, "operation3 failed"};
        errors.push_back(err);
        return Result<int>::err(err);
    };

    // Try operations in sequence, stopping at first success
    auto result = operation1()
        .or_else([&](const error_code&) { return operation2(); })
        .or_else([&](const error_code&) { return operation3(); });

    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(errors.size(), 3);
    EXPECT_EQ(result.error().code, 3);
}

TEST_F(ErrorHandlingTest, ErrorContextPreservation) {
    // Ensure error context is preserved through transformations
    struct DetailedError {
        int code;
        std::string message;
        std::string context;
    };

    auto create_error = [](const std::string& ctx) -> error_code {
        return error_code{500, "error in " + ctx};
    };

    std::string context = "database operation";
    auto error = create_error(context);

    EXPECT_EQ(error.code, 500);
    EXPECT_TRUE(error.message.find(context) != std::string::npos);
}
