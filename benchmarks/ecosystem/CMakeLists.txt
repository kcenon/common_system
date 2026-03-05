cmake_minimum_required(VERSION 3.16)

project(EcosystemBenchmarks)

find_package(benchmark REQUIRED)
find_package(Threads REQUIRED)

set(ECOSYSTEM_BENCHMARK_SOURCES
    interface_overhead_benchmark.cpp
    data_pipeline_benchmark.cpp
    request_response_benchmark.cpp
    scaling_benchmark.cpp
)

add_executable(ecosystem_benchmarks ${ECOSYSTEM_BENCHMARK_SOURCES})

target_link_libraries(ecosystem_benchmarks PRIVATE
    kcenon::common
    benchmark::benchmark
    benchmark::benchmark_main
    Threads::Threads
)

target_compile_features(ecosystem_benchmarks PRIVATE cxx_std_20)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(ecosystem_benchmarks PRIVATE -O3 -DNDEBUG -fno-lto)
    target_link_options(ecosystem_benchmarks PRIVATE -fno-lto)
elseif(MSVC)
    target_compile_options(ecosystem_benchmarks PRIVATE /O2 /DNDEBUG)
endif()

# Register as CTest target (manual trigger recommended due to duration)
add_test(
    NAME ecosystem_benchmarks
    COMMAND ecosystem_benchmarks
        --benchmark_format=console
        --benchmark_filter="BM_"
)
