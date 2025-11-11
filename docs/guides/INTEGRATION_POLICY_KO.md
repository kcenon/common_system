# common_system 통합 정책

> **Language:** [English](INTEGRATION_POLICY.md) | **한국어**

## 개요

이 문서는 프로젝트의 모든 시스템에 대한 common_system의 공식 통합 정책을 정의합니다. 이 정책은 시스템 의존성의 일관성, 예측 가능성 및 적절한 문서화를 보장합니다.

## 통합 계층

### 계층 1: 핵심 인터페이스 시스템 (필수)

이 시스템들은 common_system을 **필수로** 요구하며 없이는 작동할 수 없습니다:

| 시스템 | 근거 |
|--------|-----------|
| **logger_system** | `ILogger` 인터페이스를 구현하고 오류 처리를 위해 `Result<T>` 사용 |
| **monitoring_system** | `IMonitor` 인터페이스를 구현하고 `Result<T>` 패턴 사용 |

**CMake 구성**:
```cmake
# 핵심 시스템 - common_system은 필수
find_package(common_system CONFIG QUIET)
if(NOT common_system_FOUND)
    # 경로 기반 검색으로 대체
    # ... 찾지 못하면 오류
endif()
```

### 계층 2: 인프라 시스템 (선택 사항, 기본값 ON)

이 시스템들은 common_system과 **선택적으로** 통합되지만 상당한 이점을 얻습니다:

| 시스템 | 기본값 | 근거 |
|--------|---------|-----------|
| **thread_system** | ON | 오류 처리를 위해 `Result<T>` 사용, 표준 인터페이스의 이점 |
| **container_system** | ON | 검증 및 오류 보고를 위해 `Result<T>` 사용 가능 |
| **database_system** | ON | 데이터베이스 작업 결과를 위해 `Result<T>` 사용 |
| **network_system** | ON | 로깅, 모니터링 및 실행을 위한 공통 인터페이스 사용 가능 |

**CMake 구성**:
```cmake
# 인프라 시스템 - common_system 통합은 선택 사항이지만 권장
option(BUILD_WITH_COMMON_SYSTEM "Enable common_system integration" ON)

if(BUILD_WITH_COMMON_SYSTEM)
    find_package(common_system CONFIG QUIET)
    if(NOT common_system_FOUND)
        message(WARNING "common_system not found, falling back to standalone mode")
        set(BUILD_WITH_COMMON_SYSTEM OFF)
    else()
        target_compile_definitions(${PROJECT_NAME} PUBLIC BUILD_WITH_COMMON_SYSTEM)
        target_link_libraries(${PROJECT_NAME} PUBLIC kcenon::common_system)
    endif()
endif()
```

### 계층 3: 독립 시스템

| 시스템 | 통합 상태 |
|--------|-------------------|
| **common_system** | 기반 - 다른 모든 시스템을 위한 인터페이스 제공 |

## 계층별 통합 기능

### 계층 1 이점 (핵심 시스템)

- **필수 인터페이스**: `ILogger`, `IMonitor`, `IExecutor`
- **타입 안전한 오류 처리**: `Result<T>`, `VoidResult`
- **표준화된 패턴**: 팩토리 패턴, 의존성 주입
- **시스템 간 호환성**: 모든 시스템과 보장된 호환성

### 계층 2 이점 (인프라 시스템)

`BUILD_WITH_COMMON_SYSTEM=ON`일 때:
- **향상된 오류 처리**: 작업을 위한 `Result<T>` 패턴
- **인터페이스 호환성**: 표준 `ILogger`, `IMonitor` 인터페이스 사용 가능
- **더 나은 진단**: 오류 코드와 함께 풍부한 오류 정보
- **미래 지향적**: 고급 통합 기능을 위한 준비

`BUILD_WITH_COMMON_SYSTEM=OFF`일 때:
- **독립 작동**: 전통적인 오류 처리 (bool, 예외)
- **최소 의존성**: 독립적으로 사용 가능
- **레거시 호환성**: 하위 호환성 유지

## 기본 구성 근거

### 인프라 시스템의 기본값이 ON인 이유?

1. **모범 사례 정렬**: 현대 C++ 프로젝트는 표준화된 오류 처리의 이점을 얻습니다
2. **통합 준비**: 대부분의 사용자는 시스템들이 원활하게 함께 작동하기를 원합니다
3. **향상된 기능**: `Result<T>`는 bool보다 더 나은 오류 진단을 제공합니다
4. **미래 호환성**: 고급 기능을 위한 코드베이스 준비

### 마이그레이션 경로

사용자는 필요시 여전히 선택 해제할 수 있습니다:
```bash
cmake -B build -S . -DBUILD_WITH_COMMON_SYSTEM=OFF
```

## 검증 체크리스트

- [ ] 핵심 시스템(계층 1)은 항상 common_system을 찾거나 우아하게 실패
- [ ] 인프라 시스템(계층 2)은 `BUILD_WITH_COMMON_SYSTEM=ON`을 기본값으로 설정
- [ ] 모든 시스템은 `ON`과 `OFF` 모두로 성공적으로 빌드 (계층 1 제외)
- [ ] 문서에 통합 요구사항이 명확하게 기술
- [ ] CMake 메시지가 사용자에게 통합 상태를 알림

## 통합 상태

| 시스템 | 계층 | 기본값 | 상태 | 마지막 검증 |
|--------|------|---------|--------|---------------|
| common_system | 3 | N/A | 기반 | 2025-10-03 |
| logger_system | 1 | 필수 | ✅ 구현됨 | 2025-10-03 |
| monitoring_system | 1 | 필수 | ✅ 구현됨 | 2025-10-03 |
| thread_system | 2 | ON | ✅ 구현됨 | 2025-10-03 |
| container_system | 2 | ON | ✅ 구현됨 | 2025-10-03 |
| database_system | 2 | ON | ✅ 구현됨 | 2025-10-03 |
| network_system | 2 | ON | ✅ 구현됨 | 2025-10-03 |

## 버전 히스토리

- **v1.0.0** (2025-10-03): 초기 정책 정의
  - 3계층 통합 모델 수립
  - 인프라 시스템의 기본값을 ON으로 표준화
  - network_system을 다른 인프라 시스템과 정렬

## 참조

- [INTEGRATION.md](./INTEGRATION.md) - 통합 가이드 및 예제
- [ARCHITECTURE.md](./ARCHITECTURE.md) - 시스템 아키텍처 개요
- [NEED_TO_FIX.md](./NEED_TO_FIX.md) - 구현 추적
