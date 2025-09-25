#!/bin/bash

# Test script for common_system
set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
MAGENTA='\033[0;35m'
NC='\033[0m' # No Color

# Configuration
BUILD_DIR="build"
BUILD_TYPE="Debug"
VERBOSE="OFF"
COVERAGE="OFF"
BENCHMARK="OFF"
FILTER=""
OUTPUT_FORMAT="console"

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --release)
            BUILD_TYPE="Release"
            shift
            ;;
        --verbose|-v)
            VERBOSE="ON"
            shift
            ;;
        --coverage)
            COVERAGE="ON"
            shift
            ;;
        --benchmark)
            BENCHMARK="ON"
            shift
            ;;
        --filter)
            FILTER="$2"
            shift 2
            ;;
        --junit)
            OUTPUT_FORMAT="junit"
            shift
            ;;
        --build-dir)
            BUILD_DIR="$2"
            shift 2
            ;;
        --help)
            echo "Usage: $0 [options]"
            echo "Options:"
            echo "  --release        Run tests in Release mode"
            echo "  --verbose, -v    Enable verbose output"
            echo "  --coverage       Generate code coverage report"
            echo "  --benchmark      Run benchmarks"
            echo "  --filter PATTERN Filter tests by pattern"
            echo "  --junit          Output in JUnit XML format"
            echo "  --build-dir DIR  Specify build directory (default: build)"
            echo "  --help           Show this help message"
            exit 0
            ;;
        *)
            echo -e "${RED}Unknown option: $1${NC}"
            echo "Use --help for usage information"
            exit 1
            ;;
    esac
done

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}Running common_system tests${NC}"
echo -e "${BLUE}========================================${NC}"
echo "Configuration:"
echo "  Build Type:     ${BUILD_TYPE}"
echo "  Build Dir:      ${BUILD_DIR}"
echo "  Verbose:        ${VERBOSE}"
echo "  Coverage:       ${COVERAGE}"
echo "  Benchmarks:     ${BENCHMARK}"
if [ -n "${FILTER}" ]; then
    echo "  Filter:         ${FILTER}"
fi
echo -e "${BLUE}========================================${NC}"

# Check if build directory exists
if [ ! -d "${BUILD_DIR}" ]; then
    echo -e "${RED}Error: Build directory '${BUILD_DIR}' not found${NC}"
    echo "Please run ./build.sh first"
    exit 1
fi

cd "${BUILD_DIR}"

# Check if tests are built
if [ ! -f "tests/common_system_tests" ] && [ ! -f "tests/Debug/common_system_tests" ] && [ ! -f "tests/Release/common_system_tests" ]; then
    echo -e "${YELLOW}Tests not found. Building tests...${NC}"

    if command -v ninja &> /dev/null && [ -f "build.ninja" ]; then
        ninja common_system_tests
    else
        make common_system_tests
    fi
fi

# Run unit tests
echo -e "${YELLOW}Running unit tests...${NC}"

CTEST_ARGS="-C ${BUILD_TYPE}"

if [ "${VERBOSE}" = "ON" ]; then
    CTEST_ARGS="${CTEST_ARGS} --verbose"
else
    CTEST_ARGS="${CTEST_ARGS} --output-on-failure"
fi

if [ -n "${FILTER}" ]; then
    CTEST_ARGS="${CTEST_ARGS} -R ${FILTER}"
fi

if [ "${OUTPUT_FORMAT}" = "junit" ]; then
    CTEST_ARGS="${CTEST_ARGS} --output-junit test_results.xml"
fi

# Run tests
ctest ${CTEST_ARGS}
TEST_RESULT=$?

# Run benchmarks if requested
if [ "${BENCHMARK}" = "ON" ]; then
    echo -e "${YELLOW}Running benchmarks...${NC}"

    if [ -f "benchmarks/common_system_benchmarks" ]; then
        ./benchmarks/common_system_benchmarks --benchmark_out=benchmark_results.json --benchmark_out_format=json
    elif [ -f "benchmarks/${BUILD_TYPE}/common_system_benchmarks" ]; then
        ./benchmarks/${BUILD_TYPE}/common_system_benchmarks --benchmark_out=benchmark_results.json --benchmark_out_format=json
    else
        echo -e "${YELLOW}Benchmarks not found${NC}"
    fi
fi

# Generate coverage report if requested
if [ "${COVERAGE}" = "ON" ]; then
    echo -e "${YELLOW}Generating coverage report...${NC}"

    if command -v gcovr &> /dev/null; then
        # Create coverage directory
        mkdir -p coverage

        # Generate coverage report
        gcovr --root .. \
              --filter '../include/.*' \
              --exclude '../tests/.*' \
              --exclude '../examples/.*' \
              --html-details coverage/index.html \
              --xml coverage/coverage.xml \
              --print-summary

        echo -e "${GREEN}Coverage report generated: ${BUILD_DIR}/coverage/index.html${NC}"
    elif command -v llvm-cov &> /dev/null; then
        # LLVM coverage
        llvm-cov gcov -p -o . $(find . -name '*.gcda')
        echo -e "${GREEN}Coverage files generated${NC}"
    else
        echo -e "${RED}Error: Coverage tool not found (gcovr or llvm-cov required)${NC}"
    fi
fi

cd ..

# Print summary
echo -e "${BLUE}========================================${NC}"
if [ ${TEST_RESULT} -eq 0 ]; then
    echo -e "${GREEN}All tests passed!${NC}"
else
    echo -e "${RED}Some tests failed!${NC}"
fi
echo -e "${BLUE}========================================${NC}"

# Generate test report if JUnit format was used
if [ "${OUTPUT_FORMAT}" = "junit" ] && [ -f "${BUILD_DIR}/test_results.xml" ]; then
    echo "Test results saved to: ${BUILD_DIR}/test_results.xml"
fi

exit ${TEST_RESULT}