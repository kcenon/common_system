# =============================================================================
# kcenon-cmake-template :: warnings.cmake
# -----------------------------------------------------------------------------
# Responsibility
#   Provide compiler warning flags scoped per target so they never leak into
#   downstream `add_subdirectory()`d dependencies. The flag list is selected
#   per compiler family and gated on the project's `<PREFIX>_WARNINGS_AS_ERRORS`
#   option declared by `kcenon_template_define_standard_options()`.
#
# Required input variables
#   <PREFIX>_WARNINGS_AS_ERRORS
#       Boolean cache option from options.cmake. The PREFIX is the same
#       string passed to `kcenon_template_define_standard_options()` and is
#       supplied at call time via `kcenon_template_init_warnings(<PREFIX>)`.
#
# Provided helpers
#   kcenon_template_init_warnings(<PREFIX>)
#       Compute the warning-flag list for the current compiler and store it
#       in a global property so `kcenon_template_apply_warnings()` can
#       reuse it for every target without recomputing.
#
#   kcenon_template_apply_warnings(<target>)
#       Apply the precomputed warning flags PRIVATE to <target>. No-op when
#       the target does not exist (lets callers register optional targets
#       unconditionally).
#
# Usage notes
#   * Always uses target_compile_options PRIVATE — never add_compile_options.
#   * Header-only INTERFACE targets are skipped because `PRIVATE` is invalid
#     for INTERFACE libraries; project warnings only apply to compiled targets.
# =============================================================================

include_guard(GLOBAL)

function(kcenon_template_init_warnings prefix)
    set(_werror_var "${prefix}_WARNINGS_AS_ERRORS")
    set(_flags "")

    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU|AppleClang")
        list(APPEND _flags -Wall -Wextra -Wpedantic)
        if(${_werror_var})
            list(APPEND _flags -Werror)
        endif()
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
        list(APPEND _flags /W4)
        if(${_werror_var})
            list(APPEND _flags /WX)
        endif()
    endif()

    set_property(GLOBAL PROPERTY KCENON_TEMPLATE_WARNING_FLAGS "${_flags}")
endfunction()

function(kcenon_template_apply_warnings target)
    if(NOT TARGET ${target})
        return()
    endif()
    get_target_property(_type ${target} TYPE)
    if(_type STREQUAL "INTERFACE_LIBRARY")
        return()
    endif()
    get_property(_flags GLOBAL PROPERTY KCENON_TEMPLATE_WARNING_FLAGS)
    if(_flags)
        target_compile_options(${target} PRIVATE ${_flags})
    endif()
endfunction()
