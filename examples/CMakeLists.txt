cmake_minimum_required(VERSION 3.16)

find_package(Threads REQUIRED)

# Example 1: Using Result pattern
add_executable(result_example result_example.cpp)
target_link_libraries(result_example PRIVATE kcenon::common)
target_compile_features(result_example PRIVATE cxx_std_17)

# Example 2: Using IExecutor interface
add_executable(executor_example executor_example.cpp)
target_link_libraries(executor_example PRIVATE kcenon::common Threads::Threads)
target_compile_features(executor_example PRIVATE cxx_std_17)

# Example 3: Integration with thread_system (if available)
if(TARGET thread_system)
    add_executable(thread_integration_example thread_integration_example.cpp)
    target_link_libraries(thread_integration_example PRIVATE
        kcenon::common
        thread_system
    )
    target_compile_features(thread_integration_example PRIVATE cxx_std_20)
endif()

# Example 4: Integration with network_system (if available)
if(TARGET network_system)
    add_executable(network_integration_example network_integration_example.cpp)
    target_link_libraries(network_integration_example PRIVATE
        kcenon::common
        network_system
    )
    target_compile_features(network_integration_example PRIVATE cxx_std_17)
endif()