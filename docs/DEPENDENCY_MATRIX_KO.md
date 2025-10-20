# Dependency Matrix - 상세 분석

> **Language:** [English](DEPENDENCY_MATRIX.md) | **한국어**


**날짜**: 2025-10-10
**단계**: Phase 4 - 의존성 리팩토링
**분석 유형**: 상세한 Include 및 CMake 의존성 매핑

---

## 요약

### 현재 의존성 상태

| 시스템 | CMake 의존성 | Include 의존성 | 순환? | 상태 |
|--------|-----------|--------------|-----------|---------|
| common_system | 0 | 1 (monitoring*) | ⚠️ Yes | *조건부 컴파일 |
| thread_system | 0 | 1 (common) | ⚠️ Yes (순환 경유) | service_container DI 사용 |
| logger_system | 2 | 2 | ⚠️ Yes (순환 경유) | 유효한 계층 구조 |
| monitoring_system | 3 | 2 | ⚠️ Yes | common용 구현 제공 |
| container_system | 0 | 1 (common) | ⚠️ Yes (Cycle 3 경유) | 최소 의존성 |
| database_system | 0 | 0 | ✅ No | 독립적 |
| network_system | 0 | 3 | ⚠️ Yes (Cycle 3 경유) | thread 통합 포함 |

**주요 발견**: 대부분의 의존성이 **include-only** (CMakeLists.txt에 없음)이며, 이는 헤더 전용 사용을 나타냅니다.

---

## Cycle 1: common_system ↔ monitoring_system

### 의존성 체인

```
common_system → monitoring_system → (logger_system → thread_system) → common_system
```

### 상세 분석

#### common_system → monitoring_system

**파일**: `/Users/dongcheolshin/Sources/common_system/include/kcenon/common/patterns/event_bus.h:32-33`

```cpp
#if defined(ENABLE_MONITORING_INTEGRATION) || defined(WITH_MONITORING_SYSTEM) || defined(USE_MONITORING_SYSTEM)

// Forward to the actual event bus implementation
#include <kcenon/monitoring/core/event_bus.h>
#include <kcenon/monitoring/interfaces/event_bus_interface.h>

namespace common {
    using event_bus = monitoring_system::event_bus;
    // ...
}

#else // Monitoring integration disabled

// Provide a stub implementation when monitoring is disabled
class null_event_bus {
    // No-op implementation
};

#endif
```

**의존성 유형**: 조건부 Include (헤더 전용)
**CMake 의존성**: 없음
**영향도**: 낮음 - 이미 폴백 구현 존재

**상태**: ✅ **이미 완화됨**
- 조건부 컴파일이 하드 의존성 방지
- `null_event_bus`가 no-op 폴백 제공
- 사용자는 monitoring_system 없이 common_system 빌드 가능

#### monitoring_system → common_system

**파일**:
1. `monitoring_system/include/kcenon/monitoring/core/performance_monitor.h:` → `common/interfaces/monitoring_interface.h`
2. `monitoring_system/include/kcenon/monitoring/core/result_types.h` → `common/patterns/result.h`
3. 여러 어댑터 파일 → `common_system`

**의존성 유형**: 직접 Include (필수)
**CMake 의존성**: Yes - `target_link_libraries(monitoring_system PUBLIC common_system)`
**영향도**: 중간 - 유효한 계층 구조 (monitoring이 상위 레벨)

**상태**: ✅ **유효한 의존성**
- monitoring_system은 Level 2, common_system은 Level 0
- 상향 의존성은 아키텍처적으로 올바름

### Cycle 1 해결 전략

**결론**: ✅ **조치 불필요**

**근거**:
1. common_system → monitoring은 **선택적** (조건부 컴파일)
2. monitoring → common은 **유효** (상위 레벨이 기반에 의존)
3. 순환은 조건부 include를 통해 **설계상 깨짐**

**검증**:
```bash
# monitoring 없이 common_system 빌드
cd common_system
cmake -B build -DENABLE_MONITORING_INTEGRATION=OFF
cmake --build build
# 예상: 성공 ✅
```

---

## Cycle 2: Cycle 1에 의해 해결됨

### 분석

**체인**: `logger_system → thread_system → common_system → monitoring_system → logger_system`

**상태**: ✅ **자동 해결**
- Cycle 1이 조건부이므로, 이 순환도 깨짐
- logger → thread는 유효 (상위가 하위에 의존)
- thread → common은 유효 (모두가 기반에 의존)

**조치 불필요**: monitoring 통합이 비활성화되면 Cycle 2는 존재하지 않습니다.

---

## Cycle 3: thread_system ↔ container_system ↔ network_system

### 의존성 체인

```
thread_system → container_system → network_system → thread_system
```

### 상세 분석

#### thread_system → container_system

**파일**: `/Users/dongcheolshin/Sources/thread_system/include/kcenon/thread/interfaces/thread_context.h:39`

```cpp
#include "service_container.h"
```

**의존성 유형**: Include (service locator 패턴)
**CMake 의존성**: 없음
**목적**: Service Container를 통한 의존성 주입

**분석**:
```cpp
class thread_context {
public:
    thread_context()
        : logger_(service_container::global().resolve<logger_interface>())
        , monitoring_(service_container::global().resolve<monitoring_interface::monitoring_interface>()) {
    }
    // ...
};
```

**상태**: ✅ **이미 DI 패턴 사용 중**
- 런타임 주입을 위해 service_container 사용
- container_system 데이터 구조에 대한 컴파일 타임 의존성 없음
- service_container 인터페이스에만 의존

**발견**: 이것은 `container_system` (variant container)에 대한 의존성이 **아닙니다**.
**service locator** 패턴에 대한 의존성입니다 (thread_system 내부일 가능성).

**조치**: `service_container.h` 위치 확인

```bash
find /Users/dongcheolshin/Sources/thread_system -name "service_container.h"
```

#### container_system → network_system

**검색 결과**: include를 찾을 수 없음

```bash
grep -r "#include.*network" /Users/dongcheolshin/Sources/container_system/
# 결과: 비어있음
```

**상태**: ✅ **거짓 양성**
- 의존성 분석기가 전이 의존성을 감지했을 수 있음
- container_system에서 network_system의 직접 include 없음

#### network_system → thread_system

**파일**:
1. `network_system/include/network_system/integration/messaging_bridge.h:60` → `kcenon/thread/core/thread_pool.h`
2. `network_system/include/network_system/compatibility.h:21` → `network_system/integration/thread_integration.h`
3. 여러 파일 → `<thread>` (std::thread, thread_system 아님)

**의존성 유형**: 직접 Include
**CMake 의존성**: 없음 (include-only)
**목적**: 비동기 I/O thread pool 통합

**상태**: ⚠️ **조사 필요**
- network_system이 thread_system의 thread_pool을 필요로 하는가?
- 아니면 std::thread를 직접 사용할 수 있는가?

### Cycle 3 해결 전략

**조사 필요**:

1. **service_container.h 위치 파악**
   ```bash
   find thread_system -name "service_container.h"
   ```

2. **network가 정말로 thread_system을 필요로 하는지 확인**
   ```bash
   grep -A5 -B5 "kcenon/thread" network_system/include/network_system/integration/messaging_bridge.h
   ```

3. **의존성이 필수인지 편의성인지 결정**

**가능한 해결 방법**:

**옵션 A**: `IThreadPool` 인터페이스를 `common_system`으로 추출
- network_system이 인터페이스 사용
- thread_system이 인터페이스 구현
- 컴파일 타임 의존성 제거

**옵션 B**: 의존성 제거 (std::thread로 충분한 경우)
- network_system이 std::thread를 직접 사용
- 외부 thread pool 불필요
- 가장 간단한 해결책

**옵션 C**: 의존성을 단방향으로 만들기 (network → thread만)
- network가 상위 레벨인 경우 유효
- thread가 network를 include하지 않는지 확인

---

## 시스템별 의존성

### common_system (기반 - Level 0)

**예상**: 의존성 없음
**실제**:
- CMake: 없음 ✅
- Include: monitoring_system (조건부) ✅

**의존성이 있는 파일**:
```
include/kcenon/common/patterns/event_bus.h
├── #if ENABLE_MONITORING_INTEGRATION
│   ├── #include <kcenon/monitoring/core/event_bus.h>
│   └── #include <kcenon/monitoring/interfaces/event_bus_interface.h>
└── #else
    └── // null_event_bus (no-op)
```

**평가**: ✅ **준수** - 조건부 의존성은 허용됨

---

### thread_system (Level 1)

**예상**: common_system만
**실제**:
- CMake: 없음
- Include: common_system ✅, service_container (내부?)

**의존성이 있는 파일**:
```
include/kcenon/thread/interfaces/thread_context.h
├── "logger_interface.h" (common 또는 내부일 가능성)
├── "monitoring_interface.h" (common 또는 내부일 가능성)
└── "service_container.h" (❓ 위치 불명)
```

**평가**: ⚠️ **검증 필요** - service_container 위치 확인

---

### logger_system (Level 1)

**예상**: common_system, thread_system
**실제**:
- CMake: common_system, thread_system ✅
- Include: common_system, thread_system ✅

**평가**: ✅ **준수** - 유효한 계층 구조

---

### monitoring_system (Level 2)

**예상**: common_system, logger_system, thread_system
**실제**:
- CMake: common_system, logger_system, thread_system ✅
- Include: common_system, thread_system ✅ (logger는 CMake에만)

**의존성이 있는 파일**:
```
include/kcenon/monitoring/core/performance_monitor.h
├── #include <kcenon/common/interfaces/monitoring_interface.h>

include/kcenon/monitoring/core/result_types.h
├── #if BUILD_WITH_COMMON_SYSTEM
│   ├── #include <kcenon/common/patterns/result.h>
└── #else
    └── // Fallback result.h
```

**평가**: ✅ **준수** - common_system의 적절한 사용

---

### container_system (Level 1)

**예상**: common_system
**실제**:
- CMake: 없음
- Include: common_system ✅

**평가**: ✅ **준수**

---

### database_system (Level 2)

**예상**: common_system, 기타 가능
**실제**:
- CMake: 없음 ✅
- Include: 없음 ✅

**평가**: ✅ **독립적** - 의존성 없음 (놀랍지만 유효함)

---

### network_system (Level 2)

**예상**: common_system, thread_system, logger_system
**실제**:
- CMake: 없음
- Include: common_system, logger_system, thread_system

**의존성이 있는 파일**:
```
include/network_system/integration/messaging_bridge.h
├── #include <kcenon/thread/core/thread_pool.h>

include/network_system/integration/thread_integration.h
├── // thread system 통합 가능성
```

**평가**: ⚠️ **리팩토링 필요** - Include-only 의존성은 CMake에 있어야 함

---

## 필수 vs. 편의 의존성

### 필수 (Essential)

| 시스템 | 의존 대상 | 이유 |
|--------|-----------|--------|
| logger_system | thread_system | 비동기 로깅에 thread pool 필요 |
| logger_system | common_system | Result<T>, 인터페이스 |
| monitoring_system | common_system | 인터페이스, Result<T> |
| monitoring_system | logger_system | 모니터링 이벤트 로깅 |
| monitoring_system | thread_system | thread pool 메트릭 모니터링 |
| network_system | common_system | Result<T>, 인터페이스 |

### 편의 (Convenience)

| 시스템 | 의존 대상 | 이유 | 제거 가능? |
|--------|-----------|--------|-------------|
| common_system | monitoring_system | Event bus 전달 | ✅ Yes (조건부) |
| network_system | thread_system | 비동기 I/O용 thread pool | ⚠️ Maybe (std::thread 사용) |
| network_system | logger_system | 네트워크 이벤트 로깅 | ⚠️ Maybe (common 인터페이스 사용) |

---

## 액션 아이템

### 즉시 조치

1. ✅ **Cycle 1이 이미 완화되었는지 확인** (조건부 컴파일)
   ```bash
   cd common_system
   cmake -B build -DENABLE_MONITORING_INTEGRATION=OFF
   ```

2. ❓ **service_container.h 위치 파악**
   ```bash
   find thread_system -name "service_container.h"
   ```

3. ⚠️ **network → thread 의존성 조사**
   - thread_pool이 필수인가 아니면 std::thread로 가능한가?
   - common에서 IThreadPool 인터페이스를 사용할 수 있는가?

### 단기 조치 (1주차)

1. **include-only 시스템에 대한 CMakeLists.txt 의존성 추가**
   - network_system이 logger_system, thread_system을 올바르게 링크해야 함
   - include-only "유령 의존성" 방지

2. **IThreadPool 인터페이스 추출** (필요한 경우)
   - common_system/include/kcenon/common/interfaces/thread_pool_interface.h
   - thread_system이 구현
   - network_system이 인터페이스 사용

3. **의존성 규칙 문서화**
   - ARCHITECTURE.md 업데이트
   - 의존성 검증 테스트 추가

### 장기 조치 (2-4주차)

1. **의존성 검증 CI 체크 생성**
   ```yaml
   - name: Validate Dependencies
     run: python3 tools/dependency_analyzer.py
   ```

2. **CMake 의존성 체커 추가**
   ```cmake
   if(DEFINED ALLOWED_DEPENDENCIES_${PROJECT_NAME})
     check_dependencies_match_whitelist()
   endif()
   ```

---

## 성공 지표

### Phase 4 목표

- ✅ **하드 순환 의존성 제로** (조건부 컴파일을 통해 이미 달성)
- ⚠️ **모든 include 의존성이 CMake와 일치** (network_system 동기화 필요)
- ✅ **기반(common)에 상향 하드 의존성 없음** (조건부만)
- ⏳ **자동화된 의존성 검증** (구현 예정)

### 현재 점수: 75% ✅

**세부사항**:
- Cycle 1: ✅ 완화됨 (조건부)
- Cycle 2: ✅ 자동 해결
- Cycle 3: ⚠️ 부분 해결 (검증 필요)
- CMake 동기화: ⏳ 보류 중 (network_system)

---

## 결론

### 주요 발견 사항

1. **Cycle 1 & 2**: ✅ **조건부 컴파일을 통해 이미 완화됨**
   - common_system이 우아한 `#ifdef` 설계 보유
   - 파괴적 변경 불필요

2. **Cycle 3**: ⚠️ **조사 필요**
   - service_container 위치 불명확
   - network → thread 의존성 제거 가능할 수 있음

3. **Include-only 의존성**: ⚠️ **아키텍처 문제**
   - network_system이 include하지만 링크하지 않음
   - CMakeLists.txt에서 명시적으로 만들어야 함

### 권장 다음 단계

1. **1일차 남은 작업**: Cycle 3 세부사항 조사
2. **2일차**: IThreadPool 인터페이스 설계 (필요한 경우)
3. **3일차**: 실제 include와 CMakeLists.txt 동기화

### 예상 작업량 조정

**초기**: 80시간 (4주)
**수정**: 40시간 (2주)

**이유**: Cycle 1 & 2가 이미 해결되었으며, Cycle 3와 CMake 정리만 남음.

---

**문서 상태**: 완료
**다음 업데이트**: service_container 조사 후
**담당자**: kcenon
