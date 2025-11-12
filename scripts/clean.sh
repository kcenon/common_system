#!/bin/bash

# Clean script for common_system
set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}Cleaning common_system build artifacts${NC}"
echo -e "${BLUE}========================================${NC}"

# Directories to clean
BUILD_DIRS=("build" "cmake-build-debug" "cmake-build-release" "cmake-build-relwithdebinfo")
CACHE_DIRS=(".cache" "vcpkg_installed")
TEMP_FILES=("compile_commands.json" ".DS_Store" "*.log")

# Clean build directories
for dir in "${BUILD_DIRS[@]}"; do
    if [ -d "$dir" ]; then
        echo -e "${YELLOW}Removing $dir...${NC}"
        rm -rf "$dir"
    fi
done

# Clean cache directories
for dir in "${CACHE_DIRS[@]}"; do
    if [ -d "$dir" ]; then
        echo -e "${YELLOW}Removing $dir...${NC}"
        rm -rf "$dir"
    fi
done

# Clean temporary files
for pattern in "${TEMP_FILES[@]}"; do
    files=$(find . -maxdepth 1 -name "$pattern" 2>/dev/null)
    if [ -n "$files" ]; then
        echo -e "${YELLOW}Removing $pattern files...${NC}"
        rm -f $files
    fi
done

# Clean test outputs
if [ -d "tests/output" ]; then
    echo -e "${YELLOW}Removing test output directory...${NC}"
    rm -rf "tests/output"
fi

# Clean benchmark results
if [ -d "benchmark_results" ]; then
    echo -e "${YELLOW}Removing benchmark results...${NC}"
    rm -rf "benchmark_results"
fi

# Clean documentation if generated
if [ -d "docs/html" ]; then
    echo -e "${YELLOW}Removing generated documentation...${NC}"
    rm -rf "docs/html"
    rm -rf "docs/latex"
fi

echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}Clean completed successfully!${NC}"
echo -e "${GREEN}========================================${NC}"