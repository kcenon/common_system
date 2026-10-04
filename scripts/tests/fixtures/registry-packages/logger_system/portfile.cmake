# kcenon-logger-system portfile
# High-performance C++20 async logging library with 4.34M msg/sec throughput

vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO kcenon/logger_system
    REF "v${VERSION}"
    SHA512 4ffc4b2457066ba8345389a3222b6f1d739485e46f35f26c57f7f02c67e0377e2a6d0c8c591bd79dcd5f76c9b28373202055b73f123e0fa51043b9e98a96e531
    HEAD_REF main
)

# Feature-based options
vcpkg_check_features(OUT_FEATURE_OPTIONS FEATURE_OPTIONS
    FEATURES
        encryption    LOGGER_USE_ENCRYPTION
        otlp          LOGGER_ENABLE_OTLP
        thread-system LOGGER_USE_THREAD_SYSTEM
)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -DBUILD_TESTS=OFF
        -DBUILD_BENCHMARKS=OFF
        -DBUILD_SAMPLES=OFF
        -DLOGGER_BUILD_INTEGRATION_TESTS=OFF
        -DLOGGER_ENABLE_COVERAGE=OFF
        -DBUILD_WITH_COMMON_SYSTEM=ON
        -DFETCHCONTENT_FULLY_DISCONNECTED=ON
        ${FEATURE_OPTIONS}
)

vcpkg_cmake_install()

vcpkg_cmake_config_fixup(
    PACKAGE_NAME LoggerSystem
    CONFIG_PATH lib/cmake/LoggerSystem
)

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/share")

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")

configure_file("${CMAKE_CURRENT_LIST_DIR}/usage" "${CURRENT_PACKAGES_DIR}/share/${PORT}/usage" COPYONLY)
