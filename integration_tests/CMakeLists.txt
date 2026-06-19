cmake_minimum_required(VERSION 3.16)

project(CommonSystemIntegrationTests)

# =============================================================================
# STRICT WARNING FLAGS
# =============================================================================
# Enable strict warning flags to ensure code quality.
# This aligns with the unified_system policy to eliminate warning suppressions.
# See: https://github.com/kcenon/common_system/issues/245
# =============================================================================

# Compiler-specific warning flags
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    set(COMMON_WARNING_FLAGS
        -Wall
        -Wextra
        -Wpedantic
    )
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(COMMON_WARNING_FLAGS
        /W4
    )
endif()

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

# Collect all integration test sources.
# Note: enumerated explicitly per layout standard rule R-19 (no file(GLOB)).
# Add new files here when extending integration coverage.
set(SCENARIO_TESTS
    scenarios/event_bus_integration_test.cpp
    scenarios/full_system_integration_test.cpp
    scenarios/result_pattern_integration_test.cpp
    scenarios/runtime_binding_integration_test.cpp
)
set(FAILURE_TESTS
    failures/error_handling_test.cpp
)
set(PERFORMANCE_TESTS
    performance/memory_pressure_test.cpp
    performance/result_performance_test.cpp
)
set(STRESS_TESTS
    stress/stress_test.cpp
)

# Add integration test executable (renamed to avoid conflicts with other systems)
add_executable(common_integration_tests
    ${SCENARIO_TESTS}
    ${FAILURE_TESTS}
    ${PERFORMANCE_TESTS}
    ${STRESS_TESTS}
)

target_link_libraries(common_integration_tests PRIVATE
    integration_test_framework
    kcenon::common
)

# Set C++20 standard
target_compile_features(common_integration_tests PRIVATE cxx_std_20)
set_target_properties(common_integration_tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Register with CTest
include(GoogleTest)
gtest_discover_tests(common_integration_tests)

# =============================================================================
# APPLY STRICT WARNING FLAGS
# =============================================================================
# Apply warning flags to integration test executable
target_compile_options(common_integration_tests PRIVATE ${COMMON_WARNING_FLAGS})
