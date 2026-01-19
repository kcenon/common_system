# 변경 이력

Common System 프로젝트의 모든 주요 변경 사항이 이 파일에 문서화됩니다.

이 문서는 [Keep a Changelog](https://keepachangelog.com/ko/1.0.0/) 형식을 따르며,
프로젝트는 [Semantic Versioning](https://semver.org/lang/ko/)을 준수합니다.

> **언어:** [English](CHANGELOG.md) | **한국어**

---

## [Unreleased]

### Removed
- **Deprecated THREAD_LOG_* 매크로 제거** (#289)
  - v3.0.0에서 제거 예정이었던 deprecated 로깅 매크로 제거:
    - `THREAD_LOG_TRACE(msg)` → `LOG_TRACE(msg)` 사용
    - `THREAD_LOG_DEBUG(msg)` → `LOG_DEBUG(msg)` 사용
    - `THREAD_LOG_INFO(msg)` → `LOG_INFO(msg)` 사용
    - `THREAD_LOG_WARNING(msg)` → `LOG_WARNING(msg)` 사용
    - `THREAD_LOG_ERROR(msg)` → `LOG_ERROR(msg)` 사용
    - `THREAD_LOG_CRITICAL(msg)` → `LOG_CRITICAL(msg)` 사용
  - **BREAKING CHANGE**: 업그레이드 전 `LOG_*` 매크로로 마이그레이션 필요
  - 마이그레이션 방법은 `docs/DEPRECATION.kr.md` 참조

### Breaking Changes
- **ILogger 인터페이스에서 deprecated file/line/function log() 메서드 제거** (#217)
  - 제거됨: `virtual VoidResult log(log_level, const std::string&, const std::string& file, int line, const std::string& function)`
  - 대신 `log(log_level, std::string_view, const source_location&)` 사용
  - 커스텀 ILogger 구현은 이 메서드 오버라이드를 제거해야 함
  - 마이그레이션 가이드는 `docs/DEPRECATION.kr.md` 참조

### Added
- **C++20 모듈 마이그레이션 Phase 3 시작** (#275)
  - 전체 8개 시스템에서 Phase 1, 1.5, Phase 2 이슈 모두 완료
  - Phase 3 (안정화) 이슈 생성:
    - 모듈 빌드 테스트 검증
    - 성능 벤치마킹
    - 문서 업데이트
    - 컴파일러 호환성 검증
  - 전체 마이그레이션 추적은 EPIC #256 참조

- **통합 Metric Collection 인터페이스** (#234)
  - 메트릭 추상화를 위한 새로운 `include/kcenon/common/interfaces/monitoring/` 디렉토리
  - `IMetricCollector`: monitoring_system 직접 의존성 없이 크로스 모듈 메트릭 리포팅을 위한 인터페이스
    - `increment()`: Counter 메트릭 (단조 증가 값)
    - `gauge()`: Gauge 메트릭 (증감 가능한 값)
    - `histogram()`: Histogram 메트릭 (값의 분포)
    - `timing()`: Timing 메트릭 (지속 시간 측정)
  - `scoped_timer`: 자동 타이밍 측정을 위한 RAII 헬퍼
    - 생성자에서 소멸자까지의 경과 시간 측정
    - 우발적인 이중 보고 방지를 위해 복사/이동 불가
    - 타이머 실행 중 시간 확인을 위한 `elapsed()` 메서드
  - `null_metric_collector`: 비활성화된 메트릭을 위한 No-op 구현
  - `IMetricCollectorProvider`: 의존성 주입 지원을 위한 인터페이스
  - `monitoring.h`: 모든 monitoring 인터페이스를 위한 Umbrella 헤더
  - 기존 `IMonitor` 인터페이스와의 상호 보완:
    - `IMonitor`: Pull 기반 (상태 읽기, 스냅샷 가져오기)
    - `IMetricCollector`: Push 기반 (실시간 메트릭 발행)
  - 런타임 의존성 없는 헤더 전용 설계
  - 인터페이스 개념을 위한 포괄적인 단위 테스트 (31개 테스트)

- **통합 Transport 인터페이스** (#233)
  - transport 추상화를 위한 새로운 `include/kcenon/common/interfaces/transport/` 디렉토리
  - `IHttpClient`: 의존성 주입을 위한 HTTP 클라이언트 인터페이스
    - 빌더 패턴을 지원하는 `http_request`/`http_response` 구조체
    - 상태 헬퍼: `is_success()`, `is_client_error()`, `is_server_error()`
    - `null_http_client`: 비활성화된 transport를 위한 No-op 구현
  - `IUdpClient`: 메트릭 리포팅 및 저지연 메시징을 위한 UDP 클라이언트 인터페이스
    - 연결 모드 (`connect()` + `send()`): 커널 라우팅 최적화
    - 비연결 모드 (`send_to()`): ad-hoc 패킷 전송
    - 통계 추적: `packets_sent`, `bytes_sent`, `send_failures`
    - 메트릭 문자열 전송을 위한 String 편의 메서드
    - `null_udp_client`: 비활성화된 transport를 위한 No-op 구현
  - `transport.h`: 모든 transport 인터페이스를 위한 Umbrella 헤더
  - 런타임 의존성 없는 헤더 전용 설계
  - 인터페이스 개념을 위한 포괄적인 단위 테스트 (26개 테스트)

- **KCENON_WITH_COMMON_SYSTEM 플래그** (#230)
  - `feature_system_deps.h`에 `KCENON_WITH_COMMON_SYSTEM=1` 추가
  - `feature_flags.h` 포함 시 자동 정의
  - CMake 컴파일 정의를 통해 종속 프로젝트에 전파
  - 다운스트림 프로젝트(예: network_system)에서 ABI 비호환성 방지
  - `kcenon_configure_features()` 함수에 `COMMON_SYSTEM` 옵션 추가

- **통합 Feature Flag 헤더** (#224)
  - 모든 feature 감지를 위한 메인 진입점인 새로운 `feature_flags.h`
  - `feature_flags_core.h`: 전처리기 헬퍼, 컴파일러/플랫폼 감지 (KCENON_COMPILER_*, KCENON_PLATFORM_*, KCENON_HAS_CPP*)
  - `feature_detection.h`: C++ 표준 라이브러리 기능 감지
    - KCENON_HAS_SOURCE_LOCATION (C++20 std::source_location)
    - KCENON_HAS_JTHREAD / KCENON_HAS_STOP_TOKEN (C++20 협력적 스레딩)
    - KCENON_HAS_FORMAT, KCENON_HAS_SPAN, KCENON_HAS_RANGES (C++20)
    - KCENON_HAS_EXPECTED, KCENON_HAS_STACKTRACE (C++23)
    - KCENON_HAS_CONCEPTS, KCENON_HAS_COROUTINES
  - `feature_system_deps.h`: 시스템 모듈 통합 플래그 (KCENON_WITH_THREAD_SYSTEM, KCENON_WITH_LOGGER_SYSTEM 등)
  - KCENON_ENABLE_LEGACY_ALIASES를 통한 레거시 별칭 지원 (기본값: 활성화)
  - `features.cmake`에 `export_kcenon_features()` 함수 추가
  - 통합 feature 감지를 사용하도록 `source_location.h` 리팩토링

- **다운스트림 시스템 Deprecation 알림** (#220)
  - 모든 의존 시스템에 마이그레이션 추적 이슈 생성
  - thread_system, logger_system, monitoring_system, pacs_system, database_system에 알림 완료
  - 각 이슈에 마이그레이션 가이드 링크 및 v3.0.0 제거 타임라인 포함
  - `docs/DEPRECATION.md`에 알림 상태 테이블 업데이트

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

### Fixed
- **크로스 컴파일러 모듈 빌드 문제 수정** (#279)
  - using 선언을 export 네임스페이스 밖으로 이동하여 MSVC C1117 심볼 중복 해결
  - logging 모듈에서 `ok()` 호출에 명시적 네임스페이스 지정으로 GCC-14 잠재적 ICE 수정
  - `COMMON_BUILD_INTEGRATION_TESTS=OFF` 추가로 Clang-16 CMake 구성 오류 수정
  - `typeid()` 연산자를 위해 patterns.cppm에 누락된 `<typeinfo>` 헤더 추가

- **MSVC 모듈 빌드 심볼 중복 오류 수정** (#283)
  - MSVC 모듈 빌드에서 발생하던 C1117 오류 "symbol 'error_info' has already been defined" 해결
  - `interfaces/core.cppm`에서 타입을 재정의하는 대신 `result.core` 파티션을 import하도록 변경
  - interfaces 모듈에서 `error_info`, `Result<T>`, `source_location`의 중복 정의 제거
  - interfaces 네임스페이스에서 적절한 타입 접근을 위한 using 선언 추가

- **Clang-16 모듈 빌드 VoidResult 가시성 오류 수정** (#283)
  - VoidResult 타입 가시성을 위해 `logging.cppm`에 누락된 `import :result.core;` 추가
  - Clang 모듈 빌드 오류 해결: "declaration of 'VoidResult' must be imported from module 'kcenon.common:result.core'"

- **GCC-14 모듈 빌드 Internal Compiler Error 수정** (#283)
  - `interfaces/core.cppm`을 별도 파티션으로 분리: `interfaces/logger.cppm`과 `interfaces/executor.cppm`
  - GCC 14 ICE는 `logging.cppm`이 `interfaces.core`를 통해 `IExecutor`를 import할 때 가상 소멸자 처리에서 트리거됨
  - `logging.cppm`이 이제 `interfaces.logger` 파티션만 import하여 문제가 되는 `IExecutor` 코드 경로를 우회
  - 하위 호환성 유지: `interfaces.core`가 `logger`와 `executor` 파티션을 모두 re-export

- **테스트 코드의 모든 컴파일러 경고 수정** (#245)
  - `config_watcher.h`의 멤버 초기화 순서 경고 수정
  - `[[maybe_unused]]` 속성으로 미사용 매개변수 경고 수정
  - const 변수에 대한 불필요한 람다 캡처 제거
  - 테스트 코드의 미사용 변수 제거

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
