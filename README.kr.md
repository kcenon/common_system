[![CI](https://github.com/kcenon/common_system/actions/workflows/ci.yml/badge.svg)](https://github.com/kcenon/common_system/actions/workflows/ci.yml)
[![Code Coverage](https://github.com/kcenon/common_system/actions/workflows/coverage.yml/badge.svg)](https://github.com/kcenon/common_system/actions/workflows/coverage.yml)
[![Static Analysis](https://github.com/kcenon/common_system/actions/workflows/static-analysis.yml/badge.svg)](https://github.com/kcenon/common_system/actions/workflows/static-analysis.yml)
[![codecov](https://codecov.io/gh/kcenon/common_system/branch/main/graph/badge.svg)](https://codecov.io/gh/kcenon/common_system)
[![Doxygen](https://github.com/kcenon/common_system/actions/workflows/build-Doxygen.yaml/badge.svg)](https://github.com/kcenon/common_system/actions/workflows/build-Doxygen.yaml)
[![License](https://img.shields.io/github/license/kcenon/common_system)](https://github.com/kcenon/common_system/blob/main/LICENSE)

# Common System

> **Language:** [English](README.md) | **한국어**

## 목차

- [개요](#개요)
- [주요 기능](#주요-기능)
- [요구사항](#요구사항)
- [빠른 시작](#빠른-시작)
- [설치](#설치)
- [아키텍처](#아키텍처)
- [핵심 개념](#핵심-개념)
- [API 개요](#api-개요)
- [예제](#예제)
- [성능](#성능)
- [생태계 통합](#생태계-통합)
- [기여하기](#기여하기)
- [라이선스](#라이선스)

---

## 개요

모듈식, 느슨하게 결합된 시스템 아키텍처를 구축하기 위한 핵심 인터페이스와 디자인 패턴을 제공하는 C++20 header-only 라이브러리입니다. 생태계의 초석으로 설계되어, 템플릿 기반 추상화와 인터페이스 주도 설계를 통해 런타임 오버헤드 없이 시스템 모듈 간 원활한 통합을 가능하게 합니다.

**핵심 가치**:
- **제로 오버헤드 추상화**: 컴파일 타임 해석을 통한 템플릿 기반 인터페이스
- **충분한 테스트**: 80%+ 테스트 커버리지, 제로 sanitizer 경고, 완전한 CI/CD
- **Header-only 설계**: 라이브러리 링킹 불필요, 의존성 없음, 즉시 통합
- **C++20 모듈 지원**: 더 빠른 컴파일을 위한 선택적 모듈 기반 빌드
- **생태계 기반**: thread_system, network_system, database_system 등을 지원

**v1.0.0** — 안정 API 릴리스. 모든 공개 헤더가 SemVer 보증 하에 동결됩니다.
주요 변경은 향후 메이저 버전(v2.0+)에서만 발생합니다.

### API 안정성

v1.0.0부터 common_system은 다음 보증을 제공합니다:

- 동일 메이저 버전 내에서 공개 헤더에 대한 **호환성 파괴 변경 없음**
- 메이저 버전 범프 없이 공개 함수, 클래스, 타입 별칭 **제거 없음**
- **안정 CMake 타겟**: `common_system::common_system`, `kcenon::common_system`, `kcenon::common`
- **안정 `#include` 경로**: `kcenon/common/` 하위 모든 헤더가 공개 API의 일부
- **Result\<T\> 기반 오류 처리**: 공개 API는 예외 대신 `Result<T>`를 반환. `unwrap()` 메서드는 오류 결과에서 호출 시 의도적으로 예외를 발생(Rust 스타일 패닉 의미론)

자세한 내용은 [VERSIONING.md](VERSIONING.md)를 참조하세요.

---

## 주요 기능

| 카테고리 | 기능 | 설명 | 상태 |
|----------|------|------|------|
| **패턴** | Result<T> | Rust 영감 모나딕 오류 처리 (and_then, map, or_else) | 안정 |
| **패턴** | Circuit Breaker | CLOSED/OPEN/HALF_OPEN 상태의 복원력 패턴 | 안정 |
| **패턴** | Event Bus | 스레드 안전 동기 pub/sub | 안정 |
| **인터페이스** | IExecutor / IJob | 범용 작업 실행 추상화 | 안정 |
| **인터페이스** | ILogger / IMetricCollector | 모니터링 및 로깅 인터페이스 | 안정 |
| **DI** | Service Container | singleton/transient/scoped 생명주기의 스레드 안전 DI | 안정 |
| **설정** | Config Loader / Watcher | 파일 감시를 포함한 설정 관리 | 안정 |
| **설정** | CLI Parser | 명령줄 인수 파싱 | 안정 |
| **유틸** | Circular Buffer / Object Pool | 고성능 유틸리티 데이터 구조 | 안정 |
| **개념** | C++20 Concepts | Resultable, Unwrappable, callable, container 등 | 안정 |

---

## 요구사항

| 의존성 | 버전 | 필수 | 설명 |
|--------|------|------|------|
| C++20 컴파일러 | GCC 11+ / Clang 14+ / MSVC 2022+ / Apple Clang 14+ | 예 | C++20 기능 (concepts) |
| CMake | 3.28+ | 예 | 빌드 시스템 |

### 컴파일러 요구사항

| 빌드 모드 | GCC | Clang | MSVC | Apple Clang |
|-----------|-----|-------|------|-------------|
| **Header-only** (기본) | 11+ | 14+ | 2022 (19.30+) | 14+ |
| **C++20 모듈** (선택) | 14+ | 16+ | 2022 17.4 (19.34+) | 미지원 |

### 생태계 전체 컴파일러 요구사항

여러 시스템을 함께 사용할 때는 의존성 체인에서 **가장 높은** 요구사항을 사용하세요:

| 사용 시나리오 | GCC | Clang | MSVC | Apple Clang | 비고 |
|---------------|-----|-------|------|-------------|------|
| common_system 단독 | 11+ | 14+ | 2022+ | 14+ | 기준선 |
| + thread_system | **13+** | **17+** | 2022+ | 14+ | 더 높은 요구사항 |
| + logger_system | 11+ | 14+ | 2022+ | 14+ | thread_system 선택적 |
| + container_system | 11+ | 14+ | 2022+ | 14+ | common_system 사용 |
| + monitoring_system | **13+** | **17+** | 2022+ | 14+ | thread_system 필요 |
| + database_system | **13+** | **17+** | 2022+ | 14+ | 전체 생태계 |
| + network_system | **13+** | **17+** | 2022+ | 14+ | thread_system 필요 |

> **참고**: thread_system에 의존하는 시스템을 사용하는 경우 GCC 13+ 또는 Clang 17+가 필요합니다.

### 의존성 구조

```
common_system (기반 계층 - 의존성 없음)
       |
       | 인터페이스 제공
       |
       +-- thread_system (IExecutor 구현)
       +-- logger_system (Result<T> 사용)
       +-- container_system (Result<T> 사용)
       +-- monitoring_system (이벤트 버스)
       +-- network_system (IExecutor 사용)
       +-- database_system (Result<T> 및 IExecutor 사용)
```

---

## 빠른 시작

```cpp
#include <kcenon/common/patterns/result.h>

using namespace kcenon::common;

Result<Config> load_config(const std::string& path) {
    if (!std::filesystem::exists(path)) {
        return make_error<Config>(
            error_codes::NOT_FOUND,
            "Configuration file not found",
            "config_loader"
        );
    }
    auto config = parse_json_file(path);
    return ok(config);
}

// Monadic 연산 사용
auto result = load_config("app.conf")
    .and_then(validate_config)
    .map(apply_defaults);
```

[전체 시작 가이드](docs/guides/QUICK_START.md)

---

## 설치

### vcpkg를 통한 설치

```bash
vcpkg install kcenon-common-system
```

`CMakeLists.txt`에서:
```cmake
find_package(common_system CONFIG REQUIRED)
target_link_libraries(your_target PRIVATE kcenon::common_system)
```

### CMake FetchContent (권장)

```cmake
include(FetchContent)
FetchContent_Declare(
    common_system
    GIT_REPOSITORY https://github.com/kcenon/common_system.git
    GIT_TAG v1.0.0
)
FetchContent_MakeAvailable(common_system)

target_link_libraries(your_target PRIVATE kcenon::common)
```

### Header-Only 사용 (가장 간단)

```bash
git clone https://github.com/kcenon/common_system.git
# 헤더를 직접 포함 - 빌드 불필요!
```

```cpp
#include <kcenon/common/interfaces/executor_interface.h>
#include <kcenon/common/patterns/result.h>
```

### C++20 모듈

```bash
cmake -G Ninja -B build -DCOMMON_BUILD_MODULES=ON
cmake --build build
```

```cpp
import kcenon.common;

int main() {
    auto result = kcenon::common::ok(42);
    if (result.is_ok()) {
        std::cout << result.value() << std::endl;
    }
    return 0;
}
```

---

## 아키텍처

### 모듈 구조

```
include/kcenon/common/
  adapters/       - 어댑터 패턴 (adapter.h, smart_adapter.h)
  bootstrap/      - 시스템 부트스트래퍼
  concepts/       - C++20 개념 (Resultable, Unwrappable, callable, container 등)
  config/         - 기능 플래그, ABI 버전, 설정 로더/감시자, CLI 파서
  di/             - 의존성 주입 (service_container, unified_bootstrapper)
  error/          - 오류 코드 및 오류 카테고리 시스템
  interfaces/     - 핵심 추상화 (IExecutor, IJob, ILogger, IDatabase 등)
  logging/        - 로그 함수 및 매크로
  patterns/       - Result<T>, event_bus
  resilience/     - 서킷 브레이커 (CLOSED/OPEN/HALF_OPEN 상태 머신)
  utils/          - Circular buffer, object pool, enum 직렬화
```

### 생태계 위치

```
                    +------------------+
                    |  common_system   | <-- 기반 계층
                    |  (interfaces)    |
                    +--------+---------+
                             | 인터페이스 제공
       +---------------------+---------------------+
       |                     |                     |
+------v-------+    +--------v--------+   +-------v--------+
|thread_system |    |network_system   |   |monitoring_sys. |
|(IExecutor    |    |(IExecutor 사용)  |   |(이벤트 버스)    |
| 구현)         |    +-----------------+   +----------------+
+--------------+             |                     |
       |                     |                     |
       +---------------------+---------------------+
                             | 모두 사용
                    +--------v---------+
                    | Result<T> 패턴   |
                    | 오류 처리         |
                    +------------------+
```

[전체 아키텍처 가이드](docs/ARCHITECTURE.md)

---

## 핵심 개념

### Result<T> 패턴

Rust에서 영감을 받은 예외 없는 타입 안전한 오류 처리:

```cpp
auto result = load_config("app.conf")
    .and_then(validate_config)
    .map(apply_defaults)
    .or_else([](const auto& error) {
        log_error(error);
        return load_fallback_config();
    });
```

### IExecutor 인터페이스

모든 스레딩 백엔드를 위한 범용 작업 실행 추상화:

```cpp
class MyService {
    std::shared_ptr<common::interfaces::IExecutor> executor_;
public:
    void process_async(const Data& data) {
        auto future = executor_->submit([data]() { return process(data); });
    }
};
```

### 상태 모니터링

의존성 그래프를 포함한 상태 체크 시스템:

```cpp
auto& monitor = global_health_monitor();
auto db_check = health_check_builder()
    .name("database")
    .type(health_check_type::dependency)
    .timeout(std::chrono::seconds{5})
    .with_check([]() { /* 체크 로직 */ })
    .build();
monitor.register_check("database", db_check.value());
```

### 오류 코드 레지스트리

| 시스템 | 범위 | 용도 |
|--------|------|------|
| common_system | -1 ~ -99 | 핵심 오류 |
| thread_system | -100 ~ -199 | 스레딩 오류 |
| logger_system | -200 ~ -299 | 로깅 오류 |
| monitoring_system | -300 ~ -399 | 모니터링 오류 |
| container_system | -400 ~ -499 | 컨테이너 오류 |
| database_system | -500 ~ -599 | 데이터베이스 오류 |
| network_system | -600 ~ -699 | 네트워크 오류 |

### 서킷 브레이커

장애 허용을 위한 복원력 패턴:

```cpp
auto breaker = circuit_breaker("db_connection", {
    .failure_threshold = 5,
    .recovery_timeout = std::chrono::seconds{30}
});
auto result = breaker.execute([&]() { return db.query("SELECT 1"); });
```

---

## API 개요

| 컴포넌트 | 용도 | 헤더 |
|----------|------|------|
| `Result<T>` / `VoidResult` | 모나딕 오류 처리 | `patterns/result.h` |
| `IExecutor` / `IJob` | 작업 실행 인터페이스 | `interfaces/executor_interface.h` |
| `ILogger` | 로깅 추상화 | `interfaces/logger_interface.h` |
| `service_container` | 의존성 주입 | `di/service_container.h` |
| `simple_event_bus` | 동기 pub/sub | `patterns/event_bus.h` |
| `circuit_breaker` | 복원력 패턴 | `resilience/circuit_breaker.h` |
| `config_loader` | 설정 관리 | `config/config_loader.h` |

[전체 API 레퍼런스](docs/API_REFERENCE.md)

---

## 예제

| 예제 | 설명 | 난이도 |
|------|------|--------|
| [result_example](examples/result_example.cpp) | Result<T> 오류 처리 패턴 | 초급 |
| [executor_example](examples/executor_example.cpp) | Executor 인터페이스와 스레드 관리 | 초급 |
| [abi_version_example](examples/abi_version_example.cpp) | ABI 버전 확인 및 호환성 | 중급 |
| [unwrap_demo](examples/unwrap_demo.cpp) | Result unwrapping 및 체이닝 | 중급 |
| [multi_system_app](examples/multi_system_app/) | 멀티 시스템 통합 예제 | 고급 |

### 예제 실행

```bash
cmake -B build -DCOMMON_BUILD_EXAMPLES=ON
cmake --build build
./build/examples/result_example
```

---

## 성능

| 작업 | 시간 (ns) | 할당 | 비고 |
|------|-----------|------|------|
| Result<T> 생성 | 2.3 | 0 | 스택 전용 작업 |
| Result<T> 오류 확인 | 0.8 | 0 | 단일 bool 확인 |
| IExecutor submit | 45.2 | 1 | 작업 큐 삽입 |
| Event publish | 12.4 | 0 | Lock-free 작업 |

**주요 성능 특성:**
- Result<T>는 오류 경로에서 예외보다 400배 빠름
- IExecutor는 고빈도 작업에 대해 std::async보다 53배 빠름
- 제로 오버헤드 추상화 - 컴파일러가 모든 추상화 레이어를 최적화

**품질 메트릭**:
- **테스트 커버리지**: 80%+ (목표: 85%)
- **Sanitizer 테스트**: 18/18 통과, 제로 경고
- **크로스 플랫폼**: Ubuntu, macOS, Windows
- **메모리 누수 없음**: AddressSanitizer 검증
- **데이터 레이스 없음**: ThreadSanitizer 검증
- **RAII 등급: A** - 스마트 포인터를 통해 관리되는 모든 리소스

[전체 벤치마크](docs/BENCHMARKS.md)

---

## 생태계 통합

이 common system은 모든 시스템 모듈이 구축하는 기반 계층(Tier 0)으로 역할합니다:

```
common_system (Tier 0 - 기반)
       |
       +-- thread_system     (Tier 1) - IExecutor 구현
       +-- container_system  (Tier 1) - Result<T> 사용
       +-- logger_system     (Tier 2) - ILogger, Result<T> 사용
       +-- monitoring_system (Tier 3) - Event Bus 사용
       +-- database_system   (Tier 3) - Result<T>, IExecutor 사용
       +-- network_system    (Tier 4) - IExecutor 사용
       +-- pacs_system       (Tier 5) - 전체 생태계 소비자
```

### 통합 예제

```cpp
#include <kcenon/common/patterns/result.h>
#include <kcenon/common/interfaces/executor_interface.h>

// Result<T>는 범용 오류 처리 패턴
auto result = do_something();
if (result.is_err()) {
    // 모든 프로젝트에서 일관된 오류 처리
    auto error = result.error();
    std::cerr << error.message << " (code: " << error.code << ")\n";
}
```

### 문서

| 카테고리 | 문서 | 설명 |
|----------|------|------|
| **가이드** | [빠른 시작](docs/guides/QUICK_START.md) | 몇 분 안에 시작하기 |
| | [모범 사례](docs/guides/BEST_PRACTICES.md) | 권장 사용 패턴 |
| | [FAQ](docs/guides/FAQ.md) | 자주 묻는 질문 |
| | [문제 해결](docs/guides/TROUBLESHOOTING.md) | 일반적인 문제 및 해결책 |
| **고급** | [아키텍처](docs/ARCHITECTURE.md) | 시스템 설계 및 원칙 |
| | [마이그레이션](docs/advanced/MIGRATION.md) | 버전 업그레이드 가이드 |
| **기여** | [기여 가이드](CONTRIBUTING.md) | 기여 방법 |

---

## 기여하기

기여를 환영합니다! 가이드라인은 [CONTRIBUTING.md](docs/contributing/CONTRIBUTING.md)를 참조하세요.

### 빠른 링크

- [개발 환경 설정](docs/contributing/CONTRIBUTING.md#development-workflow)
- [코드 스타일](docs/contributing/CONTRIBUTING.md#code-style)
- [Pull Request 프로세스](docs/contributing/CONTRIBUTING.md#development-workflow)

### 지원

- **이슈**: [GitHub Issues](https://github.com/kcenon/common_system/issues)
- **토론**: [GitHub Discussions](https://github.com/kcenon/common_system/discussions)
- **이메일**: kcenon@naver.com

---

## 라이선스

이 프로젝트는 BSD 3-Clause License 하에 라이선스됩니다 - 자세한 내용은 [LICENSE](LICENSE) 파일을 참조하세요.

---

<p align="center">
  Made with care by the kcenon team
</p>
