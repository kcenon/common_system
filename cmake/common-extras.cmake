# =============================================================================
# common_system :: common-extras.cmake
# -----------------------------------------------------------------------------
# Responsibility
#   Wire the optional, non-mandatory build artefacts that are not part of the
#   layout-standard canonical decomposition: examples, benchmarks, Doxygen
#   documentation, and code-coverage instrumentation for the INTERFACE library.
#
# Required input variables
#   COMMON_BUILD_EXAMPLES    - Cache option declared by the root.
#   COMMON_BUILD_BENCHMARKS  - Cache option (template-defined).
#   COMMON_BUILD_DOCS        - Cache option (template-defined).
#   ENABLE_COVERAGE          - Cache option declared by the root.
#
# Side effects
#   - Adds examples/ subdirectory when COMMON_BUILD_EXAMPLES is ON and the
#     subdirectory CMakeLists.txt exists.
#   - Adds benchmarks/ subdirectory when COMMON_BUILD_BENCHMARKS is ON and
#     Google Benchmark is available.
#   - Defines a `common_docs` Doxygen target when COMMON_BUILD_DOCS is ON
#     and Doxygen is available.
#   - Appends --coverage flags to the common_system INTERFACE target when
#     ENABLE_COVERAGE is ON and the compiler is GCC or Clang.
# =============================================================================

# -----------------------------------------------------------------------------
# Examples
# -----------------------------------------------------------------------------
if(COMMON_BUILD_EXAMPLES AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/examples/CMakeLists.txt")
    add_subdirectory(examples)
endif()

# -----------------------------------------------------------------------------
# Benchmarks (project-specific, not standardised by the template)
# -----------------------------------------------------------------------------
if(COMMON_BUILD_BENCHMARKS)
    find_package(benchmark QUIET)
    if(benchmark_FOUND)
        add_subdirectory(benchmarks)
    else()
        message(STATUS "Google Benchmark not found, skipping benchmarks")
    endif()
endif()

# -----------------------------------------------------------------------------
# Documentation (project-specific Doxygen target)
# -----------------------------------------------------------------------------
if(COMMON_BUILD_DOCS)
    find_package(Doxygen)
    if(DOXYGEN_FOUND)
        set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in)
        set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

        configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)

        add_custom_target(common_docs ALL
            COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Generating API documentation with Doxygen"
            VERBATIM
        )
    endif()
endif()

# -----------------------------------------------------------------------------
# Coverage support (project-specific, applies to INTERFACE library)
# -----------------------------------------------------------------------------
if(ENABLE_COVERAGE)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(common_system INTERFACE --coverage)
        target_link_options(common_system INTERFACE --coverage)
        message(STATUS "Code coverage enabled")
    else()
        message(WARNING "Coverage not supported on this compiler")
    endif()
endif()
