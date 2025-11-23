# COM-002: Event Bus Retry Logic 테스트 추가

**Status**: TODO
**Priority**: HIGH
**Category**: TEST
**Estimated Duration**: 4-5h
**Dependencies**: None

---

## 개요

Event Bus의 재시도 및 실패 복구 시나리오에 대한 테스트를 추가합니다.

## 현재 상태

**기존 테스트 파일**:
- `tests/improved_event_bus_test.cpp`: 기본 기능 테스트
- `integration_tests/scenarios/event_bus_integration_test.cpp`: 통합 테스트

**누락된 테스트**:
- 핸들러 예외 발생 시 동작
- 다중 핸들러 중 일부 실패 시나리오
- 구독 해제 중 이벤트 발행

## 작업 항목

### 1. 예외 처리 테스트
```cpp
// tests/event_bus_failure_test.cpp
TEST(EventBusFailure, HandlerThrowsException)
TEST(EventBusFailure, PartialHandlerFailure)
TEST(EventBusFailure, ExceptionIsolation)
```

### 2. 동시성 엣지 케이스
```cpp
TEST(EventBusConcurrency, UnsubscribeDuringPublish)
TEST(EventBusConcurrency, SubscribeDuringPublish)
TEST(EventBusConcurrency, RapidSubscribeUnsubscribe)
```

### 3. 리소스 관리 테스트
```cpp
TEST(EventBusResource, MemoryLeakOnException)
TEST(EventBusResource, HandlerLifetime)
```

## 테스트 파일 위치

- `tests/event_bus_failure_test.cpp` (신규)
- `tests/CMakeLists.txt` 업데이트

## 수락 기준

- [ ] 최소 10개의 새로운 테스트 케이스
- [ ] 모든 테스트 통과
- [ ] AddressSanitizer 클린
- [ ] ThreadSanitizer 클린
