# Common System

**Status: active** | **최신 공개 릴리스:** [v0.2.0](https://github.com/kcenon/common_system/releases/tag/v0.2.0)

[VERSION](VERSION)에 기록된 `1.0.0`은 준비 중인 버전이며 아직 태그나 릴리스로 공개되지 않았습니다.
[변경 이력](CHANGELOG.md)과 [버전 관리 정책](VERSIONING.md)을 참고하세요.

**언어:** [English](README.md) | **한국어**

[![CI](https://github.com/kcenon/common_system/actions/workflows/ci.yml/badge.svg)](https://github.com/kcenon/common_system/actions/workflows/ci.yml)
[![Coverage](https://github.com/kcenon/common_system/actions/workflows/coverage.yml/badge.svg)](https://github.com/kcenon/common_system/actions/workflows/coverage.yml)
[![Static Analysis](https://github.com/kcenon/common_system/actions/workflows/static-analysis.yml/badge.svg)](https://github.com/kcenon/common_system/actions/workflows/static-analysis.yml)
[![Documentation](https://github.com/kcenon/common_system/actions/workflows/build-Doxygen.yaml/badge.svg)](https://github.com/kcenon/common_system/actions/workflows/build-Doxygen.yaml)

## 목차

- [개요](#개요)
- [아키텍처](#아키텍처)
- [시작하기](#시작하기)
- [API 레퍼런스](#api-레퍼런스)
- [기능](#기능)
- [설정](#설정)
- [통합](#통합)
- [성능](#성능)
- [테스트](#테스트)
- [문제 해결](#문제-해결)
- [기여](#기여)
- [변경 이력](#변경-이력)
- [라이선스](#라이선스)

## 개요

Common System은 Result 기반 오류 처리, 작업 및 로깅 인터페이스, 의존성 주입,
이벤트 전달, 회복 패턴을 제공하는 C++20 헤더 라이브러리입니다.
kcenon 시스템 라이브러리들이 사용하는 공통 인터페이스를 정의합니다.

기본 모드는 헤더 방식입니다. 선택적으로 사용하는 명명된 모듈은 실험적 기능이며,
모듈 의존성 스캔을 지원하는 컴파일러가 필요합니다.

[문서 색인](docs/README.kr.md)에서 가이드와 설계 문서를 찾을 수 있습니다.
[생성된 API 문서](https://kcenon.github.io/common_system/)는 공개 헤더를 설명합니다.

## 아키텍처

애플리케이션은 `IExecutor`, `ILogger`와 같은 인터페이스의 구현을 제공합니다.
`Result<T>`는 성공 값 또는 오류 정보를 전달하며, 서비스 컨테이너는
애플리케이션 의존성을 등록하고 조회합니다.

`simple_event_bus`는 뮤텍스를 잡은 상태에서 핸들러 목록을 복사한 뒤,
뮤텍스를 해제하고 핸들러를 동기적으로 호출합니다. 핸들러는 게시 스레드에서 실행됩니다.
[이벤트 버스 구현](include/kcenon/common/patterns/event_bus.h)을 참고하세요.

[아키텍처 가이드](docs/ARCHITECTURE.kr.md)에 계층과 의존성 다이어그램이 있습니다.
[레이아웃 표준](docs/kcenon-system-layout.md)은 생태계가 공유하는 디렉터리,
빌드 및 테스트 규칙을 설명합니다.

## 시작하기

### 요구사항

- 헤더 사용: C++20, GCC 11+, Clang 14+, MSVC 2022 (19.30+), Apple Clang 14+.
- 현재 소스의 헤더 빌드는 CMake 3.20+, v0.2.0과 모듈 빌드는 CMake 3.28+ 필요.
- 명명된 모듈은 지원 컴파일러와 스캐너가 필요하며 Apple Clang은 지원하지 않습니다.

여러 시스템을 조합할 때는 [컴파일러 및 의존성 표](docs/guides/QUICK_START.md#requirements)를
확인하고 의존성 중 가장 높은 컴파일러 요구사항을 적용하세요.

### 공개 릴리스 설치

애플리케이션 디렉터리에 `CMakeLists.txt`와 `main.cpp`를 만드세요.
아래 FetchContent 예제는 공개된 릴리스를 고정하고 해당 버전의 `kcenon::common`
타깃을 사용합니다. 의존 라이브러리의 테스트, 예제 및 벤치마크 빌드는 끕니다.

```cmake
cmake_minimum_required(VERSION 3.28)
project(common_example LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
set(COMMON_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(COMMON_BUILD_INTEGRATION_TESTS OFF CACHE BOOL "" FORCE)
set(COMMON_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(COMMON_BUILD_BENCHMARKS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    common_system
    GIT_REPOSITORY https://github.com/kcenon/common_system.git
    GIT_TAG v0.2.0
)
FetchContent_MakeAvailable(common_system)

add_executable(common_example main.cpp)
target_link_libraries(common_example PRIVATE kcenon::common)
```

### 첫 프로그램

다음을 `main.cpp`로 저장하세요. 성공 값을 읽기 전에 결과 상태를 확인합니다.

```cpp
#include <kcenon/common/patterns/result.h>

using namespace kcenon::common;

Result<int> validate_port(int port) {
    if (port < 1 || port > 65535) {
        return make_error<int>(error_codes::INVALID_ARGUMENT,
                               "Port must be between 1 and 65535");
    }
    return ok(port);
}

int main() {
    auto result = validate_port(8080);
    return result.is_ok() && result.value() == 8080 ? 0 : 1;
}
```

애플리케이션을 구성하고 빌드한 후 실행합니다.

```bash
cmake -S . -B build
cmake --build build --config Release
./build/common_example
```

다중 구성 생성기를 사용한다면 Windows에서는 `build/Release/common_example.exe`,
Unix에서는 `build/Release/common_example`을 실행하세요.

[vcpkg 오버레이](docs/guides/QUICK_START.md#vcpkg-overlay),
[헤더 직접 사용](docs/guides/QUICK_START.md#direct-headers-and-source-targets),
[명명된 모듈](docs/guides/CXX20_MODULES.md)은 각 설정 가이드를 따르세요.

## API 레퍼런스

아래 헤더 경로의 기준은 `kcenon/common/`입니다.

| 구성 요소 | 용도 | 헤더 |
| --- | --- | --- |
| `Result<T>` / `VoidResult` | 성공 값 또는 오류 값 | `patterns/result.h` |
| `IExecutor` / `IJob` | 작업 제출 및 완료 | `interfaces/executor_interface.h` |
| `ILogger` | 로깅 인터페이스 | `interfaces/logger_interface.h` |
| `service_container` | 의존성 등록 및 조회 | `di/service_container.h` |
| `simple_event_bus` | 동기 이벤트 전달 | `patterns/event_bus.h` |
| `circuit_breaker` | 실패 추적 및 복구 | `resilience/circuit_breaker.h` |
| `config_loader` | 설정 로딩 | `config/config_loader.h` |

[API 레퍼런스](docs/API_REFERENCE.kr.md)와
[설정 및 API 예제](docs/guides/QUICK_START.md#api-examples)에서 작업 제출,
상태 점검, 서킷 브레이커, 오류 처리 방법을 확인할 수 있습니다.

## 기능

- `and_then`, `map`, `or_else`로 `Result<T>` 연산을 조합합니다.
- 싱글턴, 일시적 또는 범위 기반 수명으로 의존성을 등록합니다.
- 타입이 지정된 이벤트에 핸들러를 구독하고 동기적으로 게시합니다.
- 설정과 가드 API를 사용하여 요청 사이의 서킷 브레이커 상태를 추적합니다.
- 설정 로더, 변경 감시 및 명령줄 파서를 사용합니다.
- 원형 버퍼, 객체 풀 및 C++20 concepts를 활용합니다.

[Result](examples/result_example.cpp), [실행기](examples/executor_example.cpp),
[다중 시스템](examples/multi_system_app/) 예제가 있습니다.
[예제 빌드 안내](docs/guides/QUICK_START.md#running-repository-examples)는
소스 체크아웃에서 예제를 실행하는 방법을 설명합니다.

## 설정

[CMake 옵션](CMakeLists.txt)은 헤더 모드, 예제, 통합 테스트 및 선택적 YAML 지원을
제어합니다. [모듈 가이드](docs/guides/CXX20_MODULES.md)는 별도 모듈 타깃과
도구 체인 검사를 설명합니다.

애플리케이션 설정은 [설정 가이드](docs/CONFIG_GUIDE.md),
[로더](docs/CONFIG_LOADER.md), [변경 감시](docs/CONFIG_WATCHER.md),
[CLI 파서](docs/CONFIG_CLI_PARSER.md) 문서를 참고하세요.

## 통합

[생태계 개요](docs/ECOSYSTEM_OVERVIEW.md)는 하위 소비 시스템의 관계를 보여 줍니다.
호환되는 포트 버전을 선택할 때 [버전 기준선](docs/ECOSYSTEM_OVERVIEW.md#versions)을
사용하고 의존성 중 가장 높은 컴파일러 요구사항을 적용하세요.

[생태계 vcpkg 워크플로](https://github.com/kcenon/common_system/actions/workflows/ecosystem-vcpkg-integration.yml)는
Ubuntu와 macOS에서 소비자 포트를 빌드합니다.
[워크플로 정의](.github/workflows/ecosystem-vcpkg-integration.yml)에 PR 경로 필터,
수요일 03:43 UTC 일정 및 수동 실행 설정이 기록되어 있습니다.

## 성능

[벤치마크 근거 및 측정 방법](docs/BENCHMARKS.kr.md)에서 기록된 작업 부하,
측정 환경, 명령, 원본 결과 및 해석의 한계를 확인하세요.

## 테스트

[CI](https://github.com/kcenon/common_system/actions/workflows/ci.yml)는 Linux, macOS,
Windows에서 빌드합니다. Ubuntu sanitizer 행렬은 address, thread, undefined
sanitizer로 테스트를 실행합니다. [워크플로 정의](.github/workflows/ci.yml)를 참고하세요.

커버리지 보고서는 [커버리지 워크플로](https://github.com/kcenon/common_system/actions/workflows/coverage.yml)에서 생성합니다.
설정된 커버리지 정책은 [codecov.yml](codecov.yml)에 있습니다.
실행별 결과는 각 워크플로 페이지에서 확인하세요.

문서 변경에는 [README 린터](scripts/readme_lint.py)와
[문서 감사](.github/workflows/doc-audit.yml)가 실행됩니다.
주장과 배지에 관한 규칙은 [README 정책](docs/contributing/README_POLICY.md)을 참고하세요.

## 문제 해결

[설정 점검](docs/guides/QUICK_START.md#troubleshooting),
[문제 해결 가이드](docs/guides/TROUBLESHOOTING.md), [FAQ](docs/guides/FAQ.md)를 확인하세요.
재현 가능한 문제는 [GitHub Issues](https://github.com/kcenon/common_system/issues)에 보고해 주세요.

## 기여

[기여 가이드](docs/contributing/CONTRIBUTING.md)에서 개발 환경 설정,
코드 스타일 및 풀 리퀘스트 절차를 확인하세요.
오류를 추가할 때는 [오류 코드 지침](docs/guides/ERROR_CODE_GUIDELINES.md)을 따르세요.

## 변경 이력

[CHANGELOG.md](CHANGELOG.md)는 저장소 변경 사항을 기록하며,
공개된 버전은 [GitHub Releases](https://github.com/kcenon/common_system/releases)에 표시됩니다.
준비 중인 v1.0.0의 API 정책은 해당 릴리스가 태그된 후 적용됩니다.
[VERSIONING.md](VERSIONING.md)를 참고하세요.

## 라이선스

Common System은 [BSD 3-Clause 라이선스](LICENSE)를 사용합니다.
