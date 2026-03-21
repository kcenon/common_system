# kcenon-common-system portfile
# High-performance C++20 foundation library (header-only)

vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO kcenon/common_system
    REF "v${VERSION}"
    SHA512 7385ba3a073fea06604f71a7ffc016425408c768444cec2ec897537411926a7e1fad99f7215e6724b3668a6e227f0716dbdcdda462764f5c4e52709087751e26
    HEAD_REF main
)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -DCOMMON_BUILD_TESTS=OFF
        -DCOMMON_BUILD_INTEGRATION_TESTS=OFF
        -DCOMMON_BUILD_EXAMPLES=OFF
        -DCOMMON_BUILD_BENCHMARKS=OFF
        -DCOMMON_BUILD_DOCS=OFF
)

vcpkg_cmake_install()

vcpkg_cmake_config_fixup(
    PACKAGE_NAME common_system
    CONFIG_PATH lib/cmake/common_system
)

# Header-only library - remove all debug content and empty lib directories
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug")
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/lib")

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
