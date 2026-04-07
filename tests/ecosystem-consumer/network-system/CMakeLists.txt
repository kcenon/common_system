cmake_minimum_required(VERSION 3.20)
project(test_network_system LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(network_system CONFIG REQUIRED)

add_executable(test_network_system main.cpp)
target_link_libraries(test_network_system PRIVATE NetworkSystem::NetworkSystem)
