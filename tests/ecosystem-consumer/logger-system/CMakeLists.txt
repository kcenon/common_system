cmake_minimum_required(VERSION 3.20)
project(test_logger_system LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# LoggerSystemTargets.cmake references kcenon::common_system in its link
# interface but does not pull it in via find_dependency, so consumers must
# bring common_system into scope before find_package(LoggerSystem).
find_package(common_system CONFIG REQUIRED)
find_package(LoggerSystem CONFIG REQUIRED)

add_executable(test_logger_system main.cpp)
target_link_libraries(test_logger_system PRIVATE LoggerSystem::logger)
