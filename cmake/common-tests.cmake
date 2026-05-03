# =============================================================================
# common_system :: common-tests.cmake
# -----------------------------------------------------------------------------
# Responsibility
#   Wire the unit-test and integration-test subdirectories. Calls the template
#   helper kcenon_template_setup_testing(), which sets up GTest discovery and
#   invokes enable_testing() at the root scope (per layout standard rule R-34).
#   Subdirectories are included only when GTest was located, so a missing
#   GTest does not break configuration.
#
# Required input variables
#   COMMON_BUILD_TESTS              - Cache option (template-defined).
#   COMMON_BUILD_INTEGRATION_TESTS  - Cache option (root-defined).
#
# Side effects
#   - Adds tests/ and integration_tests/ subdirectories (when GTest is present
#     and the corresponding option is ON).
# =============================================================================

if(COMMON_BUILD_TESTS OR COMMON_BUILD_INTEGRATION_TESTS)
    kcenon_template_setup_testing()
    if(KCENON_TEMPLATE_GTEST_FOUND)
        if(COMMON_BUILD_TESTS)
            add_subdirectory(tests)
        endif()
        if(COMMON_BUILD_INTEGRATION_TESTS)
            add_subdirectory(integration_tests)
        endif()
    endif()
endif()
