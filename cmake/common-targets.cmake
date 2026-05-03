# =============================================================================
# common_system :: common-targets.cmake
# -----------------------------------------------------------------------------
# Responsibility
#   Define the project's public targets:
#     - common_system        (header-only INTERFACE)
#     - kcenon::common, kcenon::common_system, common_system::common_system
#       (aliases via kcenon_template_create_aliases)
#     - common_system_static (optional STATIC, only when COMMON_HEADER_ONLY=OFF)
#   Wire include directories, C++20 feature, ABI compile definitions, and the
#   optional yaml-cpp dependency for YAML configuration support.
#
# Required input variables
#   EVENT_BUS_ABI_VERSION  - Provided by common-abi.cmake.
#   COMMON_HEADER_ONLY     - Cache option declared by the root.
#   BUILD_WITH_YAML_CPP    - Cache option declared by the root.
#
# Provided targets
#   common_system          (INTERFACE library, always)
#   common_system_static   (STATIC library, when NOT COMMON_HEADER_ONLY)
# =============================================================================

# -----------------------------------------------------------------------------
# Header-only INTERFACE library + aliases
# -----------------------------------------------------------------------------
add_library(common_system INTERFACE)
kcenon_template_create_aliases(common_system
    kcenon::common
    kcenon::common_system
    common_system::common_system
)

kcenon_template_setup_target_includes(common_system INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    INSTALL_DIR "include"
)
target_include_directories(common_system INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
)

kcenon_template_set_cxx_feature_std(common_system INTERFACE 20)

# Define ABI version + KCENON_WITH_COMMON_SYSTEM marker for downstream projects
target_compile_definitions(common_system INTERFACE
    EVENT_BUS_ABI_VERSION=${EVENT_BUS_ABI_VERSION}
    KCENON_WITH_COMMON_SYSTEM=1
)

# Optional: YAML configuration support
if(BUILD_WITH_YAML_CPP)
    find_package(yaml-cpp QUIET)
    if(yaml-cpp_FOUND)
        target_link_libraries(common_system INTERFACE yaml-cpp::yaml-cpp)
        target_compile_definitions(common_system INTERFACE BUILD_WITH_YAML_CPP)
        message(STATUS "YAML configuration support: enabled (yaml-cpp found)")
    else()
        message(WARNING "BUILD_WITH_YAML_CPP is ON but yaml-cpp not found. YAML support disabled.")
    endif()
endif()

# -----------------------------------------------------------------------------
# Optional static-library variant when not header-only
# -----------------------------------------------------------------------------
if(NOT COMMON_HEADER_ONLY)
    add_library(common_system_static STATIC
        ${CMAKE_CURRENT_BINARY_DIR}/src/config/abi_version.cpp
    )
    kcenon_template_setup_target_includes(common_system_static PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        INSTALL_DIR "include"
    )
    target_include_directories(common_system_static PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
    )
    kcenon_template_set_cxx_feature_std(common_system_static PUBLIC 20)
    target_compile_definitions(common_system_static PUBLIC
        EVENT_BUS_ABI_VERSION=${EVENT_BUS_ABI_VERSION}
        KCENON_WITH_COMMON_SYSTEM=1
    )
    kcenon_template_apply_warnings(common_system_static)
endif()
