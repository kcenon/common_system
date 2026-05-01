# =============================================================================
# kcenon-cmake-template :: testing.cmake
# -----------------------------------------------------------------------------
# Responsibility
#   Centralise test infrastructure: enable_testing(), test framework discovery,
#   and a single helper to register a Google Test executable. The ecosystem
#   default framework is Google Test (1.14.0); the helper is opinionated to
#   keep registration uniform across systems.
#
# Required input variables
#   None. The caller decides when (and under which option combination) to
#   invoke the helpers below.
#
# Provided helpers
#   kcenon_template_setup_testing()
#       Call enable_testing() and find_package(GTest). Reports findings via
#       message(STATUS). Sets `KCENON_TEMPLATE_GTEST_FOUND` in the parent
#       scope so the consumer can `add_subdirectory(tests)` only when the
#       framework is actually available. The caller is responsible for
#       gating the call on its own option combination — projects that use a
#       single `<PREFIX>_BUILD_TESTS` flag should call this helper inside
#       `if(${PREFIX}_BUILD_TESTS)`; projects with multiple test flags
#       (e.g. unit + integration) wrap the call with their own composite
#       condition.
#
#   kcenon_template_add_gtest(<name>
#       SOURCES <src>...
#       [LINKS <lib>...]
#       [INCLUDES <dir>...]
#       [COMPILE_DEFINITIONS <def>...]
#       [TEST_NAME <ctest-name>])
#       Define a Google Test executable, link gtest_main and any extra
#       libraries, register it with CTest. <TEST_NAME> overrides the default
#       CTest name (which equals <name>).
# =============================================================================

include_guard(GLOBAL)

function(kcenon_template_setup_testing)
    enable_testing()

    find_package(GTest QUIET)
    if(GTest_FOUND)
        if(DEFINED GTest_VERSION)
            message(STATUS "GTest version: ${GTest_VERSION} (ecosystem pinned: 1.14.0)")
        else()
            message(STATUS "GTest found (version unknown, ecosystem pinned: 1.14.0)")
        endif()
        set(KCENON_TEMPLATE_GTEST_FOUND TRUE PARENT_SCOPE)
    else()
        message(STATUS "GTest not found, skipping tests")
        set(KCENON_TEMPLATE_GTEST_FOUND FALSE PARENT_SCOPE)
    endif()
endfunction()

function(kcenon_template_add_gtest name)
    cmake_parse_arguments(ARG ""
        "TEST_NAME"
        "SOURCES;LINKS;INCLUDES;COMPILE_DEFINITIONS" ${ARGN})

    if(NOT ARG_SOURCES)
        message(FATAL_ERROR "kcenon_template_add_gtest(${name}): SOURCES is required")
    endif()

    add_executable(${name} ${ARG_SOURCES})

    target_link_libraries(${name} PRIVATE GTest::gtest GTest::gtest_main)
    if(ARG_LINKS)
        target_link_libraries(${name} PRIVATE ${ARG_LINKS})
    endif()
    if(ARG_INCLUDES)
        target_include_directories(${name} PRIVATE ${ARG_INCLUDES})
    endif()
    if(ARG_COMPILE_DEFINITIONS)
        target_compile_definitions(${name} PRIVATE ${ARG_COMPILE_DEFINITIONS})
    endif()

    if(NOT ARG_TEST_NAME)
        set(ARG_TEST_NAME "${name}")
    endif()
    add_test(NAME ${ARG_TEST_NAME} COMMAND ${name})
endfunction()
