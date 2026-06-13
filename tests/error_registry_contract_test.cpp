/**
 * @file error_registry_contract_test.cpp
 * @brief Runtime contract gate for the centralized error-code registry.
 *
 * This test LOCKS the behavioral contract of the error-code registry so that
 * violations cannot silently accumulate (issue #699). It complements the
 * compile-time static_asserts in error_codes.h (which fix the reserved range
 * bases) and the cppm<->header SSOT drift guard
 * (scripts/check_error_registry_ssot.py).
 *
 * Locked invariants:
 *   1. get_category_name(0) == "Success"; any positive code == "Invalid"
 *      (regression lock for issue #698).
 *   2. Each reserved system base maps to the correct category name, probed via
 *      representative real codes from codes::.
 *   3. The reserved ranges are mutually DISJOINT at runtime: no code value maps
 *      to two categories, and the category:: enum bases are strictly ordered.
 *   4. get_error_message returns "Unknown error" (not "Success") for an
 *      unregistered positive code.
 *
 * @date 2026-06-13
 */

#include <gtest/gtest.h>
#include <kcenon/common/error/error_codes.h>

#include <array>
#include <set>
#include <string_view>

using namespace kcenon::common::error;

// ============================================================================
// (1) #698 lock: success sentinel vs positive codes
// ============================================================================

TEST(ErrorRegistryContractTest, SuccessIsOnlyZero)
{
    EXPECT_EQ(get_category_name(0), "Success");
    EXPECT_EQ(get_category_name(codes::common_errors::success), "Success");
}

TEST(ErrorRegistryContractTest, PositiveCodesAreInvalidNotSuccess)
{
    // Regression lock for issue #698: positive codes must never be reported as
    // "Success" (which masked real errors). The registry spans negative codes
    // only, so every positive value is "Invalid".
    for (int code : {1, 2, 42, 100, 1000, 4999, 2147483647})
    {
        EXPECT_EQ(get_category_name(code), "Invalid")
            << "positive code " << code << " must be Invalid";
        EXPECT_NE(get_category_name(code), "Success")
            << "positive code " << code << " must not be Success";
    }
}

// ============================================================================
// (2) Each reserved system base maps to the correct category name, probed via
//     representative real codes from codes::.
// ============================================================================

TEST(ErrorRegistryContractTest, ReservedBasesMapToCorrectCategory)
{
    // Probe the exact base values.
    EXPECT_EQ(get_category_name(-100), "ThreadSystem");
    EXPECT_EQ(get_category_name(-200), "LoggerSystem");
    EXPECT_EQ(get_category_name(-300), "MonitoringSystem");
    EXPECT_EQ(get_category_name(-400), "ContainerSystem");
    EXPECT_EQ(get_category_name(-500), "DatabaseSystem");
    EXPECT_EQ(get_category_name(-600), "NetworkSystem");
    EXPECT_EQ(get_category_name(-700), "PACSSystem");

    // Probe via representative real codes from each system namespace; each must
    // resolve to its own category.
    EXPECT_EQ(get_category_name(codes::common_errors::invalid_argument), "Common");
    EXPECT_EQ(get_category_name(codes::thread_system::pool_full), "ThreadSystem");
    EXPECT_EQ(get_category_name(codes::logger_system::file_open_failed), "LoggerSystem");
    EXPECT_EQ(get_category_name(codes::monitoring_system::metric_not_found), "MonitoringSystem");
    EXPECT_EQ(get_category_name(codes::container_system::value_type_mismatch), "ContainerSystem");
    EXPECT_EQ(get_category_name(codes::database_system::connection_failed), "DatabaseSystem");
    EXPECT_EQ(get_category_name(codes::network_system::connection_failed), "NetworkSystem");
    EXPECT_EQ(get_category_name(codes::pacs_system::file_not_found), "PACSSystem");
}

// ============================================================================
// (3) Reserved ranges are mutually DISJOINT at runtime.
// ============================================================================

TEST(ErrorRegistryContractTest, CategoryEnumBasesAreStrictlyOrdered)
{
    // The classifier ladder in get_category_name relies on the bases being
    // strictly descending. Lock that ordering so a reordered/duplicated enum
    // base is caught here rather than silently misrouting codes.
    EXPECT_GT(static_cast<int>(category::success), static_cast<int>(category::common));
    EXPECT_GT(static_cast<int>(category::common), static_cast<int>(category::thread_system));
    EXPECT_GT(static_cast<int>(category::thread_system), static_cast<int>(category::logger_system));
    EXPECT_GT(static_cast<int>(category::logger_system), static_cast<int>(category::monitoring_system));
    EXPECT_GT(static_cast<int>(category::monitoring_system), static_cast<int>(category::container_system));
    EXPECT_GT(static_cast<int>(category::container_system), static_cast<int>(category::database_system));
    EXPECT_GT(static_cast<int>(category::database_system), static_cast<int>(category::network_system));
    EXPECT_GT(static_cast<int>(category::network_system), static_cast<int>(category::pacs_system));
}

TEST(ErrorRegistryContractTest, ReservedRangesAreDisjoint)
{
    // Every negative code in [-799, -1] must map to exactly one category, and
    // each contiguous 100-wide block must be homogeneous. Walking every value
    // proves no code straddles two categories (no overlap, no gap).
    struct Block { int lo; int hi; std::string_view name; };
    constexpr std::array<Block, 8> blocks{{
        {-99,  -1,  "Common"},
        {-199, -100, "ThreadSystem"},
        {-299, -200, "LoggerSystem"},
        {-399, -300, "MonitoringSystem"},
        {-499, -400, "ContainerSystem"},
        {-599, -500, "DatabaseSystem"},
        {-699, -600, "NetworkSystem"},
        {-799, -700, "PACSSystem"},
    }};

    for (const auto& b : blocks)
    {
        for (int code = b.lo; code <= b.hi; ++code)
        {
            EXPECT_EQ(get_category_name(code), b.name)
                << "code " << code << " must belong to " << b.name;
        }
    }

    // Cross-check: the set of distinct category names seen across the whole
    // negative span equals the eight reserved categories plus the open-ended
    // PACS tail. No code value yields two different names because the function
    // is deterministic, but we assert the full name set to catch a collapsed
    // or duplicated branch.
    std::set<std::string_view> seen;
    for (int code = -1; code >= -799; --code)
    {
        seen.insert(get_category_name(code));
    }
    const std::set<std::string_view> expected{
        "Common", "ThreadSystem", "LoggerSystem", "MonitoringSystem",
        "ContainerSystem", "DatabaseSystem", "NetworkSystem", "PACSSystem"};
    EXPECT_EQ(seen, expected);
}

// ============================================================================
// (4) Unregistered positive codes resolve to "Unknown error", never "Success".
// ============================================================================

TEST(ErrorRegistryContractTest, UnregisteredPositiveCodeIsUnknownError)
{
    // A positive code is never a registered error; get_error_message must fall
    // through to "Unknown error" and must NOT return "Success" (which is the
    // message for code 0 only).
    for (int code : {1, 7, 100, 1000, 4999})
    {
        EXPECT_EQ(get_error_message(code), "Unknown error")
            << "positive code " << code << " must be Unknown error";
        EXPECT_NE(get_error_message(code), "Success")
            << "positive code " << code << " must not be Success";
    }

    // Sanity: code 0 alone is "Success".
    EXPECT_EQ(get_error_message(0), "Success");
}
