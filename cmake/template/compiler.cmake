# =============================================================================
# kcenon-cmake-template :: compiler.cmake
# -----------------------------------------------------------------------------
# Responsibility
#   Establish the compiler/standard baseline shared by every kcenon ecosystem
#   system: C++ standard version, position-independent code, IDE compile
#   command export, and a sane default build type for single-config
#   generators.
#
# Required input variables
#   None.
#
# Provided helpers
#   kcenon_template_set_cpp_baseline([STANDARD <num>])
#       Set CMAKE_CXX_STANDARD (default 20), require it, disable GNU
#       extensions, and enable position-independent code globally.
#
#   kcenon_template_set_default_build_type([TYPE <name>])
#       For single-config generators, set CMAKE_BUILD_TYPE to <name> when
#       not already set (default Release).
#
#   kcenon_template_enable_compile_commands()
#       Enable CMAKE_EXPORT_COMPILE_COMMANDS so IDEs / clang-tidy / clangd
#       can pick up the build database.
# =============================================================================

include_guard(GLOBAL)

function(kcenon_template_set_cpp_baseline)
    cmake_parse_arguments(ARG "" "STANDARD" "" ${ARGN})
    if(NOT ARG_STANDARD)
        set(ARG_STANDARD 20)
    endif()

    # CMake variables affecting target_compile_features and add_executable
    # defaults need to live in the directory scope, not function scope.
    set(CMAKE_CXX_STANDARD          ${ARG_STANDARD} PARENT_SCOPE)
    set(CMAKE_CXX_STANDARD_REQUIRED ON              PARENT_SCOPE)
    set(CMAKE_CXX_EXTENSIONS        OFF             PARENT_SCOPE)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON          PARENT_SCOPE)
endfunction()

function(kcenon_template_set_default_build_type)
    cmake_parse_arguments(ARG "" "TYPE" "" ${ARGN})
    if(NOT ARG_TYPE)
        set(ARG_TYPE "Release")
    endif()

    get_property(_is_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
    if(NOT _is_multi_config AND NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE "${ARG_TYPE}" CACHE STRING
            "Build type (Debug, Release, RelWithDebInfo, MinSizeRel)" FORCE)
        set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
            Debug Release RelWithDebInfo MinSizeRel)
    endif()
endfunction()

function(kcenon_template_enable_compile_commands)
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON PARENT_SCOPE)
endfunction()
