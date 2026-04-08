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

# Example 3: Improved unwrap() with source location
add_executable(unwrap_demo unwrap_demo.cpp)
target_link_libraries(unwrap_demo PRIVATE kcenon::common)
target_compile_features(unwrap_demo PRIVATE cxx_std_20)

# Example 4: DI Service Container
add_executable(service_container_example service_container_example.cpp)
target_link_libraries(service_container_example PRIVATE kcenon::common)
target_compile_features(service_container_example PRIVATE cxx_std_20)

# Example 5: Event Bus pub/sub
add_executable(event_bus_example event_bus_example.cpp)
target_link_libraries(event_bus_example PRIVATE kcenon::common)
target_compile_features(event_bus_example PRIVATE cxx_std_20)

# Example 6: Circuit Breaker resilience pattern
add_executable(circuit_breaker_example circuit_breaker_example.cpp)
target_link_libraries(circuit_breaker_example PRIVATE kcenon::common Threads::Threads)
target_compile_features(circuit_breaker_example PRIVATE cxx_std_20)

# Example 7: Config Watcher with hot-reload
add_executable(config_watcher_example config_watcher_example.cpp)
target_link_libraries(config_watcher_example PRIVATE kcenon::common Threads::Threads)
target_compile_features(config_watcher_example PRIVATE cxx_std_20)

# Example 8: CLI Argument Parser
add_executable(cli_parser_example cli_parser_example.cpp)
target_link_libraries(cli_parser_example PRIVATE kcenon::common)
target_compile_features(cli_parser_example PRIVATE cxx_std_20)

# Example 9: Feature Flags detection
add_executable(feature_flags_example feature_flags_example.cpp)
target_link_libraries(feature_flags_example PRIVATE kcenon::common)
target_compile_features(feature_flags_example PRIVATE cxx_std_20)

# Example 10: Circular Buffer and Object Pool
add_executable(utility_containers_example utility_containers_example.cpp)
target_link_libraries(utility_containers_example PRIVATE kcenon::common)
target_compile_features(utility_containers_example PRIVATE cxx_std_20)

# Example 11: C++20 Concepts showcase
add_executable(concepts_showcase_example concepts_showcase_example.cpp)
target_link_libraries(concepts_showcase_example PRIVATE kcenon::common)
target_compile_features(concepts_showcase_example PRIVATE cxx_std_20)

# Example 12: Adapter pattern
add_executable(adapter_pattern_example adapter_pattern_example.cpp)
target_link_libraries(adapter_pattern_example PRIVATE kcenon::common)
target_compile_features(adapter_pattern_example PRIVATE cxx_std_20)

# Example 13: Health Check monitoring
add_executable(health_check_example health_check_example.cpp)
target_link_libraries(health_check_example PRIVATE kcenon::common)
target_compile_features(health_check_example PRIVATE cxx_std_20)

# Integration: thread_system (if available)
if(TARGET thread_system)
    add_executable(thread_integration_example thread_integration_example.cpp)
    target_link_libraries(thread_integration_example PRIVATE
        kcenon::common
        thread_system
    )
    target_compile_features(thread_integration_example PRIVATE cxx_std_20)
endif()

# Integration: network_system (if available)
if(TARGET network_system)
    add_executable(network_integration_example network_integration_example.cpp)
    target_link_libraries(network_integration_example PRIVATE
        kcenon::common
        network_system
    )
    target_compile_features(network_integration_example PRIVATE cxx_std_17)
endif()