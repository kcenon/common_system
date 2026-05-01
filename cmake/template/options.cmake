# =============================================================================
# kcenon-cmake-template :: options.cmake
# -----------------------------------------------------------------------------
# Responsibility
#   Define the canonical set of build options shared by every kcenon ecosystem
#   system, plus a small set of utility helpers used by the other template
#   modules. Project-specific options are declared by the consumer AFTER calling
#   `kcenon_template_define_standard_options()`.
#
# Required input variables
#   None at module load time. The helpers below expect a project prefix
#   passed as an argument.
#
# Provided helpers
#   kcenon_template_define_standard_options(<PREFIX>)
#       Define the standard options:
#         <PREFIX>_BUILD_TESTS         (default ON)
#         <PREFIX>_BUILD_EXAMPLES      (default OFF)
#         <PREFIX>_BUILD_BENCHMARKS    (default OFF)
#         <PREFIX>_BUILD_DOCS          (default OFF)
#         <PREFIX>_BUILD_MODULES       (default OFF)
#         <PREFIX>_WARNINGS_AS_ERRORS  (default ON)
#       <PREFIX> MUST be uppercase ASCII (e.g. COMMON, PACS, NETWORK).
#
#   kcenon_template_get_export_bridge_target(<out_var> <build_target>
#                                            <install_target>)
#       Allocate (or reuse) an INTERFACE bridge target that decouples the
#       build-tree representation of a dependency from its install-tree
#       package alias, so install(EXPORT) sees a single resolvable name.
#
#   kcenon_template_link_external_dependency(<target> <visibility>
#                                            <build_target> <install_target>)
#       Link <target> to a dependency using BUILD_LOCAL_INTERFACE for the
#       build tree and the bridge target for the install tree.
#
#   kcenon_template_register_export_target(<target> <export_name>
#                                          <namespace>)
#       Set EXPORT_NAME and create a `<namespace>::<export_name>` ALIAS for
#       downstream consumers.
# =============================================================================

include_guard(GLOBAL)

# -----------------------------------------------------------------------------
# Standard options
# -----------------------------------------------------------------------------
function(kcenon_template_define_standard_options prefix)
    if(NOT prefix MATCHES "^[A-Z][A-Z0-9_]*$")
        message(FATAL_ERROR
            "kcenon_template_define_standard_options: prefix '${prefix}' "
            "must be uppercase ASCII (letters, digits, underscores).")
    endif()

    # Use option() with the resolved name. We pre-compute strings so the
    # option name is fully expanded before option() sees it.
    set(_opt_tests        "${prefix}_BUILD_TESTS")
    set(_opt_examples     "${prefix}_BUILD_EXAMPLES")
    set(_opt_benchmarks   "${prefix}_BUILD_BENCHMARKS")
    set(_opt_docs         "${prefix}_BUILD_DOCS")
    set(_opt_modules      "${prefix}_BUILD_MODULES")
    set(_opt_werror       "${prefix}_WARNINGS_AS_ERRORS")

    option(${_opt_tests}      "Build unit tests"                       ON)
    option(${_opt_examples}   "Build example programs"                 OFF)
    option(${_opt_benchmarks} "Build performance benchmarks"           OFF)
    option(${_opt_docs}       "Generate API documentation (Doxygen)"   OFF)
    option(${_opt_modules}    "Build C++20 modules variant"            OFF)
    option(${_opt_werror}     "Treat compiler warnings as errors"      ON)
endfunction()

# -----------------------------------------------------------------------------
# Export-bridge target helpers
# -----------------------------------------------------------------------------
function(kcenon_template_get_export_bridge_target out_var build_target install_target)
    if(NOT TARGET "${build_target}")
        set(${out_var} "" PARENT_SCOPE)
        return()
    endif()

    string(MAKE_C_IDENTIFIER "${build_target}" _suffix)
    set(_bridge "kcenon_export_dep_${_suffix}")

    if(NOT TARGET ${_bridge})
        add_library(${_bridge} INTERFACE)
        # Bridge targets stay link-empty during export generation; the
        # installed package config hydrates them after find_dependency().
        set_property(GLOBAL APPEND PROPERTY KCENON_EXPORT_BRIDGE_MAPPINGS
            "${_bridge}=${install_target}")
        set_property(GLOBAL APPEND PROPERTY KCENON_EXPORT_BRIDGE_TARGETS ${_bridge})
    endif()

    set(${out_var} ${_bridge} PARENT_SCOPE)
endfunction()

function(kcenon_template_link_external_dependency target visibility build_target install_target)
    kcenon_template_get_export_bridge_target(_bridge "${build_target}" "${install_target}")
    if(_bridge)
        target_link_libraries(${target} ${visibility}
            "$<BUILD_LOCAL_INTERFACE:${build_target}>")
        target_link_libraries(${target} ${visibility}
            "$<INSTALL_INTERFACE:${_bridge}>")
    endif()
endfunction()

function(kcenon_template_register_export_target target export_name namespace)
    if(TARGET ${target})
        set_target_properties(${target} PROPERTIES EXPORT_NAME ${export_name})
        if(NOT TARGET ${namespace}::${export_name})
            add_library(${namespace}::${export_name} ALIAS ${target})
        endif()
    endif()
endfunction()
