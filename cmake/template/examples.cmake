# =============================================================================
# kcenon-cmake-template :: examples.cmake
# -----------------------------------------------------------------------------
# Responsibility
#   Provide a thin convention for example registration: a single helper that
#   gates `add_subdirectory()` on the project's `<PREFIX>_BUILD_EXAMPLES`
#   option and emits a uniform STATUS line per example. The actual list of
#   examples is project-specific and is supplied by the consumer at call
#   time.
#
# Required input variables
#   <PREFIX>_BUILD_EXAMPLES
#       Boolean cache option from `kcenon_template_define_standard_options()`,
#       supplied at call time via `kcenon_template_add_example_dirs(<PREFIX>
#       ...)`.
#
# Provided helpers
#   kcenon_template_add_example_dirs(<PREFIX> SUBDIRS <dir>...)
#       For each <dir> in SUBDIRS, call add_subdirectory() iff
#       <PREFIX>_BUILD_EXAMPLES is ON. Emits one STATUS line per added
#       directory so configure-time output is uniform across systems.
#
#   kcenon_template_add_example(<name> SOURCES <src>... [LINKS <lib>...])
#       Define a single example executable. Useful when the example is a
#       single .cpp file and a full subdirectory is overkill.
# =============================================================================

include_guard(GLOBAL)

function(kcenon_template_add_example_dirs prefix)
    cmake_parse_arguments(ARG "" "" "SUBDIRS" ${ARGN})

    set(_opt "${prefix}_BUILD_EXAMPLES")
    if(NOT ${${_opt}})
        return()
    endif()

    foreach(_dir IN LISTS ARG_SUBDIRS)
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${_dir}/CMakeLists.txt")
            add_subdirectory(${_dir})
            message(STATUS "  [OK] example: ${_dir}")
        else()
            message(STATUS "  [--] example: ${_dir} (no CMakeLists.txt found)")
        endif()
    endforeach()
endfunction()

function(kcenon_template_add_example name)
    cmake_parse_arguments(ARG "" "" "SOURCES;LINKS" ${ARGN})

    if(NOT ARG_SOURCES)
        message(FATAL_ERROR "kcenon_template_add_example(${name}): SOURCES is required")
    endif()

    add_executable(${name} ${ARG_SOURCES})
    if(ARG_LINKS)
        target_link_libraries(${name} PRIVATE ${ARG_LINKS})
    endif()
endfunction()
