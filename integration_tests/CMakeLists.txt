cmake_minimum_required(VERSION 3.16)

project(CommonSystemIntegrationTests)

# Find GTest
# Note: If not found, parent CMakeLists.txt may provide it via FetchContent
find_package(GTest QUIET)

# Integration test framework library (shared utilities)
add_library(integration_test_framework INTERFACE)
target_include_directories(integration_test_framework INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/framework
)
target_link_libraries(integration_test_framework INTERFACE
    GTest::gtest
    GTest::gtest_main
)

# Collect all integration test sources
file(GLOB SCENARIO_TESTS scenarios/*.cpp)
file(GLOB FAILURE_TESTS failures/*.cpp)
file(GLOB PERFORMANCE_TESTS performance/*.cpp)
file(GLOB STRESS_TESTS stress/*.cpp)

# Add integration test executable
add_executable(integration_tests
    ${SCENARIO_TESTS}
    ${FAILURE_TESTS}
    ${PERFORMANCE_TESTS}
    ${STRESS_TESTS}
)

target_link_libraries(integration_tests PRIVATE
    integration_test_framework
    kcenon::common
)

# Set C++20 standard
target_compile_features(integration_tests PRIVATE cxx_std_20)

# Register with CTest
include(GoogleTest)
gtest_discover_tests(integration_tests)

# Enable testing
enable_testing()
