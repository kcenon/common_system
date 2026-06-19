# =============================================================================
# kcenon-cmake-template :: install.cmake
# -----------------------------------------------------------------------------
# Responsibility
#   Standardise install rules and CMake config-package generation. Provides a
#   single high-level helper that performs the recurring sequence:
#     1. install(TARGETS ... EXPORT)
#     2. configure_package_config_file()
#     3. write_basic_package_version_file()
#     4. install(EXPORT)
#     5. install(FILES) for the generated -config / -config-version
#     6. export(EXPORT) for the build tree
#
#   The helper accepts lists so it works for both single-target header-only
#   libraries (common_system, container_system) and multi-target stacks
#   (pacs_system).
#
# Required input variables
#   PROJECT_NAME, PROJECT_VERSION must already be set by `project()`.
#
# Provided helpers
#   kcenon_template_install_package(
#       PACKAGE_NAME <name>
#       NAMESPACE    <ns>
#       TARGETS      <tgt> [<tgt>...]
#       CONFIG_TEMPLATE <path-to-Config.cmake.in>
#       [EXTRA_FILES <file>...]
#       [COMPATIBILITY <SameMajorVersion|SameMinorVersion|AnyNewerVersion|ExactVersion>]
#   )
#       Run the full install + export sequence for <PACKAGE_NAME> using
#       <NAMESPACE>:: as the export namespace. The CONFIG_TEMPLATE is
#       processed via configure_package_config_file. EXTRA_FILES are
#       installed alongside the generated config (e.g. project-specific
#       feature helpers that the consumer might want to ship).
#
#   kcenon_template_install_headers(<source_dir>
#                                   [DESTINATION <dir>]
#                                   [PATTERNS <glob>...])
#       install(DIRECTORY <source_dir>) with FILES_MATCHING for the given
#       header patterns (default *.h *.hpp). DESTINATION defaults to
#       CMAKE_INSTALL_INCLUDEDIR.
# =============================================================================

include_guard(GLOBAL)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

function(kcenon_template_install_package)
    cmake_parse_arguments(ARG ""
        "PACKAGE_NAME;NAMESPACE;CONFIG_TEMPLATE;COMPATIBILITY"
        "TARGETS;EXTRA_FILES" ${ARGN})

    if(NOT ARG_PACKAGE_NAME)
        message(FATAL_ERROR "kcenon_template_install_package: PACKAGE_NAME is required")
    endif()
    if(NOT ARG_NAMESPACE)
        message(FATAL_ERROR "kcenon_template_install_package: NAMESPACE is required")
    endif()
    if(NOT ARG_TARGETS)
        message(FATAL_ERROR "kcenon_template_install_package: TARGETS is required")
    endif()
    if(NOT ARG_CONFIG_TEMPLATE)
        message(FATAL_ERROR "kcenon_template_install_package: CONFIG_TEMPLATE is required")
    endif()
    if(NOT ARG_COMPATIBILITY)
        set(ARG_COMPATIBILITY "AnyNewerVersion")
    endif()

    set(_install_cmake_dir "${CMAKE_INSTALL_LIBDIR}/cmake/${ARG_PACKAGE_NAME}")
    set(_export_set "${ARG_PACKAGE_NAME}-targets")
    set(_config_out "${CMAKE_CURRENT_BINARY_DIR}/${ARG_PACKAGE_NAME}-config.cmake")
    set(_version_out "${CMAKE_CURRENT_BINARY_DIR}/${ARG_PACKAGE_NAME}-config-version.cmake")

    install(TARGETS ${ARG_TARGETS}
        EXPORT ${_export_set}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

    configure_package_config_file(
        "${ARG_CONFIG_TEMPLATE}"
        "${_config_out}"
        INSTALL_DESTINATION "${_install_cmake_dir}"
        PATH_VARS CMAKE_INSTALL_INCLUDEDIR
    )

    write_basic_package_version_file(
        "${_version_out}"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY ${ARG_COMPATIBILITY}
    )

    install(EXPORT ${_export_set}
        FILE ${_export_set}.cmake
        NAMESPACE ${ARG_NAMESPACE}::
        DESTINATION "${_install_cmake_dir}"
    )

    install(FILES "${_config_out}" "${_version_out}" ${ARG_EXTRA_FILES}
        DESTINATION "${_install_cmake_dir}"
    )

    export(EXPORT ${_export_set}
        FILE "${CMAKE_CURRENT_BINARY_DIR}/${_export_set}.cmake"
        NAMESPACE ${ARG_NAMESPACE}::
    )
endfunction()

function(kcenon_template_install_headers source_dir)
    cmake_parse_arguments(ARG "" "DESTINATION" "PATTERNS" ${ARGN})
    if(NOT ARG_DESTINATION)
        set(ARG_DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
    endif()
    if(NOT ARG_PATTERNS)
        set(ARG_PATTERNS "*.h" "*.hpp")
    endif()

    set(_match_args "")
    foreach(_pat IN LISTS ARG_PATTERNS)
        list(APPEND _match_args PATTERN "${_pat}")
    endforeach()

    install(DIRECTORY "${source_dir}/"
        DESTINATION "${ARG_DESTINATION}"
        FILES_MATCHING ${_match_args}
    )
endfunction()
