---
doc_id: "COM-PERF-005"
doc_title: "Benchmark snapshot provenance - 2026-09-12"
doc_version: "1.0.0"
doc_date: "2026-09-12"
doc_status: "Released"
project: "common_system"
category: "PERF"
---

# Benchmark Snapshot Provenance

This is a copy of an existing CI artifact, not a new benchmark execution.
The documentation edit date in frontmatter is separate from the measurement date.

## Origin and Integrity

- Run: [34687250283](https://github.com/kcenon/common_system/actions/runs/34687250283).
- Job: [Run Benchmarks, 103536283604](https://github.com/kcenon/common_system/actions/runs/34687250283/job/103536283604).
- Source revision: [8558128033457ffb4635f564b0d495dddd6f8807](https://github.com/kcenon/common_system/commit/8558128033457ffb4635f564b0d495dddd6f8807).
- Artifact: `benchmark-results`, ID `10295449513`.
- Raw result: [benchmark_results.json](benchmark_results.json), copied without edits.
- Raw file size: 186508 bytes.
- SHA-256: `786fa3f5e0c22070efc2d4a484491031a9ee694a1b7ae547ab5a9ef7169dee96`.
- Artifact expiry reported by GitHub: `2026-12-11T09:59:21Z`; this copy preserves the output.
- Measurement date from JSON: `2026-09-12T10:00:09+00:00`.

## Environment

Job logs report Ubuntu 22.04.5, GNU C++ 11.4.0, and the installed Google Benchmark
package 1.6.1. JSON context reports a Release library build, host `runnervm7g52i`,
four logical CPUs, 3245 MHz per CPU, and CPU scaling disabled. It includes cache
and load-average fields. The CPU model and total RAM are not recorded.

CMake configured Release with interprocedural optimization disabled.
The [pinned benchmark build definition](https://github.com/kcenon/common_system/blob/8558128033457ffb4635f564b0d495dddd6f8807/benchmarks/CMakeLists.txt)
adds `-O3 -DNDEBUG -fno-lto` and links with `-fno-lto` for GNU/Clang.
Do not attribute this runner to the historical Intel i7-9700K environment.

## Commands

The [pinned workflow](https://github.com/kcenon/common_system/blob/8558128033457ffb4635f564b0d495dddd6f8807/.github/workflows/benchmark.yml)
and job logs record these commands from the source checkout root:

```sh
cmake -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=OFF \
  -DCOMMON_BUILD_TESTS=OFF \
  -DCOMMON_BUILD_EXAMPLES=OFF \
  -DCOMMON_BUILD_BENCHMARKS=ON \
  -DCOMMON_HEADER_ONLY=ON
cmake --build build --config Release
./build/benchmarks/common_benchmarks \
  --benchmark_format=json \
  --benchmark_out=benchmark_results.json \
  --benchmark_repetitions=3 \
  --benchmark_report_aggregates_only=true
```

These commands describe the historical run; no benchmark rerun is needed to
verify the snapshot. Read the JSON or compute its checksum instead.

## Selected Statistic

The [English](../../BENCHMARKS.md#recorded-snapshot) and
[Korean](../../BENCHMARKS.kr.md#기록된-측정) tables select the unique records named
`BM_ResultOkCreation_median`, `BM_ResultErrorCreation_median`,
`BM_EventBusPublishNoSubscribers_median`, and
`BM_EventBusPublishSingleSubscriber_median`. Each has `aggregate_name: median`,
`repetitions: 3`, and `time_unit: ns`. Display `real_time` rounded to three decimal
places, without substituting `cpu_time` or converting another aggregate.

The file has 384 aggregate records. Individual repetition samples were not exported.
No exception comparison, executor comparison, standalone `is_ok()` timing, or
allocation counts supporting the old README table are present. The named workloads
and this runner environment limit what conclusions can be drawn.
