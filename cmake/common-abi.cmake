# =============================================================================
# common_system :: common-abi.cmake
# -----------------------------------------------------------------------------
# Responsibility
#   Generate the ABI version header and translation unit from the in-tree
#   templates, and honour the umbrella BUILD_INTEGRATION_TESTS flag passed
#   from a parent build (e.g. ecosystem cross-build).
#
# Required input variables
#   CMAKE_CURRENT_SOURCE_DIR   - Project root (set by CMake automatically).
#   CMAKE_CURRENT_BINARY_DIR   - Project build dir (set by CMake automatically).
#   COMMON_BUILD_INTEGRATION_TESTS - Cache option declared by the root.
#
# Provided variables
#   EVENT_BUS_ABI_VERSION       - Cache string (default "1") used both as a
#                                 CMake message and as a preprocessor token
#                                 in the generated headers.
#   CMAKE_CONFIGURE_TIMESTAMP   - String stamped into abi_version.h.in.
#
# Side effects
#   - Writes <build>/include/kcenon/common/config/abi_version.h
#   - Writes <build>/src/config/abi_version.cpp
#   - Forces COMMON_BUILD_INTEGRATION_TESTS to follow BUILD_INTEGRATION_TESTS
#     when the umbrella build defines it.
# =============================================================================

set(EVENT_BUS_ABI_VERSION 1 CACHE STRING "Event bus ABI version (1=standalone)")
message(STATUS "Event Bus ABI Version: ${EVENT_BUS_ABI_VERSION} (standalone implementation)")

string(TIMESTAMP CMAKE_CONFIGURE_TIMESTAMP "%Y-%m-%d %H:%M:%S UTC" UTC)

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/include/kcenon/common/config/abi_version.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/include/kcenon/common/config/abi_version.h"
    @ONLY
)

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/src/config/abi_version.cpp.in"
    "${CMAKE_CURRENT_BINARY_DIR}/src/config/abi_version.cpp"
    @ONLY
)

message(STATUS "Generated ABI version header: ${CMAKE_CURRENT_BINARY_DIR}/include/kcenon/common/config/abi_version.h")
message(STATUS "Generated ABI version source: ${CMAKE_CURRENT_BINARY_DIR}/src/config/abi_version.cpp")

# Honour the umbrella BUILD_INTEGRATION_TESTS flag if set by a parent build.
if(DEFINED BUILD_INTEGRATION_TESTS)
    if(BUILD_INTEGRATION_TESTS)
        set(_COMMON_BUILD_IT_VALUE ON)
    else()
        set(_COMMON_BUILD_IT_VALUE OFF)
    endif()
    set(COMMON_BUILD_INTEGRATION_TESTS ${_COMMON_BUILD_IT_VALUE} CACHE BOOL
        "Build integration tests for common_system" FORCE)
    unset(_COMMON_BUILD_IT_VALUE)
endif()
