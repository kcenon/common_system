[![CI](https://github.com/kcenon/common_system/actions/workflows/ci.yml/badge.svg)](https://github.com/kcenon/common_system/actions/workflows/ci.yml)
[![Code Coverage](https://github.com/kcenon/common_system/actions/workflows/coverage.yml/badge.svg)](https://github.com/kcenon/common_system/actions/workflows/coverage.yml)
[![Static Analysis](https://github.com/kcenon/common_system/actions/workflows/static-analysis.yml/badge.svg)](https://github.com/kcenon/common_system/actions/workflows/static-analysis.yml)
[![Doxygen](https://github.com/kcenon/common_system/actions/workflows/build-Doxygen.yaml/badge.svg)](https://github.com/kcenon/common_system/actions/workflows/build-Doxygen.yaml)
[![License](https://img.shields.io/github/license/kcenon/common_system)](https://github.com/kcenon/common_system/blob/main/LICENSE)

# Common System Project

> **Language:** [English](README.md) | **한국어**

## 개요

Common System Project는 모듈식, 느슨하게 결합된 시스템 아키텍처를 구축하기 위한 핵심 인터페이스와 디자인 패턴을 제공하는 C++20 header-only 라이브러리입니다. 생태계의 초석으로 설계되어, 템플릿 기반 추상화와 인터페이스 주도 설계를 통해 런타임 오버헤드 없이 시스템 모듈 간 원활한 통합을 가능하게 합니다.

> **🏗️ 모듈식 아키텍처**: 의존성 없는 순수 header-only 설계로, executor 패턴, 오류 처리, 이벤트 주도 통신을 위한 범용 인터페이스 제공

> **✅ 최신 업데이트**: 개별 모듈과의 완전한 분리, 포괄적인 Result<T> 패턴 구현, ABI 버전 검사 기능을 포함한 IExecutor 인터페이스 표준화, 통합된 `kcenon::common` 네임스페이스, 이벤트 버스 통합 테스트, 향상된 문서 구조. 모든 인터페이스는 전체 생태계 호환성과 함께 프로덕션 준비 완료

## 🔗 프로젝트 생태계 및 상호 의존성

이 common system은 다른 모든 시스템 모듈이 구축하는 기초 계층으로, 표준화된 인터페이스와 패턴을 제공합니다:

### 핵심 목적
- **인터페이스 표준화**: 모듈 간 통신을 위한 범용 추상화
- **패턴 라이브러리**: 오류 처리 및 이벤트 주도 아키텍처를 위한 재사용 가능한 디자인 패턴
- **빌드 구성**: 모든 모듈을 위한 중앙화된 기능 플래그 및 빌드 옵션
- **제로 커플링**: 구현 의존성이 없는 순수 인터페이스

### 의존 프로젝트
- **[thread_system](https://github.com/kcenon/thread_system)**: 핵심 스레딩 프레임워크
  - 사용: 작업 추상화를 위한 IExecutor 인터페이스
  - 제공: IExecutor의 Thread pool 구현
  - 통합: 원활한 executor 사용을 위한 Adapter 패턴

- **[network_system](https://github.com/kcenon/network_system)**: 비동기 네트워크 라이브러리
  - 사용: 비동기 작업 스케줄링을 위한 IExecutor
  - 이점: 스레딩 백엔드 독립성
  - 통합: 네트워크 작업을 위한 Executor 어댑터

- **[logger_system](https://github.com/kcenon/logger_system)**: 고성능 로깅
  - 사용: 오류 처리를 위한 Result<T>
  - 이점: 예외 없는 오류 전파
  - 통합: 로그 라우팅을 위한 선택적 이벤트 버스

- **[monitoring_system](https://github.com/kcenon/monitoring_system)**: Metrics 및 모니터링
  - 제공: 이벤트 버스 구현
  - 사용: 공통 이벤트 타입 및 인터페이스
  - 통합: 시스템 이벤트의 중앙 허브

- **[container_system](https://github.com/kcenon/container_system)**: 데이터 직렬화
  - 사용: 작업 결과를 위한 Result<T>
  - 이점: 타입 안전한 오류 처리
  - 통합: 공통 오류 코드

- **[database_system](https://github.com/kcenon/database_system)**: 데이터베이스 추상화
  - 사용: 쿼리 결과를 위한 Result<T>
  - 이점: 일관된 오류 처리
  - 통합: IExecutor를 통한 비동기 작업

## 시작하기

### 시스템 요구사항

- **컴파일러**: C++20 호환 (GCC 10+, Clang 10+, MSVC 2019 16.11+)
- **빌드 시스템**: CMake 3.16 이상
- **선택사항**: 의존성 관리를 위한 vcpkg 또는 Conan
- **플랫폼**: Windows, Linux, macOS (모든 아키텍처)

### 빠른 설치

#### 옵션 1: Header-Only 사용 (가장 간단)
```bash
git clone https://github.com/kcenon/common_system.git
# 헤더를 직접 포함 - 빌드 불필요!
```

```cpp
// 코드에서
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
./build.sh --release --install-prefix=/usr/local
sudo cmake --build build --target install
```

## 핵심 컴포넌트

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
        return common::error<Config>(
            common::error_codes::NOT_FOUND,
            "Configuration file not found",
            "config_loader"
        );
    }

    try {
        auto config = parse_json_file(path);
        return common::ok(config);
    } catch (const std::exception& e) {
        return common::error<Config>(
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

## 테스트

프로젝트는 포괄적인 단위 테스트를 포함합니다:

```bash
# 모든 테스트 실행
./test.sh

# 커버리지와 함께 실행
./test.sh --coverage

# 특정 테스트 실행
./test.sh --filter "Result*"

# 벤치마크 테스트
./test.sh --benchmark
```

## 문서

📖 **[전체 문서 인덱스](docs/)** - 사용 가능한 모든 문서 보기

### 빠른 링크

- [아키텍처 가이드](docs/ARCHITECTURE_KO.md) - 시스템 아키텍처 및 설계
- [통합 가이드](docs/INTEGRATION_KO.md) - 통합 예제 및 패턴
- [에러 처리 가이드](docs/ERROR_HANDLING_KO.md) - Result<T> 패턴 및 모범 사례
- [마이그레이션 가이드](docs/MIGRATION_KO.md) - common_system으로 마이그레이션
- [에러 코드 가이드라인](docs/ERROR_CODE_GUIDELINES.md) - 에러 코드 관리
- [RAII 가이드라인](docs/RAII_GUIDELINES_KO.md) - 리소스 관리 패턴
- [스마트 포인터 가이드라인](docs/SMART_POINTER_GUIDELINES_KO.md) - 스마트 포인터 사용법
- [IExecutor 마이그레이션 가이드](docs/IEXECUTOR_MIGRATION_GUIDE.md) - Executor API 마이그레이션

## 성능 벤치마크

| 작업 | 시간 (ns) | 할당 |
|------|-----------|------|
| Result<T> 생성 | 2.3 | 0 |
| Result<T> 오류 확인 | 0.8 | 0 |
| IExecutor submit | 45.2 | 1 |
| Event publish | 12.4 | 0 |

*Intel i7-9700K @ 3.6GHz, GCC 11.2 -O3에서 벤치마크*

## 기여하기

기여를 환영합니다! 가이드라인은 [CONTRIBUTING.md](CONTRIBUTING.md)를 참조하세요.

### 개발 워크플로우

1. 저장소 포크
2. feature 브랜치 생성 (`git checkout -b feature/amazing-feature`)
3. 변경사항 커밋 (`git commit -m 'feat: add amazing feature'`)
4. 브랜치에 푸시 (`git push origin feature/amazing-feature`)
5. Pull Request 생성

## 로드맵

- [x] IExecutor 인터페이스 표준화
- [x] Result<T> 패턴 구현
- [x] Event bus 포워딩
- [x] 중앙화된 빌드 구성
- [x] 호환성 검증을 위한 ABI 버전 검사
- [x] 통합된 `kcenon::common` 네임스페이스
- [x] Task 기반 IExecutor 인터페이스
- [x] 포괄적인 문서 재구성
- [ ] 인터페이스 제약을 위한 C++20 concepts
- [ ] 비동기 패턴을 위한 Coroutine 지원
- [ ] std::expected 마이그레이션 (C++23)
- [ ] 추가 디자인 패턴 (Observer, Command)
- [ ] 패키지 관리자 공식 지원 (vcpkg, Conan)

## 지원

- **이슈**: [GitHub Issues](https://github.com/kcenon/common_system/issues)
- **토론**: [GitHub Discussions](https://github.com/kcenon/common_system/discussions)
- **이메일**: kcenon@naver.com

## 라이선스

이 프로젝트는 BSD 3-Clause License 하에 라이선스됩니다 - 자세한 내용은 [LICENSE](LICENSE) 파일을 참조하세요.

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
