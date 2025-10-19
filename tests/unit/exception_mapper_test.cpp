// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// Test for exception_mapper improvements

#include <kcenon/common/patterns/result.h>
#include <gtest/gtest.h>
#include <stdexcept>
#include <system_error>

using namespace common;

/**
 * @brief Test exception_mapper with various standard exceptions
 */
class ExceptionMapperTest : public ::testing::Test {
protected:
    void SetUp() override {
        // No special setup needed
    }
};

// Test bad_alloc mapping
TEST_F(ExceptionMapperTest, MapsBadAllocCorrectly) {
    auto result = try_catch<int>([]() -> int {
        throw std::bad_alloc();
    }, "test_module");

    ASSERT_TRUE(result.is_err());
    const auto& error = result.error();
    EXPECT_EQ(error.code, error_codes::OUT_OF_MEMORY);
    EXPECT_EQ(error.module, "test_module");
    EXPECT_TRUE(error.details.has_value());
    EXPECT_EQ(error.details.value(), "std::bad_alloc");
}

// Test invalid_argument mapping
TEST_F(ExceptionMapperTest, MapsInvalidArgumentCorrectly) {
    auto result = try_catch<int>([]() -> int {
        throw std::invalid_argument("Invalid input");
    }, "parser");

    ASSERT_TRUE(result.is_err());
    const auto& error = result.error();
    EXPECT_EQ(error.code, error_codes::INVALID_ARGUMENT);
    EXPECT_STREQ(error.message.c_str(), "Invalid input");
    EXPECT_EQ(error.module, "parser");
    EXPECT_TRUE(error.details.has_value());
    EXPECT_EQ(error.details.value(), "std::invalid_argument");
}

// Test out_of_range mapping
TEST_F(ExceptionMapperTest, MapsOutOfRangeCorrectly) {
    auto result = try_catch<int>([]() -> int {
        throw std::out_of_range("Index out of bounds");
    }, "container");

    ASSERT_TRUE(result.is_err());
    const auto& error = result.error();
    EXPECT_EQ(error.code, error_codes::INVALID_ARGUMENT);
    EXPECT_STREQ(error.message.c_str(), "Index out of bounds");
    EXPECT_TRUE(error.details.has_value());
    EXPECT_EQ(error.details.value(), "std::out_of_range");
}

// Test logic_error mapping
TEST_F(ExceptionMapperTest, MapsLogicErrorCorrectly) {
    auto result = try_catch<int>([]() -> int {
        throw std::logic_error("Logic failure");
    }, "algorithm");

    ASSERT_TRUE(result.is_err());
    const auto& error = result.error();
    EXPECT_EQ(error.code, error_codes::INTERNAL_ERROR);
    EXPECT_STREQ(error.message.c_str(), "Logic failure");
    EXPECT_TRUE(error.details.has_value());
    EXPECT_EQ(error.details.value(), "std::logic_error");
}

// Test runtime_error mapping
TEST_F(ExceptionMapperTest, MapsRuntimeErrorCorrectly) {
    auto result = try_catch<int>([]() -> int {
        throw std::runtime_error("Runtime failure");
    }, "execution");

    ASSERT_TRUE(result.is_err());
    const auto& error = result.error();
    EXPECT_EQ(error.code, error_codes::INTERNAL_ERROR);
    EXPECT_STREQ(error.message.c_str(), "Runtime failure");
    EXPECT_TRUE(error.details.has_value());
    EXPECT_EQ(error.details.value(), "std::runtime_error");
}

// Test system_error mapping
TEST_F(ExceptionMapperTest, MapsSystemErrorCorrectly) {
    auto result = try_catch<int>([]() -> int {
        throw std::system_error(std::make_error_code(std::errc::permission_denied),
                               "Access denied");
    }, "filesystem");

    ASSERT_TRUE(result.is_err());
    const auto& error = result.error();
    // system_error uses its own error code
    EXPECT_EQ(error.code, static_cast<int>(std::errc::permission_denied));
    EXPECT_STREQ(error.message.c_str(), "Access denied");
    EXPECT_TRUE(error.details.has_value());
    EXPECT_TRUE(error.details.value().find("std::system_error") != std::string::npos);
}

// Test unknown exception mapping
TEST_F(ExceptionMapperTest, MapsUnknownExceptionCorrectly) {
    auto result = try_catch<int>([]() -> int {
        throw 42;  // Non-standard exception
    }, "dangerous_code");

    ASSERT_TRUE(result.is_err());
    const auto& error = result.error();
    EXPECT_EQ(error.code, error_codes::INTERNAL_ERROR);
    EXPECT_STREQ(error.message.c_str(), "Unknown exception caught");
    EXPECT_EQ(error.module, "dangerous_code");
    EXPECT_TRUE(error.details.has_value());
    EXPECT_TRUE(error.details.value().find("Non-standard") != std::string::npos);
}

// Test try_catch_void with exceptions
TEST_F(ExceptionMapperTest, VoidFunctionWithException) {
    auto result = try_catch_void([]() {
        throw std::invalid_argument("Cannot process");
    }, "processor");

    ASSERT_TRUE(result.is_err());
    const auto& error = result.error();
    EXPECT_EQ(error.code, error_codes::INVALID_ARGUMENT);
    EXPECT_STREQ(error.message.c_str(), "Cannot process");
    EXPECT_EQ(error.module, "processor");
}

// Test successful execution (no exception)
TEST_F(ExceptionMapperTest, SuccessfulExecutionReturnsValue) {
    auto result = try_catch<int>([]() -> int {
        return 42;
    }, "calculator");

    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value(), 42);
}

// Test void function success
TEST_F(ExceptionMapperTest, VoidFunctionSuccess) {
    int counter = 0;
    auto result = try_catch_void([&counter]() {
        counter = 100;
    }, "setter");

    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(counter, 100);
}

// Test nested exception handling
TEST_F(ExceptionMapperTest, NestedExceptionHandling) {
    auto outer_result = try_catch<int>([]() -> int {
        auto inner_result = try_catch<int>([]() -> int {
            throw std::out_of_range("Inner error");
        }, "inner");

        if (inner_result.is_err()) {
            // Re-throw as different type
            throw std::runtime_error("Outer error: " + inner_result.error().message);
        }
        return inner_result.value();
    }, "outer");

    ASSERT_TRUE(outer_result.is_err());
    const auto& error = outer_result.error();
    EXPECT_EQ(error.code, error_codes::INTERNAL_ERROR);
    EXPECT_TRUE(error.message.find("Outer error") != std::string::npos);
    EXPECT_TRUE(error.message.find("Inner error") != std::string::npos);
}

// Benchmark: Compare old vs new error mapping overhead
TEST_F(ExceptionMapperTest, ErrorMappingPerformance) {
    constexpr int iterations = 10000;

    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        auto result = try_catch<int>([]() -> int {
            if (std::rand() % 100 == 0) {  // 1% exception rate
                throw std::invalid_argument("Random error");
            }
            return 42;
        }, "perf_test");

        // Force evaluation
        volatile bool is_ok = result.is_ok();
        (void)is_ok;
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    // Should complete in reasonable time (< 100ms for 10k iterations)
    EXPECT_LT(duration.count(), 100000);

    std::cout << "Exception mapping performance: "
              << duration.count() << " μs for " << iterations << " iterations\n"
              << "Average: " << (duration.count() / static_cast<double>(iterations))
              << " μs per call\n";
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
