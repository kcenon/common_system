# C++20 모듈 마이그레이션 가이드

**common_system에서 기존 헤더 대신 C++20 모듈을 사용하기 위한 가이드**입니다.

> **Language:** [English](MODULE_MIGRATION.md) | **한국어**

## 개요

C++20 모듈은 헤더 전용 방식 대비 더 빠른 컴파일 시간과 향상된 캡슐화를 제공합니다. 이 가이드에서는 헤더에서 모듈로 마이그레이션하는 방법을 설명합니다.

## 사전 요구 사항

- **CMake**: 3.28 이상
- **빌드 시스템**: Ninja (모듈 지원에 필수)
- **컴파일러**: 다음 중 하나:
  - Clang 16+ (권장)
  - GCC 14+
  - MSVC 2022 17.4+

> **참고**: AppleClang은 아직 C++20 모듈을 완전히 지원하지 않습니다. macOS에서는 헤더 전용 모드를 사용하세요.

## 빠른 마이그레이션

### 이전 (헤더)

```cpp
#include <kcenon/common/patterns/result.h>
#include <kcenon/common/interfaces/executor_interface.h>
#include <kcenon/common/patterns/event_bus.h>

using namespace kcenon::common;

int main() {
    auto result = ok(42);
    // ...
}
```

### 이후 (모듈)

```cpp
import kcenon.common;

using namespace kcenon::common;

int main() {
    auto result = ok(42);
    // ...
}
```

## 모듈로 빌드하기

### CMake 설정

```bash
# 모듈 지원을 활성화하여 구성
cmake -G Ninja -B build \
    -DCOMMON_BUILD_MODULES=ON \
    -DCMAKE_CXX_COMPILER=clang++

# 빌드
cmake --build build
```

### 모듈 타겟에 링크

```cmake
# CMakeLists.txt에서
cmake_minimum_required(VERSION 3.28)
project(my_project CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# common_system 가져오기
include(FetchContent)
FetchContent_Declare(
    common_system
    GIT_REPOSITORY https://github.com/kcenon/common_system.git
    GIT_TAG main
)
FetchContent_MakeAvailable(common_system)

# 모듈 타겟에 링크 (모듈로 빌드할 때)
add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE kcenon::common_modules)
```

## 모듈 구조

`kcenon.common` 모듈은 계층별로 구성되어 있습니다:

```
kcenon.common
├── :utils       (Tier 1) - CircularBuffer, ObjectPool, source_location
├── :error       (Tier 1) - 에러 코드 및 카테고리
├── :result      (Tier 2) - Result<T> 패턴 구현
│   ├── :result.core      - 핵심 Result<T>, VoidResult, error_info 타입
│   └── :result.utilities - 헬퍼 함수 및 매크로
├── :concepts    (Tier 2) - 타입 검증을 위한 C++20 컨셉
├── :interfaces  (Tier 3) - 핵심 인터페이스 (집계자)
│   ├── :interfaces.logger   - ILogger, log_level, log_entry
│   ├── :interfaces.executor - IJob, IExecutor, IThreadPool
│   └── :interfaces.core     - logger와 executor 모두 re-export
├── :config      (Tier 3) - 설정 유틸리티
├── :di          (Tier 3) - 의존성 주입
├── :patterns    (Tier 4) - EventBus 구현
└── :logging     (Tier 4) - 로깅 유틸리티
```

### 특정 파티션 가져오기

전체 모듈 또는 특정 파티션을 가져올 수 있습니다:

```cpp
// 전체 가져오기
import kcenon.common;

// 또는 특정 파티션만 가져오기 (빠른 컴파일이 필요한 경우)
import kcenon.common:result;
import kcenon.common:interfaces;
```

## 헤더 vs 모듈 비교

| 항목 | 헤더 | 모듈 |
|------|------|------|
| 컴파일 | 각 TU에서 헤더 처리 | 한 번 컴파일, 가져오기 |
| 빌드 시간 | 느림 (반복적 파싱) | 빠름 (사전 컴파일) |
| 매크로 | TU 간 누출 가능 | 격리됨 |
| 심볼 가시성 | 모든 심볼 표시 | export 제어 |
| CMake 버전 | 3.16+ | 3.28+ |
| 컴파일러 지원 | 모든 현대 컴파일러 | Clang 16+, GCC 14+, MSVC 2022 |

## 하위 호환성

헤더 전용 인터페이스는 완전히 지원되며 기본값입니다. 다음을 수행할 수 있습니다:

1. 변경 없이 헤더 계속 사용
2. 프로젝트의 다른 부분에서 헤더와 모듈 혼합
3. 모듈 컴파일 실패 시 헤더로 대체

```cmake
# 기본값: 헤더 전용 (모듈 컴파일 없음)
cmake -B build

# 명시적: 모듈 활성화
cmake -B build -DCOMMON_BUILD_MODULES=ON
```

## 문제 해결

### "Module not found" 오류

올바른 CMake 생성기와 컴파일러를 사용하고 있는지 확인하세요:

```bash
cmake -G Ninja -DCMAKE_CXX_COMPILER=clang++ -B build
```

### "Unsupported compiler" 경고

모듈 지원에는 특정 컴파일러 버전이 필요합니다:

| 컴파일러 | 최소 버전 | 상태 |
|----------|-----------|------|
| Clang | 16.0 | 지원 |
| GCC | 14.0 | 지원 |
| MSVC | 17.4 (2022) | 지원 |
| AppleClang | - | 미지원 |

### 빌드 순서 문제

모듈은 사용하는 타겟보다 먼저 빌드되어야 합니다. CMake 3.28+는 `CXX_SCAN_FOR_MODULES`로 이를 자동 처리합니다.

## 마이그레이션 체크리스트

- [ ] 컴파일러 버전 확인 (Clang 16+, GCC 14+, MSVC 2022)
- [ ] CMake 3.28+로 업데이트
- [ ] Ninja 생성기 사용
- [ ] `-DCOMMON_BUILD_MODULES=ON` 활성화
- [ ] `#include`를 `import` 문으로 교체
- [ ] `kcenon::common` 대신 `kcenon::common_modules`에 링크
- [ ] 빌드 및 런타임 동작 테스트

## 관련 문서

- [아키텍처 개요](../ARCHITECTURE_KO.md)
- [프로젝트 구조](../PROJECT_STRUCTURE_KO.md)
- [빠른 시작 가이드](QUICK_START.md)
- [통합 가이드](INTEGRATION_KO.md)
