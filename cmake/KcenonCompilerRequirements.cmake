# BSD 3-Clause License
# Copyright (c) 2025, kcenon
# See the LICENSE file in the project root for full license information.

#[=============================================================================[
KcenonCompilerRequirements.cmake - Unified compiler version enforcement

This module provides a single function to verify that the current compiler
meets the minimum version requirements for the KCENON ecosystem.

Downstream systems should include this module and call the check function
during their CMake configuration:

    include(KcenonCompilerRequirements)
    kcenon_check_compiler_requirements()

    # For module builds, also check module-capable versions:
    kcenon_check_compiler_requirements(MODULES)

Minimum Compiler Requirements (Header-Only):
    GCC          11.0+
    Clang        14.0+
    MSVC         19.30+ (Visual Studio 2022)
    Apple Clang  14.0+

Minimum Compiler Requirements (C++20 Modules):
    GCC          14.0+
    Clang        16.0+
    MSVC         19.34+ (Visual Studio 2022 17.4)
    Apple Clang  Not supported

#]=============================================================================]

include_guard(GLOBAL)

# --- Version constants ---

# Header-only minimum versions
set(KCENON_MIN_GCC_VERSION          "11.0" CACHE INTERNAL "")
set(KCENON_MIN_CLANG_VERSION        "14.0" CACHE INTERNAL "")
set(KCENON_MIN_MSVC_VERSION         "19.30" CACHE INTERNAL "")
set(KCENON_MIN_APPLECLANG_VERSION   "14.0" CACHE INTERNAL "")

# Module build minimum versions
set(KCENON_MIN_GCC_MODULE_VERSION   "14.0" CACHE INTERNAL "")
set(KCENON_MIN_CLANG_MODULE_VERSION "16.0" CACHE INTERNAL "")
set(KCENON_MIN_MSVC_MODULE_VERSION  "19.34" CACHE INTERNAL "")

#[=============================================================================[
kcenon_check_compiler_requirements([MODULES] [WARNING_ONLY])

Check that the current compiler meets KCENON ecosystem minimum requirements.

Options:
    MODULES       Also check module-capable compiler versions
    WARNING_ONLY  Emit warnings instead of fatal errors (for informational use)

Examples:
    # Basic check (header-only requirements)
    kcenon_check_compiler_requirements()

    # Check including module support
    kcenon_check_compiler_requirements(MODULES)

    # Non-fatal check for diagnostic purposes
    kcenon_check_compiler_requirements(WARNING_ONLY)
#]=============================================================================]
function(kcenon_check_compiler_requirements)
    cmake_parse_arguments(PARSE_ARGV 0 ARG "MODULES;WARNING_ONLY" "" "")

    if(ARG_WARNING_ONLY)
        set(_MSG_TYPE WARNING)
    else()
        set(_MSG_TYPE FATAL_ERROR)
    endif()

    set(_COMPILER_ID "${CMAKE_CXX_COMPILER_ID}")
    set(_COMPILER_VER "${CMAKE_CXX_COMPILER_VERSION}")

    # --- Header-only checks ---
    if(_COMPILER_ID STREQUAL "GNU")
        if(_COMPILER_VER VERSION_LESS "${KCENON_MIN_GCC_VERSION}")
            message(${_MSG_TYPE}
                "KCENON ecosystem requires GCC >= ${KCENON_MIN_GCC_VERSION}. "
                "Detected GCC ${_COMPILER_VER}. "
                "Please upgrade your compiler.")
        endif()

    elseif(_COMPILER_ID STREQUAL "Clang")
        if(_COMPILER_VER VERSION_LESS "${KCENON_MIN_CLANG_VERSION}")
            message(${_MSG_TYPE}
                "KCENON ecosystem requires Clang >= ${KCENON_MIN_CLANG_VERSION}. "
                "Detected Clang ${_COMPILER_VER}. "
                "Please upgrade your compiler.")
        endif()

    elseif(_COMPILER_ID STREQUAL "AppleClang")
        if(_COMPILER_VER VERSION_LESS "${KCENON_MIN_APPLECLANG_VERSION}")
            message(${_MSG_TYPE}
                "KCENON ecosystem requires Apple Clang >= ${KCENON_MIN_APPLECLANG_VERSION}. "
                "Detected Apple Clang ${_COMPILER_VER}. "
                "Please upgrade Xcode or Command Line Tools.")
        endif()

    elseif(_COMPILER_ID STREQUAL "MSVC")
        if(_COMPILER_VER VERSION_LESS "${KCENON_MIN_MSVC_VERSION}")
            message(${_MSG_TYPE}
                "KCENON ecosystem requires MSVC >= ${KCENON_MIN_MSVC_VERSION} (Visual Studio 2022). "
                "Detected MSVC ${_COMPILER_VER}. "
                "Please upgrade to Visual Studio 2022 or later.")
        endif()

    else()
        message(WARNING
            "KCENON: Unrecognized compiler '${_COMPILER_ID}' version ${_COMPILER_VER}. "
            "Supported compilers: GCC ${KCENON_MIN_GCC_VERSION}+, "
            "Clang ${KCENON_MIN_CLANG_VERSION}+, "
            "MSVC ${KCENON_MIN_MSVC_VERSION}+ (VS 2022), "
            "Apple Clang ${KCENON_MIN_APPLECLANG_VERSION}+.")
    endif()

    # --- Module build checks ---
    if(ARG_MODULES)
        if(_COMPILER_ID STREQUAL "GNU")
            if(_COMPILER_VER VERSION_LESS "${KCENON_MIN_GCC_MODULE_VERSION}")
                message(${_MSG_TYPE}
                    "KCENON C++20 modules require GCC >= ${KCENON_MIN_GCC_MODULE_VERSION}. "
                    "Detected GCC ${_COMPILER_VER}. "
                    "Disable modules with -DCOMMON_BUILD_MODULES=OFF or upgrade GCC.")
            endif()

        elseif(_COMPILER_ID STREQUAL "Clang")
            if(_COMPILER_VER VERSION_LESS "${KCENON_MIN_CLANG_MODULE_VERSION}")
                message(${_MSG_TYPE}
                    "KCENON C++20 modules require Clang >= ${KCENON_MIN_CLANG_MODULE_VERSION}. "
                    "Detected Clang ${_COMPILER_VER}. "
                    "Disable modules with -DCOMMON_BUILD_MODULES=OFF or upgrade Clang.")
            endif()

        elseif(_COMPILER_ID STREQUAL "AppleClang")
            message(${_MSG_TYPE}
                "Apple Clang does not support C++20 module dependency scanning. "
                "Use Clang ${KCENON_MIN_CLANG_MODULE_VERSION}+, "
                "GCC ${KCENON_MIN_GCC_MODULE_VERSION}+, or "
                "MSVC ${KCENON_MIN_MSVC_MODULE_VERSION}+ instead. "
                "Disable modules with -DCOMMON_BUILD_MODULES=OFF.")

        elseif(_COMPILER_ID STREQUAL "MSVC")
            if(_COMPILER_VER VERSION_LESS "${KCENON_MIN_MSVC_MODULE_VERSION}")
                message(${_MSG_TYPE}
                    "KCENON C++20 modules require MSVC >= ${KCENON_MIN_MSVC_MODULE_VERSION} "
                    "(Visual Studio 2022 17.4+). "
                    "Detected MSVC ${_COMPILER_VER}. "
                    "Disable modules with -DCOMMON_BUILD_MODULES=OFF or upgrade.")
            endif()
        endif()
    endif()

    # --- Summary ---
    if(ARG_MODULES)
        message(STATUS "KCENON compiler check: ${_COMPILER_ID} ${_COMPILER_VER} "
                       "(modules enabled)")
    else()
        message(STATUS "KCENON compiler check: ${_COMPILER_ID} ${_COMPILER_VER}")
    endif()
endfunction()
