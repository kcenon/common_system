/**
 * @file result_test.cpp
 * @brief Unit tests for Result<T> pattern
 */

#include <gtest/gtest.h>
#include <kcenon/common/patterns/result.h>

using namespace common;

class ResultTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(ResultTest, CreateSuccessResult) {
    auto result = ok(42);
    EXPECT_TRUE(is_ok(result));
    EXPECT_FALSE(is_error(result));
    EXPECT_EQ(get_value(result), 42);
}

TEST_F(ResultTest, CreateErrorResult) {
    auto result = error<int>(-1, "Test error", "test_module");
    EXPECT_FALSE(is_ok(result));
    EXPECT_TRUE(is_error(result));

    auto& err = get_error(result);
    EXPECT_EQ(err.code, -1);
    EXPECT_EQ(err.message, "Test error");
    EXPECT_EQ(err.module, "test_module");
}

TEST_F(ResultTest, ValueOr) {
    auto success = ok(10);
    auto failure = error<int>(-1, "Error");

    EXPECT_EQ(value_or(success, 0), 10);
    EXPECT_EQ(value_or(failure, 0), 0);
}

TEST_F(ResultTest, GetIfOk) {
    auto result = ok(100);
    auto* value = get_if_ok(result);
    ASSERT_NE(value, nullptr);
    EXPECT_EQ(*value, 100);

    auto error_result = error<int>(-1, "Error");
    EXPECT_EQ(get_if_ok(error_result), nullptr);
}

TEST_F(ResultTest, GetIfError) {
    auto result = error<int>(-1, "Test error");
    auto* err = get_if_error(result);
    ASSERT_NE(err, nullptr);
    EXPECT_EQ(err->code, -1);

    auto ok_result = ok(42);
    EXPECT_EQ(get_if_error(ok_result), nullptr);
}

TEST_F(ResultTest, MapFunction) {
    auto result = ok(10);
    auto mapped = map(result, [](int x) { return x * 2; });

    EXPECT_TRUE(is_ok(mapped));
    EXPECT_EQ(get_value(mapped), 20);

    auto error_result = error<int>(-1, "Error");
    auto mapped_error = map(error_result, [](int x) { return x * 2; });

    EXPECT_TRUE(is_error(mapped_error));
    EXPECT_EQ(get_error(mapped_error).code, -1);
}

TEST_F(ResultTest, AndThen) {
    auto divide = [](int x, int y) -> Result<int> {
        if (y == 0) {
            return error<int>(-1, "Division by zero");
        }
        return ok(x / y);
    };

    auto result = ok(20);
    auto chained = and_then(result, [&divide](int x) {
        return divide(x, 2);
    });

    EXPECT_TRUE(is_ok(chained));
    EXPECT_EQ(get_value(chained), 10);

    auto chained_error = and_then(result, [&divide](int x) {
        return divide(x, 0);
    });

    EXPECT_TRUE(is_error(chained_error));
}

TEST_F(ResultTest, OrElse) {
    auto error_result = error<int>(-1, "Error");
    auto recovered = or_else(error_result, [](const error_info&) {
        return ok(42);
    });

    EXPECT_TRUE(is_ok(recovered));
    EXPECT_EQ(get_value(recovered), 42);

    auto ok_result = ok(10);
    auto unchanged = or_else(ok_result, [](const error_info&) {
        return ok(0);
    });

    EXPECT_TRUE(is_ok(unchanged));
    EXPECT_EQ(get_value(unchanged), 10);
}

TEST_F(ResultTest, VoidResult) {
    auto void_ok = ok();
    EXPECT_TRUE(is_ok(void_ok));

    auto void_error = error<std::monostate>(-1, "Void error");
    EXPECT_TRUE(is_error(void_error));
}

TEST_F(ResultTest, TryCatch) {
    auto result = try_catch<int>([]() {
        return 42;
    }, "test_module");

    EXPECT_TRUE(is_ok(result));
    EXPECT_EQ(get_value(result), 42);

    auto error_result = try_catch<int>([]() -> int {
        throw std::runtime_error("Test exception");
    }, "test_module");

    EXPECT_TRUE(is_error(error_result));
    EXPECT_EQ(get_error(error_result).code, error_codes::INTERNAL_ERROR);
}