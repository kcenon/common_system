# =============================================================================
# common_system :: common-summary.cmake
# -----------------------------------------------------------------------------
# Responsibility
#   Print a final configuration summary block with the resolved option values
#   so contributors can verify the configure step at a glance. Implemented as
#   the reference adopter of the template helper introduced in
#   kcenon/common_system#668.
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
#
# Side effects
#   - Emits a series of message(STATUS ...) lines during configure via
#     `kcenon_template_print_summary()`.
# =============================================================================

include(summary)

kcenon_template_print_summary(
    PROJECT  common_system
    OPTIONS
        COMMON_HEADER_ONLY
        COMMON_BUILD_TESTS
        COMMON_BUILD_INTEGRATION_TESTS
        COMMON_BUILD_EXAMPLES
        COMMON_BUILD_BENCHMARKS
        COMMON_BUILD_DOCS
        ENABLE_COVERAGE
        BUILD_WITH_YAML_CPP
        CMAKE_INSTALL_PREFIX
)
