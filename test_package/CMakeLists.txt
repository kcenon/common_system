cmake_minimum_required(VERSION 3.16)
project(test_common_system CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(common_system REQUIRED)

add_executable(test_common_system example.cpp)
target_link_libraries(test_common_system PRIVATE kcenon::common)
