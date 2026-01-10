[![CI](https://github.com/kcenon/common_system/actions/workflows/ci.yml/badge.svg)](https://github.com/kcenon/common_system/actions/workflows/ci.yml)
[![Code Coverage](https://github.com/kcenon/common_system/actions/workflows/coverage.yml/badge.svg)](https://github.com/kcenon/common_system/actions/workflows/coverage.yml)
[![Static Analysis](https://github.com/kcenon/common_system/actions/workflows/static-analysis.yml/badge.svg)](https://github.com/kcenon/common_system/actions/workflows/static-analysis.yml)
[![Doxygen](https://github.com/kcenon/common_system/actions/workflows/build-Doxygen.yaml/badge.svg)](https://github.com/kcenon/common_system/actions/workflows/build-Doxygen.yaml)
[![License](https://img.shields.io/github/license/kcenon/common_system)](https://github.com/kcenon/common_system/blob/main/LICENSE)

# Common System Project

> **Language:** [English](README.md) | **한국어**

## 개요

Common System Project는 모듈식, 느슨하게 결합된 시스템 아키텍처를 구축하기 위한 핵심 인터페이스와 디자인 패턴을 제공하는 C++20 header-only 라이브러리입니다. 생태계의 초석으로 설계되어, 템플릿 기반 추상화와 인터페이스 주도 설계를 통해 런타임 오버헤드 없이 시스템 모듈 간 원활한 통합을 가능하게 합니다.

### 주요 특징

- **제로 오버헤드 추상화**: 컴파일 타임 해석을 통한 템플릿 기반 인터페이스
- **Header-only 설계**: 라이브러리 링킹 불필요, 의존성 없음, 즉시 통합
- **충분한 테스트**: 80%+ 테스트 커버리지, 제로 sanitizer 경고, 완전한 CI/CD
- **범용 호환성**: C++20 표준, 현대적 언어 기능 지원
- **C++20 모듈 지원**: 더 빠른 컴파일을 위한 선택적 모듈 기반 빌드
- **생태계 기반**: thread_system, network_system, database_system 등을 지원

> **최신 업데이트**: 개별 모듈과의 완전한 분리, 포괄적인 Result<T> 패턴 구현, ABI 버전 검사를 포함한 IExecutor 인터페이스 표준화, 통합된 `kcenon::common` 네임스페이스, 이벤트 버스 통합 테스트, 향상된 문서 구조

## 핵심 기능

- **IExecutor 인터페이스**: 모든 스레딩 백엔드를 위한 범용 작업 실행 추상화
- **Result<T> 패턴**: Rust에서 영감을 받은 예외 없는 타입 안전한 오류 처리
- **Event Bus**: 분리된 이벤트 주도 아키텍처를 위한 Publish-Subscribe 패턴
- **Error Code Registry**: 모든 생태계 모듈 간 중앙화된 오류 코드 시스템
- **Smart Interfaces**: 쉬운 테스트 및 의존성 주입을 위한 모의 가능 추상화
- **C++20 Concepts**: 명확한 오류 메시지를 제공하는 컴파일 타임 타입 검증

[📚 상세 기능 문서 →](docs/FEATURES_KO.md)

## 프로젝트 생태계

이 common system은 다른 모든 시스템 모듈이 구축하는 기초 계층으로 역할합니다:

### 의존성 아키텍처

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

### 의존 프로젝트

- **[thread_system](https://github.com/kcenon/thread_system)**: IExecutor 구현 핵심 스레딩 프레임워크
- **[network_system](https://github.com/kcenon/network_system)**: IExecutor 사용 비동기 네트워크 라이브러리
- **[logger_system](https://github.com/kcenon/logger_system)**: Result<T> 사용 고성능 로깅
- **[monitoring_system](https://github.com/kcenon/monitoring_system)**: Metrics 및 이벤트 버스 구현
- **[container_system](https://github.com/kcenon/container_system)**: Result<T> 사용 데이터 직렬화
- **[database_system](https://github.com/kcenon/database_system)**: Result<T> 및 IExecutor 사용 데이터베이스 추상화

[🏗️ 전체 아키텍처 가이드 →](docs/ARCHITECTURE_KO.md)

## 빠른 시작

### 시스템 요구사항

- **컴파일러**: C++20 호환 (GCC 11+, Clang 14+, MSVC 2022+, Apple Clang 14+)
- **빌드 시스템**: CMake 3.20 이상
- **플랫폼**: Windows, Linux, macOS (x86_64, ARM64)

### 설치

#### 옵션 1: Header-Only 사용 (가장 간단)

```bash
git clone https://github.com/kcenon/common_system.git
# 헤더를 직접 포함 - 빌드 불필요!
```

```cpp
#include <kcenon/common/interfaces/executor_interface.h>
#include <kcenon/common/patterns/result.h>
```

#### 옵션 2: CMake 통합

```cmake
# FetchContent 사용 (권장)
include(FetchContent)
FetchContent_Declare(
    common_system
    GIT_REPOSITORY https://github.com/kcenon/common_system.git
    GIT_TAG main
)
FetchContent_MakeAvailable(common_system)

target_link_libraries(your_target PRIVATE kcenon::common)
```

#### 옵션 3: 시스템 전역 설치

```bash
git clone https://github.com/kcenon/common_system.git
cd common_system
./scripts/build.sh --release --install-prefix=/usr/local
sudo cmake --build build --target install
```

#### 옵션 4: C++20 모듈

```bash
# C++20 모듈 지원으로 빌드 (CMake 3.28+, Ninja, Clang 16+/GCC 14+ 필요)
cmake -G Ninja -B build -DCOMMON_BUILD_MODULES=ON
cmake --build build
```

```cpp
// 헤더 대신 모듈 사용
import kcenon.common;

int main() {
    auto result = kcenon::common::ok(42);
    if (result.is_ok()) {
        std::cout << result.value() << std::endl;
    }
    return 0;
}
```

> **참고**: 모듈 지원은 Ninja 생성기와 모듈을 지원하는 C++20 호환 컴파일러가 필요합니다 (Clang 16+, GCC 14+, MSVC 2022 17.4+). AppleClang은 아직 모듈을 완전히 지원하지 않습니다. 자세한 내용은 [모듈 마이그레이션 가이드](docs/guides/MODULE_MIGRATION_KO.md)를 참조하세요.

### 소스에서 빌드

```bash
# 저장소 클론
git clone https://github.com/kcenon/common_system.git
cd common_system

# 테스트 및 예제와 함께 빌드
./scripts/build.sh --release --tests --examples

# 테스트 실행
./scripts/test.sh

# 빌드 아티팩트 정리
./scripts/clean.sh
```

### Windows 빌드

```batch
REM Visual Studio 2022 사용
scripts\build.bat --vs2022 --release

REM 테스트 실행
scripts\test.bat --release

REM 아티팩트 정리
scripts\clean.bat
```

[📖 전체 시작 가이드 →](docs/guides/QUICK_START.md)

## 사용 예제

### IExecutor 인터페이스

모든 스레딩 백엔드를 위한 범용 작업 실행 추상화:

```cpp
#include <kcenon/common/interfaces/executor_interface.h>

class MyService {
    std::shared_ptr<common::interfaces::IExecutor> executor_;

public:
    void process_async(const Data& data) {
        auto future = executor_->submit([data]() {
            // 비동기로 데이터 처리
            return process(data);
        });

        // 다른 작업 계속...
    }
};
```

### Result<T> 패턴

예외 없는 타입 안전한 오류 처리:

```cpp
#include <kcenon/common/patterns/result.h>

common::Result<Config> load_config(const std::string& path) {
    if (!std::filesystem::exists(path)) {
        return common::make_error<Config>(
            common::error_codes::NOT_FOUND,
            "Configuration file not found",
            "config_loader"
        );
    }

    try {
        auto config = parse_json_file(path);
        return common::ok(config);
    } catch (const std::exception& e) {
        return common::make_error<Config>(
            common::error_codes::INVALID_ARGUMENT,
            e.what(),
            "config_loader"
        );
    }
}

// Monadic 연산 사용
auto result = load_config("app.conf")
    .and_then(validate_config)
    .map(apply_defaults)
    .or_else([](const auto& error) {
        log_error(error);
        return load_fallback_config();
    });
```

### Event Bus 통합

monitoring_system과 함께 사용 시:

```cpp
#include <kcenon/common/patterns/event_bus.h>

// 이벤트 발행
auto bus = common::get_event_bus();
bus->publish(common::events::module_started_event("my_service"));

// 이벤트 구독
bus->subscribe<common::events::error_event>([](const auto& event) {
    std::cerr << "Error in " << event.module_name
              << ": " << event.error_message << std::endl;
});
```

[📘 더 많은 예제 →](examples/)

## 통합 예제

### thread_system과 함께

```cpp
#include <kcenon/thread/core/thread_pool.h>
#include <kcenon/thread/adapters/common_executor_adapter.h>

// Thread pool 생성
auto thread_pool = std::make_shared<kcenon::thread::thread_pool>(4);

// 공통 인터페이스로 적응
auto executor = kcenon::thread::adapters::make_common_executor(thread_pool);

// IExecutor 기반 API와 함께 사용
process_with_executor(executor);
```

### network_system과 함께

```cpp
#include <network_system/integration/executor_adapter.h>

// 네트워크 시스템과 함께 공통 executor 사용
void setup_network(std::shared_ptr<common::interfaces::IExecutor> executor) {
    auto adapted_pool = kcenon::network::integration::make_thread_pool_adapter(executor);

    network_system::server server(adapted_pool);
    // 네트워크 작업이 이제 공통 executor를 사용
}
```

[🔗 통합 가이드 →](docs/guides/INTEGRATION_KO.md)

## 성능 하이라이트

| 작업 | 시간 (ns) | 할당 | 비고 |
|------|-----------|------|------|
| Result<T> 생성 | 2.3 | 0 | 스택 전용 작업 |
| Result<T> 오류 확인 | 0.8 | 0 | 단일 bool 확인 |
| IExecutor submit | 45.2 | 1 | 작업 큐 삽입 |
| Event publish | 12.4 | 0 | Lock-free 작업 |

*플랫폼: Intel i7-9700K @ 3.6GHz, GCC 11.2 -O3*

**주요 성능 특성:**
- 제로 오버헤드 추상화 - 컴파일러가 모든 추상화 레이어를 최적화
- Result<T>는 오류 경로에서 예외보다 400배 빠름
- IExecutor는 고빈도 작업에 대해 std::async보다 53배 빠름
- Event bus는 구독자 수에 따라 선형 확장

[⚡ 전체 벤치마크 →](docs/BENCHMARKS_KO.md)

## 문서

### 시작하기
- [빠른 시작 가이드](docs/guides/QUICK_START.md) - 몇 분 안에 시작하기
- [아키텍처 개요](docs/ARCHITECTURE_KO.md) - 시스템 설계 및 원칙
- [통합 가이드](docs/guides/INTEGRATION_KO.md) - 프로젝트와 통합

### 핵심 문서
- [기능](docs/FEATURES_KO.md) - 상세 기능 설명
- [오류 처리 가이드](docs/guides/ERROR_HANDLING_KO.md) - Result<T> 패턴 및 모범 사례
- [Result 마이그레이션 가이드](docs/guides/RESULT_MIGRATION_GUIDE_KO.md) - 시스템 간 Result<T> 표준화
- [모범 사례](docs/guides/BEST_PRACTICES.md) - 권장 사용 패턴
- [FAQ](docs/guides/FAQ.md) - 자주 묻는 질문

### 고급 주제
- [마이그레이션 가이드](docs/advanced/MIGRATION_KO.md) - common_system으로 마이그레이션
- [IExecutor 마이그레이션](docs/advanced/IEXECUTOR_MIGRATION_GUIDE.md) - Executor API 마이그레이션
- [RAII 가이드라인](docs/guides/RAII_GUIDELINES_KO.md) - 리소스 관리 패턴
- [스마트 포인터 가이드라인](docs/guides/SMART_POINTER_GUIDELINES_KO.md) - 스마트 포인터 사용법

### C++20 기능
- [Concepts 가이드](docs/guides/CONCEPTS_GUIDE_KO.md) - 컴파일 타임 타입 검증을 위한 C++20 Concepts

### 참조
- [오류 코드 가이드라인](docs/guides/ERROR_CODE_GUIDELINES.md) - 오류 코드 관리
- [프로젝트 구조](docs/PROJECT_STRUCTURE_KO.md) - 저장소 구성
- [의존성 매트릭스](docs/advanced/DEPENDENCY_MATRIX_KO.md) - 생태계 의존성
- [호환성 매트릭스](docs/COMPATIBILITY_KO.md) - 시스템 간 버전 호환성
- [문제 해결](docs/guides/TROUBLESHOOTING.md) - 일반적인 문제 및 해결책

[📖 전체 문서 인덱스 →](docs/)

## 테스트

프로젝트는 포괄적인 테스트를 포함합니다:

```bash
# 모든 테스트 실행
./scripts/test.sh

# 커버리지와 함께 실행
./scripts/test.sh --coverage

# 특정 테스트 실행
./scripts/test.sh --filter "Result*"

# 벤치마크 테스트
./scripts/test.sh --benchmark
```

**품질 메트릭:**
- 테스트 커버리지: 80%+ (목표: 85%)
- Sanitizer 테스트: 18/18 통과, 제로 경고
- 크로스 플랫폼: Ubuntu, macOS, Windows
- 메모리 누수 없음 (AddressSanitizer 검증)
- 데이터 레이스 없음 (ThreadSanitizer 검증)

## 프로덕션 품질

### 멀티 플랫폼 CI/CD
- 자동화된 sanitizer 빌드 (ThreadSanitizer, AddressSanitizer, UBSanitizer)
- 크로스 플랫폼 테스트: Ubuntu (GCC/Clang), macOS (Apple Clang), Windows (MSVC)
- codecov 통합을 통한 코드 커버리지 추적
- clang-tidy 및 cppcheck를 통한 정적 분석

### 스레드 안전성
- 안전한 동시 접근을 위해 설계된 모든 인터페이스
- Result<T>는 생성 후 불변이며 스레드 안전
- IExecutor 계약은 동시 호출 보장 명시
- Event bus 작업은 가능한 경우 lock-free 설계 사용

### 리소스 관리 (RAII - Grade A)
- 스마트 포인터를 통해 관리되는 모든 리소스
- 인터페이스에서 수동 메모리 관리 없음
- AddressSanitizer 검증: 18/18 테스트 통과, 메모리 누수 없음
- 예외 안전 설계 검증

### 오류 처리 기반

시스템별 범위를 제공하는 중앙화된 오류 코드 레지스트리:

- common_system: -1 ~ -99
- thread_system: -100 ~ -199
- logger_system: -200 ~ -299
- monitoring_system: -300 ~ -399
- container_system: -400 ~ -499
- database_system: -500 ~ -599
- network_system: -600 ~ -699

컴파일 타임 검증으로 모든 시스템 간 코드 충돌을 방지합니다. 모든 의존 시스템이 Result<T> 패턴과 오류 코드 레지스트리를 성공적으로 채택했습니다.

## 기여하기

기여를 환영합니다! 가이드라인은 [CONTRIBUTING.md](CONTRIBUTING.md)를 참조하세요.

### 개발 워크플로우

1. 저장소 포크
2. feature 브랜치 생성 (`git checkout -b feature/amazing-feature`)
3. 변경사항 커밋 (`git commit -m 'feat: add amazing feature'`)
4. 브랜치에 푸시 (`git push origin feature/amazing-feature`)
5. Pull Request 생성

### 코드 스타일

- 기존 코드 스타일을 따르세요 (clang-format 구성 제공)
- 새로운 기능에 대한 포괄적인 단위 테스트 작성
- 필요에 따라 문서 업데이트
- PR 제출 전 모든 테스트 통과 확인

## 로드맵

**완료:**
- [x] IExecutor 인터페이스 표준화
- [x] Result<T> 패턴 구현
- [x] Event bus 포워딩
- [x] 중앙화된 빌드 구성
- [x] 호환성 검증을 위한 ABI 버전 검사
- [x] 통합된 `kcenon::common` 네임스페이스
- [x] Task 기반 IExecutor 인터페이스
- [x] 포괄적인 문서 재구성
- [x] C++20 표준, 현대적 언어 기능
- [x] 런타임 바인딩 아키텍처 (GlobalLoggerRegistry, SystemBootstrapper)
- [x] 통합 로깅 매크로 (LOG_*)
- [x] C++20 source_location 통합
- [x] C++20 Concepts 타입 검증
- [x] 패키지 관리자 지원 (Conan)
- [x] C++20 모듈 파일을 통한 빠른 컴파일
- [x] 의존성 그래프 및 복구 핸들러를 포함한 상태 모니터링 시스템

**계획:**
- [ ] 비동기 패턴을 위한 Coroutine 지원
- [ ] std::expected 마이그레이션 (C++23)
- [ ] 추가 디자인 패턴 (Observer, Command)
- [ ] 패키지 관리자 공식 레지스트리 (vcpkg, Conan Center)

## 지원

- **이슈**: [GitHub Issues](https://github.com/kcenon/common_system/issues)
- **토론**: [GitHub Discussions](https://github.com/kcenon/common_system/discussions)
- **이메일**: kcenon@naver.com

## 라이선스

이 프로젝트는 BSD 3-Clause License 하에 라이선스됩니다 - 자세한 내용은 [LICENSE](LICENSE) 파일을 참조하세요.

## 감사의 말

- Rust의 Result<T,E> 타입 및 오류 처리에서 영감을 받음
- Java의 ExecutorService의 영향을 받은 인터페이스 설계
- 반응형 프로그래밍 프레임워크의 Event bus 패턴
- 현대 C++ 모범 사례의 빌드 시스템 패턴

## 인용

연구나 상업 프로젝트에서 이 프로젝트를 사용하는 경우, 다음과 같이 인용하세요:

```bibtex
@software{common_system,
  author = {Dongcheol Shin},
  title = {Common System: Foundational Interfaces for Modular C++ Architecture},
  year = {2024},
  url = {https://github.com/kcenon/common_system}
}
```

---

<p align="center">
  Made with ❤️ by 🍀☀🌕🌥 🌊
</p>
