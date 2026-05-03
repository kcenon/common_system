# =============================================================================
# common_system :: common-modules.cmake
# -----------------------------------------------------------------------------
# Responsibility
#   Provide an optional C++20 named-modules build of common_system. Only
#   activates when COMMON_BUILD_MODULES is ON and the toolchain supports it
#   (CMake >= 3.28, Ninja or Visual Studio generator, Clang 16+ / GCC 14+ /
#   MSVC 2022 17.4+). On unsupported toolchains the module build is silently
#   disabled with a warning, so this module is safe to always include.
#
# Required input variables
#   COMMON_BUILD_MODULES   - Cache option declared by the template
#                            (kcenon_template_define_standard_options).
#   EVENT_BUS_ABI_VERSION  - Provided by common-abi.cmake.
#   KCENON_MIN_*_MODULE_VERSION - Provided by KcenonCompilerRequirements.cmake.
#
# Provided targets (when enabled)
#   common_system_modules  - Library exposing the module unit FILE_SET.
#   kcenon::common_modules - Alias of common_system_modules.
# =============================================================================

if(NOT COMMON_BUILD_MODULES)
    return()
endif()

if(CMAKE_VERSION VERSION_LESS "3.28")
    message(WARNING "C++20 modules require CMake 3.28+. Disabling module build.")
    set(COMMON_BUILD_MODULES OFF)
    return()
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
    message(WARNING "AppleClang does not support C++20 module dependency scanning. "
                    "Use Clang ${KCENON_MIN_CLANG_MODULE_VERSION}+, "
                    "GCC ${KCENON_MIN_GCC_MODULE_VERSION}+, or "
                    "MSVC ${KCENON_MIN_MSVC_MODULE_VERSION}+. Disabling module build.")
    set(COMMON_BUILD_MODULES OFF)
    return()
endif()

if(NOT CMAKE_GENERATOR MATCHES "Ninja" AND NOT CMAKE_GENERATOR MATCHES "Visual Studio")
    message(WARNING "C++20 modules require Ninja or Visual Studio generator. "
                    "Current generator: ${CMAKE_GENERATOR}. Disabling module build.")
    set(COMMON_BUILD_MODULES OFF)
    return()
endif()

kcenon_check_compiler_requirements(MODULES)
message(STATUS "C++20 module build enabled")

add_library(common_system_modules)
add_library(kcenon::common_modules ALIAS common_system_modules)
kcenon_template_set_cxx_feature_std(common_system_modules PUBLIC 20)

set_target_properties(common_system_modules PROPERTIES
    CXX_SCAN_FOR_MODULES ON
)

target_sources(common_system_modules
    PUBLIC FILE_SET CXX_MODULES
    FILES
        src/modules/common.cppm
        src/modules/utils.cppm
        src/modules/error.cppm
        src/modules/result.cppm
        src/modules/result/core.cppm
        src/modules/result/utilities.cppm
        src/modules/concepts.cppm
        src/modules/interfaces.cppm
        src/modules/interfaces/logger.cppm
        src/modules/interfaces/executor.cppm
        src/modules/interfaces/core.cppm
        src/modules/config.cppm
        src/modules/di.cppm
        src/modules/patterns.cppm
        src/modules/logging.cppm
)

kcenon_template_setup_target_includes(common_system_modules PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    INSTALL_DIR "include"
)
target_include_directories(common_system_modules PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
)

target_compile_definitions(common_system_modules PUBLIC
    EVENT_BUS_ABI_VERSION=${EVENT_BUS_ABI_VERSION}
    KCENON_WITH_COMMON_SYSTEM=1
    KCENON_USE_MODULES=1
)

kcenon_template_apply_warnings(common_system_modules)

message(STATUS "C++20 module target: common_system_modules")
