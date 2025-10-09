#!/bin/bash

# Test script for single system benchmark
# This validates the benchmark infrastructure before running the full collection

set -e

SYSTEM_NAME="thread_system"
SYSTEM_DIR="/Users/dongcheolshin/Sources/${SYSTEM_NAME}"
BUILD_DIR="${SYSTEM_DIR}/build_test_bench"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)

echo "=================================================="
echo "Testing Benchmark Infrastructure: ${SYSTEM_NAME}"
echo "=================================================="

# Check if system directory exists
if [ ! -d "$SYSTEM_DIR" ]; then
    echo "ERROR: System directory not found: $SYSTEM_DIR"
    exit 1
fi

# Create build directory
mkdir -p "$BUILD_DIR"
cd "$SYSTEM_DIR"

# Configure
echo "[1/4] Configuring CMake..."
cmake -B "$BUILD_DIR" \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_BENCHMARKS=ON \
    -DBUILD_TESTS=OFF \
    -DBUILD_EXAMPLES=OFF

# Build
echo "[2/4] Building benchmarks..."
cmake --build "$BUILD_DIR" --target benchmarks -j$(sysctl -n hw.ncpu)

# Find benchmarks
echo "[3/4] Finding benchmark executables..."
BENCH_DIR="${BUILD_DIR}/bin"

if [ ! -d "$BENCH_DIR" ]; then
    echo "ERROR: Benchmark directory not found: $BENCH_DIR"
    exit 1
fi

BENCHMARKS=$(find "$BENCH_DIR" -type f -executable -name '*benchmark*' 2>/dev/null)

if [ -z "$BENCHMARKS" ]; then
    echo "ERROR: No benchmark executables found"
    exit 1
fi

echo "Found benchmarks:"
echo "$BENCHMARKS"

# Run one benchmark as test
echo "[4/4] Running test benchmark (1 iteration)..."
FIRST_BENCH=$(echo "$BENCHMARKS" | head -n1)
echo "Testing: $FIRST_BENCH"

"$FIRST_BENCH" \
    --benchmark_format=json \
    --benchmark_out="test_output_${TIMESTAMP}.json" \
    --benchmark_repetitions=1

echo ""
echo "=================================================="
echo "Test Successful!"
echo "=================================================="
echo "Next steps:"
echo "1. Run full collection: ./tools/collect_baseline_metrics.sh"
echo "2. Analyze results: python3 ./tools/analyze_benchmark_results.py"

# Cleanup
rm -f "test_output_${TIMESTAMP}.json"
rm -rf "$BUILD_DIR"

echo "Cleanup complete."
