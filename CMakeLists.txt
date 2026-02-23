cmake_minimum_required(VERSION 3.28)
project(common_system VERSION 0.2.0.0 LANGUAGES CXX)

# =============================================================================
# BUILD ORDER DOCUMENTATION
# =============================================================================
#
# common_system is the foundation library for the unified_system ecosystem.
# It has NO dependencies on other unified_system modules.
#
# Dependency Order (common_system must be built FIRST):
#
#   common_system (this project)
#       └─> thread_system     (depends on common_system)
#       └─> logger_system     (depends on common_system)
#       └─> network_system    (depends on common_system)
#       └─> monitoring_system (depends on common_system)
#       └─> database_system   (depends on common_system)
#       └─> container_system  (depends on common_system)
#       └─> messaging_system  (depends on common_system)
#
# Interfaces provided by common_system:
#   - IHttpClient, IUdpClient (transport interfaces)
#   - IMetricCollector (monitoring interface)
#   - Result<T>, Error (patterns)
#   - Service container and DI utilities
#   - Configuration management
#   - Event bus (standalone implementation)
#
# For circular dependency prevention, see:
#   - scripts/check_circular_deps.py
#   - docs/CROSS_MODULE_INTEGRATION.md
#
# =============================================================================

# C++20 as required standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile commands for IDE support
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Include feature flags configuration module
include(cmake/features.cmake)

# Include compiler requirements check
include(cmake/KcenonCompilerRequirements.cmake)
kcenon_check_compiler_requirements()

# Options
option(COMMON_BUILD_TESTS "Build unit tests for common_system" ON)
option(COMMON_BUILD_INTEGRATION_TESTS "Build integration tests for common_system" ON)
option(COMMON_BUILD_EXAMPLES "Build examples for common_system" ON)
option(COMMON_BUILD_DOCS "Generate documentation" OFF)
option(COMMON_BUILD_BENCHMARKS "Build performance benchmarks" OFF)
option(COMMON_HEADER_ONLY "Use as header-only library" ON)
option(ENABLE_COVERAGE "Enable code coverage" OFF)
option(BUILD_WITH_YAML_CPP "Enable YAML configuration file support" OFF)

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

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/src/config/abi_version.cpp.in"
    "${CMAKE_CURRENT_BINARY_DIR}/src/config/abi_version.cpp"
    @ONLY
)

message(STATUS "Generated ABI version header: ${CMAKE_CURRENT_BINARY_DIR}/include/kcenon/common/config/abi_version.h")
message(STATUS "Generated ABI version source: ${CMAKE_CURRENT_BINARY_DIR}/src/config/abi_version.cpp")

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
# Define KCENON_WITH_COMMON_SYSTEM to indicate common_system types are available
# This prevents ABI incompatibility in downstream projects
target_compile_definitions(common_system INTERFACE
    EVENT_BUS_ABI_VERSION=${EVENT_BUS_ABI_VERSION}
    KCENON_WITH_COMMON_SYSTEM=1
)

# Optional: YAML configuration support
if(BUILD_WITH_YAML_CPP)
    find_package(yaml-cpp QUIET)
    if(yaml-cpp_FOUND)
        target_link_libraries(common_system INTERFACE yaml-cpp::yaml-cpp)
        target_compile_definitions(common_system INTERFACE BUILD_WITH_YAML_CPP)
        message(STATUS "YAML configuration support: enabled (yaml-cpp found)")
    else()
        message(WARNING "BUILD_WITH_YAML_CPP is ON but yaml-cpp not found. YAML support disabled.")
    endif()
endif()

# Optional: If not header-only, add source files
if(NOT COMMON_HEADER_ONLY)
    # Convert to static library
    add_library(common_system_static STATIC
        ${CMAKE_CURRENT_BINARY_DIR}/src/config/abi_version.cpp
    )

    target_include_directories(common_system_static PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
        $<INSTALL_INTERFACE:include>
    )

    target_compile_features(common_system_static PUBLIC cxx_std_20)

    target_compile_definitions(common_system_static PUBLIC
        EVENT_BUS_ABI_VERSION=${EVENT_BUS_ABI_VERSION}
        KCENON_WITH_COMMON_SYSTEM=1
    )
endif()

# =============================================================================
# C++20 Module Support (Phase 2)
# =============================================================================
# Enable C++20 modules with CMake 3.28+ when building with module-capable compilers.
# This provides an alternative to the header-only interface.
#
# Usage:
#   cmake -DCOMMON_BUILD_MODULES=ON ..
#
# Requirements:
#   - CMake 3.28 or later
#   - Clang 16+, GCC 14+, or MSVC 2022 17.4+
# =============================================================================

option(COMMON_BUILD_MODULES "Build C++20 module version of common_system" OFF)

if(COMMON_BUILD_MODULES)
    # Check CMake version
    if(CMAKE_VERSION VERSION_LESS "3.28")
        message(WARNING "C++20 modules require CMake 3.28+. Disabling module build.")
        set(COMMON_BUILD_MODULES OFF)
    # Check compiler version requirements for modules
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
        message(WARNING "AppleClang does not support C++20 module dependency scanning. "
                        "Use Clang ${KCENON_MIN_CLANG_MODULE_VERSION}+, "
                        "GCC ${KCENON_MIN_GCC_MODULE_VERSION}+, or "
                        "MSVC ${KCENON_MIN_MSVC_MODULE_VERSION}+. Disabling module build.")
        set(COMMON_BUILD_MODULES OFF)
    # Check generator (modules require Ninja or Visual Studio 17.4+)
    elseif(NOT CMAKE_GENERATOR MATCHES "Ninja" AND NOT CMAKE_GENERATOR MATCHES "Visual Studio")
        message(WARNING "C++20 modules require Ninja or Visual Studio generator. "
                        "Current generator: ${CMAKE_GENERATOR}. Disabling module build.")
        set(COMMON_BUILD_MODULES OFF)
    else()
        # Verify module-capable compiler version
        kcenon_check_compiler_requirements(MODULES)
        message(STATUS "C++20 module build enabled")

        # Create module library target
        add_library(common_system_modules)
        add_library(kcenon::common_modules ALIAS common_system_modules)

        # Set C++20 standard for module target
        target_compile_features(common_system_modules PUBLIC cxx_std_20)

        # Enable module scanning
        set_target_properties(common_system_modules PROPERTIES
            CXX_SCAN_FOR_MODULES ON
        )

        # Add module source files
        target_sources(common_system_modules
            PUBLIC FILE_SET CXX_MODULES
            FILES
                # Primary module
                src/modules/common.cppm
                # Tier 1: Core partitions
                src/modules/utils.cppm
                src/modules/error.cppm
                # Tier 2: Foundation partitions
                src/modules/result.cppm
                src/modules/result/core.cppm
                src/modules/result/utilities.cppm
                src/modules/concepts.cppm
                # Tier 3: Infrastructure partitions
                src/modules/interfaces.cppm
                src/modules/interfaces/logger.cppm
                src/modules/interfaces/executor.cppm
                src/modules/interfaces/core.cppm
                src/modules/config.cppm
                src/modules/di.cppm
                # Tier 4: Application partitions
                src/modules/patterns.cppm
                src/modules/logging.cppm
        )

        # Include directories for module target
        target_include_directories(common_system_modules PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
            $<INSTALL_INTERFACE:include>
        )

        # Compile definitions
        target_compile_definitions(common_system_modules PUBLIC
            EVENT_BUS_ABI_VERSION=${EVENT_BUS_ABI_VERSION}
            KCENON_WITH_COMMON_SYSTEM=1
            KCENON_USE_MODULES=1
        )

        message(STATUS "C++20 module target: common_system_modules")
    endif()
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
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/features.cmake"
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/KcenonCompilerRequirements.cmake"
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

# Benchmarks
if(COMMON_BUILD_BENCHMARKS)
    find_package(benchmark QUIET)
    if(benchmark_FOUND)
        add_subdirectory(benchmarks)
    else()
        message(STATUS "Google Benchmark not found, skipping benchmarks")
    endif()
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
message(STATUS "Build benchmarks:    ${COMMON_BUILD_BENCHMARKS}")
message(STATUS "Build docs:          ${COMMON_BUILD_DOCS}")
message(STATUS "Enable coverage:     ${ENABLE_COVERAGE}")
message(STATUS "YAML support:        ${BUILD_WITH_YAML_CPP}")
message(STATUS "Install prefix:      ${CMAKE_INSTALL_PREFIX}")
message(STATUS "======================================")
message(STATUS "")