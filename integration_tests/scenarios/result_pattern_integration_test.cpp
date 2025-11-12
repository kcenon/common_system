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

#include "system_fixture.h"
#include "test_helpers.h"
#include <kcenon/common/patterns/result.h>
#include <string>
#include <vector>

using namespace integration_tests;
using namespace kcenon::common;

/**
 * Integration tests for Result<T> pattern across multiple use cases
 */
class ResultPatternIntegrationTest : public SystemFixture {};

TEST_F(ResultPatternIntegrationTest, BasicResultCreationAndAccess) {
    // Create successful result
    auto success = Result<int>::ok(42);
    EXPECT_TRUE(success.is_ok());
    EXPECT_FALSE(success.is_err());
    EXPECT_EQ(success.value(), 42);
}

TEST_F(ResultPatternIntegrationTest, ErrorResultCreation) {
    // Create error result
    auto error = Result<int>::err(error_code{100, "test error"});
    EXPECT_FALSE(error.is_ok());
    EXPECT_TRUE(error.is_err());
    EXPECT_EQ(error.error().code, 100);
    EXPECT_EQ(error.error().message, "test error");
}

TEST_F(ResultPatternIntegrationTest, MapTransformation) {
    // Test map transformation
    auto result = Result<int>::ok(10);
    auto mapped = result.map([](int x) { return x * 2; });

    EXPECT_TRUE(mapped.is_ok());
    EXPECT_EQ(mapped.value(), 20);
}

TEST_F(ResultPatternIntegrationTest, MapOnError) {
    // Map should not execute on error
    auto result = Result<int>::err(error_code{1, "error"});
    bool map_executed = false;

    auto mapped = result.map([&map_executed](int x) {
        map_executed = true;
        return x * 2;
    });

    EXPECT_FALSE(map_executed);
    EXPECT_TRUE(mapped.is_err());
}

TEST_F(ResultPatternIntegrationTest, AndThenChaining) {
    // Test and_then for chaining operations
    auto result = Result<int>::ok(5);
    auto chained = result.and_then([](int x) -> Result<std::string> {
        return Result<std::string>::ok("Value: " + std::to_string(x));
    });

    EXPECT_TRUE(chained.is_ok());
    EXPECT_EQ(chained.value(), "Value: 5");
}

TEST_F(ResultPatternIntegrationTest, AndThenErrorPropagation) {
    // and_then should propagate errors
    auto result = Result<int>::err(error_code{1, "initial error"});
    bool and_then_executed = false;

    auto chained = result.and_then([&and_then_executed](int x) -> Result<std::string> {
        and_then_executed = true;
        return Result<std::string>::ok("Value: " + std::to_string(x));
    });

    EXPECT_FALSE(and_then_executed);
    EXPECT_TRUE(chained.is_err());
    EXPECT_EQ(chained.error().message, "initial error");
}

TEST_F(ResultPatternIntegrationTest, OrElseRecovery) {
    // Test or_else for error recovery
    auto result = Result<int>::err(error_code{1, "error"});
    auto recovered = result.or_else([](const error_code&) {
        return Result<int>::ok(99);
    });

    EXPECT_TRUE(recovered.is_ok());
    EXPECT_EQ(recovered.value(), 99);
}

TEST_F(ResultPatternIntegrationTest, OrElseNoRecoveryNeeded) {
    // or_else should not execute on success
    auto result = Result<int>::ok(42);
    bool or_else_executed = false;

    auto unchanged = result.or_else([&or_else_executed](const error_code&) {
        or_else_executed = true;
        return Result<int>::ok(0);
    });

    EXPECT_FALSE(or_else_executed);
    EXPECT_TRUE(unchanged.is_ok());
    EXPECT_EQ(unchanged.value(), 42);
}

TEST_F(ResultPatternIntegrationTest, ComplexChaining) {
    // Test complex chain of operations
    auto result = Result<int>::ok(10)
        .map([](int x) { return x + 5; })
        .and_then([](int x) -> Result<int> {
            if (x > 10) {
                return Result<int>::ok(x * 2);
            }
            return Result<int>::err(error_code{1, "value too small"});
        })
        .map([](int x) { return x - 10; });

    EXPECT_TRUE(result.is_ok());
    EXPECT_EQ(result.value(), 20);  // (10 + 5) * 2 - 10 = 20
}

TEST_F(ResultPatternIntegrationTest, ErrorInChain) {
    // Test error propagation through chain
    auto result = Result<int>::ok(5)
        .map([](int x) { return x + 2; })
        .and_then([](int x) -> Result<int> {
            if (x > 10) {
                return Result<int>::ok(x * 2);
            }
            return Result<int>::err(error_code{1, "value too small"});
        })
        .map([](int x) { return x - 10; });

    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error().message, "value too small");
}

TEST_F(ResultPatternIntegrationTest, ValueOrDefault) {
    // Test value_or for default values
    auto success = Result<int>::ok(42);
    auto error = Result<int>::err(error_code{1, "error"});

    EXPECT_EQ(success.value_or(0), 42);
    EXPECT_EQ(error.value_or(99), 99);
}

TEST_F(ResultPatternIntegrationTest, MoveSemantics) {
    // Test move semantics with non-copyable types
    struct NonCopyable {
        std::unique_ptr<int> data;
        explicit NonCopyable(int val) : data(std::make_unique<int>(val)) {}
        NonCopyable(const NonCopyable&) = delete;
        NonCopyable& operator=(const NonCopyable&) = delete;
        NonCopyable(NonCopyable&&) = default;
        NonCopyable& operator=(NonCopyable&&) = default;
    };

    auto result = Result<NonCopyable>::ok(NonCopyable(42));
    EXPECT_TRUE(result.is_ok());
    EXPECT_EQ(*result.value().data, 42);

    // Move result
    auto moved = std::move(result);
    EXPECT_TRUE(moved.is_ok());
    EXPECT_EQ(*moved.value().data, 42);
}

TEST_F(ResultPatternIntegrationTest, ResultWithComplexTypes) {
    // Test Result with complex types
    using ComplexType = std::vector<std::pair<std::string, int>>;

    ComplexType data = {{"first", 1}, {"second", 2}, {"third", 3}};
    auto result = Result<ComplexType>::ok(data);

    EXPECT_TRUE(result.is_ok());
    EXPECT_EQ(result.value().size(), 3);
    EXPECT_EQ(result.value()[0].first, "first");
    EXPECT_EQ(result.value()[0].second, 1);
}

TEST_F(ResultPatternIntegrationTest, ErrorCodeComparison) {
    // Test error code comparison
    error_code err1{100, "error 1"};
    error_code err2{100, "error 2"};
    error_code err3{200, "error 3"};

    EXPECT_EQ(err1.code, err2.code);
    EXPECT_NE(err1.code, err3.code);
}
