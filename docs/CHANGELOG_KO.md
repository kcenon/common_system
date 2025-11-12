# 변경 이력

Common System 프로젝트의 모든 주요 변경 사항이 이 파일에 문서화됩니다.

이 문서는 [Keep a Changelog](https://keepachangelog.com/ko/1.0.0/) 형식을 따르며,
프로젝트는 [Semantic Versioning](https://semver.org/lang/ko/)을 준수합니다.

> **언어:** [English](CHANGELOG.md) | **한국어**

---

## [Unreleased]

### Added
- 생태계 전반의 포괄적인 문서 통일화
- Keep a Changelog 형식을 따르는 표준화된 CHANGELOG

---

## [1.0.0] - 2025-10-09

### Added
- **Result<T> 패턴**: Monadic 연산(`map`, `and_then`, `or_else`)을 포함한 완전한 구현
- **오류 코드 레지스트리**: 컴파일 타임 검증이 포함된 중앙화된 오류 코드 시스템
  - common_system: -1 ~ -99
  - thread_system: -100 ~ -199
  - logger_system: -200 ~ -299
  - monitoring_system: -300 ~ -399
  - container_system: -400 ~ -499
  - database_system: -500 ~ -599
  - network_system: -600 ~ -699
- **IExecutor 인터페이스**: 범용 작업 실행 추상화
- **ILogger 인터페이스**: 표준 로깅 인터페이스
- **IMonitor 인터페이스**: 표준 모니터링 인터페이스
- **IDatabase 인터페이스**: 데이터베이스 작업 인터페이스
- **Event Bus 패턴**: 느슨한 결합을 위한 게시-구독 메커니즘
- **Smart Adapters**: 원활한 타입 변환을 위한 타입 어댑터
- **Factory 패턴**: 표준 생성 패턴

### Changed
- 모놀리식 thread_system에서 독립적인 기반 라이브러리로 분리
- 런타임 오버헤드 제로를 위한 순수 헤더 전용 설계로 전환
- 모듈 간 호환성을 위한 모든 인터페이스 표준화

### Fixed
- 인터페이스 계약 모호성 해결
- 모듈 간 오류 코드 충돌 해결
- 공유 추상화의 스레드 안전성 문제 해결

---

## [0.9.0-beta] - 2025-09-13

### Added
- thread_system으로부터 초기 분리
- 기본 Result<T> 구현
- 핵심 인터페이스 정의 (IExecutor, ILogger)

### Changed
- 기반 계층 아키텍처 확립
- 독립적인 저장소 구조 생성

---

## 버전 번호 체계

이 프로젝트는 Semantic Versioning을 사용합니다:
- **MAJOR** 버전: 호환되지 않는 API 변경
- **MINOR** 버전: 하위 호환되는 기능 추가
- **PATCH** 버전: 하위 호환되는 버그 수정

---

## 마이그레이션 가이드

### 1.0.0으로 마이그레이션

#### thread_system 통합 인터페이스에서

**이전** (thread_system 1.x):
```cpp
#include <thread/interfaces/executor_interface.h>
using namespace thread_system::interfaces;
```

**이후** (common_system 1.0):
```cpp
#include <kcenon/common/interfaces/executor_interface.h>
using namespace common::interfaces;
```

#### Result<T> 패턴 적용

**이전** (예외 기반):
```cpp
Config load_config(const std::string& path) {
    if (!exists(path)) {
        throw std::runtime_error("Config not found");
    }
    return parse_config(path);
}
```

**이후** (Result<T> 패턴):
```cpp
#include <kcenon/common/patterns/result.h>

common::Result<Config> load_config(const std::string& path) {
    if (!exists(path)) {
        return common::error<Config>(
            common::error_codes::NOT_FOUND,
            "Configuration file not found",
            "config_loader"
        );
    }
    return common::ok(parse_config(path));
}
```

---

## 지원

- **이슈**: [GitHub Issues](https://github.com/kcenon/common_system/issues)
- **토론**: [GitHub Discussions](https://github.com/kcenon/common_system/discussions)
- **이메일**: kcenon@naver.com

---

## 라이선스

이 프로젝트는 BSD 3-Clause 라이선스를 따릅니다 - 자세한 내용은 [LICENSE](LICENSE) 파일을 참조하세요.

---

[Unreleased]: https://github.com/kcenon/common_system/compare/v1.0.0...HEAD
[1.0.0]: https://github.com/kcenon/common_system/releases/tag/v1.0.0
[0.9.0-beta]: https://github.com/kcenon/common_system/releases/tag/v0.9.0-beta
