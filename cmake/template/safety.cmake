# =============================================================================
# kcenon-cmake-template :: safety.cmake
# -----------------------------------------------------------------------------
# Responsibility
#   Provide guard-style helpers that prevent dangerous build configurations
#   from reaching production artifacts. Each helper encodes a defensive idiom
#   that downstream systems would otherwise have to re-implement (and
#   occasionally forget).
#
# Required input variables
#   None at module load time. Helpers consume their inputs as arguments.
#
# Provided helpers
#   kcenon_template_forbid_in_release(<flag_var> [REASON <text>])
#       Emit FATAL_ERROR if <flag_var> is truthy and CMAKE_BUILD_TYPE is
#       STREQUAL "Release". Intended for mock SDK flags, in-process test
#       stubs, and similar developer-only options that must never ship in
#       Release artifacts.
#
#       Multi-config generators (Visual Studio, Xcode, Ninja Multi-Config)
#       leave CMAKE_BUILD_TYPE empty at configure time because the build
#       type is selected per-build, not per-configure. The helper detects
#       this case and skips the configure-time check; per-config
#       enforcement on multi-config generators must be expressed as a
#       generator-expression guard at the call site if required.
#
#       Arguments:
#         <flag_var>  Name (not value) of the cache/option variable to
#                     check. Passed unquoted in CMake style.
#         REASON      Optional human-readable explanation appended to the
#                     FATAL_ERROR message. When omitted a generic message
#                     is emitted.
#
#       Example:
#         option(PACS_USE_MOCK_S3 "Use in-process S3 mock" OFF)
#         kcenon_template_forbid_in_release(PACS_USE_MOCK_S3
#             REASON "Mock S3 transport must not ship in Release builds.")
# =============================================================================

include_guard(GLOBAL)

# -----------------------------------------------------------------------------
# Release-build guard
# -----------------------------------------------------------------------------
function(kcenon_template_forbid_in_release flag_var)
    cmake_parse_arguments(_KFR "" "REASON" "" ${ARGN})

    if(_KFR_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR
            "kcenon_template_forbid_in_release: unexpected arguments: "
            "${_KFR_UNPARSED_ARGUMENTS}")
    endif()

    # Multi-config generators leave CMAKE_BUILD_TYPE empty; the per-config
    # gate happens at build time, so skip the configure-time check.
    if(NOT CMAKE_BUILD_TYPE)
        return()
    endif()

    if(NOT CMAKE_BUILD_TYPE STREQUAL "Release")
        return()
    endif()

    if(NOT ${flag_var})
        return()
    endif()

    if(_KFR_REASON)
        message(FATAL_ERROR
            "${flag_var} cannot be enabled in Release builds. ${_KFR_REASON}")
    else()
        message(FATAL_ERROR
            "${flag_var} cannot be enabled in Release builds.")
    endif()
endfunction()
