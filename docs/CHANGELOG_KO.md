# 변경 이력

Common System 프로젝트의 모든 주요 변경 사항이 이 파일에 문서화됩니다.

이 문서는 [Keep a Changelog](https://keepachangelog.com/ko/1.0.0/) 형식을 따르며,
프로젝트는 [Semantic Versioning](https://semver.org/lang/ko/)을 준수합니다.

> **언어:** [English](CHANGELOG.md) | **한국어**

---

## [Unreleased]

### Added
- **Deprecated API 문서화** (#213)
  - 모든 deprecated API를 문서화한 새로운 `docs/DEPRECATION.md`
  - Before/After 예제가 포함된 마이그레이션 가이드
  - 컴파일러 경고 억제 방법
  - deprecation 경고를 위한 CI 통합 가이드
  - v3.0.0 API 제거 타임라인
  - 영어 및 한국어 버전 제공

- **통합 버전 관리 시스템** (#204)
  - 버전 호환성 매트릭스가 포함된 새로운 `docs/COMPATIBILITY.md`
  - 7개 KCENON 에코시스템 시스템의 버전 요구사항 문서화
  - 시스템 관계를 보여주는 의존성 그래프
  - 종속 시스템을 위한 CMake 버전 검증 예제
  - 안전한 업그레이드 순서 가이드라인
  - 알려진 비호환성 및 해결 방법 문서화
  - 영어 및 한국어 버전 제공
  - 에코시스템 전체 표준화를 위한 새로운 `docs/contributing/CHANGELOG_TEMPLATE.md`

- **타입 검증을 위한 C++20 Concepts** (#192)
  - 포괄적인 concept 정의를 포함한 새로운 `include/kcenon/common/concepts/` 디렉토리
  - `core.h`: Result/Optional concepts (Resultable, Unwrappable, Mappable, Chainable, MonadicResult)
  - `callable.h`: Callable concepts (Invocable, VoidCallable, Predicate, JobLike, ExecutorLike)
  - `event.h`: Event bus concepts (EventType, EventHandler, EventFilter, TimestampedEvent)
  - `service.h`: DI concepts (ServiceInterface, ServiceImplementation, ServiceFactory)
  - `container.h`: Container concepts (Container, SequenceContainer, CircularBuffer)
  - `concepts.h`: 모든 concepts를 위한 통합 헤더
  - concept 호환성을 위해 `Result<T>`에 `value_type` 및 `error_type` 타입 별칭 추가
  - concept 호환성을 위해 `Optional<T>`에 `value_type` 타입 별칭 추가
  - `simple_event_bus`에 concepts 적용 (publish, subscribe, subscribe_filtered 메서드)
  - `IServiceContainer`에 concepts 적용 (register_type, register_factory, register_simple_factory 메서드)

### Benefits
- **더 명확한 컴파일 타임 에러**: 템플릿 에러가 SFINAE 실패 대신 concept 위반을 표시
- **자체 문서화 코드**: concepts가 타입 요구사항을 명시적으로 표현
- **보일러플레이트 감소**: `std::enable_if` 및 `static_assert` 노이즈 제거
- **향상된 IDE 지원**: 개선된 자동 완성 및 타입 힌트

---

## [2.0.0] - 2025-12-07

### Added
- **Cross-System 통합 테스트**: 런타임 바인딩 패턴을 위한 포괄적인 테스트 (#178)
  - GlobalLoggerRegistry 통합 테스트 (7개 테스트)
    - MultipleSystemsShareLogger: 여러 시스템이 통합 로거를 공유하는지 검증
    - ThreadSafeAccess: 100 스레드 × 10 로그 동시 접근 테스트
    - ConcurrentRegistrationAndRetrieval: 등록/조회 혼합 작업
    - FactoryBasedLazyInitialization: 첫 접근 시에만 팩토리 호출 확인
    - NullLoggerFallback: 미등록 로거에 대한 안전한 폴백
    - StressTestHighConcurrency: 50 스레드 × 1000 혼합 작업
    - CleanupAfterHeavyUsage: 대량 작업 후 적절한 정리
  - SystemBootstrapper 통합 테스트 (6개 테스트)
  - CrossSystem 통합 테스트 (3개 테스트)
  - LevelConversion 테스트 (4개 테스트)
- **SystemBootstrapper**: 애플리케이션 레벨 시스템 초기화를 위한 Fluent API (#176)
  - 지연 초기화를 위한 팩토리 기반 로거 등록
  - `with_default_logger()` 및 `with_logger()`를 통한 기본/명명된 로거 지원
  - `on_initialize()`를 통한 초기화 훅 (등록 순서대로 호출)
  - `on_shutdown()`을 통한 종료 훅 (역순으로 호출 - LIFO)
  - 소멸 시 자동 종료를 통한 RAII 지원
  - 소유권 이전을 위한 이동 의미론 지원
  - 중복 초기화/종료 방지
  - 재구성 시나리오를 위한 `reset()` 메서드
- **GlobalLoggerRegistry**: 런타임 로거 바인딩을 위한 스레드 안전 싱글톤 레지스트리 (#174)
- **통합 로깅 함수 및 매크로** (#175)
- **ILogger source_location 지원** (#177)
- 생태계 전반의 포괄적인 문서 통일화
- Keep a Changelog 형식을 따르는 표준화된 CHANGELOG

### Deprecated
- **ILogger 레거시 file/line/function 메서드** (#177)
  - `log(log_level, const std::string&, const std::string&, int, const std::string&)` 지원 중단
  - `log(log_level, std::string_view, const source_location&)` 사용 권장
  - v3.0.0에서 제거 예정

### Removed
- **BREAKING**: `Result<T>::is_uninitialized()` 메서드 제거
- **BREAKING**: 지원 중단된 Result 팩토리 함수 제거 (#180)
  - `Ok<T>(value)` - 소문자 `ok<T>(value)` 사용
  - `Err<T>(message)` 및 `Err<T>(code, message, module)` - `make_error<T>()` 사용
- **BREAKING**: 레거시 매크로 별칭 제거 (#180)
  - `RETURN_IF_ERROR` - `COMMON_RETURN_IF_ERROR` 사용
  - `ASSIGN_OR_RETURN` - `COMMON_ASSIGN_OR_RETURN` 사용
  - `RETURN_ERROR_IF` - `COMMON_RETURN_ERROR_IF` 사용
  - `RETURN_ERROR_IF_WITH_DETAILS` - `COMMON_RETURN_ERROR_IF_WITH_DETAILS` 사용
- **BREAKING**: 레거시 로깅 매크로 제거 (#180)
  - `THREAD_LOG_TRACE`, `THREAD_LOG_DEBUG`, `THREAD_LOG_INFO`,
    `THREAD_LOG_WARNING`, `THREAD_LOG_ERROR`, `THREAD_LOG_CRITICAL`
  - `LOG_TRACE`, `LOG_DEBUG`, `LOG_INFO`, `LOG_WARNING`, `LOG_ERROR`, `LOG_CRITICAL` 사용

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

[Unreleased]: https://github.com/kcenon/common_system/compare/v2.0.0...HEAD
[2.0.0]: https://github.com/kcenon/common_system/releases/tag/v2.0.0
[1.0.0]: https://github.com/kcenon/common_system/releases/tag/v1.0.0
[0.9.0-beta]: https://github.com/kcenon/common_system/releases/tag/v0.9.0-beta
