#!/bin/bash

# Build script for common_system
set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
PROJECT_NAME="common_system"
BUILD_DIR="build"
INSTALL_PREFIX="${HOME}/.local"
VCPKG_ROOT="${VCPKG_ROOT:-${HOME}/vcpkg}"
BUILD_TYPE="Release"
BUILD_TESTS="ON"
BUILD_EXAMPLES="ON"
BUILD_BENCHMARKS="OFF"
VERBOSE="OFF"
CLEAN_BUILD="OFF"
HEADER_ONLY="ON"

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --debug)
            BUILD_TYPE="Debug"
            shift
            ;;
        --release)
            BUILD_TYPE="Release"
            shift
            ;;
        --relwithdebinfo)
            BUILD_TYPE="RelWithDebInfo"
            shift
            ;;
        --no-tests)
            BUILD_TESTS="OFF"
            shift
            ;;
        --no-examples)
            BUILD_EXAMPLES="OFF"
            shift
            ;;
        --benchmarks)
            BUILD_BENCHMARKS="ON"
            shift
            ;;
        --static)
            HEADER_ONLY="OFF"
            shift
            ;;
        --clean)
            CLEAN_BUILD="ON"
            shift
            ;;
        --verbose)
            VERBOSE="ON"
            shift
            ;;
        --install-prefix)
            INSTALL_PREFIX="$2"
            shift 2
            ;;
        --vcpkg-root)
            VCPKG_ROOT="$2"
            shift 2
            ;;
        --help)
            echo "Usage: $0 [options]"
            echo "Options:"
            echo "  --debug              Build in Debug mode"
            echo "  --release            Build in Release mode (default)"
            echo "  --relwithdebinfo     Build in RelWithDebInfo mode"
            echo "  --no-tests           Don't build tests"
            echo "  --no-examples        Don't build examples"
            echo "  --benchmarks         Build benchmarks"
            echo "  --static             Build as static library (default: header-only)"
            echo "  --clean              Clean build directory before building"
            echo "  --verbose            Enable verbose output"
            echo "  --install-prefix DIR Set installation prefix (default: ~/.local)"
            echo "  --vcpkg-root DIR     Set vcpkg root directory"
            echo "  --help               Show this help message"
            exit 0
            ;;
        *)
            echo -e "${RED}Unknown option: $1${NC}"
            echo "Use --help for usage information"
            exit 1
            ;;
    esac
done

# Print configuration
echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}Building ${PROJECT_NAME}${NC}"
echo -e "${BLUE}========================================${NC}"
echo -e "${GREEN}Build Configuration:${NC}"
echo "  Build Type:        ${BUILD_TYPE}"
echo "  Build Directory:   ${BUILD_DIR}"
echo "  Install Prefix:    ${INSTALL_PREFIX}"
echo "  Header Only:       ${HEADER_ONLY}"
echo "  Build Tests:       ${BUILD_TESTS}"
echo "  Build Examples:    ${BUILD_EXAMPLES}"
echo "  Build Benchmarks:  ${BUILD_BENCHMARKS}"
echo "  Verbose:           ${VERBOSE}"
echo "  vcpkg Root:        ${VCPKG_ROOT}"
echo -e "${BLUE}========================================${NC}"

# Check for required tools
echo -e "${YELLOW}Checking for required tools...${NC}"

if ! command -v cmake &> /dev/null; then
    echo -e "${RED}Error: cmake is not installed${NC}"
    exit 1
fi

if ! command -v ninja &> /dev/null; then
    echo -e "${YELLOW}Warning: ninja is not installed, using make instead${NC}"
    GENERATOR="Unix Makefiles"
    BUILD_COMMAND="make"
else
    GENERATOR="Ninja"
    BUILD_COMMAND="ninja"
fi

# Check for vcpkg
if [ -d "${VCPKG_ROOT}" ]; then
    echo -e "${GREEN}Found vcpkg at ${VCPKG_ROOT}${NC}"
    VCPKG_TOOLCHAIN="${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
    if [ ! -f "${VCPKG_TOOLCHAIN}" ]; then
        echo -e "${RED}Error: vcpkg toolchain file not found${NC}"
        exit 1
    fi
else
    echo -e "${YELLOW}Warning: vcpkg not found, dependencies must be installed manually${NC}"
    VCPKG_TOOLCHAIN=""
fi

# Clean build directory if requested
if [ "${CLEAN_BUILD}" = "ON" ]; then
    echo -e "${YELLOW}Cleaning build directory...${NC}"
    rm -rf "${BUILD_DIR}"
fi

# Create build directory
mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

# Configure with CMake
echo -e "${YELLOW}Configuring with CMake...${NC}"

CMAKE_ARGS=(
    "-G${GENERATOR}"
    "-DCMAKE_BUILD_TYPE=${BUILD_TYPE}"
    "-DCMAKE_INSTALL_PREFIX=${INSTALL_PREFIX}"
    "-DCOMMON_BUILD_TESTS=${BUILD_TESTS}"
    "-DCOMMON_BUILD_EXAMPLES=${BUILD_EXAMPLES}"
    "-DCOMMON_BUILD_BENCHMARKS=${BUILD_BENCHMARKS}"
    "-DCOMMON_HEADER_ONLY=${HEADER_ONLY}"
    "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON"
)

if [ -n "${VCPKG_TOOLCHAIN}" ]; then
    CMAKE_ARGS+=("-DCMAKE_TOOLCHAIN_FILE=${VCPKG_TOOLCHAIN}")
fi

if [ "${VERBOSE}" = "ON" ]; then
    CMAKE_ARGS+=("-DCMAKE_VERBOSE_MAKEFILE=ON")
fi

cmake "${CMAKE_ARGS[@]}" ..

if [ $? -ne 0 ]; then
    echo -e "${RED}CMake configuration failed${NC}"
    exit 1
fi

# Build
echo -e "${YELLOW}Building...${NC}"

if [ "${VERBOSE}" = "ON" ]; then
    ${BUILD_COMMAND} -v
else
    ${BUILD_COMMAND}
fi

if [ $? -ne 0 ]; then
    echo -e "${RED}Build failed${NC}"
    exit 1
fi

# Run tests if built
if [ "${BUILD_TESTS}" = "ON" ]; then
    echo -e "${YELLOW}Running tests...${NC}"
    if [ -f "tests/common_system_tests" ]; then
        ctest --output-on-failure
    else
        echo -e "${YELLOW}No tests found to run${NC}"
    fi
fi

echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}Build completed successfully!${NC}"
echo -e "${GREEN}========================================${NC}"

# Print usage instructions
echo ""
echo "To install the library, run:"
echo "  cd ${BUILD_DIR} && ${BUILD_COMMAND} install"
echo ""
echo "To use in your project:"
echo "  find_package(common_system REQUIRED)"
echo "  target_link_libraries(your_target PRIVATE kcenon::common)"
echo ""

# Generate compilation database link for IDEs
if [ -f "compile_commands.json" ]; then
    ln -sf "${BUILD_DIR}/compile_commands.json" ../compile_commands.json
    echo "Compilation database linked for IDE support"
fi