cmake_minimum_required(VERSION 3.20)
project(test_container_system LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(container_system CONFIG REQUIRED)

add_executable(test_container_system main.cpp)
target_link_libraries(test_container_system PRIVATE ContainerSystem::container)
