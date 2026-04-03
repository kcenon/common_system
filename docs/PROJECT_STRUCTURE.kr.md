---
doc_id: "COM-PROJ-008"
doc_title: "Common System - 프로젝트 구조"
doc_version: "1.0.0"
doc_date: "2026-04-04"
doc_status: "Released"
project: "common_system"
category: "PROJ"
---

# Common System - 프로젝트 구조

> **SSOT**: This document is the single source of truth for **Common System - 프로젝트 구조**.

**언어:** [English](PROJECT_STRUCTURE.md) | **한국어**

이 문서는 common_system 프로젝트 구조에 대한 포괄적인 개요를 제공하며, 각 디렉토리와 주요 파일의 목적을 설명합니다.

---

## 목차

- [저장소 개요](#저장소-개요)
- [디렉토리 구조](#디렉토리-구조)
- [헤더 구성](#헤더-구성)
- [빌드 시스템](#빌드-시스템)
- [테스팅 인프라](#테스팅-인프라)
- [문서화](#문서화)
- [통합](#통합)

---

## 저장소 개요

common_system은 지원 빌드 인프라, 테스트, 예제 및 포괄적인 문서와 함께 **헤더 전용 라이브러리**로 구성되어 있습니다.

### 주요 특성

- **헤더 전용**: 모든 핵심 기능이 `include/` 디렉토리에 위치
- **의존성 제로**: 핵심 기능에 외부 라이브러리 불필요
- **CMake 기반**: 현대적인 CMake 3.16+ 빌드 시스템
- **포괄적 테스팅**: 유닛 테스트, 통합 테스트, 벤치마크
- **잘 문서화됨**: Doxygen API 문서 및 사용자 가이드

---

## 디렉토리 구조

```
common_system/
├── .github/                    # GitHub Actions CI/CD 설정
│   └── workflows/
│       ├── ci.yml             # 메인 CI 파이프라인 (빌드, 테스트, 새니타이저)
│       ├── coverage.yml        # 코드 커버리지 리포팅
│       ├── static-analysis.yml # clang-tidy, cppcheck
│       └── build-Doxygen.yaml  # 문서 생성
│
├── cmake/                      # CMake 모듈 및 설정
│   ├── Modules/               # 커스텀 CMake find 모듈
│   ├── Config.cmake.in        # 패키지 설정 템플릿
│   ├── Dependencies.cmake      # 외부 의존성 관리
│   └── CompilerFlags.cmake     # 컴파일러별 플래그
│
├── docs/                       # 문서 (Markdown 및 Doxygen)
│   ├── guides/                # 사용자 가이드 및 튜토리얼
│   │   ├── BEST_PRACTICES.md
│   │   ├── ERROR_HANDLING.md
│   │   ├── INTEGRATION.md
│   │   ├── QUICK_START.md
│   │   ├── FAQ.md
│   │   ├── TROUBLESHOOTING.md
│   │   ├── RAII_GUIDELINES.md
│   │   ├── SMART_POINTER_GUIDELINES.md
│   │   └── ERROR_CODE_GUIDELINES.md
│   │
│   ├── advanced/              # 고급 주제
│   │   ├── DEPENDENCY_MATRIX.md
│   │   ├── MIGRATION.md
│   │   ├── IEXECUTOR_MIGRATION_GUIDE.md
│   │   ├── NAMESPACE_MIGRATION.md
│   │   └── STRUCTURE.md
│   │
│   ├── contributing/          # 기여 가이드라인
│   │   ├── CODE_STYLE.md
│   │   └── PR_TEMPLATE.md
│   │
│   ├── 01-ARCHITECTURE.md     # 시스템 아키텍처 개요
│   ├── CHANGELOG.md           # 버전 히스토리 및 변경 사항
│   ├── FEATURES.md            # 상세 기능 문서
│   ├── BENCHMARKS.md          # 성능 벤치마크
│   ├── PROJECT_STRUCTURE.md   # 이 파일
│   ├── README.md              # 문서 인덱스
│   └── mainpage.dox           # Doxygen 메인 페이지
│
├── include/                    # 공개 헤더 파일 (헤더 전용 라이브러리)
│   └── kcenon/
│       └── common/
│           ├── common.h        # 마스터 포함 파일
│           │
│           ├── interfaces/     # 추상 인터페이스
│           │   ├── executor_interface.h      # IExecutor 작업 실행
│           │   └── ...
│           │
│           ├── patterns/       # 디자인 패턴
│           │   ├── result.h                  # Result<T,E> 모나드
│           │   ├── event_bus.h               # 이벤트 발행/구독
│           │   └── ...
│           │
│           ├── error/          # 오류 처리
│           │   ├── error_codes.h             # 중앙화된 오류 코드
│           │   ├── error_info.h              # 오류 컨텍스트
│           │   └── exception_mapper.h        # 예외를 Result로 매핑
│           │
│           ├── config/         # 설정 및 기능 플래그
│           │   ├── build_config.h            # 빌드 시간 설정
│           │   ├── feature_flags.h           # 선택적 기능
│           │   └── abi_version.h             # ABI 호환성
│           │
│           ├── utils/          # 유틸리티 함수
│           │   ├── source_location.h         # C++20 source_location 지원
│           │   ├── type_traits.h             # 타입 유틸리티
│           │   └── ...
│           │
│           └── adapters/       # 어댑터 패턴
│               └── ...
│
├── src/                        # 소스 파일 (최소 - 헤더 전용)
│   └── config/                # 설정 구현
│       └── build_config.cpp   # 빌드 설정 (필요시)
│
├── tests/                      # 테스트 스위트
│   ├── unit/                  # 유닛 테스트
│   │   ├── exception_mapper_test.cpp
│   │   ├── result_test.cpp
│   │   ├── executor_test.cpp
│   │   └── CMakeLists.txt
│   │
│   └── CMakeLists.txt         # 테스트 빌드 설정
│
├── integration_tests/          # 통합 및 에코시스템 테스트
│   ├── scenarios/             # 통합 테스트 시나리오
│   │   ├── executor_integration_test.cpp
│   │   ├── event_bus_integration_test.cpp
│   │   └── cross_module_test.cpp
│   │
│   ├── performance/           # 성능/벤치마크 테스트
│   │   ├── result_benchmarks.cpp
│   │   ├── executor_benchmarks.cpp
│   │   └── event_bus_benchmarks.cpp
│   │
│   ├── failures/              # 컴파일 실패 테스트
│   │   └── ...
│   │
│   ├── framework/             # 테스트 프레임워크 유틸리티
│   │   └── test_helpers.h
│   │
│   └── CMakeLists.txt         # 통합 테스트 설정
│
├── examples/                   # 예제 코드 및 튜토리얼
│   ├── result_example.cpp     # Result<T> 패턴 예제
│   ├── executor_example.cpp   # IExecutor 사용 예제
│   ├── abi_version_example.cpp # ABI 버전 확인 예제
│   ├── unwrap_demo.cpp        # 오류 처리 패턴
│   └── CMakeLists.txt         # 예제 빌드 설정
│
├── scripts/                    # 빌드 및 자동화 스크립트
│   ├── build.sh               # Unix 빌드 스크립트
│   ├── build.bat              # Windows 빌드 스크립트
│   ├── test.sh                # Unix 테스트 실행기
│   ├── test.bat               # Windows 테스트 실행기
│   ├── clean.sh               # 빌드 아티팩트 정리
│   └── clean.bat              # Windows 정리 스크립트
│
├── CMakeLists.txt              # 루트 CMake 설정
├── LICENSE                     # BSD 3-Clause 라이선스
├── README.md                   # 메인 프로젝트 README
├── README.kr.md                # 한국어 README
├── CONTRIBUTING.md             # 기여 가이드라인
├── .gitignore                  # Git ignore 규칙
├── .clang-format              # 코드 포매팅 규칙
├── .clang-tidy                # 정적 분석 설정
└── Doxyfile                   # Doxygen 설정
```

---

## 헤더 구성

### 마스터 포함

**`include/kcenon/common/common.h`**

모든 공개 인터페이스를 포함하는 마스터 헤더 파일입니다. 사용자는 이 단일 헤더를 포함하여 모든 common_system 기능에 접근할 수 있습니다:

```cpp
#include <kcenon/common/common.h>

// 이제 접근 가능:
// - Result<T> 패턴
// - IExecutor 인터페이스
// - 이벤트 버스
// - 오류 코드
// - 모든 유틸리티
```

### 인터페이스 헤더

**`include/kcenon/common/interfaces/`**

에코시스템 통합을 위한 추상 인터페이스 정의:

| 헤더 | 목적 | 주요 타입 |
|--------|---------|-----------|
| `executor_interface.h` | 작업 실행 추상화 | `IExecutor` |
| `logger_interface.h` | 로깅 추상화 | `ILogger` (존재 시) |
| `monitor_interface.h` | 모니터링 추상화 | `IMonitor` (존재 시) |

**주요 설계 원칙:**
- 순수 가상 인터페이스 (추상 베이스 클래스)
- 구현 의존성 없음
- ABI 안정 설계
- 문서화된 계약

### 패턴 헤더

**`include/kcenon/common/patterns/`**

재사용 가능한 디자인 패턴 구현:

| 헤더 | 목적 | 주요 타입 |
|--------|---------|-----------|
| `result.h` | 오류 처리를 위한 Result 모나드 | `Result<T>`, `Ok<T>`, `Error` |
| `event_bus.h` | 이벤트 발행/구독 | `EventBus`, `Event` 타입 |
| `observer.h` | 옵저버 패턴 | `Observable<T>`, `Observer<T>` |
| `command.h` | 커맨드 패턴 | `Command`, `CommandQueue` |

**사용법:**
```cpp
#include <kcenon/common/patterns/result.h>

using namespace kcenon::common;

Result<int> parse_number(const std::string& str) {
    try {
        int value = std::stoi(str);
        return ok(value);
    } catch (const std::exception& e) {
        return make_error<int>(
            error_codes::INVALID_ARGUMENT,
            e.what(),
            "parse_number"
        );
    }
}
```

### 오류 처리 헤더

**`include/kcenon/common/error/`**

포괄적인 오류 처리 인프라:

| 헤더 | 목적 | 내용 |
|--------|---------|----------|
| `error_codes.h` | 중앙화된 오류 코드 레지스트리 | 에코시스템 전체의 모든 오류 코드 |
| `error_info.h` | 오류 컨텍스트 및 메타데이터 | `ErrorInfo` 구조체 |
| `exception_mapper.h` | 예외를 Result로 변환 | `try_catch()`, `map_exception()` |

**오류 코드 레지스트리:**

오류 코드 레지스트리는 컴파일 타임에 검증되는 오류 코드 범위를 제공합니다:

```cpp
namespace kcenon::common::error_codes {
    // Common 시스템 오류 (-1 ~ -99)
    constexpr int SUCCESS = 0;
    constexpr int NOT_FOUND = -1;
    constexpr int INVALID_ARGUMENT = -2;
    constexpr int INITIALIZATION_FAILED = -3;
    // ... 더 많은 오류 코드

    // 다른 시스템의 오류 코드 범위
    namespace ranges {
        constexpr int COMMON_MIN = -1;
        constexpr int COMMON_MAX = -99;
        constexpr int THREAD_MIN = -100;
        constexpr int THREAD_MAX = -199;
        // ... 더 많은 범위
    }
}
```

### 설정 헤더

**`include/kcenon/common/config/`**

빌드 시간 설정 및 기능 플래그:

| 헤더 | 목적 | 주요 정의 |
|--------|---------|-----------------|
| `build_config.h` | 빌드 설정 | 기능 플래그, 플랫폼 감지 |
| `feature_flags.h` | 선택적 기능 | `COMMON_ENABLE_COROUTINES` 등 |
| `abi_version.h` | ABI 호환성 | 버전 상수, 호환성 검사 |

**기능 감지:**

```cpp
#include <kcenon/common/config/build_config.h>

#if COMMON_HAS_CPP20
    // C++20 기능 사용
    #include <source_location>
#else
    // C++17 폴백
    #include <kcenon/common/utils/source_location.h>
#endif
```

### 유틸리티 헤더

**`include/kcenon/common/utils/`**

유틸리티 함수 및 헬퍼 클래스:

| 헤더 | 목적 | 내용 |
|--------|---------|----------|
| `source_location.h` | 소스 위치 지원 | C++17/C++20 호환성 |
| `type_traits.h` | 타입 유틸리티 | 템플릿 메타프로그래밍 헬퍼 |
| `string_utils.h` | 문자열 유틸리티 | 문자열 조작 헬퍼 |
| `scope_guard.h` | RAII 스코프 가드 | 자동 정리 유틸리티 |

---

## 빌드 시스템

### 루트 CMakeLists.txt

**주요 설정 옵션:**

```cmake
# 빌드 옵션
option(BUILD_TESTING "테스트 빌드" ON)
option(BUILD_EXAMPLES "예제 빌드" ON)
option(BUILD_BENCHMARKS "벤치마크 빌드" OFF)
option(BUILD_DOCS "문서 빌드" OFF)

# 통합 옵션
option(BUILD_WITH_THREAD_SYSTEM "thread_system 통합 활성화" OFF)
option(BUILD_WITH_LOGGER_SYSTEM "logger_system 통합 활성화" OFF)
option(BUILD_WITH_MONITORING_SYSTEM "monitoring_system 통합 활성화" OFF)

# 기능 플래그
option(COMMON_ENABLE_CPP20 "C++20 기능 활성화" OFF)
option(COMMON_ENABLE_COROUTINES "코루틴 지원 활성화" OFF)
```

### CMake 모듈

**`cmake/` 디렉토리:**

| 파일 | 목적 |
|------|---------|
| `Config.cmake.in` | `find_package()`를 위한 패키지 설정 템플릿 |
| `Dependencies.cmake` | 외부 의존성 관리 |
| `CompilerFlags.cmake` | 컴파일러별 플래그 및 최적화 |
| `Testing.cmake` | 테스트 설정 및 발견 |
| `Sanitizers.cmake` | Address/Thread/UB 새니타이저 설정 |

### 빌드 타겟

**사용 가능한 타겟:**

```bash
# 메인 라이브러리 타겟 (인터페이스 라이브러리)
kcenon::common

# 테스트 타겟
common_tests           # 유닛 테스트
common_integration     # 통합 테스트
common_benchmarks      # 성능 벤치마크

# 예제 타겟
result_example
executor_example
abi_version_example

# 문서 타겟
doxygen               # API 문서 생성
```

---

## 테스팅 인프라

### 유닛 테스트

**위치:** `tests/unit/`

유닛 테스트는 Google Test 프레임워크를 사용하며 다음을 커버합니다:
- Result<T> 패턴 연산 (성공, 오류, 모나딕 합성)
- 오류 코드 레지스트리 검증
- 예외 매퍼 기능
- 타입 트레이트 유틸리티

**유닛 테스트 실행:**

```bash
./scripts/test.sh
# 또는
ctest --test-dir build
```

### 통합 테스트

**위치:** `integration_tests/`

통합 테스트는 다음을 검증합니다:
- 크로스 모듈 통합 패턴
- thread_system의 IExecutor 구현
- monitoring_system과 이벤트 버스 통합
- 전체 에코시스템 호환성

**구성:**

```
integration_tests/
├── scenarios/        # 실제 통합 시나리오
├── performance/      # 성능 및 벤치마크 테스트
├── failures/         # 컴파일 실패 테스트 (네거티브 테스트)
└── framework/        # 테스트 유틸리티 및 헬퍼
```

### 새니타이저 테스팅

**CI에서 자동화된 새니타이저 빌드:**

- **AddressSanitizer (ASan)**: 메모리 오류 탐지
- **ThreadSanitizer (TSan)**: 데이터 레이스 탐지
- **UndefinedBehaviorSanitizer (UBSan)**: 정의되지 않은 동작 감지

**로컬 실행:**

```bash
# 새니타이저로 빌드
./scripts/build.sh --sanitize

# 새니타이저로 테스트 실행
./scripts/test.sh --sanitize
```

### 커버리지 리포팅

**코드 커버리지 추적:**

```bash
# 커버리지로 빌드
./scripts/build.sh --coverage

# 테스트 실행 및 리포트 생성
./scripts/test.sh --coverage

# HTML 리포트 보기
open build/coverage/index.html
```

---

## 문서화

### 사용자 문서

**위치:** `docs/`

문서는 대상 및 주제별로 구성되어 있습니다:

**시작하기:**
- `docs/guides/QUICK_START.md` - 빠른 시작 가이드
- `docs/guides/INTEGRATION.md` - 통합 가이드
- `docs/01-ARCHITECTURE.md` - 아키텍처 개요

**사용자 가이드:**
- `docs/guides/ERROR_HANDLING.md` - 오류 처리 패턴
- `docs/guides/BEST_PRACTICES.md` - 모범 사례
- `docs/guides/TROUBLESHOOTING.md` - 일반적인 문제 및 해결책
- `docs/guides/FAQ.md` - 자주 묻는 질문

**고급 주제:**
- `docs/advanced/MIGRATION.md` - 이전 버전에서 마이그레이션
- `docs/advanced/DEPENDENCY_MATRIX.md` - 의존성 분석
- `docs/advanced/STRUCTURE.md` - 상세 구조 문서

**레퍼런스:**
- `docs/FEATURES.md` - 완전한 기능 문서
- `docs/BENCHMARKS.md` - 성능 벤치마크
- `docs/PROJECT_STRUCTURE.md` - 이 문서

### API 문서

**Doxygen 생성 API 문서:**

모든 공개 헤더에는 포괄적인 Doxygen 주석이 포함되어 있습니다:

```cpp
/**
 * @brief 비동기 실행을 위한 작업 제출
 *
 * @tparam F 호출 가능 타입 (람다, 함수 객체, 함수 포인터)
 * @tparam Args 호출 가능 객체의 인수 타입
 * @param func 실행할 호출 가능 객체
 * @param args 호출 가능 객체에 전달할 인수
 * @return 호출 가능 객체의 결과가 담긴 std::future
 *
 * @note 이 메서드는 스레드 안전하며 동시에 호출할 수 있습니다
 * @throws std::bad_alloc 작업 큐 할당 실패 시
 *
 * @example
 * @code
 * auto future = executor->submit([]() { return 42; });
 * int result = future.get();
 * @endcode
 */
template<typename F, typename... Args>
auto submit(F&& func, Args&&... args) -> std::future</*...*/>;
```

**API 문서 생성:**

```bash
./scripts/build.sh --docs
# 또는
doxygen Doxyfile
```

---

## 통합

### 에코시스템 통합

common_system은 KCENON 에코시스템의 다른 모든 시스템과 통합됩니다:

**통합 패턴:**

1. common_system에서 **인터페이스 정의** (예: `IExecutor`)
2. 전문화된 시스템에서 **인터페이스 구현** (예: `thread_system::thread_pool`)
3. 어댑터 패턴을 통해 **구현을 공통 인터페이스에 적응**
4. 느슨한 결합을 위해 클라이언트 코드에서 **공통 인터페이스 사용**

**통합 예시:**

```cpp
// 1. Common 시스템이 인터페이스 정의
namespace kcenon::common::interfaces {
    class IExecutor { /* ... */ };
}

// 2. Thread 시스템이 executor 구현
namespace kcenon::thread {
    class thread_pool { /* ... */ };
}

// 3. 어댑터가 thread_pool을 IExecutor로 변환
namespace kcenon::thread::adapters {
    std::shared_ptr<common::interfaces::IExecutor>
    make_common_executor(std::shared_ptr<thread_pool> pool);
}

// 4. 클라이언트가 공통 인터페이스 사용
void client_code(std::shared_ptr<common::interfaces::IExecutor> executor) {
    executor->submit([]() { /* 작업 */ });
}
```

### CMake 통합

**서브디렉토리로:**

```cmake
add_subdirectory(common_system)
target_link_libraries(my_app PRIVATE kcenon::common)
```

**FetchContent 사용:**

```cmake
include(FetchContent)
FetchContent_Declare(
    common_system
    GIT_REPOSITORY https://github.com/kcenon/common_system.git
    GIT_TAG v0.2.0  # Pin to a specific release tag; do NOT use main
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(common_system)
target_link_libraries(my_app PRIVATE kcenon::common)
```

**설치된 패키지로:**

```cmake
find_package(common_system REQUIRED)
target_link_libraries(my_app PRIVATE kcenon::common)
```

---

## 파일 명명 규칙

### 헤더

- **인터페이스 헤더**: `*_interface.h` (예: `executor_interface.h`)
- **패턴 헤더**: `*.h` (예: `result.h`, `event_bus.h`)
- **유틸리티 헤더**: `*_utils.h` (예: `string_utils.h`)
- **설정 헤더**: `*_config.h` (예: `build_config.h`)

### 테스트

- **유닛 테스트**: `*_test.cpp` (예: `result_test.cpp`)
- **통합 테스트**: `*_integration_test.cpp`
- **벤치마크 테스트**: `*_benchmarks.cpp`

### 예제

- **예제 파일**: `*_example.cpp` (예: `executor_example.cpp`)
- **데모 파일**: `*_demo.cpp` (예: `unwrap_demo.cpp`)

---

## 빌드 아티팩트

### 빌드 디렉토리 구조

빌드 후 `build/` 디렉토리에는 다음이 포함됩니다:

```
build/
├── include/                  # 생성/설정된 헤더
├── lib/                      # 라이브러리 (있는 경우)
├── bin/                      # 실행 파일 (테스트, 예제)
│   ├── common_tests
│   ├── result_example
│   └── executor_example
├── tests/                    # 테스트 결과
│   └── test-results.xml
├── coverage/                 # 커버리지 리포트
│   └── index.html
├── doxygen/                  # API 문서
│   └── html/
│       └── index.html
└── CMakeFiles/              # CMake 내부 파일
```

### 설치된 파일

`cmake --install` 후 다음 파일이 설치됩니다:

```
<install-prefix>/
├── include/
│   └── kcenon/
│       └── common/
│           └── [모든 헤더]
├── lib/
│   └── cmake/
│       └── common_system/
│           ├── common_systemConfig.cmake
│           └── common_systemTargets.cmake
└── share/
    └── doc/
        └── common_system/
            └── [문서]
```

---

## 개발 워크플로우

### 새 인터페이스 추가

1. `include/kcenon/common/interfaces/`에 헤더 생성
2. Doxygen 문서 추가
3. `include/kcenon/common/common.h`를 업데이트하여 새 헤더 포함
4. `tests/unit/`에 유닛 테스트 추가
5. `integration_tests/scenarios/`에 통합 테스트 추가
6. `examples/`에 예제 추가
7. `docs/guides/`에 문서화

### 새 패턴 추가

1. `include/kcenon/common/patterns/`에 헤더 생성
2. 템플릿 메서드를 인라인으로 구현 (헤더 전용)
3. 포괄적인 Doxygen 주석 추가
4. 유닛 테스트 생성
5. 사용 예제 추가
6. 문서 업데이트

---

**최종 업데이트**: 2025-11-28
**버전**: 0.1.0

---

Made with ❤️ by 🍀☀🌕🌥 🌊
