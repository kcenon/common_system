---
doc_id: "COM-PERF-002"
doc_title: "Common System - Performance Benchmarks"
doc_version: "1.1.0"
doc_date: "2026-09-12"
doc_status: "Released"
project: "common_system"
category: "PERF"
---

# Benchmark Evidence

**Language:** **English** | [한국어](BENCHMARKS.kr.md)

This page presents an existing CI measurement with its raw output and provenance.
The document date describes this edit, not a new benchmark execution.

## Recorded Snapshot

**Measurement date:** `2026-09-12T10:00:09+00:00`, from the JSON context.
**Environment:** GitHub Actions, Ubuntu 22.04.5, GNU C++ 11.4.0,
Google Benchmark 1.6.1, Release build. The CPU model was not recorded.
**Raw result:** [benchmark_results.json](benchmarks/2026-09-12-8558128/benchmark_results.json).
**Command and provenance:** [retained run record](benchmarks/2026-09-12-8558128/README.md#commands).

The following values are the `real_time` medians, in nanoseconds, rounded to three
decimal places. Each named workload has three repetitions. These are elapsed-time
statistics, not the separate `cpu_time` field. Both language editions use the same JSON.

| Raw record name | Median real time (ns) |
| --- | ---: |
| `BM_ResultOkCreation_median` | 1.558 |
| `BM_ResultErrorCreation_median` | 19.385 |
| `BM_EventBusPublishNoSubscribers_median` | 17.796 |
| `BM_EventBusPublishSingleSubscriber_median` | 56.777 |

Source: [Actions run 34687250283](https://github.com/kcenon/common_system/actions/runs/34687250283)
at [commit 8558128](https://github.com/kcenon/common_system/commit/8558128033457ffb4635f564b0d495dddd6f8807).
The [provenance record](benchmarks/2026-09-12-8558128/README.md) records the artifact ID,
checksum, environment, build flags, commands, and selection method.

## Methodology and Limits

The [benchmark workflow](https://github.com/kcenon/common_system/blob/8558128033457ffb4635f564b0d495dddd6f8807/.github/workflows/benchmark.yml)
built `build/benchmarks/common_benchmarks` in Release mode with interprocedural
optimization disabled. The [build configuration](../benchmarks/CMakeLists.txt)
adds `-O3 -DNDEBUG -fno-lto` for GNU/Clang.

The output has 384 aggregate records and three repetitions per workload.
Google Benchmark selected iteration counts; the invocation exported aggregate
records, not individual repetition samples. It does not establish confidence
intervals, fixed iteration counts, or medians over ten independent CI runs.

A CI runner's timing depends on its environment and load. Different benchmark
names represent different work, so these rows are not interchangeable headline
costs. They neither disprove measurements on other machines nor validate the
historical speedup ratios or allocation claims.

The retained output contains no exception-versus-Result comparison, executor-versus-
`std::async` comparison, standalone `is_ok()` measurement, or allocation counts
supporting the former README table. The ecosystem inline-executor benchmark is a
separate dispatch workload, not a thread-pool scheduling comparison.

## Workloads and APIs

- [Result workloads](../benchmarks/result_benchmark.cpp): success/error construction,
  transformations, chains, and throughput workloads.
- [Event workloads](../benchmarks/event_bus_benchmark.cpp): publishing with different
  subscriber counts, filters, and event types.
- [Ecosystem interface workloads](../benchmarks/ecosystem/interface_overhead_benchmark.cpp):
  separate interface-dispatch experiments.
- [Benchmark target and source list](../benchmarks/CMakeLists.txt).

Event publishing snapshots handlers under a mutex, then calls them synchronously
after releasing it; see [event_bus.h](../include/kcenon/common/patterns/event_bus.h).
The timing output does not establish allocation behavior.

Executor submission uses `execute(std::unique_ptr<IJob>&&)`, returning
`Result<std::future<void>>`. The [API examples](guides/QUICK_START.md#iexecutor-interface)
show how an application supplies the executor and checks submission errors.

## Withdrawn Historical Claims

The earlier core-operation, allocation, scaling, platform, and comparison tables
have been withdrawn from presented results. Their environment prose and document
update dates were not linked to a dated command/output record. This also removes
the derived exception/executor speedups and blanket abstraction-overhead conclusion.

The [previous page](https://github.com/kcenon/common_system/blob/8558128033457ffb4635f564b0d495dddd6f8807/docs/BENCHMARKS.md)
remains in Git history for review; it is not measurement evidence. No benchmarks
were rerun to replace those claims. Future result additions need a retained raw
file, source revision, measurement date, environment, exact command, and named statistic.

[Back to README](../README.md)
