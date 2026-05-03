# =============================================================================
# common_system :: common-install.cmake
# -----------------------------------------------------------------------------
# Responsibility
#   Install public headers and the CMake package config for downstream
#   find_package(common_system) consumers. Includes the generated
#   abi_version.h (lives in build tree, not source tree) and ships the
#   project-specific helper modules (features.cmake, KcenonCompilerRequirements)
#   alongside the package config so they remain importable post-install.
#
# Required input variables
#   CMAKE_INSTALL_INCLUDEDIR - Provided by GNUInstallDirs (loaded by template).
#
# Side effects
#   - install(...) rules registered for headers, generated header, and
#     the CMake package directory.
# =============================================================================

kcenon_template_install_headers("${CMAKE_CURRENT_SOURCE_DIR}/include"
    PATTERNS "*.h" "*.hpp"
)

# Install generated abi_version.h (produced by configure_file, lives in build tree)
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/include/kcenon/common/config/abi_version.h"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/kcenon/common/config"
)

kcenon_template_install_package(
    PACKAGE_NAME    common_system
    NAMESPACE       common_system
    TARGETS         common_system
    CONFIG_TEMPLATE "${CMAKE_CURRENT_SOURCE_DIR}/cmake/common_system-config.cmake.in"
    EXTRA_FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/features.cmake"
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/KcenonCompilerRequirements.cmake"
)
