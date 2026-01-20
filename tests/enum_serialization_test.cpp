// BSD 3-Clause License
// Copyright (c) 2025, kcenon
// See the LICENSE file in the project root for full license information.

/**
 * @file enum_serialization_test.cpp
 * @brief Unit tests for generic enum serialization utilities
 *
 * Tests the enum_traits template and enum_to_string/enum_from_string functions
 * for all supported enum types.
 *
 * @note Issue #293: Added tests for generic enum serialization template.
 */

#include <gtest/gtest.h>

#include <kcenon/common/utils/enum_serialization.h>
#include <kcenon/common/interfaces/logger_interface.h>
#include <kcenon/common/interfaces/monitoring_interface.h>
#include <kcenon/common/interfaces/monitoring/health_check.h>

using namespace kcenon::common;
using namespace kcenon::common::interfaces;

// =============================================================================
// EnumSerializable Concept Tests
// =============================================================================

static_assert(EnumSerializable<log_level>,
              "log_level should satisfy EnumSerializable concept");
static_assert(EnumSerializable<metric_type>,
              "metric_type should satisfy EnumSerializable concept");
static_assert(EnumSerializable<health_status>,
              "health_status should satisfy EnumSerializable concept");
static_assert(EnumSerializable<health_check_type>,
              "health_check_type should satisfy EnumSerializable concept");

// Negative test: regular enums without traits should not satisfy the concept
enum class unregistered_enum { value1, value2 };
static_assert(!EnumSerializable<unregistered_enum>,
              "unregistered_enum should NOT satisfy EnumSerializable concept");

// =============================================================================
// log_level Serialization Tests
// =============================================================================

class LogLevelSerializationTest : public ::testing::Test {};

TEST_F(LogLevelSerializationTest, ToStringConvertsAllValues) {
    EXPECT_EQ(enum_to_string(log_level::trace), "TRACE");
    EXPECT_EQ(enum_to_string(log_level::debug), "DEBUG");
    EXPECT_EQ(enum_to_string(log_level::info), "INFO");
    EXPECT_EQ(enum_to_string(log_level::warning), "WARNING");
    EXPECT_EQ(enum_to_string(log_level::error), "ERROR");
    EXPECT_EQ(enum_to_string(log_level::critical), "CRITICAL");
    EXPECT_EQ(enum_to_string(log_level::off), "OFF");
}

TEST_F(LogLevelSerializationTest, ToStringHandlesAliases) {
    // warn is an alias for warning (same numeric value)
    EXPECT_EQ(enum_to_string(log_level::warn), "WARNING");
    // fatal is an alias for critical (same numeric value)
    EXPECT_EQ(enum_to_string(log_level::fatal), "CRITICAL");
}

TEST_F(LogLevelSerializationTest, FromStringParsesValidValues) {
    auto trace_result = enum_from_string<log_level>("TRACE");
    ASSERT_TRUE(trace_result.is_ok());
    EXPECT_EQ(trace_result.value(), log_level::trace);

    auto debug_result = enum_from_string<log_level>("DEBUG");
    ASSERT_TRUE(debug_result.is_ok());
    EXPECT_EQ(debug_result.value(), log_level::debug);

    auto info_result = enum_from_string<log_level>("INFO");
    ASSERT_TRUE(info_result.is_ok());
    EXPECT_EQ(info_result.value(), log_level::info);

    auto warning_result = enum_from_string<log_level>("WARNING");
    ASSERT_TRUE(warning_result.is_ok());
    EXPECT_EQ(warning_result.value(), log_level::warning);

    auto error_result = enum_from_string<log_level>("ERROR");
    ASSERT_TRUE(error_result.is_ok());
    EXPECT_EQ(error_result.value(), log_level::error);

    auto critical_result = enum_from_string<log_level>("CRITICAL");
    ASSERT_TRUE(critical_result.is_ok());
    EXPECT_EQ(critical_result.value(), log_level::critical);

    auto off_result = enum_from_string<log_level>("OFF");
    ASSERT_TRUE(off_result.is_ok());
    EXPECT_EQ(off_result.value(), log_level::off);
}

TEST_F(LogLevelSerializationTest, FromStringIsCaseInsensitive) {
    auto lower = enum_from_string<log_level>("debug");
    ASSERT_TRUE(lower.is_ok());
    EXPECT_EQ(lower.value(), log_level::debug);

    auto mixed = enum_from_string<log_level>("DeBuG");
    ASSERT_TRUE(mixed.is_ok());
    EXPECT_EQ(mixed.value(), log_level::debug);
}

TEST_F(LogLevelSerializationTest, FromStringReturnsErrorForInvalidValues) {
    auto result = enum_from_string<log_level>("INVALID");
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error().module, "logger_interface");
}

TEST_F(LogLevelSerializationTest, LegacyToStringWorks) {
    EXPECT_EQ(to_string(log_level::info), "INFO");
    EXPECT_EQ(to_string(log_level::error), "ERROR");
}

TEST_F(LogLevelSerializationTest, LegacyFromStringWorks) {
    EXPECT_EQ(from_string("INFO"), log_level::info);
    EXPECT_EQ(from_string("ERROR"), log_level::error);
    // Aliases should work
    EXPECT_EQ(from_string("WARN"), log_level::warning);
    EXPECT_EQ(from_string("FATAL"), log_level::critical);
    // Invalid returns default (info)
    EXPECT_EQ(from_string("INVALID"), log_level::info);
}

// =============================================================================
// metric_type Serialization Tests
// =============================================================================

class MetricTypeSerializationTest : public ::testing::Test {};

TEST_F(MetricTypeSerializationTest, ToStringConvertsAllValues) {
    EXPECT_EQ(enum_to_string(metric_type::gauge), "GAUGE");
    EXPECT_EQ(enum_to_string(metric_type::counter), "COUNTER");
    EXPECT_EQ(enum_to_string(metric_type::histogram), "HISTOGRAM");
    EXPECT_EQ(enum_to_string(metric_type::summary), "SUMMARY");
}

TEST_F(MetricTypeSerializationTest, FromStringParsesValidValues) {
    auto gauge_result = enum_from_string<metric_type>("GAUGE");
    ASSERT_TRUE(gauge_result.is_ok());
    EXPECT_EQ(gauge_result.value(), metric_type::gauge);

    auto counter_result = enum_from_string<metric_type>("COUNTER");
    ASSERT_TRUE(counter_result.is_ok());
    EXPECT_EQ(counter_result.value(), metric_type::counter);

    auto histogram_result = enum_from_string<metric_type>("HISTOGRAM");
    ASSERT_TRUE(histogram_result.is_ok());
    EXPECT_EQ(histogram_result.value(), metric_type::histogram);

    auto summary_result = enum_from_string<metric_type>("SUMMARY");
    ASSERT_TRUE(summary_result.is_ok());
    EXPECT_EQ(summary_result.value(), metric_type::summary);
}

TEST_F(MetricTypeSerializationTest, FromStringIsCaseInsensitive) {
    auto lower = enum_from_string<metric_type>("gauge");
    ASSERT_TRUE(lower.is_ok());
    EXPECT_EQ(lower.value(), metric_type::gauge);
}

TEST_F(MetricTypeSerializationTest, FromStringReturnsErrorForInvalidValues) {
    auto result = enum_from_string<metric_type>("INVALID");
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error().module, "monitoring_interface");
}

TEST_F(MetricTypeSerializationTest, LegacyFunctionsWork) {
    EXPECT_EQ(to_string(metric_type::gauge), "GAUGE");
    auto result = metric_type_from_string("COUNTER");
    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value(), metric_type::counter);
}

// =============================================================================
// health_status Serialization Tests
// =============================================================================

class HealthStatusSerializationTest : public ::testing::Test {};

TEST_F(HealthStatusSerializationTest, ToStringConvertsAllValues) {
    EXPECT_EQ(enum_to_string(health_status::healthy), "HEALTHY");
    EXPECT_EQ(enum_to_string(health_status::degraded), "DEGRADED");
    EXPECT_EQ(enum_to_string(health_status::unhealthy), "UNHEALTHY");
    EXPECT_EQ(enum_to_string(health_status::unknown), "UNKNOWN");
}

TEST_F(HealthStatusSerializationTest, FromStringParsesValidValues) {
    auto healthy_result = enum_from_string<health_status>("HEALTHY");
    ASSERT_TRUE(healthy_result.is_ok());
    EXPECT_EQ(healthy_result.value(), health_status::healthy);

    auto degraded_result = enum_from_string<health_status>("DEGRADED");
    ASSERT_TRUE(degraded_result.is_ok());
    EXPECT_EQ(degraded_result.value(), health_status::degraded);

    auto unhealthy_result = enum_from_string<health_status>("UNHEALTHY");
    ASSERT_TRUE(unhealthy_result.is_ok());
    EXPECT_EQ(unhealthy_result.value(), health_status::unhealthy);

    auto unknown_result = enum_from_string<health_status>("UNKNOWN");
    ASSERT_TRUE(unknown_result.is_ok());
    EXPECT_EQ(unknown_result.value(), health_status::unknown);
}

TEST_F(HealthStatusSerializationTest, FromStringIsCaseInsensitive) {
    auto lower = enum_from_string<health_status>("healthy");
    ASSERT_TRUE(lower.is_ok());
    EXPECT_EQ(lower.value(), health_status::healthy);
}

TEST_F(HealthStatusSerializationTest, LegacyToStringWorks) {
    EXPECT_EQ(to_string(health_status::healthy), "HEALTHY");
    EXPECT_EQ(to_string(health_status::unhealthy), "UNHEALTHY");
}

// =============================================================================
// health_check_type Serialization Tests
// =============================================================================

class HealthCheckTypeSerializationTest : public ::testing::Test {};

TEST_F(HealthCheckTypeSerializationTest, ToStringConvertsAllValues) {
    EXPECT_EQ(enum_to_string(health_check_type::liveness), "LIVENESS");
    EXPECT_EQ(enum_to_string(health_check_type::readiness), "READINESS");
    EXPECT_EQ(enum_to_string(health_check_type::startup), "STARTUP");
    EXPECT_EQ(enum_to_string(health_check_type::dependency), "DEPENDENCY");
    EXPECT_EQ(enum_to_string(health_check_type::custom), "CUSTOM");
}

TEST_F(HealthCheckTypeSerializationTest, FromStringParsesValidValues) {
    auto liveness_result = enum_from_string<health_check_type>("LIVENESS");
    ASSERT_TRUE(liveness_result.is_ok());
    EXPECT_EQ(liveness_result.value(), health_check_type::liveness);

    auto readiness_result = enum_from_string<health_check_type>("READINESS");
    ASSERT_TRUE(readiness_result.is_ok());
    EXPECT_EQ(readiness_result.value(), health_check_type::readiness);

    auto startup_result = enum_from_string<health_check_type>("STARTUP");
    ASSERT_TRUE(startup_result.is_ok());
    EXPECT_EQ(startup_result.value(), health_check_type::startup);

    auto dependency_result = enum_from_string<health_check_type>("DEPENDENCY");
    ASSERT_TRUE(dependency_result.is_ok());
    EXPECT_EQ(dependency_result.value(), health_check_type::dependency);

    auto custom_result = enum_from_string<health_check_type>("CUSTOM");
    ASSERT_TRUE(custom_result.is_ok());
    EXPECT_EQ(custom_result.value(), health_check_type::custom);
}

TEST_F(HealthCheckTypeSerializationTest, FromStringIsCaseInsensitive) {
    auto lower = enum_from_string<health_check_type>("liveness");
    ASSERT_TRUE(lower.is_ok());
    EXPECT_EQ(lower.value(), health_check_type::liveness);
}

TEST_F(HealthCheckTypeSerializationTest, FromStringReturnsErrorForInvalidValues) {
    auto result = enum_from_string<health_check_type>("INVALID");
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error().module, "health_check");
}

TEST_F(HealthCheckTypeSerializationTest, LegacyFunctionsWork) {
    EXPECT_EQ(to_string(health_check_type::liveness), "LIVENESS");
    auto result = health_check_type_from_string("READINESS");
    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(result.value(), health_check_type::readiness);
}

// =============================================================================
// Round-trip Tests
// =============================================================================

class RoundTripSerializationTest : public ::testing::Test {};

TEST_F(RoundTripSerializationTest, LogLevelRoundTrip) {
    for (auto level : {log_level::trace, log_level::debug, log_level::info,
                       log_level::warning, log_level::error, log_level::critical,
                       log_level::off}) {
        auto str = enum_to_string(level);
        auto result = enum_from_string<log_level>(str);
        ASSERT_TRUE(result.is_ok()) << "Failed for " << str;
        EXPECT_EQ(result.value(), level) << "Round-trip failed for " << str;
    }
}

TEST_F(RoundTripSerializationTest, MetricTypeRoundTrip) {
    for (auto type : {metric_type::gauge, metric_type::counter,
                      metric_type::histogram, metric_type::summary}) {
        auto str = enum_to_string(type);
        auto result = enum_from_string<metric_type>(str);
        ASSERT_TRUE(result.is_ok()) << "Failed for " << str;
        EXPECT_EQ(result.value(), type) << "Round-trip failed for " << str;
    }
}

TEST_F(RoundTripSerializationTest, HealthStatusRoundTrip) {
    for (auto status : {health_status::healthy, health_status::degraded,
                        health_status::unhealthy, health_status::unknown}) {
        auto str = enum_to_string(status);
        auto result = enum_from_string<health_status>(str);
        ASSERT_TRUE(result.is_ok()) << "Failed for " << str;
        EXPECT_EQ(result.value(), status) << "Round-trip failed for " << str;
    }
}

TEST_F(RoundTripSerializationTest, HealthCheckTypeRoundTrip) {
    for (auto type : {health_check_type::liveness, health_check_type::readiness,
                      health_check_type::startup, health_check_type::dependency,
                      health_check_type::custom}) {
        auto str = enum_to_string(type);
        auto result = enum_from_string<health_check_type>(str);
        ASSERT_TRUE(result.is_ok()) << "Failed for " << str;
        EXPECT_EQ(result.value(), type) << "Round-trip failed for " << str;
    }
}
