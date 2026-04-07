cmake_minimum_required(VERSION 3.20)
project(test_monitoring_system LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(monitoring_system CONFIG REQUIRED)

add_executable(test_monitoring_system main.cpp)
target_link_libraries(test_monitoring_system PRIVATE monitoring_system::monitoring_system)
