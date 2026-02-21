/**
 * @file source_location_test.cpp
 * @brief Unit tests for C++17-compatible source_location implementation.
 *
 * Tests the source_location utility:
 * - current() captures correct file_name(), function_name(), line()
 * - column() returns 0 in C++17 fallback
 * - Behavior as default function argument
 *
 * @date 2026-02-21
 */

#include <gtest/gtest.h>
#include <kcenon/common/utils/source_location.h>
#include <string>

using namespace kcenon::common;

// Helper to capture source_location as default argument
static source_location capture_location(
    const source_location& loc = source_location::current())
{
    return loc;
}

// Test current() captures correct file_name
TEST(SourceLocationTest, CurrentCapturesFileName)
{
    auto loc = source_location::current();
    std::string file = loc.file_name();

    // Should contain this test file's name
    EXPECT_NE(file.find("source_location_test.cpp"), std::string::npos)
        << "file_name() returned: " << file;
}

// Test current() captures correct function_name
TEST(SourceLocationTest, CurrentCapturesFunctionName)
{
    auto loc = source_location::current();
    std::string func = loc.function_name();

    // Should contain the test function name
    EXPECT_FALSE(func.empty());
    // GTest wraps test functions; the exact name varies by compiler
    // but it should be non-empty and meaningful
}

// Test current() captures correct line number
TEST(SourceLocationTest, CurrentCapturesLineNumber)
{
    int expected_line = __LINE__ + 1;
    auto loc = source_location::current();

    EXPECT_EQ(static_cast<int>(loc.line()), expected_line);
}

// Test column() returns 0 in C++17 fallback
TEST(SourceLocationTest, ColumnReturnsZeroInFallback)
{
    auto loc = source_location::current();

#if KCENON_HAS_SOURCE_LOCATION
    // C++20 std::source_location may return non-zero column
    // Just verify it's callable
    (void)loc.column();
#else
    // C++17 fallback should always return 0
    EXPECT_EQ(loc.column(), 0);
#endif
}

// Test behavior as default function argument
TEST(SourceLocationTest, DefaultFunctionArgument)
{
    int call_line = __LINE__ + 1;
    auto loc = capture_location();

    // Should capture the caller's location, not the callee's
    std::string file = loc.file_name();
    EXPECT_NE(file.find("source_location_test.cpp"), std::string::npos);
    EXPECT_EQ(static_cast<int>(loc.line()), call_line);
}

// Test explicit source_location passed as argument
TEST(SourceLocationTest, ExplicitArgumentOverridesDefault)
{
    auto explicit_loc = source_location::current();
    int explicit_line = __LINE__ - 1;

    auto received_loc = capture_location(explicit_loc);

    EXPECT_EQ(static_cast<int>(received_loc.line()), explicit_line);
}

// Test source_location in different scopes
TEST(SourceLocationTest, DifferentLinesCaptureDifferentLocations)
{
    auto loc1 = source_location::current();
    auto loc2 = source_location::current();

    // Two consecutive calls should have different line numbers
    EXPECT_NE(loc1.line(), loc2.line());
    // But same file
    EXPECT_STREQ(loc1.file_name(), loc2.file_name());
}
