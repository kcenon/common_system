# kcenon-network-system portfile
# Modern C++20 async network library with TCP/UDP, HTTP/1.1, WebSocket, and TLS 1.3

vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO kcenon/network_system
    REF "v${VERSION}"
    SHA512 8a723086ee2481ef4e8e46ec61b1b8e063e48f53549147893f0d7e88d2e16978a604709bec10bf5312470c0c9ed6d3c2366b75df8aa3715691aa03dc6871e758
    HEAD_REF main
)

vcpkg_check_features(OUT_FEATURE_OPTIONS FEATURE_OPTIONS
    FEATURES
        logging BUILD_WITH_LOGGER_SYSTEM
)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -DBUILD_TESTS=OFF
        -DBUILD_SAMPLES=OFF
        -DBUILD_WEBSOCKET_SUPPORT=ON
        -DBUILD_MESSAGING_BRIDGE=OFF
        -DNETWORK_BUILD_BENCHMARKS=OFF
        -DNETWORK_BUILD_INTEGRATION_TESTS=OFF
        -DNETWORK_BUILD_MODULES=OFF
        -DBUILD_WITH_COMMON_SYSTEM=ON
        -DFETCHCONTENT_FULLY_DISCONNECTED=ON
        ${FEATURE_OPTIONS}
)

vcpkg_cmake_install()

# Install internal headers required by public headers.
# Several installed headers (messaging_session.h, secure_session.h, network_system.h,
# quic_session.h, network_system_bridge.h, network_config.h) reference headers from
# src/internal/ which are not installed by CMake. Copy them to the install tree so
# downstream consumers can compile against the package.
file(COPY "${SOURCE_PATH}/src/internal/"
    DESTINATION "${CURRENT_PACKAGES_DIR}/include/internal/"
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp" PATTERN "*.inl"
)

# Install network-core modular library headers required by internal headers.
# src/internal/tcp/tcp_socket.h includes kcenon/network-core/interfaces/socket_observer.h
file(COPY "${SOURCE_PATH}/network-core/include/"
    DESTINATION "${CURRENT_PACKAGES_DIR}/include/"
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
)

vcpkg_cmake_config_fixup(
    PACKAGE_NAME NetworkSystem
    CONFIG_PATH lib/cmake/NetworkSystem
)

# Remove empty directories that cause vcpkg post-build validation warnings
file(REMOVE_RECURSE
    "${CURRENT_PACKAGES_DIR}/include/kcenon/network/core"
    "${CURRENT_PACKAGES_DIR}/include/kcenon/network/experimental"
    "${CURRENT_PACKAGES_DIR}/include/kcenon/network/http"
)

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/share")

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")

configure_file("${CMAKE_CURRENT_LIST_DIR}/usage" "${CURRENT_PACKAGES_DIR}/share/${PORT}/usage" COPYONLY)
