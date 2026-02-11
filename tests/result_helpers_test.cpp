// BSD 3-Clause License
//
// Copyright (c) 2025, kcenon
// All rights reserved.

/**
 * @file result_helpers_test.cpp
 * @brief Unit tests for Result<T> helper functions
 * @since 4.0.0
 */

#include <gtest/gtest.h>

#include <kcenon/common/patterns/result_helpers.h>
#include <kcenon/common/patterns/result.h>

#include <string>
#include <stdexcept>
#include <tuple>

using namespace kcenon::common;
using namespace kcenon::common::helpers;

// =============================================================================
// return_if_error tests
// =============================================================================

TEST(ResultHelpersTest, ReturnIfErrorOnOkReturnsNullopt) {
    auto result = Result<int>(42);
    auto err = return_if_error(result);
    EXPECT_FALSE(err.has_value());
}

TEST(ResultHelpersTest, ReturnIfErrorOnErrReturnsError) {
    auto result = Result<int>(error_info{-1, "failed"});
    auto err = return_if_error(result);
    ASSERT_TRUE(err.has_value());
    EXPECT_EQ(err->code, -1);
    EXPECT_EQ(err->message, "failed");
}

// =============================================================================
// error_if tests
// =============================================================================

TEST(ResultHelpersTest, ErrorIfTrueReturnsError) {
    auto err = error_if(true, error_info{42, "condition met"});
    ASSERT_TRUE(err.has_value());
    EXPECT_EQ(err->code, 42);
    EXPECT_EQ(err->message, "condition met");
}

TEST(ResultHelpersTest, ErrorIfFalseReturnsNullopt) {
    auto err = error_if(false, error_info{42, "should not happen"});
    EXPECT_FALSE(err.has_value());
}

// =============================================================================
// make_error tests
// =============================================================================

TEST(ResultHelpersTest, MakeErrorWithCodeAndMessage) {
    auto err = make_error(100, "test error");
    EXPECT_EQ(err.code, 100);
    EXPECT_EQ(err.message, "test error");
    EXPECT_EQ(err.module, "");
}

TEST(ResultHelpersTest, MakeErrorWithModule) {
    auto err = make_error(200, "module error", "TestModule");
    EXPECT_EQ(err.code, 200);
    EXPECT_EQ(err.message, "module error");
    EXPECT_EQ(err.module, "TestModule");
}

// =============================================================================
// make_error_with_details tests
// =============================================================================

TEST(ResultHelpersTest, MakeErrorWithDetails) {
    auto err = make_error_with_details(300, "detailed error", "Mod", "extra info");
    EXPECT_EQ(err.code, 300);
    EXPECT_EQ(err.message, "detailed error");
    EXPECT_EQ(err.module, "Mod");
    ASSERT_TRUE(err.details.has_value());
    EXPECT_EQ(err.details.value(), "extra info");
}

// =============================================================================
// safe_execute tests
// =============================================================================

TEST(ResultHelpersTest, SafeExecuteSuccessNonVoid) {
    auto result = safe_execute([]() -> int { return 42; });
    EXPECT_TRUE(result.is_ok());
    EXPECT_EQ(result.value(), 42);
}

TEST(ResultHelpersTest, SafeExecuteSuccessString) {
    auto result = safe_execute([]() -> std::string { return "hello"; });
    EXPECT_TRUE(result.is_ok());
    EXPECT_EQ(result.value(), "hello");
}

TEST(ResultHelpersTest, SafeExecuteCatchesStdException) {
    auto result = safe_execute([]() -> int {
        throw std::runtime_error("boom");
    }, "TestModule");

    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error().code, -99);
    EXPECT_EQ(result.error().message, "boom");
    EXPECT_EQ(result.error().module, "TestModule");
}

TEST(ResultHelpersTest, SafeExecuteCatchesUnknownException) {
    auto result = safe_execute([]() -> int {
        throw 42;  // Non-standard exception
    });

    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error().code, -99);
    EXPECT_EQ(result.error().message, "Unknown error");
}

// =============================================================================
// unwrap_or_handle tests
// =============================================================================

TEST(ResultHelpersTest, UnwrapOrHandleOnOk) {
    auto result = Result<int>(42);
    bool handler_called = false;

    int value = unwrap_or_handle(std::move(result), [&](const error_info&) {
        handler_called = true;
    });

    EXPECT_EQ(value, 42);
    EXPECT_FALSE(handler_called);
}

TEST(ResultHelpersTest, UnwrapOrHandleOnErr) {
    auto result = Result<int>(error_info{-1, "error"});
    error_info captured_error;

    int value = unwrap_or_handle(std::move(result), [&](const error_info& err) {
        captured_error = err;
    });

    EXPECT_EQ(value, 0);  // Default-constructed int
    EXPECT_EQ(captured_error.code, -1);
    EXPECT_EQ(captured_error.message, "error");
}

TEST(ResultHelpersTest, UnwrapOrHandleStringOnErr) {
    auto result = Result<std::string>(error_info{-2, "oops"});
    bool handler_called = false;

    std::string value = unwrap_or_handle(std::move(result), [&](const error_info&) {
        handler_called = true;
    });

    EXPECT_EQ(value, "");  // Default-constructed string
    EXPECT_TRUE(handler_called);
}

// =============================================================================
// combine_results tests
// =============================================================================

TEST(ResultHelpersTest, CombineResultsAllOk) {
    auto r1 = Result<int>(10);
    auto r2 = Result<std::string>(std::string("hello"));
    auto r3 = Result<double>(3.14);

    auto combined = combine_results(std::move(r1), std::move(r2), std::move(r3));
    EXPECT_TRUE(combined.is_ok());

    auto [i, s, d] = combined.value();
    EXPECT_EQ(i, 10);
    EXPECT_EQ(s, "hello");
    EXPECT_DOUBLE_EQ(d, 3.14);
}

TEST(ResultHelpersTest, CombineResultsFirstError) {
    auto r1 = Result<int>(error_info{1, "first error"});
    auto r2 = Result<std::string>(std::string("ok"));

    auto combined = combine_results(std::move(r1), std::move(r2));
    EXPECT_TRUE(combined.is_err());
    EXPECT_EQ(combined.error().code, 1);
    EXPECT_EQ(combined.error().message, "first error");
}

TEST(ResultHelpersTest, CombineResultsSecondError) {
    auto r1 = Result<int>(42);
    auto r2 = Result<std::string>(error_info{2, "second error"});

    auto combined = combine_results(std::move(r1), std::move(r2));
    EXPECT_TRUE(combined.is_err());
    EXPECT_EQ(combined.error().code, 2);
}

TEST(ResultHelpersTest, CombineResultsTwoValues) {
    auto r1 = Result<int>(1);
    auto r2 = Result<int>(2);

    auto combined = combine_results(std::move(r1), std::move(r2));
    EXPECT_TRUE(combined.is_ok());

    auto [a, b] = combined.value();
    EXPECT_EQ(a, 1);
    EXPECT_EQ(b, 2);
}

// =============================================================================
// try_extract tests
// =============================================================================

TEST(ResultHelpersTest, TryExtractOk) {
    auto result = Result<int>(42);
    auto extracted = try_extract(std::move(result));
    EXPECT_TRUE(extracted.is_ok());
    EXPECT_EQ(extracted.value(), 42);
}

TEST(ResultHelpersTest, TryExtractErr) {
    auto result = Result<int>(error_info{-1, "extract error"});
    auto extracted = try_extract(std::move(result));
    EXPECT_TRUE(extracted.is_err());
    EXPECT_EQ(extracted.error().code, -1);
}
