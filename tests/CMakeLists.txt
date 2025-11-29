cmake_minimum_required(VERSION 3.16)

# Find GTest (should already be checked by parent)
find_package(GTest REQUIRED)
find_package(Threads REQUIRED)

# Create test executable
add_executable(common_system_tests
    result_test.cpp
    executor_test.cpp
    main.cpp
)

# Link with GTest and common_system
target_link_libraries(common_system_tests PRIVATE
    kcenon::common
    GTest::gtest
    GTest::gtest_main
    Threads::Threads
)

# Set C++ standard
target_compile_features(common_system_tests PRIVATE cxx_std_17)

# Add test
add_test(NAME common_system_tests COMMAND common_system_tests)

# Set test properties
set_tests_properties(common_system_tests PROPERTIES
    TIMEOUT 60
    LABELS "unit"
)

# Event Bus failure tests
add_executable(common_event_bus_failure_test
    event_bus_failure_test.cpp
)

target_link_libraries(common_event_bus_failure_test PRIVATE
    kcenon::common
    GTest::gtest
    GTest::gtest_main
    Threads::Threads
)

target_compile_features(common_event_bus_failure_test PRIVATE cxx_std_17)

add_test(NAME common_event_bus_failure_test COMMAND common_event_bus_failure_test)

set_tests_properties(common_event_bus_failure_test PROPERTIES
    TIMEOUT 120
    LABELS "unit;event_bus"
)

# Thread safety tests (Phase 1 - Task 1.7)
add_executable(common_thread_safety_test
    thread_safety_tests.cpp
)

target_link_libraries(common_thread_safety_test PRIVATE
    kcenon::common
    GTest::gtest
    GTest::gtest_main
    Threads::Threads
)

target_compile_features(common_thread_safety_test PRIVATE cxx_std_17)

add_test(NAME common_thread_safety_test COMMAND common_thread_safety_test)

set_tests_properties(common_thread_safety_test PROPERTIES
    TIMEOUT 120
    LABELS "thread_safety"
)

# Service container tests (TICKET-102)
add_executable(common_service_container_test
    service_container_test.cpp
)

target_link_libraries(common_service_container_test PRIVATE
    kcenon::common
    GTest::gtest
    GTest::gtest_main
    Threads::Threads
)

target_compile_features(common_service_container_test PRIVATE cxx_std_17)

add_test(NAME common_service_container_test COMMAND common_service_container_test)

set_tests_properties(common_service_container_test PROPERTIES
    TIMEOUT 120
    LABELS "unit;di"
)

# ABI version tests (Sprint 2 - Task 2.4)
# Note: These tests require the static library build to test link-time checks
if(NOT COMMON_HEADER_ONLY AND TARGET common_system_static)
    add_executable(common_abi_version_test
        abi_version_test.cpp
    )

    target_link_libraries(common_abi_version_test PRIVATE
        common_system_static
        GTest::gtest
        GTest::gtest_main
        Threads::Threads
    )

    target_compile_features(common_abi_version_test PRIVATE cxx_std_20)

    add_test(NAME common_abi_version_test COMMAND common_abi_version_test)

    set_tests_properties(common_abi_version_test PROPERTIES
        TIMEOUT 60
        LABELS "abi;integration"
    )
endif()

# Enable testing
enable_testing()

# Coverage flags for GCC/Clang
if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    option(ENABLE_COVERAGE "Enable coverage reporting" OFF)
    if(ENABLE_COVERAGE)
        target_compile_options(common_system_tests PRIVATE --coverage -O0 -g)
        target_link_options(common_system_tests PRIVATE --coverage)
    endif()
endif()