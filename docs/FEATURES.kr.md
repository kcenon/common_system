# Common System - 상세 기능

**언어:** [English](FEATURES.md) | **한국어**

이 문서는 Common System 프로젝트에서 사용 가능한 모든 기능에 대한 포괄적인 세부 정보를 제공합니다.

---

## 목차

- [핵심 장점 및 이점](#핵심-장점-및-이점)
- [핵심 컴포넌트](#핵심-컴포넌트)
- [복원력 패턴](#복원력-패턴)
- [의존성 주입 및 부트스트랩](#의존성-주입-및-부트스트랩)
- [통합 기능](#통합-기능)
- [프로덕션 품질 기능](#프로덕션-품질-기능)
- [오류 처리 기반](#오류-처리-기반)

---

## 핵심 장점 및 이점

### 🚀 **성능 우수성**

common_system은 여러 핵심 설계 결정을 통해 제로 오버헤드 추상화를 달성합니다:

- **제로 오버헤드 추상화**: 컴파일 타임 해석을 통한 템플릿 기반 인터페이스로 common_system 인터페이스 사용이 직접 작성한 코드와 비교하여 런타임 비용이 없음
- **헤더 전용 설계**: 라이브러리 링킹이 필요 없어 컴파일러가 최적화를 위한 완전한 가시성을 가짐. 더 나은 인라이닝, 데드 코드 제거, 링크 타임 최적화 가능
- **캐시 친화적 패턴**: 최소한의 간접 참조와 최적의 메모리 레이아웃으로 캐시 미스 감소 및 성능 향상
- **컴파일 타임 최적화**: 핫 패스에 대한 완전한 인라이닝 잠재력으로 컴파일러가 추상화 경계를 넘어 최적화 가능

**성능 영향:**
- 템플릿 인스턴스화는 컴파일 타임에 발생하며 런타임 오버헤드 없음
- 성능 중요 경로에서 가상 함수 호출 없음
- 컴파일러가 모든 추상화 레이어를 통해 최적화 가능
- 링크 타임 최적화(LTO)가 모든 추상화 오버헤드 제거 가능

### 🛡️ **프로덕션 등급 신뢰성**

common_system의 모든 인터페이스와 패턴은 프로덕션 신뢰성을 고려하여 설계되었습니다:

- **타입 안전 인터페이스**: 컴파일 타임에 실수를 포착하여 일반적인 런타임 오류 방지
  - 템플릿 제약이 올바른 사용 강제
  - 컴파일 타임 검사가 오용 방지
  - 안전하지 않은 캐스트나 타입 소거 없음

- **Result<T> 패턴**: 예외 없이 명시적 오류 처리
  - 호출 사이트에서 오류 처리 강제
  - 예외로 인한 숨겨진 제어 흐름 없음
  - 명확한 오류 전파 경로
  - 오류가 발생하지 않을 때 제로 비용

- **RAII 준수**: 표준 패턴을 통한 리소스 관리
  - 모든 리소스가 스마트 포인터 사용
  - 스코프 종료 시 자동 정리
  - 설계상 예외 안전
  - 수동 메모리 관리 없음

- **스레드 안전 설계**: 모든 인터페이스가 동시 사용에 안전
  - 가능한 경우 불변 타입
  - 명확한 동시성 보장
  - 적절한 경우 락프리 알고리즘
  - 설계상 데이터 레이스 없음

### 🔧 **개발자 생산성**

Common_system은 개발자 경험과 생산성을 우선시합니다:

- **자체 문서화 인터페이스**: 포괄적인 문서와 함께 명확한 계약
  - 표현력 있는 타입 이름
  - 포괄적인 Doxygen 주석
  - 헤더의 사용 예제
  - 명확한 오류 메시지

- **최소 보일러플레이트**: 깔끔한 API 설계로 코드 오버헤드 감소
  - 일반적인 패턴을 위한 플루언트 인터페이스
  - 장황함을 줄이는 타입 추론
  - 합리적인 기본값
  - 일반적인 사용 사례를 위한 헬퍼 함수

- **모킹 가능한 추상화**: 인터페이스 주입을 통한 쉬운 테스팅
  - 순수 가상 인터페이스
  - 의존성 주입 친화적
  - 모의 구현 포함
  - 테스트 헬퍼 제공

- **IDE 친화적**: 완전한 IntelliSense 및 자동 완성 지원
  - 완전한 타입 정보
  - 템플릿 인스턴스화 힌트
  - 정의로 이동 작동
  - 리팩토링 도구 호환

### 🌐 **범용 호환성**

모든 현대 C++ 환경에서 작동하도록 설계되었습니다:

- **C++17 표준**: 더 넓은 컴파일러 지원으로 하위 호환성
  - GCC 7+ (7, 9, 11, 13에서 테스트됨)
  - Clang 5+ (5, 10, 14, 16에서 테스트됨)
  - MSVC 2017+ (2017, 2019, 2022에서 테스트됨)
  - 컴파일러 특정 확장 필요 없음

- **C++20 기능**: 사용 가능한 경우 향상된 기능을 위한 선택적 지원
  - 더 나은 오류 진단을 위한 `std::source_location`
  - 더 명확한 템플릿 제약을 위한 Concepts
  - C++17 등가물로의 우아한 폴백

- **크로스 플랫폼**: 수정 없이 Windows, Linux, macOS
  - 플랫폼에 구애받지 않는 인터페이스
  - 헤더에 OS 특정 코드 없음
  - 플랫폼 간 일관된 동작
  - 모든 주요 플랫폼에서 CI 테스팅

- **빌드 시스템 독립**: 모든 빌드 시스템과 작동
  - CMake 통합 제공
  - Bazel 지원 가능
  - 일반 Make 호환
  - 헤더 전용이므로 빌드 필요 없음

### 📈 **엔터프라이즈 준비 기능**

대규모 엔터프라이즈 배포를 위해 구축됨:

- **인터페이스 버전 관리**: 신중한 설계를 통한 하위 호환성
  - 컴파일 타임에 ABI 버전 검사
  - 인터페이스의 시맨틱 버전 관리
  - 이전 API에 대한 사용 중단 경고
  - 마이그레이션 가이드 제공

- **중앙 집중식 구성**: 모든 모듈에 걸쳐 통합된 빌드 플래그
  - 선택적 컴포넌트를 위한 기능 플래그
  - 일관된 빌드 구성
  - 사용자 정의를 위한 재정의 포인트
  - 구성 유효성 검사

- **포괄적인 테스팅**: 업계 표준 프레임워크로 완전한 테스트 커버리지
  - Google Test로 유닛 테스트
  - 실제 컴포넌트로 통합 테스트
  - 성능 검증을 위한 벤치마크 테스트
  - 새니타이저 커버리지 (ASan, TSan, UBSan)

---

## 핵심 컴포넌트

### IExecutor 인터페이스

IExecutor 인터페이스는 태스크 실행을 위한 범용 추상화를 제공하여 특정 스레딩 구현으로부터 완전한 독립성을 가능하게 합니다.

**주요 기능:**

- **스레딩 백엔드 독립성**: 한 번 작성하고 모든 실행기에서 실행
- **태스크 기반 추상화**: 람다 함수, 함수 객체 또는 std::function 제출
- **Future 기반 결과**: std::future<T>를 통한 타입 안전 비동기 결과
- **예외 안전성**: future를 통해 예외 전파
- **수명 관리**: RAII를 통한 자동 정리

**인터페이스 메서드:**

```cpp
namespace kcenon::common::interfaces {
    class IExecutor {
    public:
        virtual ~IExecutor() = default;

        // 태스크를 제출하고 결과에 대한 future 반환
        template<typename F, typename... Args>
        auto submit(F&& func, Args&&... args)
            -> std::future<std::invoke_result_t<F, Args...>>;

        // 결과 반환 없이 태스크 실행
        template<typename F, typename... Args>
        void execute(F&& func, Args&&... args);

        // 실행기 정보 조회
        virtual size_t thread_count() const = 0;
        virtual bool is_running() const = 0;
    };
}
```

**사용 패턴:**

1. **Fire and forget 실행**:
```cpp
executor->execute([]() {
    // 백그라운드 태스크
    process_data();
});
```

2. **결과가 있는 비동기**:
```cpp
auto future = executor->submit([]() {
    return compute_value();
});
auto result = future.get();
```

3. **비동기 작업 체이닝**:
```cpp
auto future1 = executor->submit(load_data);
auto future2 = executor->submit([future1 = std::move(future1)]() mutable {
    auto data = future1.get();
    return process(data);
});
```

**통합:**

IExecutor 인터페이스는 다음에 의해 구현됩니다:
- 어댑터 패턴을 통한 `thread_system::thread_pool`
- 비동기 I/O 작업을 위한 `network_system`
- 특수 실행기를 위한 커스텀 구현

### Result<T> 패턴

예외 없이 타입 안전 오류 처리를 위한 Result 모나드 패턴의 포괄적인 구현입니다.

**설계 철학:**

- 모든 호출 사이트에서 명시적 오류 처리
- 예외로 인한 숨겨진 제어 흐름 없음
- 모나딕 연산을 통한 함수형 합성
- 성공 시 제로 오버헤드
- 컴파일 타임에 완전한 타입 안전성

**핵심 연산:**

```cpp
namespace kcenon::common {
    template<typename T>
    class Result {
    public:
        // 팩토리 메서드
        static Result<T> ok(T value);
        static Result<T> error(ErrorInfo info);

        // 상태 쿼리
        bool is_ok() const noexcept;
        bool is_error() const noexcept;
        explicit operator bool() const noexcept;

        // 값 접근 (오류 시 throw)
        T& value() &;
        const T& value() const &;
        T&& value() &&;

        // 값 또는 기본값 접근
        T value_or(T&& default_value) const&;
        T value_or(T&& default_value) &&;

        // 오류 접근
        const ErrorInfo& error() const;

        // 모나딕 연산
        template<typename F>
        auto map(F&& func) const& -> Result<std::invoke_result_t<F, const T&>>;

        template<typename F>
        auto and_then(F&& func) const& -> std::invoke_result_t<F, const T&>;

        template<typename F>
        auto or_else(F&& func) const& -> Result<T>;
    };
}
```

**모나딕 합성:**

Result<T> 패턴은 함수형 프로그래밍 패러다임을 지원합니다:

```cpp
// Map: 성공 값 변환
auto result = load_config("app.conf")
    .map([](const Config& cfg) {
        return cfg.with_defaults();
    });

// AndThen: Result를 반환하는 작업 체이닝
auto result = load_config("app.conf")
    .and_then(validate_config)
    .and_then(apply_schema);

// OrElse: 오류 시 폴백 제공
auto result = load_config("app.conf")
    .or_else([](const ErrorInfo& err) {
        log_error(err);
        return load_default_config();
    });

// 전체 합성
auto result = load_config("app.conf")
    .and_then(validate_config)
    .map(apply_defaults)
    .and_then(connect_to_db)
    .or_else(use_fallback_db);
```

**오류 컨텍스트:**

풍부한 오류 정보 포함:

```cpp
struct ErrorInfo {
    int code;                    // 레지스트리의 오류 코드
    std::string message;         // 사람이 읽을 수 있는 메시지
    std::string source;          // 소스 모듈/함수
    std::string file;            // 소스 파일 (가능한 경우)
    int line;                    // 소스 라인 (가능한 경우)
    std::optional<std::string>
        additional_context;      // 추가 컨텍스트
};
```

---

## 복원력 패턴

`kcenon::common::resilience` 네임스페이스는 분산 시스템을 위한 프로덕션 등급 장애 허용 프리미티브를 제공합니다. 모든 복원력 컴포넌트에 접근하려면 우산 헤더 `<kcenon/common/resilience/resilience.h>`를 포함하세요.

### 서킷 브레이커

서킷 브레이커 패턴은 장애가 발생한 서비스에 대한 요청을 일시적으로 차단하여 연쇄 장애를 방지하고, 서비스가 복구할 시간을 제공합니다.

**상태 머신:**

```
CLOSED ──(실패 임계값 초과)──► OPEN
  ▲                              │
  │                              │ (타임아웃 만료)
  │                              ▼
  └──(성공 임계값 달성)──── HALF_OPEN
                               │
                               └──(실패 발생)──► OPEN
```

| 상태 | 동작 |
|------|------|
| `CLOSED` | 정상 작동. 요청이 통과하며, 실패가 슬라이딩 타임 윈도우 내에서 추적됨. |
| `OPEN` | 실패 임계값 초과. 모든 요청이 즉시 거부됨. 타임아웃 후 `HALF_OPEN`으로 전환. |
| `HALF_OPEN` | 복구 테스트. 제한된 수의 프로브 요청이 허용됨. 성공 시 회로를 닫고, 실패 시 다시 열림. |

**설정 (`circuit_breaker_config`):**

```cpp
namespace kcenon::common::resilience {

struct circuit_breaker_config {
    // 회로를 트립하기 위한 실패 횟수 (CLOSED -> OPEN)
    std::size_t failure_threshold = 5;

    // 회로를 닫기 위한 성공 횟수 (HALF_OPEN -> CLOSED)
    std::size_t success_threshold = 2;

    // 실패 추적을 위한 슬라이딩 타임 윈도우 (윈도우 밖의 실패는 만료)
    std::chrono::milliseconds failure_window = std::chrono::seconds(60);

    // OPEN에서 HALF_OPEN으로 전환하기 전 쿨다운
    std::chrono::milliseconds timeout = std::chrono::seconds(30);

    // HALF_OPEN 상태에서 허용되는 최대 프로브 요청 수
    std::size_t half_open_max_requests = 3;
};

}
```

**핵심 API (`circuit_breaker`):**

```cpp
namespace kcenon::common::resilience {

class circuit_breaker : public interfaces::IStats {
public:
    explicit circuit_breaker(circuit_breaker_config config = {});

    // 요청 허용 여부 확인
    [[nodiscard]] auto allow_request() -> bool;

    // 작업 결과 기록
    auto record_success() -> void;
    auto record_failure(const std::exception* e = nullptr) -> void;

    // 현재 상태 조회
    [[nodiscard]] auto get_state() const -> circuit_state;

    // RAII 가드로 자동 성공/실패 기록
    [[nodiscard]] auto make_guard() -> guard;

    // IStats 인터페이스 - 관찰 가능성
    [[nodiscard]] auto get_stats() const
        -> std::unordered_map<std::string, interfaces::stats_value> override;
    [[nodiscard]] auto to_json() const -> std::string override;
    [[nodiscard]] auto name() const -> std::string_view override;
};

}
```

**RAII 가드:**

`circuit_breaker::guard` 클래스는 `record_success()`가 명시적으로 호출되지 않으면 소멸 시 자동으로 실패를 기록합니다. 이를 통해 예외를 발생시키는 작업이 올바르게 추적됩니다.

```cpp
class circuit_breaker::guard {
public:
    explicit guard(circuit_breaker& breaker);
    ~guard();  // record_success()가 호출되지 않았으면 실패 기록

    auto record_success() -> void;

    // 복사 불가, 이동 불가
    guard(const guard&) = delete;
    guard& operator=(const guard&) = delete;
};
```

**사용 예제:**

```cpp
#include <kcenon/common/resilience/resilience.h>

using namespace kcenon::common::resilience;

// 브레이커 설정
circuit_breaker_config config{
    .failure_threshold = 5,
    .success_threshold = 2,
    .failure_window = std::chrono::seconds(60),
    .timeout = std::chrono::seconds(30),
    .half_open_max_requests = 3
};
circuit_breaker breaker(config);

// 패턴 1: 수동 확인 및 기록
if (!breaker.allow_request()) {
    return make_error("Service unavailable - circuit is open");
}
try {
    auto result = call_remote_service();
    breaker.record_success();
    return result;
} catch (const std::exception& e) {
    breaker.record_failure(&e);
    throw;
}

// 패턴 2: RAII 가드 (권장)
if (breaker.allow_request()) {
    auto guard = breaker.make_guard();
    auto result = call_remote_service();
    guard.record_success();  // 자동 실패 기록 방지
    return result;
}
// call_remote_service()가 예외를 발생시키면 ~guard()가 자동으로 실패 기록
```

**관찰 가능성:**

서킷 브레이커는 `IStats`를 구현하여 실시간 메트릭을 제공합니다:

```cpp
auto stats = breaker.get_stats();
// 반환: current_state, failure_count, consecutive_successes,
//       half_open_requests, failure_threshold, is_open

auto json = breaker.to_json();
// 모든 통계의 JSON 표현 반환
```

**실패 윈도우:**

`failure_window` 클래스는 실패 추적을 위한 슬라이딩 타임 윈도우를 제공합니다. 설정된 `failure_window` 기간보다 오래된 실패는 자동으로 만료되어 임계값 계산에 포함되지 않습니다.

**스레드 안전성:**
- `circuit_breaker`와 `failure_window`의 모든 public 메서드는 스레드 안전합니다.
- 상태 전환은 내부 동기화로 보호됩니다.
- 여러 스레드에서 동시 접근 안전합니다.

---

## 의존성 주입 및 부트스트랩

common_system은 서비스 수명 관리, 의존성 연결, 애플리케이션 생명주기 제어를 위한 의존성 주입(DI) 컨테이너와 부트스트래핑 유틸리티를 제공합니다.

### 서비스 컨테이너

`service_container` (`kcenon::common::di`)는 팩토리 기반 등록, 다중 수명 정책, 스코프 컨테이너, 순환 의존성 감지를 지원하는 스레드 안전 DI 컨테이너입니다.

**서비스 수명 (`service_lifetime`):**

| 수명 | 동작 | 사용 사례 |
|------|------|----------|
| `singleton` | 전역적으로 공유되는 하나의 인스턴스; 첫 번째 해석 시 생성. | 로거, 설정, 상태 없는 서비스. |
| `transient` | 모든 해석 요청마다 새 인스턴스 생성. | 소비자별 상태 보유 서비스. |
| `scoped` | `IServiceScope`당 하나의 인스턴스; 해당 스코프 내에서 공유. | 요청 범위 서비스, 작업 단위 패턴. |

**등록 API:**

```cpp
auto& container = service_container::global();

// 구현 타입 등록 (기본 생성 가능)
container.register_type<ILogger, ConsoleLogger>(service_lifetime::singleton);

// 의존성 해석이 포함된 팩토리 등록
container.register_factory<IDatabase>(
    [](IServiceContainer& c) {
        auto logger = c.resolve<ILogger>().value();
        return std::make_shared<PostgresDatabase>(logger);
    },
    service_lifetime::scoped
);

// 단순 팩토리 등록 (컨테이너 접근 불필요)
container.register_simple_factory<ICache>(
    [] { return std::make_shared<InMemoryCache>(); },
    service_lifetime::singleton
);

// 기존 인스턴스 등록
auto config = std::make_shared<AppConfig>("config.yaml");
container.register_instance<IConfig>(config);
```

**해석 API:**

```cpp
// 오류 처리와 함께 해석
auto result = container.resolve<ILogger>();
if (result.is_ok()) {
    auto logger = result.value();
    logger->info("Resolved successfully");
}

// nullptr 반환으로 해석 (선택적 서비스용)
auto cache = container.resolve_or_null<ICache>();
if (cache) {
    cache->set("key", "value");
}

// 인트로스펙션
bool has_logger = container.is_registered<ILogger>();
auto services = container.registered_services();
```

**스코프 컨테이너:**

스코프는 싱글톤을 부모와 공유하면서 스코프 서비스에 대한 요청 수준 격리를 제공합니다:

```cpp
auto& container = service_container::global();

void handle_request() {
    auto scope = container.create_scope();

    // 각 스코프가 자체 IDatabase 인스턴스를 가짐
    auto db = scope->resolve<IDatabase>().value();
    auto db2 = scope->resolve<IDatabase>().value();
    // db == db2 (이 스코프 내에서 동일한 스코프 인스턴스)

    // 싱글톤은 부모와 공유
    auto logger = scope->resolve<ILogger>().value();
    // 부모 컨테이너와 동일한 ILogger 인스턴스

} // 스코프 소멸, 스코프 인스턴스 해제
```

**보안 제어:**

```cpp
// 초기화 후 컨테이너를 동결하여 변조 방지
container.freeze();

// 이후 등록 시도는 오류 반환
auto result = container.register_type<IFoo, FooImpl>();
// result.is_err() == true, 오류 코드: REGISTRY_FROZEN

bool frozen = container.is_frozen();  // true
```

**순환 의존성 감지:**

컨테이너는 스레드 로컬 해석 스택을 사용하여 런타임에 순환 의존성을 감지합니다:

```cpp
// A가 B에 의존하고, B가 A에 의존하는 경우:
auto result = container.resolve<A>();
// result.is_err() == true
// 오류: "Circular dependency detected: A -> B -> A"
```

**스레드 안전성:**
- 모든 public 메서드는 `std::shared_mutex` (읽기/쓰기 잠금)을 사용하여 스레드 안전합니다.
- 싱글톤 인스턴스화는 이중 확인 잠금을 사용합니다.
- 순환 의존성 감지는 오탐을 방지하기 위해 스레드 로컬 저장소를 사용합니다.

### 시스템 부트스트래퍼

`SystemBootstrapper` (`kcenon::common::bootstrap`)는 로거 등록과 생명주기 관리를 통합하는 애플리케이션 초기화를 위한 플루언트 API를 제공합니다.

**주요 기능:**
- 표현력 있는 설정을 위한 플루언트 메서드 체이닝
- 팩토리 기반 로거 지연 초기화
- 소멸자에서 자동 종료하는 RAII 지원
- 중복 초기화/종료 방지
- 스레드 안전 로거 접근을 위한 `GlobalLoggerRegistry` 통합

**API:**

```cpp
namespace kcenon::common::bootstrap {

class SystemBootstrapper {
public:
    SystemBootstrapper();
    ~SystemBootstrapper();  // 자동으로 shutdown() 호출

    // 플루언트 설정
    SystemBootstrapper& with_default_logger(LoggerFactory factory);
    SystemBootstrapper& with_logger(const std::string& name, LoggerFactory factory);
    SystemBootstrapper& on_initialize(std::function<void()> callback);
    SystemBootstrapper& on_shutdown(std::function<void()> callback);
    SystemBootstrapper& with_auto_freeze(
        bool freeze_logger_registry = true,
        bool freeze_service_container = true);

    // 생명주기
    VoidResult initialize();
    void shutdown();
    bool is_initialized() const noexcept;
    void reset();
};

}
```

**사용 예제:**

```cpp
#include <kcenon/common/bootstrap/system_bootstrapper.h>

using namespace kcenon::common::bootstrap;

int main() {
    SystemBootstrapper bootstrapper;
    bootstrapper
        .with_default_logger([] { return create_console_logger(); })
        .with_logger("database", [] { return create_file_logger("db.log"); })
        .with_auto_freeze()  // 초기화 후 레지스트리 동결
        .on_initialize([] { LOG_INFO("System started"); })
        .on_shutdown([] { LOG_INFO("System stopping"); });

    auto result = bootstrapper.initialize();
    if (result.is_err()) {
        std::cerr << "Init failed: " << result.error().message << "\n";
        return 1;
    }

    // 애플리케이션 로직...

    return 0;
    // ~SystemBootstrapper가 자동으로 shutdown() 호출
}
```

**초기화 순서:**
1. 기본 로거 생성 및 등록 (설정된 경우)
2. 모든 명명된 로거 생성 및 등록
3. 등록 순서대로 초기화 콜백 실행
4. 레지스트리 동결 (`with_auto_freeze()` 호출 시)

**종료 순서:**
1. 역순으로 종료 콜백 실행 (LIFO)
2. `GlobalLoggerRegistry`에서 모든 로거 제거

### 통합 부트스트래퍼

`unified_bootstrapper` (`kcenon::common::di`)는 서비스 컨테이너를 통해 시스템 전체 초기화와 종료를 조율하는 정적 유틸리티 클래스입니다. 시그널 처리, 종료 훅, 조건부 하위 시스템 등록을 제공합니다.

**설정 (`bootstrapper_options`):**

```cpp
namespace kcenon::common::di {

struct bootstrapper_options {
    bool enable_logging = true;           // 로깅 서비스 활성화
    bool enable_monitoring = true;        // 모니터링 서비스 활성화
    bool enable_database = false;         // 데이터베이스 서비스 활성화
    bool enable_network = false;          // 네트워크 서비스 활성화
    std::string config_path;              // 설정 파일 경로 (선택 사항)
    std::chrono::milliseconds shutdown_timeout{30000};
    bool register_signal_handlers = true; // SIGTERM/SIGINT 처리
};

}
```

**API:**

```cpp
namespace kcenon::common::di {

class unified_bootstrapper {
public:
    // 생명주기
    static VoidResult initialize(const bootstrapper_options& opts = {});
    static VoidResult shutdown(
        std::chrono::milliseconds timeout = std::chrono::seconds(30));

    // 서비스 접근
    static service_container& services();

    // 상태 조회
    static bool is_initialized();
    static bool is_shutdown_requested();

    // 종료 훅 (종료 시 LIFO 순서로 호출)
    static VoidResult register_shutdown_hook(
        const std::string& name, shutdown_hook hook);
    static VoidResult unregister_shutdown_hook(const std::string& name);

    // 시그널 처리
    static void request_shutdown(bool trigger_shutdown = false);

    // 설정
    static bootstrapper_options get_options();
};

}
```

**사용 예제:**

```cpp
#include <kcenon/common/di/unified_bootstrapper.h>

using namespace kcenon::common::di;

int main() {
    auto result = unified_bootstrapper::initialize({
        .enable_logging = true,
        .enable_monitoring = true,
        .config_path = "config.yaml",
        .register_signal_handlers = true
    });

    if (result.is_err()) {
        std::cerr << "Init failed: " << result.error().message << "\n";
        return 1;
    }

    // 커스텀 종료 훅 등록
    unified_bootstrapper::register_shutdown_hook("flush_cache",
        [](std::chrono::milliseconds remaining) {
            flush_all_caches();
        });

    // 서비스 접근
    auto& services = unified_bootstrapper::services();
    auto logger = services.resolve<ILogger>();

    // 메인 루프 - 종료 시그널 확인
    while (!unified_bootstrapper::is_shutdown_requested()) {
        process_next_request();
    }

    unified_bootstrapper::shutdown();
    return 0;
}
```

**시그널 처리:**
- `SIGTERM`과 `SIGINT`에 대한 핸들러를 자동으로 등록합니다 (설정 가능).
- 시그널 수신 시 종료 플래그를 설정합니다 (`is_shutdown_requested()`가 `true` 반환).
- 애플리케이션 코드는 협력적 종료를 위해 `is_shutdown_requested()`를 폴링해야 합니다.

**종료 훅:**
- 훅은 종료 시 역순으로 실행됩니다 (LIFO).
- 각 훅은 남은 타임아웃 시간을 수신하여 시간 예산을 고려한 정리가 가능합니다.
- 훅에서 발생한 예외는 조용히 포착되어 모든 훅이 실행되도록 합니다.

---

## 통합 기능

### thread_system과의 통합

common_system 인터페이스와 thread_system을 사용하기 위한 완전한 통합 예제:

```cpp
#include <kcenon/thread/core/thread_pool.h>
#include <kcenon/thread/adapters/common_executor_adapter.h>

// 스레드 풀 생성
auto thread_pool = std::make_shared<kcenon::thread::thread_pool>(
    4  // 워커 스레드
);

// common IExecutor 인터페이스로 적응
auto executor = kcenon::thread::adapters::make_common_executor(thread_pool);

// 이제 모든 IExecutor 기반 API와 함께 사용
void process_with_executor(std::shared_ptr<common::interfaces::IExecutor> exec) {
    auto future = exec->submit([]() {
        return compute_expensive_operation();
    });

    // 계산이 실행되는 동안 다른 작업 수행...

    auto result = future.get();
}

process_with_executor(executor);
```

### network_system과의 통합

common executor 추상화를 사용한 네트워크 작업:

```cpp
#include <network_system/integration/executor_adapter.h>
#include <network_system/server.h>

void setup_network(std::shared_ptr<common::interfaces::IExecutor> executor) {
    // common executor를 네트워크 시스템의 스레드 풀 인터페이스로 적응
    auto network_pool = kcenon::network::integration::make_thread_pool_adapter(executor);

    // 적응된 executor로 서버 생성
    network_system::server server(network_pool);

    // 모든 네트워크 작업이 이제 common executor 사용
    server.listen(8080);
}
```

### logger_system과의 통합

오류 처리 통합:

```cpp
#include <kcenon/logger/logger.h>
#include <kcenon/common/patterns/result.h>

common::Result<void> initialize_logging(const std::string& log_path) {
    try {
        auto logger = kcenon::logger::create_logger(log_path);

        if (!logger) {
            return common::make_error<void>(
                common::error_codes::INITIALIZATION_FAILED,
                "로거 생성 실패",
                "initialize_logging"
            );
        }

        return common::ok();

    } catch (const std::exception& e) {
        return common::make_error<void>(
            common::error_codes::EXCEPTION,
            e.what(),
            "initialize_logging"
        );
    }
}

// 사용
auto result = initialize_logging("/var/log/app.log");
if (!result) {
    std::cerr << "로깅 초기화 실패: "
              << result.error().message << "\n";
    return 1;
}
```

### 에코시스템 통합 플래그

에코시스템 모듈과의 유연한 통합을 위해:

**사용 가능한 플래그:**

- `KCENON_WITH_COMMON_SYSTEM`: common_system 타입 사용 가능 (헤더 포함 시 자동 설정)
- `KCENON_WITH_THREAD_SYSTEM`: thread_system 통합 활성화
- `KCENON_WITH_CONTAINER_SYSTEM`: container_system 통합 활성화
- `KCENON_WITH_LOGGER_SYSTEM`: logger_system 통합 활성화
- `KCENON_WITH_MONITORING_SYSTEM`: monitoring_system 통합 활성화
- `KCENON_WITH_NETWORK_SYSTEM`: network_system 통합 활성화
- `KCENON_WITH_DATABASE_SYSTEM`: database_system 통합 활성화

**CMake 사용:**

```cmake
include(cmake/features.cmake)

# 기능 플래그 설정
kcenon_configure_features(my_target
    THREAD_SYSTEM ON
    LOGGER_SYSTEM ON
    DATABASE_SYSTEM ON
)

# 또는 전통적인 컴파일 정의 사용
target_compile_definitions(my_target PUBLIC
    KCENON_WITH_THREAD_SYSTEM=1
    KCENON_WITH_LOGGER_SYSTEM=1
)

# 타겟에 링크
target_link_libraries(my_app
    PRIVATE
        kcenon::common
        kcenon::thread
        kcenon::logger
        kcenon::database
)
```

---

## 프로덕션 품질 기능

### 빌드 및 테스팅 인프라

**멀티 플랫폼 지속적 통합:**

common_system은 여러 플랫폼과 컴파일러에서 지속적으로 테스트됩니다:

- **Ubuntu Linux**
  - GCC 7, 9, 11, 13
  - Clang 5, 10, 14, 16
  - 완전한 새니타이저 커버리지

- **macOS**
  - Apple Clang (Xcode 12, 13, 14, 15)
  - arm64 및 x86_64 아키텍처
  - 네이티브 M1/M2 테스팅

- **Windows**
  - MSVC 2017, 2019, 2022
  - x86 및 x64 빌드 모두
  - Debug 및 Release 구성

**자동화된 새니타이저 빌드:**

모든 커밋은 다음으로 테스트됩니다:
- **ThreadSanitizer (TSan)**: 데이터 레이스 및 스레딩 문제 감지
- **AddressSanitizer (ASan)**: 메모리 오류 및 누수 감지
- **UndefinedBehaviorSanitizer (UBSan)**: 정의되지 않은 동작 포착

**품질 메트릭:**

현재 프로덕션 품질 메트릭:
- 테스트 커버리지: 80%+ (목표: 85%)
- 새니타이저 테스트: 18/18 제로 경고로 통과
- 정적 분석: 베이스라인 수립, 새로운 경고 제로
- 문서 커버리지: 공개 API의 100%

### 스레드 안전성 및 동시성

**설계상 스레드 안전:**

모든 common_system 인터페이스는 안전한 동시 접근을 위해 설계되었습니다:

- **Result<T>**: 생성 후 불변, 스레드 간 공유 안전
- **IExecutor**: 스레드 안전 submit() 및 execute() 연산
- **이벤트 버스**: 락프리 게시/구독 연산
- **오류 레지스트리**: 컴파일 타임 초기화, 런타임 읽기 전용

**동시성 보장:**

```cpp
// 안전한 동시 Result<T> 사용
void worker_thread(std::shared_ptr<Result<Data>> result) {
    // 여러 스레드가 동일한 Result를 안전하게 읽을 수 있음
    if (result->is_ok()) {
        process(result->value());
    }
}

// 안전한 동시 IExecutor 사용
void process_batch(std::shared_ptr<IExecutor> executor,
                  const std::vector<Task>& tasks) {
    // 여러 스레드가 동일한 실행기에 제출 가능
    for (const auto& task : tasks) {
        executor->submit([task]() {
            task.execute();
        });
    }
}
```

**검증:**

- 모든 에코시스템 컴포넌트에서 ThreadSanitizer 준수 확인
- 프로덕션 사용에서 제로 데이터 레이스 경고
- 포괄적인 동시성 계약 문서
- 모든 공유 상태에 대한 적절한 동기화

### 리소스 관리 (RAII - A등급)

**완벽한 RAII 준수:**

common_system의 모든 리소스는 RAII 원칙을 따릅니다:

- 스마트 포인터(`std::shared_ptr`, `std::unique_ptr`)를 통해 모든 리소스 관리
- 코드베이스 어디에서도 수동 메모리 관리 없음
- 스코프 종료 시 자동 정리
- 설계상 예외 안전

**검증 결과:**

- AddressSanitizer: 제로 메모리 누수로 18/18 테스트 통과
- 모든 오류 경로에서 리소스 정리 확인
- 프로덕션 사용에서 리소스 누수 감지되지 않음
- 모든 연산에서 예외 안전성 검증

---

## 오류 처리 기반

common_system은 에코시스템의 모든 시스템에 걸쳐 오류 처리를 위한 **기반 제공자** 역할을 합니다.

### 중앙 집중식 오류 코드 레지스트리

시스템별 범위를 제공하는 완전한 오류 코드 레지스트리:

| 시스템 | 오류 코드 범위 | 목적 |
|--------|-----------------|---------|
| common_system | -1 ~ -99 | 기반 오류 (검증, 초기화) |
| thread_system | -100 ~ -199 | 스레딩 오류 (교착 상태, 풀 고갈) |
| logger_system | -200 ~ -299 | 로깅 오류 (파일 I/O, 포맷팅) |
| monitoring_system | -300 ~ -399 | 모니터링 오류 (메트릭 수집, 게시) |
| container_system | -400 ~ -499 | 컨테이너 오류 (직렬화, 검증) |
| database_system | -500 ~ -599 | 데이터베이스 오류 (연결, 쿼리, 트랜잭션) |
| network_system | -600 ~ -699 | 네트워크 오류 (연결, 타임아웃, 프로토콜) |

**컴파일 타임 검증:**

오류 코드 범위는 충돌을 방지하기 위해 컴파일 타임에 강제됩니다:

```cpp
namespace common::error_codes {
    // 컴파일 타임 범위 검사
    constexpr int COMMON_MIN = -1;
    constexpr int COMMON_MAX = -99;

    constexpr bool is_valid_common_code(int code) {
        return code >= COMMON_MIN && code <= COMMON_MAX;
    }

    static_assert(is_valid_common_code(NOT_FOUND));
    static_assert(is_valid_common_code(INVALID_ARGUMENT));
}
```

### 에코시스템 채택

모든 의존 시스템이 Result<T> 패턴과 오류 코드 레지스트리를 성공적으로 채택했습니다:

**채택 상태:**

- ✅ thread_system: 모든 연산에 대한 완전한 Result<T> 통합
- ✅ logger_system: Result<T>를 통한 오류 처리, 예외 없음
- ✅ monitoring_system: 메트릭 연산에 Result<T>
- ✅ container_system: 직렬화 연산이 Result<T> 반환
- ✅ database_system: 쿼리 결과 및 트랜잭션이 Result<T> 사용
- ✅ network_system: 연결 및 I/O 연산이 Result<T> 반환

**실현된 이점:**

- 모든 시스템에서 일관된 오류 처리
- 프로덕션에서 예기치 않은 예외 없음
- 명확한 오류 전파 경로
- 개선된 오류 복구 및 복원력
- 더 나은 오류 로깅 및 진단

---

## 고급 기능

### Source Location 지원 (C++20)

C++20으로 컴파일 시 common_system은 향상된 오류 진단을 제공합니다:

```cpp
#include <kcenon/common/utils/source_location.h>

// 자동 source_location 캡처로 로깅
logger->log(log_level::info, "작업 완료");
// 파일, 라인, 함수 정보가 자동으로 캡처됨

// log_entry 팩토리 메서드 사용
auto entry = log_entry::create(log_level::error, "연결 실패");
// entry.file, entry.line, entry.function이 자동으로 채워짐

// Result<T>에서 향상된 오류 메시지
auto result = some_operation();
if (result.is_err()) {
    result.unwrap();  // 에러 시 파일/라인/함수 정보 포함 예외 발생
}
```

**C++17 폴백**:
- C++17에서는 `__FILE__`, `__LINE__`, `__FUNCTION__` 매크로로 폴백
- 동일한 API 유지, 런타임 비용 없음

### ABI 버전 검사

컴파일 타임 ABI 호환성 검증:

```cpp
namespace kcenon::common::abi {
    constexpr int MAJOR = 1;
    constexpr int MINOR = 0;
    constexpr int PATCH = 0;

    constexpr int VERSION = (MAJOR << 16) | (MINOR << 8) | PATCH;
}

// 클라이언트 코드에서 ABI 호환성 검증 가능
static_assert(kcenon::common::abi::MAJOR == 1, "호환되지 않는 ABI 버전");
```

### 커스텀 에러 타입

에러 시스템을 커스텀 에러 타입으로 확장:

```cpp
namespace my_system::errors {
    // common 범위 외부의 커스텀 에러 코드
    constexpr int MY_CUSTOM_ERROR = -1001;

    inline std::string get_error_message(int code) {
        if (code == MY_CUSTOM_ERROR) {
            return "커스텀 에러 발생";
        }
        return kcenon::common::get_error_message(code);
    }
}

// 강타입 enum 에러 코드도 지원
enum class DatabaseError {
    connection_failed = -501,
    query_failed = -502,
    transaction_failed = -503
};

// error_info 생성 시 enum 직접 사용 가능
auto err = error_info{DatabaseError::connection_failed, "연결 실패"};
```

### vcpkg 및 Conan 지원

패키지 관리자를 통한 간편한 통합:

**Conan 사용**:
```bash
# 소스에서 패키지 생성
conan create . --build=missing

# conanfile.txt에 추가
[requires]
common_system/1.0.0

# 설치
conan install . --build=missing
```

**CMake FetchContent 사용 (권장)**:
```cmake
include(FetchContent)
FetchContent_Declare(
    common_system
    GIT_REPOSITORY https://github.com/kcenon/common_system.git
    GIT_TAG main
)
FetchContent_MakeAvailable(common_system)

target_link_libraries(your_target PRIVATE kcenon::common)
```

### 프로덕션 예제

#### 완전한 애플리케이션 초기화

```cpp
#include <kcenon/common/bootstrap/system_bootstrapper.h>
#include <kcenon/common/logging/log_macros.h>

using namespace kcenon::common::bootstrap;
using namespace kcenon::common::interfaces;

int main() {
    SystemBootstrapper bootstrapper;
    bootstrapper
        .with_default_logger([]() {
            return std::make_shared<ConsoleLogger>();
        })
        .with_logger("network", []() {
            return std::make_shared<FileLogger>("network.log");
        })
        .on_initialize([]() {
            LOG_INFO("시스템 초기화 완료");
        })
        .on_shutdown([]() {
            LOG_INFO("시스템 종료 중");
        });

    if (auto result = bootstrapper.initialize(); result.is_err()) {
        std::cerr << "초기화 실패: " << result.error().message;
        return 1;
    }

    // 애플리케이션 로직...
    LOG_INFO("서버 시작");
    LOG_INFO_TO("network", "포트 8080에서 대기 중");

    // RAII로 자동 정리
    return 0;
}
```

#### Job 기반 Executor 사용

```cpp
#include <kcenon/common/interfaces/executor_interface.h>

using namespace kcenon::common;
using namespace kcenon::common::interfaces;

class DataProcessingJob : public IJob {
    std::vector<int> data_;
public:
    DataProcessingJob(std::vector<int> data) : data_(std::move(data)) {}

    VoidResult execute() override {
        // 데이터 처리 로직
        for (auto& item : data_) {
            item *= 2;
        }
        return ok();
    }

    std::string get_name() const override { return "data_processing"; }
    int get_priority() const override { return 10; }
};

void process_data(std::shared_ptr<IExecutor> executor) {
    auto job = std::make_unique<DataProcessingJob>(std::vector{1, 2, 3, 4, 5});

    auto result = executor->execute(std::move(job));
    if (result.is_ok()) {
        result.value().wait();  // 완료 대기
        LOG_INFO("데이터 처리 완료");
    } else {
        LOG_ERROR("실행 실패: " + result.error().message);
    }
}
```

#### Result<T> 모나딕 체이닝

```cpp
#include <kcenon/common/patterns/result.h>

using namespace kcenon::common;

Result<Config> load_and_validate_config(const std::string& path) {
    return load_config(path)
        .and_then([](const Config& cfg) -> Result<Config> {
            if (cfg.port <= 0 || cfg.port > 65535) {
                return make_error<Config>(
                    error_codes::INVALID_ARGUMENT,
                    "잘못된 포트 번호",
                    "config_validator"
                );
            }
            return Result<Config>::ok(cfg);
        })
        .map([](const Config& cfg) {
            auto validated = cfg;
            validated.validated = true;
            return validated;
        })
        .or_else([](const error_info& err) -> Result<Config> {
            LOG_WARNING("설정 로드 실패, 기본값 사용: " + err.message);
            return Result<Config>::ok(Config::default_config());
        });
}
```

---

**최종 업데이트**: 2026-02-08
**버전**: 0.2.0

---

Made with ❤️ by 🍀☀🌕🌥 🌊
