# =============================================================================
# kcenon-cmake-template :: summary.cmake
# -----------------------------------------------------------------------------
# Responsibility
#   Print a uniform build-configuration summary block at the end of the
#   configure step so contributors and CI logs share the same shape across
#   every system in the kcenon ecosystem. Promoted from the pacs_system local
#   helper (see kcenon/common_system#668).
#
# Required input variables
#   None at module load time. Inputs are passed as keyword arguments to the
#   helper below.
#
# Provided helpers
#   kcenon_template_print_summary(
#       PROJECT       <name>
#       [VERSION      <ver>]
#       [BANNER       <text>]
#       [OPTIONS      <var>...]
#       [TARGETS      <tgt>...]
#       [DEPENDENCIES <var=display>...])
#
#       Print a uniform configuration summary block to STDOUT at configure
#       time using `message(STATUS ...)`.
#
#       Arguments:
#         PROJECT       Required. Display name to put in the banner line.
#         VERSION       Optional. Version string. Defaults to ${PROJECT_VERSION}.
#         BANNER        Optional. Extra one-line subtitle below the project
#                       banner (e.g. a tagline or tier hint).
#         OPTIONS       Optional. List of cache variable names to print in an
#                       "Options:" block as `<name>: <value>`.
#         TARGETS       Optional. List of CMake target names to print as
#                       `<name>: ON|OFF` based on TARGET existence.
#         DEPENDENCIES  Optional. List of `<var>=<display>` pairs. Prints
#                       `<display>: ON|OFF` based on the value of <var>
#                       (boolean truthy/falsy).
#
#       The helper itself supplies the surrounding banner, divider, and
#       trailing blank lines so callers do not need to reproduce them.
# =============================================================================

include_guard(GLOBAL)

function(kcenon_template_print_summary)
    set(_options "")
    set(_one_value PROJECT VERSION BANNER)
    set(_multi_value OPTIONS TARGETS DEPENDENCIES)
    cmake_parse_arguments(_S "${_options}" "${_one_value}" "${_multi_value}" ${ARGN})

    if(NOT _S_PROJECT)
        message(FATAL_ERROR
            "kcenon_template_print_summary: PROJECT <name> is required.")
    endif()

    if(_S_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR
            "kcenon_template_print_summary: unrecognized arguments: "
            "${_S_UNPARSED_ARGUMENTS}")
    endif()

    if(NOT _S_VERSION)
        set(_S_VERSION "${PROJECT_VERSION}")
    endif()

    set(_divider "========================================")

    message(STATUS "")
    message(STATUS "${_divider}")
    if(_S_VERSION)
        message(STATUS "${_S_PROJECT} v${_S_VERSION}")
    else()
        message(STATUS "${_S_PROJECT}")
    endif()
    if(_S_BANNER)
        message(STATUS "${_S_BANNER}")
    endif()
    message(STATUS "${_divider}")

    if(_S_OPTIONS)
        message(STATUS "")
        message(STATUS "Options:")
        foreach(_var IN LISTS _S_OPTIONS)
            if(DEFINED ${_var})
                message(STATUS "  ${_var}: ${${_var}}")
            else()
                message(STATUS "  ${_var}: <unset>")
            endif()
        endforeach()
    endif()

    if(_S_TARGETS)
        message(STATUS "")
        message(STATUS "Targets:")
        foreach(_tgt IN LISTS _S_TARGETS)
            if(TARGET "${_tgt}")
                message(STATUS "  ${_tgt}: ON")
            else()
                message(STATUS "  ${_tgt}: OFF")
            endif()
        endforeach()
    endif()

    if(_S_DEPENDENCIES)
        message(STATUS "")
        message(STATUS "Dependencies:")
        foreach(_pair IN LISTS _S_DEPENDENCIES)
            string(FIND "${_pair}" "=" _eq_pos)
            if(_eq_pos GREATER -1)
                string(SUBSTRING "${_pair}" 0 ${_eq_pos} _var)
                math(EXPR _val_start "${_eq_pos} + 1")
                string(SUBSTRING "${_pair}" ${_val_start} -1 _display)
            else()
                set(_var "${_pair}")
                set(_display "${_pair}")
            endif()
            if(${_var})
                message(STATUS "  ${_display}: ON")
            else()
                message(STATUS "  ${_display}: OFF")
            endif()
        endforeach()
    endif()

    message(STATUS "${_divider}")
    message(STATUS "")
endfunction()
