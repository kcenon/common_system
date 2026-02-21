cmake_minimum_required(VERSION 3.16)

project(CommonSystemBenchmarks)

# Find Google Benchmark
find_package(benchmark REQUIRED)

# Common benchmark settings
set(BENCHMARK_SOURCES
    result_benchmark.cpp
    event_bus_benchmark.cpp
    object_pool_benchmark.cpp
    circular_buffer_benchmark.cpp
    circuit_breaker_benchmark.cpp
    service_container_benchmark.cpp
    global_logger_registry_benchmark.cpp
)

# Create benchmark executable
add_executable(common_benchmarks ${BENCHMARK_SOURCES})

target_link_libraries(common_benchmarks PRIVATE
    kcenon::common
    benchmark::benchmark
    benchmark::benchmark_main
)

target_compile_features(common_benchmarks PRIVATE cxx_std_20)

# Platform-specific settings
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    # Disable LTO to avoid version mismatch with system libbenchmark
    target_compile_options(common_benchmarks PRIVATE -O3 -DNDEBUG -fno-lto)
    target_link_options(common_benchmarks PRIVATE -fno-lto)
elseif(MSVC)
    target_compile_options(common_benchmarks PRIVATE /O2 /DNDEBUG)
endif()

# Register benchmark as a test (optional, for CI integration)
add_test(NAME common_benchmarks COMMAND common_benchmarks --benchmark_format=console)
