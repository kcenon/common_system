---
doc_id: "COM-PERF-001"
doc_title: "Common System - 성능 벤치마크"
doc_version: "1.1.0"
doc_date: "2026-09-12"
doc_status: "Released"
project: "common_system"
category: "PERF"
---

# 벤치마크 근거

**언어:** [English](BENCHMARKS.md) | **한국어**

기존 CI 측정의 원본 결과와 출처를 정리한 문서입니다.
문서 날짜는 편집 날짜이며 새 벤치마크 실행 날짜가 아닙니다.

## 기록된 측정

**측정 날짜:** JSON context의 `2026-09-12T10:00:09+00:00`.
**환경:** GitHub Actions, Ubuntu 22.04.5, GNU C++ 11.4.0,
Google Benchmark 1.6.1, Release 빌드. CPU 모델은 기록되지 않았습니다.
**원본 결과:** [benchmark_results.json](benchmarks/2026-09-12-8558128/benchmark_results.json).
**명령과 출처:** [보존된 실행 기록](benchmarks/2026-09-12-8558128/README.md#commands).

다음 값은 `real_time` 중앙값을 나노초 단위로 소수점 셋째 자리까지 반올림한 것입니다.
각 작업 부하는 세 번 반복했습니다. 별도의 `cpu_time` 필드가 아닌 경과 시간 통계이며,
한국어판과 영어판은 동일한 JSON을 사용합니다.

| 원본 레코드 이름 | 경과 시간 중앙값 (ns) |
| --- | ---: |
| `BM_ResultOkCreation_median` | 1.558 |
| `BM_ResultErrorCreation_median` | 19.385 |
| `BM_EventBusPublishNoSubscribers_median` | 17.796 |
| `BM_EventBusPublishSingleSubscriber_median` | 56.777 |

출처: [Actions 실행 34687250283](https://github.com/kcenon/common_system/actions/runs/34687250283),
[커밋 8558128](https://github.com/kcenon/common_system/commit/8558128033457ffb4635f564b0d495dddd6f8807).
[출처 기록](benchmarks/2026-09-12-8558128/README.md)에 아티팩트 ID, 체크섬,
환경, 빌드 플래그, 명령 및 통계 선택 방법을 보존했습니다.

## 측정 방법과 한계

[벤치마크 워크플로](https://github.com/kcenon/common_system/blob/8558128033457ffb4635f564b0d495dddd6f8807/.github/workflows/benchmark.yml)는
프로시저 간 최적화를 끄고 `build/benchmarks/common_benchmarks`를 Release 모드로 빌드했습니다.
[빌드 설정](../benchmarks/CMakeLists.txt)은 GNU/Clang에 `-O3 -DNDEBUG -fno-lto`를 추가합니다.

출력에는 집계 레코드 384개가 있으며 작업 부하별 반복 횟수는 세 번입니다.
Google Benchmark가 내부 반복 횟수를 선택했고, 실행 명령은 개별 반복 표본 대신
집계 결과를 내보냈습니다. 신뢰 구간, 고정 내부 반복 횟수 또는 독립적인 CI 실행
열 번의 중앙값을 입증하는 자료가 아닙니다.

CI 타이밍은 환경과 부하에 따라 달라집니다. 벤치마크 이름마다 수행하는 작업이
다르므로 표의 값을 서로 대체할 수 없습니다. 다른 기계에서의 측정을 반박하거나
과거 속도 배율 및 할당 주장을 검증하는 결과도 아닙니다.

보존된 출력에는 예외와 Result의 비교, 실행기와 `std::async`의 비교,
독립적인 `is_ok()` 측정 또는 이전 README 표를 뒷받침하는 할당 횟수가 없습니다.
생태계 인라인 실행기 벤치마크는 별도의 인터페이스 디스패치 작업이며
스레드 풀 스케줄링 비교가 아닙니다.

## 작업 부하와 API

- [Result 작업 부하](../benchmarks/result_benchmark.cpp): 성공/오류 생성, 변환, 체이닝, 처리량.
- [이벤트 작업 부하](../benchmarks/event_bus_benchmark.cpp): 구독자 수, 필터 및 이벤트 타입별 게시.
- [생태계 인터페이스 작업 부하](../benchmarks/ecosystem/interface_overhead_benchmark.cpp): 별도의 호출 실험.
- [벤치마크 타깃과 소스 목록](../benchmarks/CMakeLists.txt).

이벤트 게시 시 뮤텍스를 잡고 핸들러 목록을 복사한 뒤 잠금을 해제하고 동기적으로
호출합니다. [event_bus.h](../include/kcenon/common/patterns/event_bus.h)를 참고하세요.
타이밍 출력만으로 할당 동작을 확인할 수는 없습니다.

작업 제출 API는 `execute(std::unique_ptr<IJob>&&)`이며 반환형은
`Result<std::future<void>>`입니다. [API 예제](guides/QUICK_START.md#iexecutor-interface)는
애플리케이션이 실행기를 제공하고 제출 오류를 처리하는 방법을 보여 줍니다.

## 철회한 과거 주장

기존의 핵심 연산, 할당, 확장성, 플랫폼 및 비교 표를 결과에서 제거했습니다.
당시 환경 설명과 문서 수정 날짜가 측정 날짜, 명령 및 원본 출력에 연결되어 있지
않았기 때문입니다. 해당 표에서 파생된 예외/실행기 속도 배율과
추상화 비용에 대한 일괄적인 결론도 제거했습니다.

[이전 문서](https://github.com/kcenon/common_system/blob/8558128033457ffb4635f564b0d495dddd6f8807/docs/BENCHMARKS.kr.md)는
검토용 Git 이력으로 남아 있으며 측정 근거로 사용하지 않습니다.
이번 변경에서는 벤치마크를 다시 실행하지 않았습니다. 향후 결과에는 원본 파일,
소스 리비전, 측정 날짜, 환경, 정확한 명령 및 선택한 통계 이름을 함께 기록해야 합니다.

[README로 돌아가기](../README.kr.md)
