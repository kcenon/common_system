cmake_minimum_required(VERSION 3.20)
project(test_common_system LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(common_system CONFIG REQUIRED)

add_executable(test_common_system main.cpp)
target_link_libraries(test_common_system PRIVATE kcenon::common_system)
