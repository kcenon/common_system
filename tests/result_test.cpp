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
 * @file result_test.cpp
 * @brief Free function contract tests for Result<T> pattern
 *
 * This test file validates the free function API for Result<T> operations.
 * Free functions are provided for backward compatibility and specific use cases
 * such as macro internals and ADL-dependent templates.
 *
 * For new code, prefer member methods (see improved_result_test.cpp).
 *
 * API Style: Free functions
 * - is_ok(result), is_error(result)
 * - get_value(result), get_error(result)
 * - value_or(result, default)
 * - get_if_ok(result), get_if_error(result)
 * - map(result, func), and_then(result, func), or_else(result, func)
 *
 * @see improved_result_test.cpp for member method API tests
 * @see docs/guides/RESULT_MIGRATION_GUIDE.md for API migration guidance
 */

#include <gtest/gtest.h>
#include <kcenon/common/patterns/result.h>

using namespace kcenon::common;

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
    auto result = make_error<int>(-1, "Test error", "test_module");
    EXPECT_FALSE(is_ok(result));
    EXPECT_TRUE(is_error(result));

    auto& err = get_error(result);
    EXPECT_EQ(err.code, -1);
    EXPECT_EQ(err.message, "Test error");
    EXPECT_EQ(err.module, "test_module");
}

TEST_F(ResultTest, ValueOr) {
    auto success = ok(10);
    auto failure = make_error<int>(-1, "Error");

    EXPECT_EQ(value_or(success, 0), 10);
    EXPECT_EQ(value_or(failure, 0), 0);
}

TEST_F(ResultTest, GetIfOk) {
    auto result = ok(100);
    auto* value = get_if_ok(result);
    ASSERT_NE(value, nullptr);
    EXPECT_EQ(*value, 100);

    auto error_result = make_error<int>(-1, "Error");
    EXPECT_EQ(get_if_ok(error_result), nullptr);
}

TEST_F(ResultTest, GetIfError) {
    auto result = make_error<int>(-1, "Test error");
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

    auto error_result = make_error<int>(-1, "Error");
    auto mapped_error = map(error_result, [](int x) { return x * 2; });

    EXPECT_TRUE(is_error(mapped_error));
    EXPECT_EQ(get_error(mapped_error).code, -1);
}

TEST_F(ResultTest, AndThen) {
    auto divide = [](int x, int y) -> Result<int> {
        if (y == 0) {
            return make_error<int>(-1, "Division by zero");
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
    auto error_result = make_error<int>(-1, "Error");
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

    auto void_error = make_error<std::monostate>(-1, "Void error");
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

// Enhanced exception mapper tests
TEST_F(ResultTest, ExceptionMapper_BadAlloc) {
    auto result = try_catch<int>([]() -> int {
        throw std::bad_alloc();
    }, "memory_test");

    ASSERT_TRUE(is_error(result));
    const auto& err = get_error(result);
    EXPECT_EQ(err.code, error_codes::OUT_OF_MEMORY);
    EXPECT_EQ(err.module, "memory_test");
    EXPECT_TRUE(err.details.has_value());
    EXPECT_EQ(err.details.value(), "std::bad_alloc");
}

TEST_F(ResultTest, ExceptionMapper_InvalidArgument) {
    auto result = try_catch<int>([]() -> int {
        throw std::invalid_argument("Invalid input value");
    }, "parser");

    ASSERT_TRUE(is_error(result));
    const auto& err = get_error(result);
    EXPECT_EQ(err.code, error_codes::INVALID_ARGUMENT);
    EXPECT_EQ(err.message, "Invalid input value");
    EXPECT_EQ(err.module, "parser");
    EXPECT_TRUE(err.details.has_value());
    EXPECT_EQ(err.details.value(), "std::invalid_argument");
}

TEST_F(ResultTest, ExceptionMapper_OutOfRange) {
    auto result = try_catch<int>([]() -> int {
        throw std::out_of_range("Index out of bounds");
    }, "container");

    ASSERT_TRUE(is_error(result));
    const auto& err = get_error(result);
    EXPECT_EQ(err.code, error_codes::INVALID_ARGUMENT);
    EXPECT_EQ(err.message, "Index out of bounds");
    EXPECT_TRUE(err.details.has_value());
    EXPECT_EQ(err.details.value(), "std::out_of_range");
}

TEST_F(ResultTest, ExceptionMapper_LogicError) {
    auto result = try_catch<int>([]() -> int {
        throw std::logic_error("Logic failure");
    }, "algorithm");

    ASSERT_TRUE(is_error(result));
    const auto& err = get_error(result);
    EXPECT_EQ(err.code, error_codes::INTERNAL_ERROR);
    EXPECT_TRUE(err.details.has_value());
    EXPECT_EQ(err.details.value(), "std::logic_error");
}

TEST_F(ResultTest, ExceptionMapper_SystemError) {
    auto result = try_catch<int>([]() -> int {
        throw std::system_error(std::make_error_code(std::errc::permission_denied),
                               "Access denied");
    }, "filesystem");

    ASSERT_TRUE(is_error(result));
    const auto& err = get_error(result);
    EXPECT_EQ(err.code, static_cast<int>(std::errc::permission_denied));
    EXPECT_TRUE(err.details.has_value());
    EXPECT_TRUE(err.details.value().find("std::system_error") != std::string::npos);
}

TEST_F(ResultTest, ExceptionMapper_UnknownException) {
    auto result = try_catch<int>([]() -> int {
        throw 42;  // Non-standard exception
    }, "dangerous_code");

    ASSERT_TRUE(is_error(result));
    const auto& err = get_error(result);
    EXPECT_EQ(err.code, error_codes::INTERNAL_ERROR);
    EXPECT_EQ(err.message, "Unknown exception caught");
    EXPECT_TRUE(err.details.has_value());
    EXPECT_TRUE(err.details.value().find("Non-standard") != std::string::npos);
}

TEST_F(ResultTest, TryCatchVoid_WithException) {
    auto result = try_catch_void([]() {
        throw std::invalid_argument("Cannot process");
    }, "processor");

    ASSERT_TRUE(is_error(result));
    const auto& err = get_error(result);
    EXPECT_EQ(err.code, error_codes::INVALID_ARGUMENT);
    EXPECT_EQ(err.message, "Cannot process");
    EXPECT_EQ(err.module, "processor");
}

TEST_F(ResultTest, TryCatchVoid_Success) {
    int counter = 0;
    auto result = try_catch_void([&counter]() {
        counter = 100;
    }, "setter");

    EXPECT_TRUE(is_ok(result));
    EXPECT_EQ(counter, 100);
}