/**
 * @file error_codes_test.cpp
 * @brief Unit tests for error code registry and helper functions.
 *
 * Tests the centralized error code system:
 * - get_error_message() for all common_errors range
 * - get_error_message() for each system range boundary
 * - get_error_message() for unknown codes
 * - get_category_name() boundary tests for all 8 category ranges
 *
 * @date 2026-02-21
 */

#include <gtest/gtest.h>
#include <kcenon/common/error/error_codes.h>
#include <string>

using namespace kcenon::common::error;

// ============================================================================
// get_error_message: Common errors (-1 to -99)
// ============================================================================

TEST(ErrorCodesTest, GetErrorMessageSuccess)
{
    EXPECT_EQ(get_error_message(codes::common_errors::success), "Success");
}

TEST(ErrorCodesTest, GetErrorMessageCommonErrors)
{
    EXPECT_EQ(get_error_message(codes::common_errors::invalid_argument), "Invalid argument");
    EXPECT_EQ(get_error_message(codes::common_errors::not_found), "Not found");
    EXPECT_EQ(get_error_message(codes::common_errors::permission_denied), "Permission denied");
    EXPECT_EQ(get_error_message(codes::common_errors::timeout), "Timeout");
    EXPECT_EQ(get_error_message(codes::common_errors::cancelled), "Cancelled");
    EXPECT_EQ(get_error_message(codes::common_errors::not_initialized), "Not initialized");
    EXPECT_EQ(get_error_message(codes::common_errors::already_exists), "Already exists");
    EXPECT_EQ(get_error_message(codes::common_errors::out_of_memory), "Out of memory");
    EXPECT_EQ(get_error_message(codes::common_errors::io_error), "I/O error");
    EXPECT_EQ(get_error_message(codes::common_errors::network_error), "Network error");
    EXPECT_EQ(get_error_message(codes::common_errors::registry_frozen), "Registry is frozen");
    EXPECT_EQ(get_error_message(codes::common_errors::internal_error), "Internal error");
}

// ============================================================================
// get_error_message: thread_system errors
// ============================================================================

TEST(ErrorCodesTest, GetErrorMessageThreadSystem)
{
    EXPECT_EQ(get_error_message(codes::thread_system::pool_full), "Thread pool full");
    EXPECT_EQ(get_error_message(codes::thread_system::pool_shutdown), "Thread pool shutdown");
    EXPECT_EQ(get_error_message(codes::thread_system::job_rejected), "Job rejected");
    EXPECT_EQ(get_error_message(codes::thread_system::job_timeout), "Job timeout");
}

// ============================================================================
// get_error_message: logger_system errors
// ============================================================================

TEST(ErrorCodesTest, GetErrorMessageLoggerSystem)
{
    EXPECT_EQ(get_error_message(codes::logger_system::file_open_failed), "Failed to open log file");
    EXPECT_EQ(get_error_message(codes::logger_system::file_write_failed), "Failed to write to log file");
    EXPECT_EQ(get_error_message(codes::logger_system::file_rotation_failed), "Log file rotation failed");
}

// ============================================================================
// get_error_message: monitoring_system errors
// ============================================================================

TEST(ErrorCodesTest, GetErrorMessageMonitoringSystem)
{
    EXPECT_EQ(get_error_message(codes::monitoring_system::metric_not_found), "Metric not found");
    EXPECT_EQ(get_error_message(codes::monitoring_system::storage_full), "Metric storage full");
}

// ============================================================================
// get_error_message: container_system errors
// ============================================================================

TEST(ErrorCodesTest, GetErrorMessageContainerSystem)
{
    EXPECT_EQ(get_error_message(codes::container_system::value_type_mismatch), "Value type mismatch");
    EXPECT_EQ(get_error_message(codes::container_system::serialization_failed), "Serialization failed");
    EXPECT_EQ(get_error_message(codes::container_system::pool_exhausted), "Memory pool exhausted");
}

// ============================================================================
// get_error_message: database_system errors
// ============================================================================

TEST(ErrorCodesTest, GetErrorMessageDatabaseSystem)
{
    EXPECT_EQ(get_error_message(codes::database_system::connection_failed), "Database connection failed");
    EXPECT_EQ(get_error_message(codes::database_system::pool_exhausted), "Connection pool exhausted");
    EXPECT_EQ(get_error_message(codes::database_system::query_failed), "Database query failed");
}

// ============================================================================
// get_error_message: network_system errors
// ============================================================================

TEST(ErrorCodesTest, GetErrorMessageNetworkSystem)
{
    EXPECT_EQ(get_error_message(codes::network_system::connection_failed), "Network connection failed");
    EXPECT_EQ(get_error_message(codes::network_system::send_failed), "Network send failed");
    EXPECT_EQ(get_error_message(codes::network_system::server_not_started), "Server not started");
}

// ============================================================================
// get_error_message: pacs_system errors
// ============================================================================

TEST(ErrorCodesTest, GetErrorMessagePacsSystem)
{
    EXPECT_EQ(get_error_message(codes::pacs_system::file_not_found), "DICOM file not found");
    EXPECT_EQ(get_error_message(codes::pacs_system::file_read_error), "Failed to read DICOM file");
    EXPECT_EQ(get_error_message(codes::pacs_system::file_write_error), "Failed to write DICOM file");
    EXPECT_EQ(get_error_message(codes::pacs_system::invalid_dicom_file), "Invalid DICOM file format");
    EXPECT_EQ(get_error_message(codes::pacs_system::missing_dicm_prefix), "Missing DICM prefix");
    EXPECT_EQ(get_error_message(codes::pacs_system::invalid_meta_info), "Invalid File Meta Information");
    EXPECT_EQ(get_error_message(codes::pacs_system::missing_transfer_syntax), "Missing Transfer Syntax");
    EXPECT_EQ(get_error_message(codes::pacs_system::unsupported_transfer_syntax), "Unsupported Transfer Syntax");
    EXPECT_EQ(get_error_message(codes::pacs_system::element_not_found), "DICOM element not found");
    EXPECT_EQ(get_error_message(codes::pacs_system::value_conversion_error), "Value conversion failed");
    EXPECT_EQ(get_error_message(codes::pacs_system::decode_error), "DICOM decode error");
    EXPECT_EQ(get_error_message(codes::pacs_system::encode_error), "DICOM encode error");
    EXPECT_EQ(get_error_message(codes::pacs_system::association_rejected), "DICOM association rejected");
    EXPECT_EQ(get_error_message(codes::pacs_system::storage_failed), "DICOM storage failed");
}

// ============================================================================
// get_error_message: Unknown error codes
// ============================================================================

TEST(ErrorCodesTest, GetErrorMessageUnknownCode)
{
    EXPECT_EQ(get_error_message(-999), "Unknown error");
    EXPECT_EQ(get_error_message(-1000), "Unknown error");
    EXPECT_EQ(get_error_message(999), "Unknown error");
    EXPECT_EQ(get_error_message(-50), "Unknown error");
}

// ============================================================================
// get_category_name: All category ranges
// ============================================================================

TEST(ErrorCodesTest, GetCategoryNameSuccess)
{
    EXPECT_EQ(get_category_name(0), "Success");
    EXPECT_EQ(get_category_name(1), "Success");
    EXPECT_EQ(get_category_name(100), "Success");
}

TEST(ErrorCodesTest, GetCategoryNameCommon)
{
    // Common range: -1 to -99
    EXPECT_EQ(get_category_name(-1), "Common");
    EXPECT_EQ(get_category_name(-50), "Common");
    EXPECT_EQ(get_category_name(-99), "Common");
}

TEST(ErrorCodesTest, GetCategoryNameThreadSystem)
{
    // ThreadSystem range: -100 to -199
    EXPECT_EQ(get_category_name(-100), "ThreadSystem");
    EXPECT_EQ(get_category_name(-150), "ThreadSystem");
    EXPECT_EQ(get_category_name(-199), "ThreadSystem");
}

TEST(ErrorCodesTest, GetCategoryNameLoggerSystem)
{
    // LoggerSystem range: -200 to -299
    EXPECT_EQ(get_category_name(-200), "LoggerSystem");
    EXPECT_EQ(get_category_name(-250), "LoggerSystem");
    EXPECT_EQ(get_category_name(-299), "LoggerSystem");
}

TEST(ErrorCodesTest, GetCategoryNameMonitoringSystem)
{
    // MonitoringSystem range: -300 to -399
    EXPECT_EQ(get_category_name(-300), "MonitoringSystem");
    EXPECT_EQ(get_category_name(-350), "MonitoringSystem");
    EXPECT_EQ(get_category_name(-399), "MonitoringSystem");
}

TEST(ErrorCodesTest, GetCategoryNameContainerSystem)
{
    // ContainerSystem range: -400 to -499
    EXPECT_EQ(get_category_name(-400), "ContainerSystem");
    EXPECT_EQ(get_category_name(-450), "ContainerSystem");
    EXPECT_EQ(get_category_name(-499), "ContainerSystem");
}

TEST(ErrorCodesTest, GetCategoryNameDatabaseSystem)
{
    // DatabaseSystem range: -500 to -599
    EXPECT_EQ(get_category_name(-500), "DatabaseSystem");
    EXPECT_EQ(get_category_name(-550), "DatabaseSystem");
    EXPECT_EQ(get_category_name(-599), "DatabaseSystem");
}

TEST(ErrorCodesTest, GetCategoryNameNetworkSystem)
{
    // NetworkSystem range: -600 to -699
    EXPECT_EQ(get_category_name(-600), "NetworkSystem");
    EXPECT_EQ(get_category_name(-650), "NetworkSystem");
    EXPECT_EQ(get_category_name(-699), "NetworkSystem");
}

TEST(ErrorCodesTest, GetCategoryNamePACSSystem)
{
    // PACSSystem range: -700 and beyond
    EXPECT_EQ(get_category_name(-700), "PACSSystem");
    EXPECT_EQ(get_category_name(-750), "PACSSystem");
    EXPECT_EQ(get_category_name(-799), "PACSSystem");
    EXPECT_EQ(get_category_name(-1000), "PACSSystem");
}

// ============================================================================
// get_category_name: Boundary values between categories
// ============================================================================

TEST(ErrorCodesTest, GetCategoryNameBoundaryValues)
{
    // Exact boundary between Common and ThreadSystem
    EXPECT_EQ(get_category_name(-99), "Common");
    EXPECT_EQ(get_category_name(-100), "ThreadSystem");

    // Exact boundary between ThreadSystem and LoggerSystem
    EXPECT_EQ(get_category_name(-199), "ThreadSystem");
    EXPECT_EQ(get_category_name(-200), "LoggerSystem");

    // Exact boundary between LoggerSystem and MonitoringSystem
    EXPECT_EQ(get_category_name(-299), "LoggerSystem");
    EXPECT_EQ(get_category_name(-300), "MonitoringSystem");

    // Exact boundary between MonitoringSystem and ContainerSystem
    EXPECT_EQ(get_category_name(-399), "MonitoringSystem");
    EXPECT_EQ(get_category_name(-400), "ContainerSystem");

    // Exact boundary between ContainerSystem and DatabaseSystem
    EXPECT_EQ(get_category_name(-499), "ContainerSystem");
    EXPECT_EQ(get_category_name(-500), "DatabaseSystem");

    // Exact boundary between DatabaseSystem and NetworkSystem
    EXPECT_EQ(get_category_name(-599), "DatabaseSystem");
    EXPECT_EQ(get_category_name(-600), "NetworkSystem");

    // Exact boundary between NetworkSystem and PACSSystem
    EXPECT_EQ(get_category_name(-699), "NetworkSystem");
    EXPECT_EQ(get_category_name(-700), "PACSSystem");
}
