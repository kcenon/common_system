# Ecosystem Pipeline Benchmarks

Cross-system end-to-end integration benchmarks for the kcenon ecosystem pipeline.
Measures interface-level overhead and integration costs across all 8 ecosystem systems.

**Related issue**: [#365](https://github.com/kcenon/common_system/issues/365)

---

## Overview

Each ecosystem system maintains isolated component benchmarks. This benchmark suite
measures the **integration overhead** — the cost paid when systems interact through
their interface boundaries.

### Simulated Pipeline

```
network_system (receive)
    -> container_system (deserialize)   [serialized_payload mock]
    -> thread_system (dispatch)         [inline_mock_executor]
    -> database_system (query)          [volatile compute simulation]
    -> logger_system (log)              [null_mock_logger]
    -> monitoring_system (record)       [null_mock_collector]
    -> container_system (serialize)     [serialized_payload mock]
    -> network_system (send)
```

### What "Interface Overhead" Means

These benchmarks measure the minimum cost any real implementation must pay:

| Cost Component | What It Includes |
|---------------|-----------------|
| **vtable dispatch** | Virtual function call + pointer indirection |
| **atomic counter** | Sequentially-consistent increment in null_mock_collector |
| **string construction** | Copying log message into log_entry |
| **Result<T> propagation** | ok/err branching and value wrapping |
| **EventBus routing** | TypeIndex lookup + subscriber vector iteration |

Real implementations will add I/O, lock contention, and memory allocation on top.

---

## Benchmark Categories

### 1. Interface Overhead (Phase 1 baseline)

**File**: `benchmarks/ecosystem/interface_overhead_benchmark.cpp`

| Benchmark | Measures |
|-----------|---------|
| `BM_ExecutorDispatch_Inline` | `IExecutor::execute()` vtable + future overhead |
| `BM_LoggerCall_SimpleMessage` | `ILogger::log()` with small string copy |
| `BM_LoggerCall_LargeMessage` | `ILogger::log()` with 1KB string copy |
| `BM_LoggerCall_LogEntry` | `log_entry::create()` + `ILogger::log(entry)` |
| `BM_MetricCollectorIncrement` | `IMetricCollector::increment()` call cost |
| `BM_MetricCollectorGauge` | `IMetricCollector::gauge()` call cost |
| `BM_MetricCollectorHistogram` | `IMetricCollector::histogram()` call cost |
| `BM_MetricCollectorTiming` | `IMetricCollector::timing()` call cost |
| `BM_EventBusPublishToExecutor` | EventBus publish → executor dispatch chain |

### 2. Data Pipeline (Phase 2)

**File**: `benchmarks/ecosystem/data_pipeline_benchmark.cpp`

| Benchmark | Measures |
|-----------|---------|
| `BM_Serialize_SmallPayload` | Serialize 64B (header + memcpy) |
| `BM_Serialize_MediumPayload` | Serialize 1KB |
| `BM_Serialize_LargePayload` | Serialize 64KB |
| `BM_Deserialize_ValidPayload` | Deserialize 1KB + Result<T> propagation |
| `BM_SerializeDeserializeRoundtrip` | Full roundtrip (parametric: 64B–64KB) |
| `BM_DispatchLogMetric` | Executor → Logger → Collector chain |
| `BM_FullDataPipeline_Small` | End-to-end pipeline with 64B payload |
| `BM_FullDataPipeline_Medium` | End-to-end pipeline with 1KB payload |
| `BM_LogAndCollect_Throughput` | Logger + Collector throughput (items/s) |

### 3. Request/Response Cycle (Phase 3)

**File**: `benchmarks/ecosystem/request_response_benchmark.cpp`

| Benchmark | Measures |
|-----------|---------|
| `BM_RequestResponseCycle_Single` | Single-client roundtrip latency |
| `BM_RequestResponseCycle_Batched` | Batched request throughput |
| `BM_ConcurrentClients` | N concurrent clients (1, 2, 4, 8) |
| `BM_MonitoringOverhead_Enabled` | Pipeline cost WITH monitoring |
| `BM_MonitoringOverhead_Disabled` | Pipeline cost WITHOUT monitoring |
| `BM_EventBusPipeline_Chained` | 2-stage event-driven chained pipeline |

### 4. Thread Scaling and Contention (Phase 4)

**File**: `benchmarks/ecosystem/scaling_benchmark.cpp`

| Benchmark | Measures |
|-----------|---------|
| `BM_ExecutorScaling` | Async executor throughput vs thread count (1–16) |
| `BM_SharedLogger_Contention` | Logger throughput under shared concurrent access |
| `BM_SharedCollector_Contention` | Collector throughput under shared concurrent access |
| `BM_SharedEventBus_Contention` | EventBus publish throughput under concurrency |
| `BM_MixedWorkload_Pipeline` | 60% read, 30% write, 10% monitor workload |

---

## Building

```bash
cmake -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=OFF \
  -DCOMMON_BUILD_BENCHMARKS=ON \
  -DCOMMON_HEADER_ONLY=ON

cmake --build build --target ecosystem_benchmarks
```

## Running

```bash
# Run all ecosystem benchmarks
./build/benchmarks/ecosystem/ecosystem_benchmarks

# Run with JSON output for trend tracking
./build/benchmarks/ecosystem/ecosystem_benchmarks \
  --benchmark_format=json \
  --benchmark_out=results.json \
  --benchmark_repetitions=3 \
  --benchmark_report_aggregates_only=true

# Run specific category
./build/benchmarks/ecosystem/ecosystem_benchmarks \
  --benchmark_filter="BM_FullDataPipeline"

# Run scaling benchmarks only
./build/benchmarks/ecosystem/ecosystem_benchmarks \
  --benchmark_filter="BM_.*Scaling|BM_.*Contention"
```

## CI Integration

Ecosystem benchmarks run via **manual workflow dispatch** only:

```bash
gh workflow run ecosystem-benchmarks.yml \
  --field filter="BM_FullDataPipeline" \
  --field repetitions=5
```

Regression detection threshold: **10%** slowdown triggers a workflow failure.
Results are archived as GitHub Actions artifacts with 90-day retention.

---

## Baseline Results

> **Note**: Baseline results are hardware-specific. Run on your target hardware
> and commit the JSON output for your environment.

### Hardware Profile

| Property | Value |
|----------|-------|
| Platform | To be measured |
| CPU | To be measured |
| Cores / Threads | To be measured |
| OS | To be measured |
| Compiler | To be measured |

Run the benchmarks and populate this section with your baseline numbers:

```bash
./build/benchmarks/ecosystem/ecosystem_benchmarks \
  --benchmark_format=json \
  --benchmark_out=docs/baseline_results.json
```

### Key Metrics to Watch

| Pipeline Stage | Expected Range | Regression Signal |
|---------------|----------------|-------------------|
| `IExecutor::execute()` vtable | 10–50 ns | > 100 ns |
| `ILogger::log()` small string | 5–30 ns | > 100 ns |
| `IMetricCollector::increment()` | 5–20 ns | > 50 ns |
| EventBus publish (1 subscriber) | 20–100 ns | > 500 ns |
| Full pipeline (64B) | 100–500 ns | > 2000 ns |
| Full pipeline (1KB) | 200–1000 ns | > 5000 ns |

---

## Design Notes

### Why Mock Implementations?

`common_system` defines the interface contracts but does not implement them.
Mocks measure the **minimum irreducible cost** of each interface boundary:

- `null_mock_logger`: atomic counter increment + vtable dispatch
- `null_mock_collector`: atomic counter increment + `DoNotOptimize(value)`
- `inline_mock_executor`: synchronous job dispatch (no thread scheduling)
- `async_mock_executor`: real thread-pool for concurrency benchmarks
- `serialized_payload`: header + memcpy (no compression or checksumming)

Real implementations will be slower due to I/O, locking, and serialization overhead.
The gap between these numbers and real system numbers reveals the implementation cost.

### Benchmark Reusability

The harness (`ecosystem_harness.h`) can be extended when new ecosystem systems are added:

1. Add a new mock implementing the system's common_system interface
2. Add it to `pipeline_context` if it participates in the main pipeline
3. Create a new `*_benchmark.cpp` file following the existing patterns
4. Register it in `benchmarks/ecosystem/CMakeLists.txt`

### Interpreting Scaling Results

`BM_ExecutorScaling` shows throughput (items/sec) vs thread count:

- **Linear scaling**: interface overhead is thread-local, no contention
- **Sub-linear scaling**: shared state introduces lock contention
- **Plateau**: hardware thread count limit reached

Ideal: `BM_SharedLogger_Contention` shows near-linear scaling because
`null_mock_logger` uses a single atomic counter without mutexes.
Real `logger_system` implementations using mutexed queues will plateau earlier.
