cmake_minimum_required(VERSION 3.16)

# Find GTest (should already be checked by parent)
find_package(GTest REQUIRED)
find_package(Threads REQUIRED)

# Create test executable
add_executable(common_system_tests
    result_test.cpp
    executor_test.cpp
    main.cpp
)

# Link with GTest and common_system
target_link_libraries(common_system_tests PRIVATE
    kcenon::common
    GTest::gtest
    GTest::gtest_main
    Threads::Threads
)

# Set C++ standard
target_compile_features(common_system_tests PRIVATE cxx_std_17)

# Add test
add_test(NAME common_system_tests COMMAND common_system_tests)

# Set test properties
set_tests_properties(common_system_tests PROPERTIES
    TIMEOUT 60
    LABELS "unit"
)

# Enable testing
enable_testing()

# Coverage flags for GCC/Clang
if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    option(ENABLE_COVERAGE "Enable coverage reporting" OFF)
    if(ENABLE_COVERAGE)
        target_compile_options(common_system_tests PRIVATE --coverage -O0 -g)
        target_link_options(common_system_tests PRIVATE --coverage)
    endif()
endif()