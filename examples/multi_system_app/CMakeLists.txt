cmake_minimum_required(VERSION 3.20)
project(multi_system_app VERSION 1.0.0 LANGUAGES CXX)

# C++20 required
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find kcenon systems
find_package(common_system CONFIG REQUIRED)
find_package(thread_system CONFIG REQUIRED)
find_package(logger_system CONFIG REQUIRED)
find_package(database_system CONFIG REQUIRED)

# Executable
add_executable(multi_system_app main.cpp)

# Link libraries
target_link_libraries(multi_system_app PRIVATE
    kcenon::common_system
    kcenon::thread_system
    kcenon::logger_system
    kcenon::database_system
)

# Copy config file to build directory
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/app.yaml
    ${CMAKE_CURRENT_BINARY_DIR}/app.yaml
    COPYONLY
)

# Install
install(TARGETS multi_system_app
    RUNTIME DESTINATION bin
)
