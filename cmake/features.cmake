# BSD 3-Clause License
# Copyright (c) 2025, kcenon
# See the LICENSE file in the project root for full license information.

#[=============================================================================[
features.cmake - KCENON Feature Flags Configuration

This module provides CMake functions for configuring KCENON_* feature flags
and system integration definitions. It works in conjunction with the
feature_flags.h header family to provide unified feature detection.

Usage:
    include(cmake/features.cmake)

    # Configure feature flags for a target
    kcenon_configure_features(my_target
        THREAD_SYSTEM ON
        LOGGER_SYSTEM ON
        LEGACY_ALIASES ON
    )

Available options:
    THREAD_SYSTEM     - Enable KCENON_WITH_THREAD_SYSTEM
    LOGGER_SYSTEM     - Enable KCENON_WITH_LOGGER_SYSTEM
    MONITORING_SYSTEM - Enable KCENON_WITH_MONITORING_SYSTEM
    CONTAINER_SYSTEM  - Enable KCENON_WITH_CONTAINER_SYSTEM
    NETWORK_SYSTEM    - Enable KCENON_WITH_NETWORK_SYSTEM
    DATABASE_SYSTEM   - Enable KCENON_WITH_DATABASE_SYSTEM
    MESSAGING_SYSTEM  - Enable KCENON_WITH_MESSAGING_SYSTEM
    LEGACY_ALIASES    - Enable KCENON_ENABLE_LEGACY_ALIASES (default: ON)
    PRINT_SUMMARY     - Enable KCENON_PRINT_FEATURE_SUMMARY

#]=============================================================================]

include_guard(GLOBAL)

#[=============================================================================[
kcenon_configure_features(<target>
    [THREAD_SYSTEM <bool>]
    [LOGGER_SYSTEM <bool>]
    [MONITORING_SYSTEM <bool>]
    [CONTAINER_SYSTEM <bool>]
    [NETWORK_SYSTEM <bool>]
    [DATABASE_SYSTEM <bool>]
    [MESSAGING_SYSTEM <bool>]
    [LEGACY_ALIASES <bool>]
    [PRINT_SUMMARY <bool>]
)

Configure KCENON feature flags for the specified target.
#]=============================================================================]
function(kcenon_configure_features TARGET)
    cmake_parse_arguments(PARSE_ARGV 1 ARG
        ""
        "THREAD_SYSTEM;LOGGER_SYSTEM;MONITORING_SYSTEM;CONTAINER_SYSTEM;NETWORK_SYSTEM;DATABASE_SYSTEM;MESSAGING_SYSTEM;LEGACY_ALIASES;PRINT_SUMMARY"
        ""
    )

    # Validate target exists
    if(NOT TARGET ${TARGET})
        message(FATAL_ERROR "kcenon_configure_features: Target '${TARGET}' does not exist")
    endif()

    # System integration flags
    if(DEFINED ARG_THREAD_SYSTEM)
        if(ARG_THREAD_SYSTEM)
            target_compile_definitions(${TARGET} PUBLIC KCENON_WITH_THREAD_SYSTEM=1)
        else()
            target_compile_definitions(${TARGET} PUBLIC KCENON_WITH_THREAD_SYSTEM=0)
        endif()
    endif()

    if(DEFINED ARG_LOGGER_SYSTEM)
        if(ARG_LOGGER_SYSTEM)
            target_compile_definitions(${TARGET} PUBLIC KCENON_WITH_LOGGER_SYSTEM=1)
        else()
            target_compile_definitions(${TARGET} PUBLIC KCENON_WITH_LOGGER_SYSTEM=0)
        endif()
    endif()

    if(DEFINED ARG_MONITORING_SYSTEM)
        if(ARG_MONITORING_SYSTEM)
            target_compile_definitions(${TARGET} PUBLIC KCENON_WITH_MONITORING_SYSTEM=1)
        else()
            target_compile_definitions(${TARGET} PUBLIC KCENON_WITH_MONITORING_SYSTEM=0)
        endif()
    endif()

    if(DEFINED ARG_CONTAINER_SYSTEM)
        if(ARG_CONTAINER_SYSTEM)
            target_compile_definitions(${TARGET} PUBLIC KCENON_WITH_CONTAINER_SYSTEM=1)
        else()
            target_compile_definitions(${TARGET} PUBLIC KCENON_WITH_CONTAINER_SYSTEM=0)
        endif()
    endif()

    if(DEFINED ARG_NETWORK_SYSTEM)
        if(ARG_NETWORK_SYSTEM)
            target_compile_definitions(${TARGET} PUBLIC KCENON_WITH_NETWORK_SYSTEM=1)
        else()
            target_compile_definitions(${TARGET} PUBLIC KCENON_WITH_NETWORK_SYSTEM=0)
        endif()
    endif()

    if(DEFINED ARG_DATABASE_SYSTEM)
        if(ARG_DATABASE_SYSTEM)
            target_compile_definitions(${TARGET} PUBLIC KCENON_WITH_DATABASE_SYSTEM=1)
        else()
            target_compile_definitions(${TARGET} PUBLIC KCENON_WITH_DATABASE_SYSTEM=0)
        endif()
    endif()

    if(DEFINED ARG_MESSAGING_SYSTEM)
        if(ARG_MESSAGING_SYSTEM)
            target_compile_definitions(${TARGET} PUBLIC KCENON_WITH_MESSAGING_SYSTEM=1)
        else()
            target_compile_definitions(${TARGET} PUBLIC KCENON_WITH_MESSAGING_SYSTEM=0)
        endif()
    endif()

    # Legacy aliases (default: enabled for backward compatibility)
    if(DEFINED ARG_LEGACY_ALIASES)
        if(ARG_LEGACY_ALIASES)
            target_compile_definitions(${TARGET} PUBLIC KCENON_ENABLE_LEGACY_ALIASES=1)
        else()
            target_compile_definitions(${TARGET} PUBLIC KCENON_ENABLE_LEGACY_ALIASES=0)
        endif()
    endif()

    # Debug: print feature summary at compile time
    if(ARG_PRINT_SUMMARY)
        target_compile_definitions(${TARGET} PUBLIC KCENON_PRINT_FEATURE_SUMMARY)
    endif()
endfunction()

#[=============================================================================[
kcenon_detect_features()

Detect C++ features at CMake configure time and set corresponding variables.
This is useful for conditional CMake logic based on compiler capabilities.

Sets the following variables:
    KCENON_DETECTED_CPP17       - TRUE if C++17 is available
    KCENON_DETECTED_CPP20       - TRUE if C++20 is available
    KCENON_DETECTED_CONCEPTS    - TRUE if C++20 concepts are available
    KCENON_DETECTED_JTHREAD     - TRUE if std::jthread is available

#]=============================================================================]
function(kcenon_detect_features)
    include(CheckCXXSourceCompiles)

    # Save current required flags
    set(_CMAKE_REQUIRED_FLAGS_SAVE "${CMAKE_REQUIRED_FLAGS}")

    # Ensure C++20 standard is used for feature detection
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} -std=c++20")
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
        set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} /std:c++20")
    endif()

    # Check for C++20
    check_cxx_source_compiles("
        #if __cplusplus >= 202002L || (defined(_MSVC_LANG) && _MSVC_LANG >= 202002L)
        int main() { return 0; }
        #else
        #error C++20 not available
        #endif
    " KCENON_DETECTED_CPP20)

    # Check for std::jthread
    check_cxx_source_compiles("
        #include <thread>
        #include <stop_token>
        int main() {
            std::jthread t([](std::stop_token) {});
            return 0;
        }
    " KCENON_DETECTED_JTHREAD)

    # Check for concepts
    check_cxx_source_compiles("
        #include <concepts>
        template<typename T>
        concept Addable = requires(T a, T b) { a + b; };
        int main() { return 0; }
    " KCENON_DETECTED_CONCEPTS)

    # Restore required flags
    set(CMAKE_REQUIRED_FLAGS "${_CMAKE_REQUIRED_FLAGS_SAVE}")

    # Export to parent scope
    set(KCENON_DETECTED_CPP20 ${KCENON_DETECTED_CPP20} PARENT_SCOPE)
    set(KCENON_DETECTED_JTHREAD ${KCENON_DETECTED_JTHREAD} PARENT_SCOPE)
    set(KCENON_DETECTED_CONCEPTS ${KCENON_DETECTED_CONCEPTS} PARENT_SCOPE)

    # Print summary
    message(STATUS "KCENON Feature Detection:")
    message(STATUS "  C++20:    ${KCENON_DETECTED_CPP20}")
    message(STATUS "  jthread:  ${KCENON_DETECTED_JTHREAD}")
    message(STATUS "  concepts: ${KCENON_DETECTED_CONCEPTS}")
endfunction()

#[=============================================================================[
kcenon_print_feature_config(<target>)

Print the feature configuration for a target (for debugging).
#]=============================================================================]
function(kcenon_print_feature_config TARGET)
    if(NOT TARGET ${TARGET})
        message(WARNING "kcenon_print_feature_config: Target '${TARGET}' does not exist")
        return()
    endif()

    get_target_property(_DEFS ${TARGET} COMPILE_DEFINITIONS)
    get_target_property(_INTERFACE_DEFS ${TARGET} INTERFACE_COMPILE_DEFINITIONS)

    message(STATUS "Feature configuration for ${TARGET}:")
    if(_DEFS)
        foreach(_DEF ${_DEFS})
            if(_DEF MATCHES "^KCENON_")
                message(STATUS "  ${_DEF}")
            endif()
        endforeach()
    endif()
    if(_INTERFACE_DEFS)
        foreach(_DEF ${_INTERFACE_DEFS})
            if(_DEF MATCHES "^KCENON_")
                message(STATUS "  ${_DEF} (INTERFACE)")
            endif()
        endforeach()
    endif()
endfunction()
