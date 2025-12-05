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

# Unified bootstrapper tests (TICKET-104)
add_executable(common_unified_bootstrapper_test
    unified_bootstrapper_test.cpp
)

target_link_libraries(common_unified_bootstrapper_test PRIVATE
    kcenon::common
    GTest::gtest
    GTest::gtest_main
    Threads::Threads
)

target_compile_features(common_unified_bootstrapper_test PRIVATE cxx_std_17)

add_test(NAME common_unified_bootstrapper_test COMMAND common_unified_bootstrapper_test)

set_tests_properties(common_unified_bootstrapper_test PROPERTIES
    TIMEOUT 120
    LABELS "unit;di"
)

# Unified config tests (TICKET-201)
add_executable(common_unified_config_test
    unified_config_test.cpp
)

target_link_libraries(common_unified_config_test PRIVATE
    kcenon::common
    GTest::gtest
    GTest::gtest_main
    Threads::Threads
)

target_compile_features(common_unified_config_test PRIVATE cxx_std_17)

add_test(NAME common_unified_config_test COMMAND common_unified_config_test)

set_tests_properties(common_unified_config_test PROPERTIES
    TIMEOUT 60
    LABELS "unit;config"
)

# Config loader tests (TICKET-202)
add_executable(common_config_loader_test
    config_loader_test.cpp
)

target_link_libraries(common_config_loader_test PRIVATE
    kcenon::common
    GTest::gtest
    GTest::gtest_main
    Threads::Threads
)

# Optionally link yaml-cpp if available
if(BUILD_WITH_YAML_CPP)
    find_package(yaml-cpp QUIET)
    if(yaml-cpp_FOUND)
        target_link_libraries(common_config_loader_test PRIVATE yaml-cpp::yaml-cpp)
        target_compile_definitions(common_config_loader_test PRIVATE BUILD_WITH_YAML_CPP)
    endif()
endif()

target_compile_features(common_config_loader_test PRIVATE cxx_std_17)

add_test(NAME common_config_loader_test COMMAND common_config_loader_test)

set_tests_properties(common_config_loader_test PROPERTIES
    TIMEOUT 60
    LABELS "unit;config"
)

# CLI config parser tests (TICKET-204)
add_executable(common_cli_config_parser_test
    cli_config_parser_test.cpp
)

target_link_libraries(common_cli_config_parser_test PRIVATE
    kcenon::common
    GTest::gtest
    GTest::gtest_main
    Threads::Threads
)

target_compile_features(common_cli_config_parser_test PRIVATE cxx_std_17)

add_test(NAME common_cli_config_parser_test COMMAND common_cli_config_parser_test)

set_tests_properties(common_cli_config_parser_test PROPERTIES
    TIMEOUT 60
    LABELS "unit;config;cli"
)

# Config watcher tests (TICKET-203)
add_executable(common_config_watcher_test
    config_watcher_test.cpp
)

target_link_libraries(common_config_watcher_test PRIVATE
    kcenon::common
    GTest::gtest
    GTest::gtest_main
    Threads::Threads
)

# Optionally link yaml-cpp if available
if(BUILD_WITH_YAML_CPP)
    find_package(yaml-cpp QUIET)
    if(yaml-cpp_FOUND)
        target_link_libraries(common_config_watcher_test PRIVATE yaml-cpp::yaml-cpp)
        target_compile_definitions(common_config_watcher_test PRIVATE BUILD_WITH_YAML_CPP)
    endif()
endif()

target_compile_features(common_config_watcher_test PRIVATE cxx_std_17)

add_test(NAME common_config_watcher_test COMMAND common_config_watcher_test)

set_tests_properties(common_config_watcher_test PROPERTIES
    TIMEOUT 120
    LABELS "unit;config;watcher"
)

# Global logger registry tests (Issue #174)
add_executable(common_global_logger_registry_test
    global_logger_registry_test.cpp
)

target_link_libraries(common_global_logger_registry_test PRIVATE
    kcenon::common
    GTest::gtest
    GTest::gtest_main
    Threads::Threads
)

target_compile_features(common_global_logger_registry_test PRIVATE cxx_std_17)

add_test(NAME common_global_logger_registry_test COMMAND common_global_logger_registry_test)

set_tests_properties(common_global_logger_registry_test PROPERTIES
    TIMEOUT 120
    LABELS "unit;interfaces;logging"
)

# Unified logging functions and macros tests (Issue #175)
add_executable(common_log_functions_test
    log_functions_test.cpp
)

target_link_libraries(common_log_functions_test PRIVATE
    kcenon::common
    GTest::gtest
    GTest::gtest_main
    Threads::Threads
)

target_compile_features(common_log_functions_test PRIVATE cxx_std_17)

add_test(NAME common_log_functions_test COMMAND common_log_functions_test)

set_tests_properties(common_log_functions_test PROPERTIES
    TIMEOUT 120
    LABELS "unit;logging"
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