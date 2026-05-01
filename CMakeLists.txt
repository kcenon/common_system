cmake_minimum_required(VERSION 3.20)
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" PROJECT_VERSION_RAW)
string(STRIP "${PROJECT_VERSION_RAW}" PROJECT_VERSION)
project(common_system VERSION ${PROJECT_VERSION} LANGUAGES CXX)

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

# -----------------------------------------------------------------------------
# kcenon-cmake-template adoption
# -----------------------------------------------------------------------------
# common_system is the reference adopter of the template suite at
# cmake/template/. See cmake/template/README.md for the canonical adoption
# checklist; the calls below mirror that checklist 1-to-1.
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/template")

include(options)
include(compiler)
include(dependencies)
include(warnings)
include(targets)
include(install)
include(testing)
include(examples)

# Step 4: standard baseline
kcenon_template_set_cpp_baseline(STANDARD 20)
kcenon_template_set_default_build_type(TYPE Release)
kcenon_template_enable_compile_commands()

# Step 5a: pre-set cache defaults that diverge from the template defaults so
# the subsequent option() calls inside `kcenon_template_define_standard_options`
# act as no-ops while preserving common_system's historical behaviour
# (examples ON by default, header-only ON by default).
set(COMMON_BUILD_EXAMPLES ON CACHE BOOL "Build example programs")

# Step 5b: standard options
kcenon_template_define_standard_options(COMMON)

# Step 5c: project-specific options
option(COMMON_BUILD_INTEGRATION_TESTS "Build integration tests for common_system" ON)
option(COMMON_HEADER_ONLY              "Use as header-only library"                ON)
option(ENABLE_COVERAGE                 "Enable code coverage"                      OFF)
option(BUILD_WITH_YAML_CPP             "Enable YAML configuration file support"    OFF)

# Step 6: warnings (header-only INTERFACE targets are skipped by the helper)
kcenon_template_init_warnings(COMMON)

# -----------------------------------------------------------------------------
# common_system-specific helpers (not part of the template)
# -----------------------------------------------------------------------------
include(cmake/features.cmake)
include(cmake/KcenonCompilerRequirements.cmake)
kcenon_check_compiler_requirements()

# -----------------------------------------------------------------------------
# Event Bus ABI Configuration
# -----------------------------------------------------------------------------
# The event bus uses a standalone implementation without external
# dependencies, eliminating the circular dependency with monitoring_system.
set(EVENT_BUS_ABI_VERSION 1 CACHE STRING "Event bus ABI version (1=standalone)")
message(STATUS "Event Bus ABI Version: ${EVENT_BUS_ABI_VERSION} (standalone implementation)")

string(TIMESTAMP CMAKE_CONFIGURE_TIMESTAMP "%Y-%m-%d %H:%M:%S UTC" UTC)

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

# Honour the umbrella BUILD_INTEGRATION_TESTS flag if set by a parent build.
if(DEFINED BUILD_INTEGRATION_TESTS)
    if(BUILD_INTEGRATION_TESTS)
        set(_COMMON_BUILD_IT_VALUE ON)
    else()
        set(_COMMON_BUILD_IT_VALUE OFF)
    endif()
    set(COMMON_BUILD_INTEGRATION_TESTS ${_COMMON_BUILD_IT_VALUE} CACHE BOOL
        "Build integration tests for common_system" FORCE)
endif()

# -----------------------------------------------------------------------------
# Step 7: target definitions (header-only INTERFACE library + aliases)
# -----------------------------------------------------------------------------
add_library(common_system INTERFACE)
kcenon_template_create_aliases(common_system
    kcenon::common
    kcenon::common_system
    common_system::common_system
)

kcenon_template_setup_target_includes(common_system INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    INSTALL_DIR "include"
)
target_include_directories(common_system INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
)

kcenon_template_set_cxx_feature_std(common_system INTERFACE 20)

# Define ABI version + KCENON_WITH_COMMON_SYSTEM marker for downstream projects
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

# Optional: static-library variant when not header-only
if(NOT COMMON_HEADER_ONLY)
    add_library(common_system_static STATIC
        ${CMAKE_CURRENT_BINARY_DIR}/src/config/abi_version.cpp
    )
    kcenon_template_setup_target_includes(common_system_static PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        INSTALL_DIR "include"
    )
    target_include_directories(common_system_static PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
    )
    kcenon_template_set_cxx_feature_std(common_system_static PUBLIC 20)
    target_compile_definitions(common_system_static PUBLIC
        EVENT_BUS_ABI_VERSION=${EVENT_BUS_ABI_VERSION}
        KCENON_WITH_COMMON_SYSTEM=1
    )
    kcenon_template_apply_warnings(common_system_static)
endif()

# -----------------------------------------------------------------------------
# C++20 Module Support (Phase 2)
# -----------------------------------------------------------------------------
# Enable C++20 modules with CMake 3.28+ when building with module-capable
# compilers. This provides an alternative to the header-only interface.
#
# Usage:   cmake -DCOMMON_BUILD_MODULES=ON ..
# Needs:   CMake 3.28+, Clang 16+ / GCC 14+ / MSVC 2022 17.4+
# -----------------------------------------------------------------------------
if(COMMON_BUILD_MODULES)
    if(CMAKE_VERSION VERSION_LESS "3.28")
        message(WARNING "C++20 modules require CMake 3.28+. Disabling module build.")
        set(COMMON_BUILD_MODULES OFF)
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
        message(WARNING "AppleClang does not support C++20 module dependency scanning. "
                        "Use Clang ${KCENON_MIN_CLANG_MODULE_VERSION}+, "
                        "GCC ${KCENON_MIN_GCC_MODULE_VERSION}+, or "
                        "MSVC ${KCENON_MIN_MSVC_MODULE_VERSION}+. Disabling module build.")
        set(COMMON_BUILD_MODULES OFF)
    elseif(NOT CMAKE_GENERATOR MATCHES "Ninja" AND NOT CMAKE_GENERATOR MATCHES "Visual Studio")
        message(WARNING "C++20 modules require Ninja or Visual Studio generator. "
                        "Current generator: ${CMAKE_GENERATOR}. Disabling module build.")
        set(COMMON_BUILD_MODULES OFF)
    else()
        kcenon_check_compiler_requirements(MODULES)
        message(STATUS "C++20 module build enabled")

        add_library(common_system_modules)
        add_library(kcenon::common_modules ALIAS common_system_modules)
        kcenon_template_set_cxx_feature_std(common_system_modules PUBLIC 20)

        set_target_properties(common_system_modules PROPERTIES
            CXX_SCAN_FOR_MODULES ON
        )

        target_sources(common_system_modules
            PUBLIC FILE_SET CXX_MODULES
            FILES
                src/modules/common.cppm
                src/modules/utils.cppm
                src/modules/error.cppm
                src/modules/result.cppm
                src/modules/result/core.cppm
                src/modules/result/utilities.cppm
                src/modules/concepts.cppm
                src/modules/interfaces.cppm
                src/modules/interfaces/logger.cppm
                src/modules/interfaces/executor.cppm
                src/modules/interfaces/core.cppm
                src/modules/config.cppm
                src/modules/di.cppm
                src/modules/patterns.cppm
                src/modules/logging.cppm
        )

        kcenon_template_setup_target_includes(common_system_modules PUBLIC
            "${CMAKE_CURRENT_SOURCE_DIR}/include"
            INSTALL_DIR "include"
        )
        target_include_directories(common_system_modules PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
        )

        target_compile_definitions(common_system_modules PUBLIC
            EVENT_BUS_ABI_VERSION=${EVENT_BUS_ABI_VERSION}
            KCENON_WITH_COMMON_SYSTEM=1
            KCENON_USE_MODULES=1
        )

        kcenon_template_apply_warnings(common_system_modules)

        message(STATUS "C++20 module target: common_system_modules")
    endif()
endif()

# -----------------------------------------------------------------------------
# Step 8: install rules
# -----------------------------------------------------------------------------
kcenon_template_install_headers("${CMAKE_CURRENT_SOURCE_DIR}/include"
    PATTERNS "*.h" "*.hpp"
)

# Install generated abi_version.h (produced by configure_file, lives in build tree)
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/include/kcenon/common/config/abi_version.h"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/kcenon/common/config"
)

kcenon_template_install_package(
    PACKAGE_NAME    common_system
    NAMESPACE       common_system
    TARGETS         common_system
    CONFIG_TEMPLATE "${CMAKE_CURRENT_SOURCE_DIR}/cmake/common_system-config.cmake.in"
    EXTRA_FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/features.cmake"
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/KcenonCompilerRequirements.cmake"
)

# -----------------------------------------------------------------------------
# Step 9: tests
# -----------------------------------------------------------------------------
if(COMMON_BUILD_TESTS OR COMMON_BUILD_INTEGRATION_TESTS)
    kcenon_template_setup_testing()
    if(KCENON_TEMPLATE_GTEST_FOUND)
        if(COMMON_BUILD_TESTS)
            add_subdirectory(tests)
        endif()
        if(COMMON_BUILD_INTEGRATION_TESTS)
            add_subdirectory(integration_tests)
        endif()
    endif()
endif()

# -----------------------------------------------------------------------------
# Step 10: examples
# -----------------------------------------------------------------------------
if(COMMON_BUILD_EXAMPLES AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/examples/CMakeLists.txt")
    add_subdirectory(examples)
endif()

# -----------------------------------------------------------------------------
# Benchmarks (project-specific, not standardised by the template)
# -----------------------------------------------------------------------------
if(COMMON_BUILD_BENCHMARKS)
    find_package(benchmark QUIET)
    if(benchmark_FOUND)
        add_subdirectory(benchmarks)
    else()
        message(STATUS "Google Benchmark not found, skipping benchmarks")
    endif()
endif()

# -----------------------------------------------------------------------------
# Documentation (project-specific Doxygen target)
# -----------------------------------------------------------------------------
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

# -----------------------------------------------------------------------------
# Coverage support (project-specific, applies to INTERFACE library)
# -----------------------------------------------------------------------------
if(ENABLE_COVERAGE)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(common_system INTERFACE --coverage)
        target_link_options(common_system INTERFACE --coverage)
        message(STATUS "Code coverage enabled")
    else()
        message(WARNING "Coverage not supported on this compiler")
    endif()
endif()

# -----------------------------------------------------------------------------
# Configuration summary
# -----------------------------------------------------------------------------
message(STATUS "")
message(STATUS "=== common_system Configuration ===")
message(STATUS "Version:             ${PROJECT_VERSION}")
message(STATUS "Header-only:         ${COMMON_HEADER_ONLY}")
if(GTest_FOUND AND DEFINED GTest_VERSION)
    message(STATUS "GTest version:       ${GTest_VERSION}")
endif()
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
