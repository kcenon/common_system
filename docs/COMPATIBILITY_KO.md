# KCENON 에코시스템 호환성 매트릭스

이 문서는 모든 KCENON 에코시스템 구성 요소 간의 버전 호환성 및 의존성 요구 사항을 정의합니다.

> **Language:** [English](COMPATIBILITY.md) | **한국어**

---

## 개요

KCENON 에코시스템은 공통 패턴과 인터페이스를 공유하는 상호 연결된 C++20 라이브러리로 구성됩니다. 이러한 시스템 간의 호환성을 보장하는 것은 안정적인 통합을 위해 매우 중요합니다.

### 버전 관리 표준

모든 KCENON 시스템은 [Semantic Versioning 2.0.0](https://semver.org/spec/v2.0.0.html)을 따릅니다:

| 버전 구성 요소 | 의미 |
|---------------|------|
| **MAJOR** (X.0.0) | 호환되지 않는 API 변경 - 마이그레이션 필요 |
| **MINOR** (0.X.0) | 하위 호환되는 새로운 기능 |
| **PATCH** (0.0.X) | 하위 호환되는 버그 수정 |
| **BUILD** (0.0.0.X) | 빌드 메타데이터 (선택사항) |

---

## 현재 버전 매트릭스

| 시스템 | 현재 버전 | 최소 common_system | 상태 |
|--------|----------|-------------------|------|
| **common_system** | 0.2.0 | - | 안정 |
| **thread_system** | 0.1.0 | 0.2.0 | 안정 |
| **logger_system** | 0.1.0 | 0.2.0 | 안정 |
| **monitoring_system** | 0.1.0 | 0.2.0 | 안정 |
| **container_system** | 0.1.0 | 0.2.0 | 안정 |
| **network_system** | 0.1.0 | 0.2.0 | 안정 |
| **database_system** | 0.1.0 | 0.2.0 | 안정 |

---

## 의존성 그래프

```
                    ┌─────────────────┐
                    │  common_system  │
                    │     v0.2.0      │
                    └────────┬────────┘
                             │
         ┌───────────────────┼───────────────────┐
         │                   │                   │
         ▼                   ▼                   ▼
┌─────────────────┐ ┌─────────────────┐ ┌─────────────────┐
│  thread_system  │ │  logger_system  │ │container_system │
│     v0.1.0      │ │     v0.1.0      │ │     v0.1.0      │
└────────┬────────┘ └────────┬────────┘ └─────────────────┘
         │                   │
         ▼                   ▼
┌─────────────────┐ ┌─────────────────┐
│monitoring_system│ │ network_system  │
│     v0.1.0      │ │     v0.1.0      │
└─────────────────┘ └────────┬────────┘
                             │
                             ▼
                    ┌─────────────────┐
                    │ database_system │
                    │     v0.1.0      │
                    └─────────────────┘
```

---

## 상세 의존성

### common_system (기반 레이어)

외부 KCENON 의존성이 없는 기반 라이브러리입니다. 모든 다른 시스템이 이에 의존합니다.

**제공 기능:**
- `Result<T>` 오류 처리 패턴
- 오류 코드 레지스트리 및 정의
- 핵심 인터페이스: `IExecutor`, `ILogger`, `IMonitor`, `IDatabase`
- 분리된 통신을 위한 이벤트 버스 패턴
- 스마트 어댑터 및 팩토리 패턴
- 타입 검증을 위한 C++20 개념(concepts)

**외부 의존성:**
- C++20 컴파일러 (GCC 10+, Clang 12+, MSVC 19.29+)
- CMake 3.16+
- (선택) yaml-cpp - YAML 설정 지원
- (선택) GTest - 테스트용

---

### thread_system

**필수:**
| 의존성 | 최소 버전 | 용도 |
|--------|----------|------|
| common_system | 0.2.0 | 핵심 인터페이스 (IExecutor, Result<T>) |

**선택:**
| 의존성 | 최소 버전 | 용도 |
|--------|----------|------|
| logger_system | 0.1.0 | 로깅 통합 |
| monitoring_system | 0.1.0 | 성능 모니터링 |

---

### logger_system

**필수:**
| 의존성 | 최소 버전 | 용도 |
|--------|----------|------|
| common_system | 0.2.0 | ILogger 인터페이스, GlobalLoggerRegistry |

**외부:**
| 의존성 | 버전 | 용도 |
|--------|------|------|
| spdlog | 1.12.0+ | 고성능 로깅 백엔드 |
| fmt | 10.0.0+ | 문자열 포맷팅 |

---

### monitoring_system

**필수:**
| 의존성 | 최소 버전 | 용도 |
|--------|----------|------|
| common_system | 0.2.0 | IMonitor 인터페이스, Result<T> |
| thread_system | 0.1.0 | 백그라운드 작업 실행 |
| logger_system | 0.1.0 | 메트릭 및 이벤트 로깅 |

---

### container_system

**필수:**
| 의존성 | 최소 버전 | 용도 |
|--------|----------|------|
| common_system | 0.2.0 | Result<T>, 값 타입 |

**선택:**
| 의존성 | 최소 버전 | 용도 |
|--------|----------|------|
| thread_system | 0.1.0 | 스레드 안전 컨테이너 |

---

### network_system

**필수:**
| 의존성 | 최소 버전 | 용도 |
|--------|----------|------|
| common_system | 0.2.0 | 핵심 인터페이스, Result<T> |
| thread_system | 0.1.0 | 비동기 I/O, 연결 풀 |
| logger_system | 0.1.0 | 네트워크 이벤트 로깅 |
| container_system | 0.1.0 | 메시지 컨테이너 |

**외부:**
| 의존성 | 버전 | 용도 |
|--------|------|------|
| Boost.Asio | 1.80.0+ | 비동기 네트워킹 |

---

### database_system

**필수:**
| 의존성 | 최소 버전 | 용도 |
|--------|----------|------|
| common_system | 0.2.0 | IDatabase 인터페이스, Result<T> |
| thread_system | 0.1.0 | 쿼리 실행 |
| logger_system | 0.1.0 | 쿼리 로깅 |
| network_system | 0.1.0 | 원격 데이터베이스 연결 |
| container_system | 0.1.0 | 레코드 컨테이너 |

**외부:**
| 의존성 | 버전 | 용도 |
|--------|------|------|
| SQLite | 3.35.0+ | 임베디드 데이터베이스 |
| (선택) PostgreSQL client | 14.0+ | PostgreSQL 지원 |

---

## 알려진 비호환성

### 주요 변경 이력

| 버전 조합 | 문제 | 해결 방법 |
|----------|------|----------|
| common_system < 2.0.0 | 레거시 `Result<T>` 팩토리 함수 | common_system 2.0.0+로 업그레이드 |
| thread_system with common_system < 0.2.0 | GlobalLoggerRegistry 누락 | common_system을 0.2.0+로 업그레이드 |

---

## CMake 버전 검증

빌드 시 호환성을 보장하려면 CMakeLists.txt에 버전 검사를 추가하세요:

```cmake
# 최소 버전 요구사항으로 common_system 찾기
find_package(common_system 0.2.0 REQUIRED)

# 선택사항: 프로그래밍 방식으로 버전 확인
if(common_system_VERSION VERSION_LESS "0.2.0")
    message(FATAL_ERROR "common_system >= 0.2.0이 필요합니다. 발견된 버전: ${common_system_VERSION}")
endif()

# 버전 요구사항이 있는 여러 의존성
find_package(thread_system 0.1.0 REQUIRED)
find_package(logger_system 0.1.0 REQUIRED)

# 버전 호환성 검사 예제
if(thread_system_VERSION VERSION_LESS "0.1.0")
    message(FATAL_ERROR "모니터링 기능을 위해 thread_system >= 0.1.0이 필요합니다")
endif()
```

---

## 업그레이드 가이드라인

### 안전한 업그레이드 순서

여러 시스템을 업그레이드할 때 호환성 문제를 피하려면 다음 순서를 따르세요:

1. **common_system** (항상 먼저 - 기반 레이어)
2. **thread_system** (실행 프리미티브)
3. **logger_system** (로깅 인프라)
4. **container_system** (데이터 구조)
5. **monitoring_system** (thread + logger에 의존)
6. **network_system** (thread + logger + container에 의존)
7. **database_system** (위의 모든 것에 의존)

### 업그레이드 전 체크리스트

- [ ] 주요 변경 사항에 대해 CHANGELOG.md 검토
- [ ] 버전 요구 사항에 대해 이 호환성 매트릭스 확인
- [ ] 격리된 환경에서 새 버전으로 기존 테스트 실행
- [ ] CMakeLists.txt 버전 요구 사항 업데이트
- [ ] 시스템 간 통합 지점 테스트

---

## 런타임 호환성 검증

시스템은 버전 API를 사용하여 런타임에 호환성을 확인할 수 있습니다:

```cpp
#include <kcenon/common/config/abi_version.h>

void verify_compatibility() {
    auto version = kcenon::common::get_version();

    // 시맨틱 버전 확인
    if (version.major < 2) {
        throw std::runtime_error("common_system 2.0.0 이상이 필요합니다");
    }

    // 바이너리 호환성을 위한 ABI 버전 확인
    if (kcenon::common::get_abi_version() != expected_abi) {
        throw std::runtime_error("ABI 불일치 - 재컴파일이 필요합니다");
    }
}
```

---

## 호환성 문제 보고

호환성 문제가 발생하면:

1. 알려진 비호환성에 대해 **이 매트릭스 확인**
2. 영향 받는 시스템의 **CHANGELOG.md 검토**
3. 다음 정보와 함께 [GitHub Issues](https://github.com/kcenon/common_system/issues)에 **이슈 생성**:
   - 관련된 모든 시스템의 버전
   - CMake 설정 출력
   - 오류 메시지 또는 예상치 못한 동작
   - 최소 재현 케이스

---

## 버전 이력

| 날짜 | 변경 사항 |
|------|----------|
| 2025-12-16 | 초기 호환성 매트릭스 생성 |

---

## 라이선스

이 문서는 KCENON 에코시스템의 일부이며 BSD 3-Clause 라이선스에 따라 라이선스가 부여됩니다.
