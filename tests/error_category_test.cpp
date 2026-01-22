// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file error_category_test.cpp
 * @brief Unit tests for the decentralized error category system.
 *
 * These tests verify:
 * - error_category base class functionality
 * - common_error_category implementation
 * - typed_error_code class with category support
 * - Integration with Result<T>
 *
 * @see https://github.com/kcenon/common_system/issues/300
 */

#include <gtest/gtest.h>

#include <kcenon/common/error/error_category.h>
#include <kcenon/common/patterns/result.h>

#include <map>
#include <set>
#include <string>
#include <thread>
#include <vector>

namespace kcenon::common {
namespace {

// ============================================================================
// Test Fixtures
// ============================================================================

/**
 * @brief Custom error category for testing.
 *
 * Simulates a system-specific error category (e.g., network_error_category)
 * to verify the extensibility of the error category system.
 */
class test_error_category : public error_category {
public:
    enum codes : int {
        success = 0,
        test_error_1 = 1,
        test_error_2 = 2,
        test_error_3 = 3
    };

    static const test_error_category& instance() noexcept {
        static test_error_category inst;
        return inst;
    }

    std::string_view name() const noexcept override {
        return "test";
    }

    std::string message(int code) const override {
        switch (code) {
            case success: return "Success";
            case test_error_1: return "Test error 1";
            case test_error_2: return "Test error 2";
            case test_error_3: return "Test error 3";
            default: return "Unknown test error";
        }
    }

private:
    test_error_category() = default;
};

/**
 * @brief Helper to create test category error codes.
 */
inline typed_error_code make_test_typed_error_code(test_error_category::codes code) noexcept {
    return typed_error_code(static_cast<int>(code), test_error_category::instance());
}

// ============================================================================
// error_category Tests
// ============================================================================

class ErrorCategoryTest : public ::testing::Test {};

TEST_F(ErrorCategoryTest, CommonCategorySingleton) {
    // Verify singleton pattern
    const auto& cat1 = common_error_category::instance();
    const auto& cat2 = common_error_category::instance();

    EXPECT_EQ(&cat1, &cat2);
}

TEST_F(ErrorCategoryTest, CommonCategoryName) {
    const auto& cat = common_error_category::instance();

    EXPECT_EQ(cat.name(), "common");
}

TEST_F(ErrorCategoryTest, CommonCategoryMessages) {
    const auto& cat = common_error_category::instance();

    EXPECT_EQ(cat.message(common_error_category::success), "Success");
    EXPECT_EQ(cat.message(common_error_category::unknown_error), "Unknown error");
    EXPECT_EQ(cat.message(common_error_category::invalid_argument), "Invalid argument");
    EXPECT_EQ(cat.message(common_error_category::not_found), "Not found");
    EXPECT_EQ(cat.message(common_error_category::timeout), "Operation timed out");
    EXPECT_EQ(cat.message(common_error_category::internal_error), "Internal error");
}

TEST_F(ErrorCategoryTest, CommonCategoryUnknownCode) {
    const auto& cat = common_error_category::instance();

    // Unknown codes should return a descriptive message
    std::string msg = cat.message(9999);
    EXPECT_TRUE(msg.find("Unknown") != std::string::npos ||
                msg.find("unknown") != std::string::npos);
}

TEST_F(ErrorCategoryTest, CustomCategorySingleton) {
    const auto& cat1 = test_error_category::instance();
    const auto& cat2 = test_error_category::instance();

    EXPECT_EQ(&cat1, &cat2);
}

TEST_F(ErrorCategoryTest, CustomCategoryName) {
    const auto& cat = test_error_category::instance();

    EXPECT_EQ(cat.name(), "test");
}

TEST_F(ErrorCategoryTest, CategoryEquality) {
    const auto& common1 = common_error_category::instance();
    const auto& common2 = common_error_category::instance();
    const auto& test_cat = test_error_category::instance();

    // Same category instances are equal
    EXPECT_EQ(common1, common2);

    // Different categories are not equal
    EXPECT_NE(common1, test_cat);
}

TEST_F(ErrorCategoryTest, CategoryComparison) {
    const auto& common_cat = common_error_category::instance();
    const auto& test_cat = test_error_category::instance();

    // Categories should have a consistent ordering
    bool common_less = common_cat < test_cat;
    bool test_less = test_cat < common_cat;

    // Exactly one should be true (strict weak ordering)
    EXPECT_NE(common_less, test_less);
}

// ============================================================================
// typed_error_code Tests
// ============================================================================

class ErrorCodeTest : public ::testing::Test {};

TEST_F(ErrorCodeTest, DefaultConstruction) {
    typed_error_code ec;

    EXPECT_EQ(ec.value(), 0);
    EXPECT_EQ(ec.category(), common_error_category::instance());
    EXPECT_FALSE(ec);  // Success is "falsy"
}

TEST_F(ErrorCodeTest, ConstructWithCodeAndCategory) {
    typed_error_code ec(common_error_category::not_found, common_error_category::instance());

    EXPECT_EQ(ec.value(), common_error_category::not_found);
    EXPECT_EQ(ec.category(), common_error_category::instance());
    EXPECT_TRUE(ec);  // Error is "truthy"
}

TEST_F(ErrorCodeTest, ConstructFromCommonEnum) {
    typed_error_code ec(common_error_category::timeout);

    EXPECT_EQ(ec.value(), common_error_category::timeout);
    EXPECT_EQ(ec.category(), common_error_category::instance());
    EXPECT_EQ(ec.message(), "Operation timed out");
}

TEST_F(ErrorCodeTest, Message) {
    typed_error_code ec(common_error_category::invalid_argument);

    EXPECT_EQ(ec.message(), "Invalid argument");
}

TEST_F(ErrorCodeTest, CategoryName) {
    typed_error_code common_ec(common_error_category::success);
    typed_error_code test_ec = make_test_typed_error_code(test_error_category::test_error_1);

    EXPECT_EQ(common_ec.category_name(), "common");
    EXPECT_EQ(test_ec.category_name(), "test");
}

TEST_F(ErrorCodeTest, BoolConversion) {
    typed_error_code success_ec(common_error_category::success);
    typed_error_code error_ec(common_error_category::not_found);

    EXPECT_FALSE(success_ec);
    EXPECT_TRUE(error_ec);
}

TEST_F(ErrorCodeTest, Clear) {
    typed_error_code ec(common_error_category::not_found);
    EXPECT_TRUE(ec);

    ec.clear();

    EXPECT_FALSE(ec);
    EXPECT_EQ(ec.value(), 0);
}

TEST_F(ErrorCodeTest, Assign) {
    typed_error_code ec;
    EXPECT_FALSE(ec);

    ec.assign(test_error_category::test_error_2, test_error_category::instance());

    EXPECT_TRUE(ec);
    EXPECT_EQ(ec.value(), test_error_category::test_error_2);
    EXPECT_EQ(ec.category(), test_error_category::instance());
}

TEST_F(ErrorCodeTest, Equality) {
    typed_error_code ec1(common_error_category::not_found);
    typed_error_code ec2(common_error_category::not_found);
    typed_error_code ec3(common_error_category::timeout);
    typed_error_code ec4 = make_test_typed_error_code(test_error_category::test_error_1);

    // Same category and code
    EXPECT_EQ(ec1, ec2);

    // Same category, different code
    EXPECT_NE(ec1, ec3);

    // Different category
    EXPECT_NE(ec1, ec4);
}

TEST_F(ErrorCodeTest, LessThanComparison) {
    typed_error_code ec1(common_error_category::not_found);
    typed_error_code ec2(common_error_category::timeout);
    typed_error_code ec3 = make_test_typed_error_code(test_error_category::test_error_1);

    // Can be used in ordered containers
    std::set<typed_error_code> error_set;
    error_set.insert(ec1);
    error_set.insert(ec2);
    error_set.insert(ec3);

    EXPECT_EQ(error_set.size(), 3u);
}

TEST_F(ErrorCodeTest, UseInMap) {
    std::map<typed_error_code, std::string> error_descriptions;

    typed_error_code ec1(common_error_category::not_found);
    typed_error_code ec2(common_error_category::timeout);

    error_descriptions[ec1] = "Resource not found";
    error_descriptions[ec2] = "Operation timeout";

    EXPECT_EQ(error_descriptions[ec1], "Resource not found");
    EXPECT_EQ(error_descriptions[ec2], "Operation timeout");
}

// ============================================================================
// make_typed_error_code Helper Tests
// ============================================================================

class MakeErrorCodeTest : public ::testing::Test {};

TEST_F(MakeErrorCodeTest, MakeCommonErrorCode) {
    auto ec = make_typed_error_code(common_error_category::invalid_argument);

    EXPECT_EQ(ec.value(), common_error_category::invalid_argument);
    EXPECT_EQ(ec.category(), common_error_category::instance());
}

TEST_F(MakeErrorCodeTest, MakeCustomErrorCode) {
    auto ec = make_test_typed_error_code(test_error_category::test_error_2);

    EXPECT_EQ(ec.value(), test_error_category::test_error_2);
    EXPECT_EQ(ec.category(), test_error_category::instance());
}

TEST_F(MakeErrorCodeTest, IsSuccessHelper) {
    typed_error_code success_ec(common_error_category::success);
    typed_error_code error_ec(common_error_category::not_found);

    EXPECT_TRUE(is_success(success_ec));
    EXPECT_FALSE(is_success(error_ec));
}

TEST_F(MakeErrorCodeTest, IsErrorHelper) {
    typed_error_code success_ec(common_error_category::success);
    typed_error_code error_ec(common_error_category::not_found);

    EXPECT_FALSE(is_error(success_ec));
    EXPECT_TRUE(is_error(error_ec));
}

// ============================================================================
// Integration with Result<T> Tests
// ============================================================================

class ErrorCodeResultIntegrationTest : public ::testing::Test {};

TEST_F(ErrorCodeResultIntegrationTest, ResultFromErrorCode) {
    auto ec = make_typed_error_code(common_error_category::not_found);
    Result<int> result(ec);

    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error().code, common_error_category::not_found);
    EXPECT_EQ(result.error().message, "Not found");
    EXPECT_EQ(result.error().module, "common");
}

TEST_F(ErrorCodeResultIntegrationTest, ResultErrFactoryWithErrorCode) {
    auto ec = make_typed_error_code(common_error_category::timeout);
    auto result = Result<std::string>::err(ec);

    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error().code, common_error_category::timeout);
    EXPECT_EQ(result.error().message, "Operation timed out");
}

TEST_F(ErrorCodeResultIntegrationTest, ResultWithCustomCategory) {
    auto ec = make_test_typed_error_code(test_error_category::test_error_1);
    auto result = Result<double>::err(ec);

    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error().code, test_error_category::test_error_1);
    EXPECT_EQ(result.error().message, "Test error 1");
    EXPECT_EQ(result.error().module, "test");
}

TEST_F(ErrorCodeResultIntegrationTest, ErrorInfoFromErrorCode) {
    auto ec = make_typed_error_code(common_error_category::invalid_argument);
    error_info info(ec);

    EXPECT_EQ(info.code, common_error_category::invalid_argument);
    EXPECT_EQ(info.message, "Invalid argument");
    EXPECT_EQ(info.module, "common");
}

TEST_F(ErrorCodeResultIntegrationTest, MixedUsageWithResult) {
    // Test that both old and new error handling styles work together

    // Old style: direct error_info
    auto result1 = Result<int>::err(error_info{-1, "Old style error", "legacy"});

    // New style: typed_error_code
    auto result2 = Result<int>::err(make_typed_error_code(common_error_category::not_found));

    EXPECT_TRUE(result1.is_err());
    EXPECT_TRUE(result2.is_err());

    EXPECT_EQ(result1.error().module, "legacy");
    EXPECT_EQ(result2.error().module, "common");
}

// ============================================================================
// Thread Safety Tests (Basic)
// ============================================================================

TEST_F(ErrorCategoryTest, CategorySingletonThreadSafety) {
    // C++11 guarantees thread-safe initialization of static locals
    // This test verifies the singleton can be accessed from multiple threads

    std::vector<std::thread> threads;
    std::vector<const error_category*> addresses(10, nullptr);

    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([i, &addresses]() {
            addresses[i] = &common_error_category::instance();
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    // All threads should get the same address
    for (int i = 1; i < 10; ++i) {
        EXPECT_EQ(addresses[0], addresses[i]);
    }
}

}  // namespace
}  // namespace kcenon::common
