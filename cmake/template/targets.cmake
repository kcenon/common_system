# =============================================================================
# kcenon-cmake-template :: targets.cmake
# -----------------------------------------------------------------------------
# Responsibility
#   Provide helpers for the canonical kcenon target conventions:
#     * project-prefixed target names
#     * `<namespace>::<export_name>` ALIAS for downstream consumers
#     * explicit source enumeration (no globbing)
#     * `BUILD_INTERFACE` / `INSTALL_INTERFACE` include directories
#
#   Library and executable target *definitions themselves* are project-specific
#   and must be authored by the consumer. The template only standardises the
#   conventions and reduces boilerplate around them.
#
# Required input variables
#   None.
#
# Provided helpers
#   kcenon_template_setup_target_includes(<target> <visibility>
#                                         <build_dir> [INSTALL_DIR <dir>])
#       Attach BUILD_INTERFACE include from <build_dir> and INSTALL_INTERFACE
#       include from <install_dir> (default: include) to <target>.
#
#   kcenon_template_set_cxx_feature_std(<target> <visibility> <standard>)
#       Apply target_compile_features for the requested C++ standard, e.g.
#       `kcenon_template_set_cxx_feature_std(my_lib INTERFACE 20)`.
#
#   kcenon_template_create_aliases(<target> <namespace>... )
#       Create one ALIAS per <namespace>::<target> for backward compatibility
#       with multiple historical alias names. Idempotent.
#
# Conventions enforced by reviewers (not by code)
#   * Library names use the project name as prefix
#       add_library(${PROJECT_NAME}_core STATIC ...)
#   * Sources are enumerated explicitly — never `file(GLOB ...)`
#   * Public include directories use BUILD_INTERFACE / INSTALL_INTERFACE
#   * After defining a target, call
#       kcenon_template_register_export_target(<target> <export_name> <namespace>)
#     to assign EXPORT_NAME and create the namespaced ALIAS.
# =============================================================================

include_guard(GLOBAL)

function(kcenon_template_setup_target_includes target visibility build_dir)
    cmake_parse_arguments(ARG "" "INSTALL_DIR" "" ${ARGN})
    if(NOT ARG_INSTALL_DIR)
        set(ARG_INSTALL_DIR "include")
    endif()

    target_include_directories(${target} ${visibility}
        $<BUILD_INTERFACE:${build_dir}>
        $<INSTALL_INTERFACE:${ARG_INSTALL_DIR}>
    )
endfunction()

function(kcenon_template_set_cxx_feature_std target visibility standard)
    target_compile_features(${target} ${visibility} cxx_std_${standard})
endfunction()

function(kcenon_template_create_aliases target)
    foreach(_alias IN LISTS ARGN)
        if(NOT TARGET ${_alias})
            add_library(${_alias} ALIAS ${target})
        endif()
    endforeach()
endfunction()
