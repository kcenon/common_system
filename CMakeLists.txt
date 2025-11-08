cmake_minimum_required(VERSION 3.16)
project(common_system VERSION 1.0.0 LANGUAGES CXX)

# C++20 as required standard (unified across all systems)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile commands for IDE support
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Options
option(COMMON_BUILD_TESTS "Build unit tests for common_system" ON)
option(COMMON_BUILD_INTEGRATION_TESTS "Build integration tests for common_system" ON)
option(COMMON_BUILD_EXAMPLES "Build examples for common_system" ON)
option(COMMON_BUILD_DOCS "Generate documentation" OFF)
option(COMMON_HEADER_ONLY "Use as header-only library" ON)
option(ENABLE_COVERAGE "Enable code coverage" OFF)

# Event Bus ABI Configuration
# The event bus now uses a standalone implementation without external dependencies
# This eliminates the circular dependency with monitoring_system
set(EVENT_BUS_ABI_VERSION 1 CACHE STRING "Event bus ABI version (1=standalone)")

message(STATUS "Event Bus ABI Version: ${EVENT_BUS_ABI_VERSION} (standalone implementation)")

# Generate ABI version header
string(TIMESTAMP CMAKE_CONFIGURE_TIMESTAMP "%Y-%m-%d %H:%M:%S UTC" UTC)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/include/kcenon/common/config/abi_version.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/include/kcenon/common/config/abi_version.h"
    @ONLY
)

message(STATUS "Generated ABI version header: ${CMAKE_CURRENT_BINARY_DIR}/include/kcenon/common/config/abi_version.h")

# Respect global BUILD_INTEGRATION_TESTS flag if set
if(DEFINED BUILD_INTEGRATION_TESTS)
    if(BUILD_INTEGRATION_TESTS)
        set(_COMMON_BUILD_IT_VALUE ON)
    else()
        set(_COMMON_BUILD_IT_VALUE OFF)
    endif()
    set(COMMON_BUILD_INTEGRATION_TESTS ${_COMMON_BUILD_IT_VALUE} CACHE BOOL "Build integration tests for common_system" FORCE)
endif()

# Create interface library (header-only by default)
add_library(common_system INTERFACE)
add_library(kcenon::common ALIAS common_system)

# Set include directories
target_include_directories(common_system INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# C++ feature requirements
target_compile_features(common_system INTERFACE cxx_std_20)

# Define ABI version for event bus
target_compile_definitions(common_system INTERFACE
    EVENT_BUS_ABI_VERSION=${EVENT_BUS_ABI_VERSION}
)

# Optional: If not header-only, add source files
if(NOT COMMON_HEADER_ONLY)
    # Convert to static library
    add_library(common_system_static STATIC
        src/config/abi_version.cpp
    )

    target_include_directories(common_system_static PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
        $<INSTALL_INTERFACE:include>
    )

    target_compile_features(common_system_static PUBLIC cxx_std_20)

    target_compile_definitions(common_system_static PUBLIC
        EVENT_BUS_ABI_VERSION=${EVENT_BUS_ABI_VERSION}
    )
endif()

# Installation rules
include(GNUInstallDirs)

# Install headers
install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
)

# Install CMake config files
install(TARGETS common_system
    EXPORT common_systemTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(EXPORT common_systemTargets
    FILE common_systemTargets.cmake
    NAMESPACE kcenon::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/common_system
)

# Create Config.cmake file
include(CMakePackageConfigHelpers)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/common_systemConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/common_systemConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/common_system
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/common_systemConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/common_systemConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/common_systemConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/common_system
)

# Export for build tree
export(EXPORT common_systemTargets
    FILE "${CMAKE_CURRENT_BINARY_DIR}/common_systemTargets.cmake"
    NAMESPACE kcenon::
)

# Tests
if(COMMON_BUILD_TESTS OR COMMON_BUILD_INTEGRATION_TESTS)
    enable_testing()
    # Only add tests directory if GTest is available
    find_package(GTest QUIET)
    if(GTest_FOUND)
        if(COMMON_BUILD_TESTS)
            add_subdirectory(tests)
        endif()
        if(COMMON_BUILD_INTEGRATION_TESTS)
            add_subdirectory(integration_tests)
        endif()
    else()
        message(STATUS "GTest not found, skipping tests")
    endif()
endif()

# Examples
if(COMMON_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Documentation
if(COMMON_BUILD_DOCS)
    find_package(Doxygen)
    if(DOXYGEN_FOUND)
        set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in)
        set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

        configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)

        add_custom_target(common_docs ALL
            COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Generating API documentation with Doxygen"
            VERBATIM
        )
    endif()
endif()

# Coverage support
if(ENABLE_COVERAGE)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(common_system INTERFACE --coverage)
        target_link_options(common_system INTERFACE --coverage)
        message(STATUS "Code coverage enabled")
    else()
        message(WARNING "Coverage not supported on this compiler")
    endif()
endif()

# Print configuration summary
message(STATUS "")
message(STATUS "=== common_system Configuration ===")
message(STATUS "Version:             ${PROJECT_VERSION}")
message(STATUS "Header-only:         ${COMMON_HEADER_ONLY}")
message(STATUS "Build tests:         ${COMMON_BUILD_TESTS}")
message(STATUS "Build integration:   ${COMMON_BUILD_INTEGRATION_TESTS}")
message(STATUS "Build examples:      ${COMMON_BUILD_EXAMPLES}")
message(STATUS "Build docs:          ${COMMON_BUILD_DOCS}")
message(STATUS "Enable coverage:     ${ENABLE_COVERAGE}")
message(STATUS "Install prefix:      ${CMAKE_INSTALL_PREFIX}")
message(STATUS "======================================")
message(STATUS "")