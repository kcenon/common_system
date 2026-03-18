[![CI](https://github.com/kcenon/common_system/actions/workflows/ci.yml/badge.svg)](https://github.com/kcenon/common_system/actions/workflows/ci.yml)
[![Code Coverage](https://github.com/kcenon/common_system/actions/workflows/coverage.yml/badge.svg)](https://github.com/kcenon/common_system/actions/workflows/coverage.yml)
[![Static Analysis](https://github.com/kcenon/common_system/actions/workflows/static-analysis.yml/badge.svg)](https://github.com/kcenon/common_system/actions/workflows/static-analysis.yml)
[![Doxygen](https://github.com/kcenon/common_system/actions/workflows/build-Doxygen.yaml/badge.svg)](https://github.com/kcenon/common_system/actions/workflows/build-Doxygen.yaml)
[![License](https://img.shields.io/github/license/kcenon/common_system)](https://github.com/kcenon/common_system/blob/main/LICENSE)

# Common System

> **Language:** [English](README.md) | **한국어**

## 목차

- [개요](#개요)
- [빠른 시작](#빠른-시작)
- [요구사항](#요구사항)
- [설치](#설치)
- [아키텍처](#아키텍처)
- [핵심 기능](#핵심-기능)
- [문서](#문서)
- [성능](#성능)
- [오류 처리 기반](#오류-처리-기반)
- [프로덕션 품질](#프로덕션-품질)
- [기여하기](#기여하기)
- [라이선스](#라이선스)

## 개요

모듈식, 느슨하게 결합된 시스템 아키텍처를 구축하기 위한 핵심 인터페이스와 디자인 패턴을 제공하는 C++20 header-only 라이브러리입니다. 생태계의 초석으로 설계되어, 템플릿 기반 추상화와 인터페이스 주도 설계를 통해 런타임 오버헤드 없이 시스템 모듈 간 원활한 통합을 가능하게 합니다.

**핵심 가치**:
- 🚀 **제로 오버헤드 추상화**: 컴파일 타임 해석을 통한 템플릿 기반 인터페이스
- 🔒 **충분한 테스트**: 80%+ 테스트 커버리지, 제로 sanitizer 경고, 완전한 CI/CD
- 🏗️ **Header-only 설계**: 라이브러리 링킹 불필요, 의존성 없음, 즉시 통합
- 🛡️ **C++20 모듈 지원**: 더 빠른 컴파일을 위한 선택적 모듈 기반 빌드
- 🌐 **생태계 기반**: thread_system, network_system, database_system 등을 지원

**최신 업데이트** (2026-01):
- ✅ 개별 모듈과의 완전한 분리
- ✅ 포괄적인 Result<T> 패턴 구현
- ✅ ABI 버전 검사를 포함한 IExecutor 인터페이스 표준화
- ✅ 의존성 그래프 및 복구 핸들러를 포함한 상태 모니터링 시스템
- ✅ 장애 허용 및 복원력을 위한 서킷 브레이커 패턴
- ✅ 통합 통계 수집 및 모니터링을 위한 IStats 인터페이스

---

## 빠른 시작

### 기본 예제

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

📖 **[전체 시작 가이드 →](docs/guides/QUICK_START.md)**

---

## 요구사항

| 의존성 | 버전 | 필수 | 설명 |
|--------|------|------|------|
| C++20 컴파일러 | GCC 11+ / Clang 14+ / MSVC 2022+ / Apple Clang 14+ | 예 | C++20 기능 필요 |
| CMake | 3.20+ | 예 | 빌드 시스템 |

### 의존성 구조

```
common_system (기반 계층 - 의존성 없음)
       │
       │ 인터페이스 제공
       │
       ├── thread_system (IExecutor 구현)
       ├── logger_system (Result<T> 사용)
       ├── container_system (Result<T> 사용)
       ├── monitoring_system (이벤트 버스)
       ├── network_system (IExecutor 사용)
       └── database_system (Result<T> 및 IExecutor 사용)
```

### 생태계 전체 컴파일러 요구사항

여러 시스템을 함께 사용할 때는 의존성 체인에서 **가장 높은** 요구사항을 사용하세요:

| 사용 시나리오 | GCC | Clang | MSVC | 비고 |
|---------------|-----|-------|------|------|
| common_system 단독 | 11+ | 14+ | 2022+ | 기준선 |
| + thread_system | **13+** | **17+** | 2022+ | 더 높은 요구사항 |
| + logger_system | 11+ | 14+ | 2022+ | thread_system 선택적 |
| + container_system | 11+ | 14+ | 2022+ | common_system 사용 |
| + monitoring_system | **13+** | **17+** | 2022+ | thread_system 필요 |
| + database_system | **13+** | **17+** | 2022+ | 전체 생태계 |
| + network_system | **13+** | **17+** | 2022+ | thread_system 필요 |

> **참고**: thread_system에 의존하는 시스템을 사용하는 경우 GCC 13+ 또는 Clang 17+가 필요합니다.

---

## 설치

### 옵션 1: Header-Only 사용 (가장 간단)

```bash
git clone https://github.com/kcenon/common_system.git
# 헤더를 직접 포함 - 빌드 불필요!
```

```cpp
#include <kcenon/common/interfaces/executor_interface.h>
#include <kcenon/common/patterns/result.h>
```

### 옵션 2: CMake 통합 (권장)

```cmake
include(FetchContent)
FetchContent_Declare(
    common_system
    GIT_REPOSITORY https://github.com/kcenon/common_system.git
    GIT_TAG v0.2.0
)
FetchContent_MakeAvailable(common_system)

target_link_libraries(your_target PRIVATE kcenon::common)
```

### 옵션 3: C++20 모듈

```bash
# C++20 모듈 지원으로 빌드 (CMake 3.28+, Ninja, Clang 16+/GCC 14+ 필요)
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

### 생태계 통합

이 common system은 다른 모든 시스템 모듈이 구축하는 기반 계층으로 역할합니다:

```
                    ┌──────────────────┐
                    │  common_system   │ ◄── 기반 계층
                    │  (interfaces)    │
                    └────────┬─────────┘
                             │ 인터페이스 제공
       ┌─────────────────────┼─────────────────────┐
       │                     │                     │
┌──────▼───────┐    ┌────────▼────────┐   ┌───────▼────────┐
│thread_system │    │network_system   │   │monitoring_sys. │
│(implements   │    │(uses IExecutor) │   │(event bus)     │
│ IExecutor)   │    └─────────────────┘   └────────────────┘
└──────────────┘             │                     │
       │                     │                     │
       └─────────────────────┼─────────────────────┘
                             │ 모두 사용
                    ┌────────▼─────────┐
                    │ Result<T> 패턴   │
                    │ Error handling   │
                    └──────────────────┘
```

📖 **[전체 아키텍처 가이드 →](docs/01-ARCHITECTURE.md)**

---

## 핵심 기능

### IExecutor 인터페이스

모든 스레딩 백엔드를 위한 범용 작업 실행 추상화:

```cpp
#include <kcenon/common/interfaces/executor_interface.h>

class MyService {
    std::shared_ptr<common::interfaces::IExecutor> executor_;

public:
    void process_async(const Data& data) {
        auto future = executor_->submit([data]() {
            return process(data);
        });
    }
};
```

### Result<T> 패턴

Rust에서 영감을 받은 예외 없는 타입 안전한 오류 처리:

```cpp
#include <kcenon/common/patterns/result.h>

auto result = load_config("app.conf")
    .and_then(validate_config)
    .map(apply_defaults)
    .or_else([](const auto& error) {
        log_error(error);
        return load_fallback_config();
    });
```

### 상태 모니터링

의존성 그래프를 포함한 포괄적인 상태 체크 시스템:

```cpp
#include <kcenon/common/interfaces/monitoring.h>

auto& monitor = global_health_monitor();

auto db_check = health_check_builder()
    .name("database")
    .type(health_check_type::dependency)
    .timeout(std::chrono::seconds{5})
    .with_check([]() { /* 체크 로직 */ })
    .build();

monitor.register_check("database", db_check.value());
monitor.add_dependency("api", "database");
```

📖 **[상세 기능 문서 →](docs/FEATURES.md)**

---

## 문서

| 카테고리 | 문서 | 설명 |
|----------|------|------|
| **가이드** | [빠른 시작](docs/guides/QUICK_START.md) | 몇 분 안에 시작하기 |
| | [모범 사례](docs/guides/BEST_PRACTICES.md) | 권장 사용 패턴 |
| | [FAQ](docs/guides/FAQ.md) | 자주 묻는 질문 |
| | [문제 해결](docs/guides/TROUBLESHOOTING.md) | 일반적인 문제 및 해결책 |
| **고급** | [아키텍처](docs/01-ARCHITECTURE.md) | 시스템 설계 및 원칙 |
| | [마이그레이션](docs/advanced/MIGRATION.md) | 버전 업그레이드 가이드 |
| | [IExecutor 마이그레이션](docs/advanced/IEXECUTOR_MIGRATION_GUIDE.md) | Executor API 마이그레이션 |
| | [런타임 바인딩](docs/architecture/RUNTIME_BINDING.md) | 핵심 디자인 패턴 |
| **기여** | [기여 가이드](CONTRIBUTING.md) | 기여 방법 |
| | [오류 코드 가이드라인](docs/guides/ERROR_CODE_GUIDELINES.md) | 오류 코드 관리 |

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

📖 **[전체 벤치마크 →](docs/BENCHMARKS.md)**

---

## 오류 처리 기반

시스템별 범위를 제공하는 중앙화된 오류 코드 레지스트리:

| 시스템 | 범위 | 용도 |
|--------|------|------|
| common_system | -1 ~ -99 | 핵심 오류 |
| thread_system | -100 ~ -199 | 스레딩 오류 |
| logger_system | -200 ~ -299 | 로깅 오류 |
| monitoring_system | -300 ~ -399 | 모니터링 오류 |
| container_system | -400 ~ -499 | 컨테이너 오류 |
| database_system | -500 ~ -599 | 데이터베이스 오류 |
| network_system | -600 ~ -699 | 네트워크 오류 |

---

## 프로덕션 품질

### 품질 메트릭
- **테스트 커버리지**: 80%+ (목표: 85%)
- **Sanitizer 테스트**: 18/18 통과, 제로 경고
- **크로스 플랫폼**: Ubuntu, macOS, Windows
- **메모리 누수 없음**: AddressSanitizer 검증
- **데이터 레이스 없음**: ThreadSanitizer 검증

### RAII 등급: A
- 스마트 포인터를 통해 관리되는 모든 리소스
- 인터페이스에서 수동 메모리 관리 없음
- 예외 안전 설계 검증

---

## 기여하기

기여를 환영합니다! 가이드라인은 [CONTRIBUTING.md](docs/contributing/CONTRIBUTING.md)를 참조하세요.

### 빠른 링크

- [개발 환경 설정](docs/contributing/CONTRIBUTING.md#development-workflow)
- [코드 스타일](docs/contributing/CONTRIBUTING.md#code-style)
- [Pull Request 프로세스](docs/contributing/CONTRIBUTING.md#development-workflow)

---

## 지원

- **이슈**: [GitHub Issues](https://github.com/kcenon/common_system/issues)
- **토론**: [GitHub Discussions](https://github.com/kcenon/common_system/discussions)
- **이메일**: kcenon@naver.com

---

## 라이선스

이 프로젝트는 BSD 3-Clause License 하에 라이선스됩니다 - 자세한 내용은 [LICENSE](LICENSE) 파일을 참조하세요.

---

## 감사의 말

- Rust의 Result<T,E> 타입 및 오류 처리에서 영감을 받음
- Java의 ExecutorService의 영향을 받은 인터페이스 설계
- 반응형 프로그래밍 프레임워크의 Event bus 패턴
