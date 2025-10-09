#!/bin/bash

# Baseline Metrics Collection Script
# This script runs benchmarks multiple times and collects statistics
# for baseline performance documentation

set -e

# Configuration
RUNS=10
OUTPUT_DIR="baseline_results"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Function to print colored messages
log_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Function to check if system has benchmarks
has_benchmarks() {
    local system_dir=$1
    [ -d "$system_dir/benchmarks" ] && [ -n "$(find "$system_dir/benchmarks" -name '*_bench' -o -name '*_benchmark' 2>/dev/null)" ]
}

# Function to run benchmarks for a system
run_system_benchmarks() {
    local system_name=$1
    local system_dir=$2

    log_info "Processing $system_name..."

    # Check if benchmarks exist
    if [ ! -d "$system_dir/benchmarks" ]; then
        log_warn "No benchmarks directory found for $system_name"
        return 1
    fi

    # Create build directory
    local build_dir="$system_dir/build_benchmark_${TIMESTAMP}"
    mkdir -p "$build_dir"

    # Configure with CMake
    log_info "Configuring $system_name with benchmarks enabled..."
    cd "$system_dir"
    cmake -B "$build_dir" \
        -DCMAKE_BUILD_TYPE=Release \
        -DBUILD_BENCHMARKS=ON \
        -DBUILD_TESTS=OFF \
        -DBUILD_EXAMPLES=OFF \
        2>&1 | tee "$OUTPUT_DIR/${system_name}_cmake.log"

    # Build benchmarks
    log_info "Building benchmarks for $system_name..."
    cmake --build "$build_dir" --target benchmarks -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4) \
        2>&1 | tee "$OUTPUT_DIR/${system_name}_build.log"

    # Find benchmark executables
    local benchmark_dir="$build_dir/benchmarks"
    if [ ! -d "$benchmark_dir" ]; then
        log_warn "Benchmark directory not found at $benchmark_dir"
        return 1
    fi

    # Create results directory for this system
    local results_dir="$OUTPUT_DIR/${system_name}"
    mkdir -p "$results_dir"

    # Find all benchmark executables
    local benchmarks=($(find "$benchmark_dir" -type f -executable -name '*bench*' 2>/dev/null))

    if [ ${#benchmarks[@]} -eq 0 ]; then
        log_warn "No benchmark executables found for $system_name"
        return 1
    fi

    log_info "Found ${#benchmarks[@]} benchmark executable(s) for $system_name"

    # Run each benchmark multiple times
    for benchmark in "${benchmarks[@]}"; do
        local bench_name=$(basename "$benchmark")
        log_info "Running $bench_name ($RUNS times)..."

        for run in $(seq 1 $RUNS); do
            log_info "  Run $run/$RUNS..."

            # Run benchmark with JSON output
            "$benchmark" \
                --benchmark_format=json \
                --benchmark_out="$results_dir/${bench_name}_run${run}.json" \
                --benchmark_repetitions=1 \
                2>&1 | tee "$results_dir/${bench_name}_run${run}.log"
        done

        log_info "Completed $bench_name"
    done

    # Clean up build directory
    log_info "Cleaning up build directory..."
    rm -rf "$build_dir"

    log_info "Finished processing $system_name"
    return 0
}

# Function to analyze results and generate statistics
analyze_results() {
    local system_name=$1
    local results_dir="$OUTPUT_DIR/${system_name}"

    log_info "Analyzing results for $system_name..."

    # This would require a Python script for JSON processing
    # For now, just inform the user
    log_info "Results saved to $results_dir"
    log_info "Use analyze_benchmark_results.py to generate statistics"
}

# Main execution
main() {
    log_info "Starting baseline metrics collection..."
    log_info "Runs per benchmark: $RUNS"

    # Create output directory
    mkdir -p "$OUTPUT_DIR"

    # Get system information
    log_info "Collecting system information..."
    {
        echo "# System Information"
        echo "Timestamp: $(date)"
        echo "Hostname: $(hostname)"
        echo "OS: $(uname -s)"
        echo "Kernel: $(uname -r)"
        echo "Architecture: $(uname -m)"

        if command -v lscpu &> /dev/null; then
            echo -e "\n# CPU Information"
            lscpu | grep -E 'Model name|CPU\(s\)|Thread|Core|Socket|MHz'
        elif command -v sysctl &> /dev/null; then
            echo -e "\n# CPU Information (macOS)"
            sysctl -n machdep.cpu.brand_string
            sysctl -n hw.ncpu
            sysctl -n hw.cpufrequency
        fi

        echo -e "\n# Memory Information"
        if command -v free &> /dev/null; then
            free -h
        elif command -v vm_stat &> /dev/null; then
            vm_stat
        fi
    } > "$OUTPUT_DIR/system_info.txt"

    # List of systems to process
    local systems=(
        "thread_system"
        "logger_system"
        "monitoring_system"
        "container_system"
        "database_system"
        "network_system"
    )

    local success_count=0
    local fail_count=0

    # Process each system
    for system in "${systems[@]}"; do
        local system_dir="/Users/dongcheolshin/Sources/${system}"

        if [ ! -d "$system_dir" ]; then
            log_warn "System directory not found: $system_dir"
            ((fail_count++))
            continue
        fi

        if run_system_benchmarks "$system" "$system_dir"; then
            analyze_results "$system"
            ((success_count++))
        else
            log_error "Failed to process $system"
            ((fail_count++))
        fi
    done

    # Summary
    log_info "======================================="
    log_info "Baseline Metrics Collection Complete"
    log_info "======================================="
    log_info "Successful: $success_count"
    log_info "Failed: $fail_count"
    log_info "Results directory: $OUTPUT_DIR"
    log_info ""
    log_info "Next steps:"
    log_info "1. Run: python3 tools/analyze_benchmark_results.py"
    log_info "2. Review generated statistics"
    log_info "3. Update BASELINE.md files with actual values"
    log_info "4. Commit updated BASELINE.md files"
}

# Run main function
main "$@"
