# =============================================================================
# common_system :: common-summary.cmake
# -----------------------------------------------------------------------------
# Responsibility
#   Print a final configuration summary block with the resolved option values
#   so contributors can verify the configure step at a glance.
#
# Required input variables
#   PROJECT_VERSION                - Set by project() in the root.
#   COMMON_HEADER_ONLY             - Cache option declared by the root.
#   COMMON_BUILD_TESTS             - Cache option (template-defined).
#   COMMON_BUILD_INTEGRATION_TESTS - Cache option declared by the root.
#   COMMON_BUILD_EXAMPLES          - Cache option declared by the root.
#   COMMON_BUILD_BENCHMARKS        - Cache option (template-defined).
#   COMMON_BUILD_DOCS              - Cache option (template-defined).
#   ENABLE_COVERAGE                - Cache option declared by the root.
#   BUILD_WITH_YAML_CPP            - Cache option declared by the root.
#   CMAKE_INSTALL_PREFIX           - Standard CMake variable.
#   GTest_FOUND, GTest_VERSION     - Provided by find_package(GTest) when run.
#
# Side effects
#   - Emits a series of message(STATUS ...) lines during configure.
# =============================================================================

message(STATUS "")
message(STATUS "=== common_system Configuration ===")
message(STATUS "Version:             ${PROJECT_VERSION}")
message(STATUS "Header-only:         ${COMMON_HEADER_ONLY}")
if(GTest_FOUND AND DEFINED GTest_VERSION)
    message(STATUS "GTest version:       ${GTest_VERSION}")
endif()
message(STATUS "Build tests:         ${COMMON_BUILD_TESTS}")
message(STATUS "Build integration:   ${COMMON_BUILD_INTEGRATION_TESTS}")
message(STATUS "Build examples:      ${COMMON_BUILD_EXAMPLES}")
message(STATUS "Build benchmarks:    ${COMMON_BUILD_BENCHMARKS}")
message(STATUS "Build docs:          ${COMMON_BUILD_DOCS}")
message(STATUS "Enable coverage:     ${ENABLE_COVERAGE}")
message(STATUS "YAML support:        ${BUILD_WITH_YAML_CPP}")
message(STATUS "Install prefix:      ${CMAKE_INSTALL_PREFIX}")
message(STATUS "======================================")
message(STATUS "")
