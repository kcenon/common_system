# =============================================================================
# kcenon-cmake-template :: dependencies.cmake
# -----------------------------------------------------------------------------
# Responsibility
#   Provide shared helpers that wrap recurring dependency discovery patterns
#   (Homebrew prefix probing on macOS, Threads, the find_package CONFIG/MODULE
#   fallback ladder). The actual `find_package` and `FetchContent_Declare`
#   calls remain in each consumer's own dependencies module — only the
#   common scaffolding belongs here.
#
# Required input variables
#   None.
#
# Provided helpers
#   kcenon_template_homebrew_prefix(<package> <out_var>)
#       On Apple platforms, query `brew --prefix <package>` and append it to
#       CMAKE_PREFIX_PATH so keg-only formulae (icu4c, openssl@3, ...) are
#       discoverable. Sets <out_var> to the prefix or empty string.
#
#   kcenon_template_find_threads()
#       Locate Threads::Threads (POSIX threads on UNIX, native on Windows).
#       Idempotent.
#
#   kcenon_template_find_package_with_fallback(<name> [REQUIRED]
#                                              [CONFIG_COMPONENTS <c>...]
#                                              [MODULE_COMPONENTS <c>...])
#       Try CONFIG mode first (vcpkg / installed package configs), then fall
#       back to MODULE mode (CMake bundled FindFOO.cmake). Sets
#       <name>_FOUND in the parent scope.
# =============================================================================

include_guard(GLOBAL)

function(kcenon_template_homebrew_prefix package out_var)
    set(${out_var} "" PARENT_SCOPE)
    if(NOT APPLE)
        return()
    endif()
    find_program(_brew_exe brew)
    if(NOT _brew_exe)
        return()
    endif()
    execute_process(
        COMMAND ${_brew_exe} --prefix ${package}
        OUTPUT_VARIABLE _prefix
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
        RESULT_VARIABLE _rc)
    if(_rc EQUAL 0 AND _prefix)
        list(APPEND CMAKE_PREFIX_PATH "${_prefix}")
        set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH}" PARENT_SCOPE)
        set(${out_var} "${_prefix}" PARENT_SCOPE)
    endif()
endfunction()

function(kcenon_template_find_threads)
    if(NOT TARGET Threads::Threads)
        set(THREADS_PREFER_PTHREAD_FLAG ON)
        find_package(Threads REQUIRED)
    endif()
endfunction()

function(kcenon_template_find_package_with_fallback name)
    cmake_parse_arguments(ARG "REQUIRED"
        "" "CONFIG_COMPONENTS;MODULE_COMPONENTS" ${ARGN})

    set(_config_args "")
    if(ARG_CONFIG_COMPONENTS)
        set(_config_args COMPONENTS ${ARG_CONFIG_COMPONENTS})
    endif()
    find_package(${name} CONFIG QUIET ${_config_args})

    if(NOT ${name}_FOUND)
        set(_module_args "")
        if(ARG_MODULE_COMPONENTS)
            set(_module_args COMPONENTS ${ARG_MODULE_COMPONENTS})
        endif()
        if(ARG_REQUIRED)
            find_package(${name} REQUIRED ${_module_args})
        else()
            find_package(${name} QUIET ${_module_args})
        endif()
    endif()

    set(${name}_FOUND ${${name}_FOUND} PARENT_SCOPE)
endfunction()
