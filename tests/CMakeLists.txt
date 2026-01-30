cmake_minimum_required(VERSION 3.16)

# =============================================================================
# STRICT WARNING FLAGS
# =============================================================================
# Enable strict warning flags to ensure code quality.
# This aligns with the unified_system policy to eliminate warning suppressions.
# See: https://github.com/kcenon/common_system/issues/245
#
# All code should compile without warnings when using these flags.
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

# Find GTest (should already be checked by parent)
find_package(GTest REQUIRED)
find_package(Threads REQUIRED)

# Create test executable
add_executable(common_system_tests
    improved_result_test.cpp
    executor_test.cpp
    improved_event_bus_test.cpp
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

# SystemBootstrapper tests (Issue #176)
add_executable(common_system_bootstrapper_test
    system_bootstrapper_test.cpp
)

target_link_libraries(common_system_bootstrapper_test PRIVATE
    kcenon::common
    GTest::gtest
    GTest::gtest_main
    Threads::Threads
)

target_compile_features(common_system_bootstrapper_test PRIVATE cxx_std_17)

add_test(NAME common_system_bootstrapper_test COMMAND common_system_bootstrapper_test)

set_tests_properties(common_system_bootstrapper_test PROPERTIES
    TIMEOUT 120
    LABELS "unit;bootstrap"
)

# Security controls tests (Issue #206)
add_executable(common_security_controls_test
    security_controls_test.cpp
)

target_link_libraries(common_security_controls_test PRIVATE
    kcenon::common
    GTest::gtest
    GTest::gtest_main
    Threads::Threads
)

target_compile_features(common_security_controls_test PRIVATE cxx_std_20)

add_test(NAME common_security_controls_test COMMAND common_security_controls_test)

set_tests_properties(common_security_controls_test PROPERTIES
    TIMEOUT 120
    LABELS "unit;security"
)

# Transport interface tests (Issue #233)
add_executable(common_transport_interface_test
    transport_interface_test.cpp
)

target_link_libraries(common_transport_interface_test PRIVATE
    kcenon::common
    GTest::gtest
    GTest::gtest_main
    Threads::Threads
)

target_compile_features(common_transport_interface_test PRIVATE cxx_std_20)

add_test(NAME common_transport_interface_test COMMAND common_transport_interface_test)

set_tests_properties(common_transport_interface_test PROPERTIES
    TIMEOUT 60
    LABELS "unit;interfaces;transport"
)

# Metric collector interface tests (Issue #234)
add_executable(common_metric_collector_interface_test
    metric_collector_interface_test.cpp
)

target_link_libraries(common_metric_collector_interface_test PRIVATE
    kcenon::common
    GTest::gtest
    GTest::gtest_main
    Threads::Threads
)

target_compile_features(common_metric_collector_interface_test PRIVATE cxx_std_20)

add_test(NAME common_metric_collector_interface_test COMMAND common_metric_collector_interface_test)

set_tests_properties(common_metric_collector_interface_test PROPERTIES
    TIMEOUT 60
    LABELS "unit;interfaces;monitoring"
)

# C++20 Concepts tests (Issue #244)
add_executable(common_concepts_test
    concepts_test.cpp
)

target_link_libraries(common_concepts_test PRIVATE
    kcenon::common
    GTest::gtest
    GTest::gtest_main
    Threads::Threads
)

target_compile_features(common_concepts_test PRIVATE cxx_std_20)

add_test(NAME common_concepts_test COMMAND common_concepts_test)

set_tests_properties(common_concepts_test PROPERTIES
    TIMEOUT 60
    LABELS "unit;concepts"
)

# Health monitoring tests (Issue #271)
add_executable(common_health_monitoring_test
    health_monitoring_test.cpp
)

target_link_libraries(common_health_monitoring_test PRIVATE
    kcenon::common
    GTest::gtest
    GTest::gtest_main
    Threads::Threads
)

target_compile_features(common_health_monitoring_test PRIVATE cxx_std_20)

add_test(NAME common_health_monitoring_test COMMAND common_health_monitoring_test)

set_tests_properties(common_health_monitoring_test PROPERTIES
    TIMEOUT 120
    LABELS "unit;interfaces;monitoring;health"
)

# =============================================================================
# Module Verification Tests (Issue #277)
# =============================================================================
# These tests verify C++20 module functionality when modules are enabled.
# The same test file runs in both header-only and module modes to ensure
# API compatibility.
# =============================================================================

# Header-only mode verification test (always built)
add_executable(common_module_verification_test
    module_verification_test.cpp
)

target_link_libraries(common_module_verification_test PRIVATE
    kcenon::common
    GTest::gtest
    GTest::gtest_main
    Threads::Threads
)

target_compile_features(common_module_verification_test PRIVATE cxx_std_20)

add_test(NAME common_module_verification_test COMMAND common_module_verification_test)

set_tests_properties(common_module_verification_test PROPERTIES
    TIMEOUT 60
    LABELS "unit;module"
)

# Module mode verification test (only when COMMON_BUILD_MODULES is ON)
if(COMMON_BUILD_MODULES AND TARGET common_system_modules)
    add_executable(common_module_mode_test
        module_verification_test.cpp
    )

    target_link_libraries(common_module_mode_test PRIVATE
        kcenon::common_modules
        GTest::gtest
        GTest::gtest_main
        Threads::Threads
    )

    target_compile_features(common_module_mode_test PRIVATE cxx_std_20)

    # Define KCENON_USE_MODULES to switch to module imports
    target_compile_definitions(common_module_mode_test PRIVATE KCENON_USE_MODULES=1)

    add_test(NAME common_module_mode_test COMMAND common_module_mode_test)

    set_tests_properties(common_module_mode_test PROPERTIES
        TIMEOUT 60
        LABELS "unit;module;cpp20"
    )

    message(STATUS "Module verification test (module mode): enabled")
endif()

# Enum serialization tests (Issue #293)
add_executable(common_enum_serialization_test
    enum_serialization_test.cpp
)

target_link_libraries(common_enum_serialization_test PRIVATE
    kcenon::common
    GTest::gtest
    GTest::gtest_main
    Threads::Threads
)

target_compile_features(common_enum_serialization_test PRIVATE cxx_std_20)

add_test(NAME common_enum_serialization_test COMMAND common_enum_serialization_test)

set_tests_properties(common_enum_serialization_test PROPERTIES
    TIMEOUT 60
    LABELS "unit;utils;serialization"
)

# Unified adapter tests (Issue #299)
add_executable(common_adapter_test
    adapter_test.cpp
)

target_link_libraries(common_adapter_test PRIVATE
    kcenon::common
    GTest::gtest
    GTest::gtest_main
    Threads::Threads
)

target_compile_features(common_adapter_test PRIVATE cxx_std_20)

add_test(NAME common_adapter_test COMMAND common_adapter_test)

set_tests_properties(common_adapter_test PROPERTIES
    TIMEOUT 60
    LABELS "unit;adapters"
)

# Error category tests (Issue #300)
add_executable(common_error_category_test
    error_category_test.cpp
)

target_link_libraries(common_error_category_test PRIVATE
    kcenon::common
    GTest::gtest
    GTest::gtest_main
    Threads::Threads
)

target_compile_features(common_error_category_test PRIVATE cxx_std_20)

add_test(NAME common_error_category_test COMMAND common_error_category_test)

set_tests_properties(common_error_category_test PROPERTIES
    TIMEOUT 60
    LABELS "unit;error"
)

# Circuit breaker tests (Issue #315)
add_executable(common_circuit_breaker_test
    circuit_breaker_test.cpp
)

target_link_libraries(common_circuit_breaker_test PRIVATE
    kcenon::common
    GTest::gtest
    GTest::gtest_main
    Threads::Threads
)

target_compile_features(common_circuit_breaker_test PRIVATE cxx_std_17)

add_test(NAME common_circuit_breaker_test COMMAND common_circuit_breaker_test)

set_tests_properties(common_circuit_breaker_test PROPERTIES
    TIMEOUT 120
    LABELS "unit;resilience"
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

# =============================================================================
# APPLY STRICT WARNING FLAGS TO ALL TEST TARGETS
# =============================================================================
# Apply warning flags to all test executables to ensure code quality.
# This ensures that both the header-only library and test code compile
# without warnings.
#
# Note: Linker warnings (e.g., duplicate libraries) are not affected by
# these flags and are handled by the build system.
# =============================================================================

set(COMMON_TEST_TARGETS
    common_system_tests
    common_event_bus_failure_test
    common_thread_safety_test
    common_service_container_test
    common_unified_bootstrapper_test
    common_unified_config_test
    common_config_loader_test
    common_cli_config_parser_test
    common_config_watcher_test
    common_global_logger_registry_test
    common_log_functions_test
    common_system_bootstrapper_test
    common_security_controls_test
    common_transport_interface_test
    common_metric_collector_interface_test
    common_concepts_test
    common_health_monitoring_test
    common_module_verification_test
    common_enum_serialization_test
    common_adapter_test
    common_error_category_test
    common_circuit_breaker_test
)

# Apply warning flags to all test targets
foreach(target ${COMMON_TEST_TARGETS})
    if(TARGET ${target})
        target_compile_options(${target} PRIVATE ${COMMON_WARNING_FLAGS})
    endif()
endforeach()

# Apply to ABI version test if it exists
if(TARGET common_abi_version_test)
    target_compile_options(common_abi_version_test PRIVATE ${COMMON_WARNING_FLAGS})
endif()

# Apply to module mode test if it exists
if(TARGET common_module_mode_test)
    target_compile_options(common_module_mode_test PRIVATE ${COMMON_WARNING_FLAGS})
endif()