cmake_minimum_required(VERSION 3.20)
project(test_thread_system LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(thread_system CONFIG REQUIRED)

add_executable(test_thread_system main.cpp)
target_link_libraries(test_thread_system PRIVATE thread_system::thread_system)
