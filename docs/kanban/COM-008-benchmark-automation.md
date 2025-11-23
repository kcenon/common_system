# COM-008: Benchmark 자동화 및 회귀 감지

**Status**: TODO
**Priority**: MEDIUM
**Category**: PERF
**Estimated Duration**: 5-6h
**Dependencies**: None

---

## 개요

성능 벤치마크를 CI에 통합하고 성능 회귀를 자동 감지합니다.

## 현재 상태

**기존 성능 테스트**:
- `integration_tests/performance/result_performance_test.cpp`
- Google Benchmark 의존성 (`vcpkg.json`)

**누락**:
- CI 통합 벤치마크
- 성능 회귀 자동 감지
- 벤치마크 결과 추적

## 작업 항목

### 1. 벤치마크 확장
```cpp
// benchmarks/
├── result_benchmark.cpp
├── event_bus_benchmark.cpp
├── object_pool_benchmark.cpp
└── CMakeLists.txt
```

### 2. CI 워크플로우
```yaml
# .github/workflows/benchmark.yml
name: Performance Benchmark
on:
  push:
    branches: [main]
  pull_request:

jobs:
  benchmark:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - name: Run benchmarks
        run: |
          cmake -B build -DBUILD_BENCHMARKS=ON
          cmake --build build
          ./build/benchmarks/run_benchmarks --benchmark_format=json > results.json
      - name: Compare with baseline
        # benchmark-action 또는 커스텀 스크립트
```

### 3. 결과 저장 및 비교
- [ ] 베이스라인 결과 저장
- [ ] PR별 비교 리포트
- [ ] 임계값 초과 시 경고

## 수락 기준

- [ ] 주요 컴포넌트별 벤치마크 존재
- [ ] CI에서 자동 실행
- [ ] 10% 이상 성능 저하 시 경고
- [ ] 벤치마크 결과 히스토리 추적
