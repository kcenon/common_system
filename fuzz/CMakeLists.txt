# Fuzz targets for common_system foundation parsing/decoding surfaces.
#
# Gated behind BUILD_FUZZERS so the default build, tests, examples, and
# benchmarks are completely unaffected. libFuzzer + AddressSanitizer require a
# Clang toolchain.
#
#   cmake -B build-fuzz -G Ninja \
#       -DCMAKE_CXX_COMPILER=clang++ \
#       -DBUILD_FUZZERS=ON \
#       -DCOMMON_SYSTEM_BUILD_TESTS=OFF \
#       -DCOMMON_SYSTEM_BUILD_EXAMPLES=OFF
#   cmake --build build-fuzz
#   ./build-fuzz/fuzz/result_error_fuzzer fuzz/corpus/result_error

if(NOT BUILD_FUZZERS)
    return()
endif()

# libFuzzer is a Clang feature; refuse to misconfigure under other compilers.
if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    message(FATAL_ERROR
        "BUILD_FUZZERS=ON requires a Clang toolchain (libFuzzer). "
        "Configure with -DCMAKE_CXX_COMPILER=clang++.")
endif()

set(FUZZ_FLAGS -fsanitize=fuzzer,address -g -O1)

set(FUZZ_TARGETS
    result_error_fuzzer
)

foreach(fuzz_src ${FUZZ_TARGETS})
    add_executable(${fuzz_src} ${fuzz_src}.cpp)
    target_link_libraries(${fuzz_src} PRIVATE common_system)
    target_include_directories(${fuzz_src} PRIVATE ${CMAKE_SOURCE_DIR}/include)
    target_compile_options(${fuzz_src} PRIVATE ${FUZZ_FLAGS})
    target_link_options(${fuzz_src} PRIVATE ${FUZZ_FLAGS})
    set_target_properties(${fuzz_src} PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF)
endforeach()
