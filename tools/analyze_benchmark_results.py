#!/usr/bin/env python3
"""
Analyze Benchmark Results and Generate Statistics

This script processes JSON benchmark results from Google Benchmark
and generates statistics (mean, median, p95, p99) for BASELINE.md updates.
"""

import json
import sys
import os
from pathlib import Path
from typing import List, Dict, Any
from collections import defaultdict
import statistics


def parse_benchmark_json(filepath: Path) -> List[Dict[str, Any]]:
    """Parse a Google Benchmark JSON output file."""
    try:
        with open(filepath, 'r') as f:
            data = json.load(f)
            return data.get('benchmarks', [])
    except (json.JSONDecodeError, FileNotFoundError) as e:
        print(f"Error parsing {filepath}: {e}", file=sys.stderr)
        return []


def collect_benchmark_runs(results_dir: Path) -> Dict[str, List[float]]:
    """Collect all runs for each benchmark."""
    benchmark_data = defaultdict(lambda: defaultdict(list))

    # Find all JSON result files
    json_files = list(results_dir.glob('*.json'))

    for json_file in json_files:
        benchmarks = parse_benchmark_json(json_file)

        for bench in benchmarks:
            name = bench.get('name', '')
            time_unit = bench.get('time_unit', 'ns')

            # Get the actual time value
            if 'real_time' in bench:
                time_value = bench['real_time']
            elif 'cpu_time' in bench:
                time_value = bench['cpu_time']
            else:
                continue

            # Store with time unit
            benchmark_data[name]['values'].append(time_value)
            benchmark_data[name]['unit'] = time_unit

            # Store other metrics if available
            if 'bytes_per_second' in bench:
                benchmark_data[name]['throughput'].append(bench['bytes_per_second'])
            if 'items_per_second' in bench:
                benchmark_data[name]['items_per_second'].append(bench['items_per_second'])

    return benchmark_data


def calculate_statistics(values: List[float]) -> Dict[str, float]:
    """Calculate statistical metrics from a list of values."""
    if not values:
        return {}

    sorted_values = sorted(values)
    n = len(sorted_values)

    return {
        'mean': statistics.mean(values),
        'median': statistics.median(values),
        'min': min(values),
        'max': max(values),
        'stddev': statistics.stdev(values) if n > 1 else 0.0,
        'p95': sorted_values[int(n * 0.95)] if n > 0 else 0.0,
        'p99': sorted_values[int(n * 0.99)] if n > 0 else 0.0,
        'count': n,
    }


def format_time(value: float, unit: str) -> str:
    """Format time value with appropriate unit."""
    # Convert to appropriate unit for readability
    if unit == 'ns':
        if value < 1000:
            return f"{value:.2f} ns"
        elif value < 1000000:
            return f"{value / 1000:.2f} μs"
        else:
            return f"{value / 1000000:.2f} ms"
    elif unit == 'us':
        if value < 1000:
            return f"{value:.2f} μs"
        else:
            return f"{value / 1000:.2f} ms"
    elif unit == 'ms':
        if value < 1000:
            return f"{value:.2f} ms"
        else:
            return f"{value / 1000:.2f} s"
    else:
        return f"{value:.2f} {unit}"


def format_throughput(value: float) -> str:
    """Format throughput value."""
    if value < 1000:
        return f"{value:.2f} /s"
    elif value < 1000000:
        return f"{value / 1000:.2f} K/s"
    else:
        return f"{value / 1000000:.2f} M/s"


def generate_markdown_table(benchmark_data: Dict[str, Dict[str, Any]]) -> str:
    """Generate a markdown table with benchmark statistics."""
    output = []

    output.append("# Benchmark Results Summary\n")
    output.append("## Statistics\n")

    for bench_name, data in sorted(benchmark_data.items()):
        if 'values' not in data or not data['values']:
            continue

        stats = calculate_statistics(data['values'])
        unit = data.get('unit', 'ns')

        output.append(f"\n### {bench_name}\n")
        output.append("| Metric | Value |")
        output.append("|--------|-------|")
        output.append(f"| Mean | {format_time(stats['mean'], unit)} |")
        output.append(f"| Median | {format_time(stats['median'], unit)} |")
        output.append(f"| Min | {format_time(stats['min'], unit)} |")
        output.append(f"| Max | {format_time(stats['max'], unit)} |")
        output.append(f"| P95 | {format_time(stats['p95'], unit)} |")
        output.append(f"| P99 | {format_time(stats['p99'], unit)} |")
        output.append(f"| Std Dev | {format_time(stats['stddev'], unit)} |")
        output.append(f"| Runs | {stats['count']} |")

        # Add throughput if available
        if 'items_per_second' in data and data['items_per_second']:
            throughput_stats = calculate_statistics(data['items_per_second'])
            output.append(f"\n**Throughput:**")
            output.append(f"- Mean: {format_throughput(throughput_stats['mean'])}")
            output.append(f"- Median: {format_throughput(throughput_stats['median'])}")

    return '\n'.join(output)


def generate_csv_output(benchmark_data: Dict[str, Dict[str, Any]]) -> str:
    """Generate CSV output for further analysis."""
    output = []
    output.append("Benchmark,Mean,Median,Min,Max,P95,P99,StdDev,Unit,Runs")

    for bench_name, data in sorted(benchmark_data.items()):
        if 'values' not in data or not data['values']:
            continue

        stats = calculate_statistics(data['values'])
        unit = data.get('unit', 'ns')

        output.append(
            f"{bench_name},{stats['mean']:.2f},{stats['median']:.2f},"
            f"{stats['min']:.2f},{stats['max']:.2f},"
            f"{stats['p95']:.2f},{stats['p99']:.2f},"
            f"{stats['stddev']:.2f},{unit},{stats['count']}"
        )

    return '\n'.join(output)


def process_system(system_name: str, results_dir: Path) -> None:
    """Process benchmark results for a single system."""
    print(f"\n{'='*60}")
    print(f"Processing: {system_name}")
    print(f"{'='*60}")

    if not results_dir.exists():
        print(f"Results directory not found: {results_dir}")
        return

    # Collect benchmark data
    benchmark_data = collect_benchmark_runs(results_dir)

    if not benchmark_data:
        print(f"No benchmark data found for {system_name}")
        return

    print(f"Found {len(benchmark_data)} unique benchmarks")

    # Generate outputs
    markdown_output = generate_markdown_table(benchmark_data)
    csv_output = generate_csv_output(benchmark_data)

    # Save markdown report
    markdown_file = results_dir / 'summary.md'
    with open(markdown_file, 'w') as f:
        f.write(markdown_output)
    print(f"Markdown summary: {markdown_file}")

    # Save CSV data
    csv_file = results_dir / 'statistics.csv'
    with open(csv_file, 'w') as f:
        f.write(csv_output)
    print(f"CSV statistics: {csv_file}")

    # Print summary to console
    print("\n" + markdown_output)


def main():
    """Main entry point."""
    # Default results directory
    results_base = Path('/Users/dongcheolshin/Sources/baseline_results')

    if not results_base.exists():
        print(f"Results directory not found: {results_base}")
        print("Run collect_baseline_metrics.sh first to generate benchmark results.")
        sys.exit(1)

    # Process each system
    systems = [
        'thread_system',
        'logger_system',
        'monitoring_system',
        'container_system',
        'database_system',
        'network_system',
    ]

    processed_count = 0

    for system in systems:
        system_dir = results_base / system
        if system_dir.exists():
            process_system(system, system_dir)
            processed_count += 1

    print(f"\n{'='*60}")
    print(f"Processed {processed_count} system(s)")
    print(f"{'='*60}")

    if processed_count > 0:
        print("\nNext steps:")
        print("1. Review summary.md files in each system's results directory")
        print("2. Update BASELINE.md files with actual values")
        print("3. Commit updated BASELINE.md files to git")


if __name__ == '__main__':
    main()
