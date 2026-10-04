# kcenon-container-system portfile
# Advanced C++20 Container System with Thread-Safe Operations and Messaging Integration

vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO kcenon/container_system
    REF "v${VERSION}"
    SHA512 3cf36f0beb9a8f7d01f2504126ecb8472fa27ed5d0091192e078846309c1e66abd87e8b02bccd41af1412b81053d053ac57f3810b202f70a9b91d1c3ca8a5b4e
    HEAD_REF main
)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -DBUILD_WITH_COMMON_SYSTEM=ON
        -DCOMMON_SYSTEM_ROOT=${CURRENT_INSTALLED_DIR}
        -DBUILD_TESTS=OFF
        -DCONTAINER_BUILD_INTEGRATION_TESTS=OFF
        -DCONTAINER_BUILD_BENCHMARKS=OFF
        -DBUILD_DOCUMENTATION=OFF
        -DBUILD_CONTAINER_SAMPLES=OFF
        -DBUILD_CONTAINER_EXAMPLES=OFF
        -DFETCHCONTENT_FULLY_DISCONNECTED=ON
)

vcpkg_cmake_install()

vcpkg_cmake_config_fixup(
    PACKAGE_NAME ContainerSystem
    CONFIG_PATH lib/cmake/ContainerSystem
)

# Remove example/sample executables and empty bin directories
file(REMOVE_RECURSE
    "${CURRENT_PACKAGES_DIR}/bin/examples"
    "${CURRENT_PACKAGES_DIR}/bin/samples"
    "${CURRENT_PACKAGES_DIR}/debug/bin/examples"
    "${CURRENT_PACKAGES_DIR}/debug/bin/samples"
)
# Clean up empty bin/debug/bin dirs left after removal (no DLLs on non-Windows)
foreach(_bindir IN ITEMS "${CURRENT_PACKAGES_DIR}/bin" "${CURRENT_PACKAGES_DIR}/debug/bin")
    if(IS_DIRECTORY "${_bindir}")
        file(GLOB _remaining "${_bindir}/*")
        if(NOT _remaining)
            file(REMOVE_RECURSE "${_bindir}")
        endif()
    endif()
endforeach()

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/share")

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")

configure_file("${CMAKE_CURRENT_LIST_DIR}/usage" "${CURRENT_PACKAGES_DIR}/share/${PORT}/usage" COPYONLY)
