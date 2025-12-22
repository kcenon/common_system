# common_system API 레퍼런스

> **버전**: 0.2.3
> **최종 업데이트**: 2025-12-10
> **상태**: 개발 중 (Tier 0)

## 목차

1. [네임스페이스](#네임스페이스)
2. [Result<T> 패턴 (권장)](#resultt-패턴-권장)
3. [Concepts](#concepts)
4. [인터페이스](#인터페이스)
5. [통합 로깅](#통합-로깅)
6. [부트스트랩](#부트스트랩)
7. [유틸리티](#유틸리티)

---

## 네임스페이스

### `kcenon::common`

common_system의 모든 공개 API는 이 네임스페이스에 포함됩니다.

**포함 항목**:
- `Result<T>` - 결과 타입
- `VoidResult` - void용 결과 (`Result<std::monostate>`의 별칭)
- 모든 인터페이스 (`IExecutor`, `ILogger`, `IMonitor`, `IDatabase`)

---

## Result<T> 패턴 (권장)

### `Result<T>`

**헤더**: `#include <kcenon/common/patterns/result.h>`

**설명**: 성공 값 또는 에러를 나타내는 타입 안전 컨테이너

#### 생성자

```cpp
// 기본 생성자는 삭제됨 - 명시적 초기화 강제
Result() = delete;

// 값으로 생성 (암시적)
Result(const T& value);
Result(T&& value);

// 에러로 생성 (암시적)
Result(const error_info& error);
Result(error_info&& error);
```

#### 팩토리 메서드 (권장)

```cpp
// 성공 값으로 생성
template<typename U = T>
static Result<T> ok(U&& value);

// 에러로 생성
static Result<T> err(const error_info& error);
static Result<T> err(error_info&& error);
static Result<T> err(int code, const std::string& message, const std::string& module = "");

// 명시적으로 초기화되지 않은 결과 생성 (주의해서 사용)
static Result<T> uninitialized();
```

#### 핵심 메서드

##### `is_ok()` / `is_err()`

```cpp
bool is_ok() const;
bool is_err() const;
```

**설명**: Result 상태 확인

**예시**:
```cpp
Result<int> res = Result<int>::ok(42);
if (res.is_ok()) {
    // 성공 처리
}
```

##### `value()`

```cpp
const T& value() const;
T& value();
```

**설명**: 성공 값 접근 (에러 상태에서 호출 시 예외 발생)

**예외**:
- `std::runtime_error`: 에러 상태에서 호출 시 발생

##### `unwrap()` / `unwrap_or()`

```cpp
const T& unwrap() const;  // 에러 시 throw
T& unwrap();
T unwrap_or(T default_value) const;  // 에러 시 기본값 반환
T value_or(T default_value) const;   // unwrap_or의 별칭 (C++23 호환)
```

**예시**:
```cpp
auto res = Result<int>::ok(42);
int val = res.unwrap();  // 42
int safe_val = res.unwrap_or(0);  // 42 또는 에러 시 0
```

##### `error()`

```cpp
const error_info& error() const;
```

**설명**: 에러 정보 접근

#### 모나딕 연산

```cpp
// 성공 값에 함수 적용
template<typename F>
auto map(F&& func) const -> Result<decltype(func(std::declval<T>()))>;

// Result를 반환하는 함수 체이닝 (flatMap)
template<typename F>
auto and_then(F&& func) const -> decltype(func(std::declval<T>()));

// 에러 시 대체 값 제공
template<typename F>
Result<T> or_else(F&& func) const;
```

**사용 예시**:
```cpp
auto result = load_config("app.conf")
    .and_then(validate_config)
    .map(apply_defaults)
    .or_else([](const auto& error) {
        log_error(error);
        return load_fallback_config();
    });
```

---

### `VoidResult`

**헤더**: `#include <kcenon/common/patterns/result.h>`

**설명**: void 반환용 결과 (`Result<std::monostate>`의 별칭)

#### 사용 예시

```cpp
VoidResult process() {
    if (/* 성공 조건 */) {
        return ok();  // 헬퍼 함수 사용
    }
    return make_error<std::monostate>(1, "처리 실패");
}

auto res = process();
if (res.is_ok()) {
    // 성공 처리
}
```

---

### 헬퍼 함수

```cpp
// 성공 Result 생성
template<typename T>
Result<T> ok(T value);

// void 성공 Result 생성
VoidResult ok();

// 에러 Result 생성
template<typename T>
Result<T> make_error(int code, const std::string& message,
                     const std::string& module = "");

template<typename T>
Result<T> make_error(const error_info& err);
```

---

## Concepts

C++20 concepts는 명확한 오류 메시지와 함께 컴파일 타임 타입 검증을 제공합니다. 모든 concepts는 `kcenon::common::concepts` 네임스페이스에 정의되어 있습니다.

**헤더**: `#include <kcenon/common/concepts/concepts.h>`

### Core Concepts (core.h)

| Concept | 설명 |
|---------|------|
| `Resultable` | `is_ok()` 및 `is_err()` 메서드를 가진 타입 |
| `Unwrappable` | 값 추출을 지원하는 타입 (`unwrap()`, `unwrap_or()`) |
| `Mappable` | `map()` 변환을 지원하는 타입 |
| `Chainable` | `and_then()` 체이닝을 지원하는 타입 |
| `MonadicResult` | 모든 모나딕 연산을 갖춘 완전한 Result 유사 타입 |
| `OptionalLike` | 선택적 값 컨테이너 (`has_value()`, `is_some()`, `is_none()`) |
| `ErrorInfo` | `code`, `message`, `module`을 가진 에러 정보 타입 |
| `ValueOrError` | 값 또는 에러를 보유하는 타입 |

### Callable Concepts (callable.h)

| Concept | 설명 |
|---------|------|
| `Invocable<F, Args...>` | 호출 가능한 타입 |
| `VoidCallable<F, Args...>` | void를 반환하는 호출 가능 타입 |
| `ReturnsResult<F, R, Args...>` | 특정 타입을 반환하는 호출 가능 타입 |
| `NoexceptCallable<F, Args...>` | noexcept 호출 가능 타입 |
| `Predicate<F, Args...>` | bool을 반환하는 호출 가능 타입 |
| `UnaryFunction<F, Arg>` | 단일 인자 호출 가능 타입 |
| `BinaryFunction<F, Arg1, Arg2>` | 두 인자 호출 가능 타입 |
| `JobLike` | IJob 인터페이스를 만족하는 타입 |
| `ExecutorLike` | IExecutor 인터페이스를 만족하는 타입 |

### Event Concepts (event.h)

| Concept | 설명 |
|---------|------|
| `EventType` | 유효한 이벤트 타입 (클래스, 복사 생성 가능) |
| `EventHandler<H, E>` | 이벤트 핸들러 호출 가능 타입 |
| `EventFilter<F, E>` | 이벤트 필터 술어 |
| `TimestampedEvent` | 타임스탬프를 가진 이벤트 |
| `NamedEvent` | 모듈 이름을 가진 이벤트 |
| `ErrorEvent` | 메시지와 코드를 가진 에러 이벤트 |
| `MetricEvent` | 이름, 값, 단위를 가진 메트릭 이벤트 |
| `EventBusLike` | 이벤트 버스 인터페이스 타입 |

### Service Concepts (service.h)

| Concept | 설명 |
|---------|------|
| `ServiceInterface` | 유효한 서비스 인터페이스 (다형적, 가상 소멸자) |
| `ServiceImplementation<TImpl, TIface>` | 서비스 구현 |
| `ServiceFactory<F, T>` | 서비스 팩토리 호출 가능 타입 (컨테이너 포함) |
| `SimpleServiceFactory<F, T>` | 단순 팩토리 호출 가능 타입 (컨테이너 없음) |
| `ServiceContainerLike` | 서비스 컨테이너 타입 |
| `InjectableService` | 자동 주입 가능한 서비스 |
| `Validatable` | 자체 검증 타입 |
| `InitializableService` | 초기화가 필요한 서비스 |
| `DisposableService` | 정리가 필요한 서비스 |

### Container Concepts (container.h)

| Concept | 설명 |
|---------|------|
| `Container` | 기본 컨테이너 요구사항 |
| `SequenceContainer` | 순차 컨테이너 (push_back, front, back) |
| `AssociativeContainer` | 키 기반 컨테이너 (find, count) |
| `MappingContainer` | 키-값 컨테이너 |
| `ResizableContainer` | 크기 조절 가능 컨테이너 (resize, reserve, capacity) |
| `ClearableContainer` | 삭제 가능 컨테이너 |
| `RandomAccessContainer` | 임의 접근 컨테이너 (operator[]) |
| `BoundedContainer` | 고정 용량 컨테이너 |
| `ThreadSafeContainer` | 스레드 안전 컨테이너 |
| `CircularBuffer` | 순환 버퍼 타입 |

### 사용 예시

```cpp
#include <kcenon/common/concepts/concepts.h>

using namespace kcenon::common::concepts;

// Result 유사 타입으로 템플릿 제약
template<MonadicResult R>
auto process_chain(const R& result) {
    return result
        .map([](auto& v) { return v * 2; })
        .and_then([](auto v) { return R::ok(v + 1); });
}

// 유효한 이벤트 핸들러로 제약
template<EventType E, EventHandler<E> H>
void subscribe_with_logging(H&& handler) {
    std::cout << "핸들러 구독 중..." << std::endl;
    bus.subscribe<E>(std::forward<H>(handler));
}

// 검증 가능한 타입으로 제약
template<Validatable T>
bool is_valid(const T& obj) {
    auto result = obj.validate();
    return result.is_ok();
}
```

상세 문서 및 마이그레이션 가이드는 [Concepts 가이드](guides/CONCEPTS_GUIDE_KO.md)를 참조하세요.

---

## 인터페이스

### `IExecutor`

**헤더**: `#include <kcenon/common/interfaces/executor_interface.h>`

**설명**: Job 기반 태스크 실행자 인터페이스

#### IJob 인터페이스

```cpp
class IJob {
public:
    virtual ~IJob() = default;

    // 작업 실행
    virtual VoidResult execute() = 0;

    // 작업 이름 (로깅/디버깅용)
    virtual std::string get_name() const { return "unnamed_job"; }

    // 작업 우선순위 (높을수록 중요)
    virtual int get_priority() const { return 0; }
};
```

#### IExecutor 순수 가상 함수

```cpp
class IExecutor {
public:
    virtual ~IExecutor() = default;

    // ===== Job 기반 실행 =====

    // Result 기반 에러 처리로 작업 실행
    virtual Result<std::future<void>> execute(std::unique_ptr<IJob>&& job) = 0;

    // 지연된 작업 실행
    virtual Result<std::future<void>> execute_delayed(
        std::unique_ptr<IJob>&& job,
        std::chrono::milliseconds delay) = 0;

    // ===== 상태 및 제어 =====

    // 워커 스레드 수 조회
    virtual size_t worker_count() const = 0;

    // 실행기 실행 중인지 확인
    virtual bool is_running() const = 0;

    // 대기 중인 태스크 수 조회
    virtual size_t pending_tasks() const = 0;

    // 실행기 정상 종료
    virtual void shutdown(bool wait_for_completion = true) = 0;
};
```

**구현 예시**:
```cpp
class MyJob : public kcenon::common::interfaces::IJob {
public:
    VoidResult execute() override {
        // 작업 로직 구현
        return ok();
    }

    std::string get_name() const override { return "my_job"; }
};

// 사용
auto job = std::make_unique<MyJob>();
auto result = executor->execute(std::move(job));
if (result.is_ok()) {
    result.value().wait();  // 완료 대기
}
```

---

### `ILogger`

**헤더**: `#include <kcenon/common/interfaces/logger_interface.h>`

**설명**: 로거 인터페이스

#### 순수 가상 함수

```cpp
// 핵심 로깅 메서드
virtual VoidResult log(log_level level, const std::string& message) = 0;

// source_location 기반 로깅 (C++20 - 권장, Issue #177)
// 기본 구현은 단순 log() 메서드로 위임
virtual VoidResult log(log_level level, std::string_view message,
                       const source_location& loc = source_location::current());

// 구조화된 엔트리 로깅
virtual VoidResult log(const log_entry& entry) = 0;

// 레벨 관리
virtual bool is_enabled(log_level level) const = 0;
virtual VoidResult set_level(log_level level) = 0;
virtual log_level get_level() const = 0;
virtual VoidResult flush() = 0;
```

**로그 레벨**:
```cpp
enum class log_level {
    trace = 0,
    debug = 1,
    info = 2,
    warning = 3,
    error = 4,
    critical = 5,  // 참고: 'fatal' 아님
    off = 6
};
```

#### log_entry 구조체

```cpp
struct log_entry {
    log_level level;
    std::string message;
    std::string file;
    int line;
    std::string function;
    std::chrono::system_clock::time_point timestamp;
    source_location location;  // C++20 source_location (Issue #177)

    // 기본 생성자 (역호환용)
    log_entry(log_level lvl = log_level::info, const std::string& msg = "");

    // 자동 source_location 캡처 팩토리 메서드 (권장)
    static log_entry create(log_level lvl, std::string_view msg,
                           const source_location& loc = source_location::current());
};
```

**사용 예시**:
```cpp
// 새 API (Issue #177) - source_location 자동 캡처
logger->log(log_level::info, "작업 완료");

// log_entry 팩토리 메서드 사용
auto entry = log_entry::create(log_level::warning, "메모리 부족");
logger->log(entry);
```

---

### `GlobalLoggerRegistry`

**헤더**: `#include <kcenon/common/interfaces/global_logger_registry.h>`

**설명**: 모든 서브시스템에서 로거 인스턴스를 관리하기 위한 스레드 안전 싱글톤 레지스트리.

이 클래스는 런타임에 바인딩할 수 있는 중앙화된 분리된 로깅 레지스트리를 제공하여 서브시스템 간 순환 의존성을 해결합니다.

#### 싱글톤 접근

```cpp
static GlobalLoggerRegistry& instance();
```

#### 기본 로거 관리

```cpp
// 기본 로거 설정
VoidResult set_default_logger(std::shared_ptr<ILogger> logger);

// 기본 로거 조회 (설정되지 않은 경우 NullLogger 반환)
std::shared_ptr<ILogger> get_default_logger();

// 기본 로거 사용 가능 여부 확인
bool has_default_logger() const;
```

#### 명명된 로거 관리

```cpp
// 이름으로 로거 등록
VoidResult register_logger(const std::string& name, std::shared_ptr<ILogger> logger);

// 이름으로 로거 조회 (없으면 NullLogger 반환)
std::shared_ptr<ILogger> get_logger(const std::string& name);

// 이름으로 로거 제거
VoidResult unregister_logger(const std::string& name);

// 로거 등록 여부 확인
bool has_logger(const std::string& name) const;
```

#### 팩토리 지원 (지연 초기화)

```cpp
// 지연 로거 생성을 위한 팩토리 등록
VoidResult register_factory(const std::string& name, LoggerFactory factory);

// 기본 로거용 팩토리 설정
VoidResult set_default_factory(LoggerFactory factory);
```

#### 유틸리티 메서드

```cpp
// 모든 등록된 로거와 팩토리 삭제
void clear();

// 등록된 로거 수 조회
size_t size() const;

// 공유 NullLogger 인스턴스 조회
static std::shared_ptr<ILogger> null_logger();
```

#### 편의 함수

```cpp
// 글로벌 레지스트리 조회
GlobalLoggerRegistry& get_registry();

// 기본 로거 조회
std::shared_ptr<ILogger> get_logger();

// 명명된 로거 조회
std::shared_ptr<ILogger> get_logger(const std::string& name);
```

**사용 예시**:
```cpp
#include <kcenon/common/interfaces/global_logger_registry.h>

using namespace kcenon::common::interfaces;

// 기본 로거 등록
auto console_logger = std::make_shared<ConsoleLogger>();
get_registry().set_default_logger(console_logger);

// 명명된 로거 등록
get_registry().register_logger("network", std::make_shared<NetworkLogger>());
get_registry().register_logger("database", std::make_shared<DatabaseLogger>());

// 로거 사용
get_logger()->log(log_level::info, "애플리케이션 시작");
get_logger("network")->log(log_level::debug, "연결 수립");
get_logger("database")->log(log_level::warning, "느린 쿼리 감지");

// 팩토리로 지연 초기화
get_registry().register_factory("metrics", []() {
    return std::make_shared<MetricsLogger>();
});
```

---

### `NullLogger`

**헤더**: `#include <kcenon/common/interfaces/global_logger_registry.h>`

**설명**: 폴백 시나리오를 위한 무작동(no-op) 로거 구현.

모든 로깅 작업은 성공을 반환하는 무작동입니다. 로깅이 구성되지 않았을 때 코드가 조용히 작동하도록 보장합니다.

```cpp
class NullLogger : public ILogger {
    // 모든 작업은 VoidResult::ok({}) 반환
    // is_enabled()는 항상 false 반환
    // get_level()은 항상 log_level::off 반환
};
```

---

### `IMonitor`

**헤더**: `#include <kcenon/common/interfaces/i_monitor.h>`

**설명**: 모니터링 인터페이스

#### 순수 가상 함수

```cpp
virtual VoidResult record_metric(const std::string& name, double value) = 0;
virtual VoidResult start_timer(const std::string& name) = 0;
virtual VoidResult stop_timer(const std::string& name) = 0;
```

---

### `IDatabase`

**헤더**: `#include <kcenon/common/interfaces/i_database.h>`

**설명**: 데이터베이스 인터페이스

#### 순수 가상 함수

```cpp
virtual VoidResult connect(const connection_info& info) = 0;
virtual Result<query_result> execute(const std::string& query) = 0;
virtual VoidResult disconnect() = 0;
```

---

## 통합 로깅

### 로깅 함수

**헤더**: `#include <kcenon/common/logging/log_functions.h>`

**설명**: 자동 source_location 캡처를 포함한 인라인 로깅 함수.

#### 핵심 함수

```cpp
namespace kcenon::common::logging {
    // 기본 로거에 지정된 레벨로 로깅
    VoidResult log(log_level level, std::string_view message,
                   const source_location& loc = source_location::current());

    // 특정 로거 인스턴스에 로깅
    VoidResult log(log_level level, std::string_view message,
                   const std::shared_ptr<ILogger>& logger,
                   const source_location& loc = source_location::current());

    // 명명된 로거에 로깅
    VoidResult log(log_level level, std::string_view message,
                   const std::string& logger_name,
                   const source_location& loc = source_location::current());
}
```

#### 레벨별 함수

```cpp
// 기본 로거에 로깅
VoidResult log_trace(std::string_view message, ...);
VoidResult log_debug(std::string_view message, ...);
VoidResult log_info(std::string_view message, ...);
VoidResult log_warning(std::string_view message, ...);
VoidResult log_error(std::string_view message, ...);
VoidResult log_critical(std::string_view message, ...);

// 명명된 로거에 로깅 ("_to" 접미사로 오버로드 모호성 회피)
VoidResult log_trace_to(const std::string& logger_name, std::string_view message, ...);
VoidResult log_debug_to(const std::string& logger_name, std::string_view message, ...);
VoidResult log_info_to(const std::string& logger_name, std::string_view message, ...);
VoidResult log_warning_to(const std::string& logger_name, std::string_view message, ...);
VoidResult log_error_to(const std::string& logger_name, std::string_view message, ...);
VoidResult log_critical_to(const std::string& logger_name, std::string_view message, ...);
```

#### 유틸리티 함수

```cpp
// 로그 레벨 활성화 여부 확인
bool is_enabled(log_level level);
bool is_enabled(log_level level, const std::string& logger_name);

// 버퍼링된 로그 메시지 플러시
VoidResult flush();
VoidResult flush(const std::string& logger_name);
```

**사용 예시**:
```cpp
#include <kcenon/common/logging/log_functions.h>

using namespace kcenon::common::logging;
using namespace kcenon::common::interfaces;

// 기본 로깅 (기본 로거 사용)
log_info("애플리케이션 시작");
log_warning("메모리 부족 상태");
log_error("연결 실패");

// 명명된 로거 사용 (참고: logger_name이 먼저)
log_info_to("network", "요청 수신됨");
log_debug_to("database", "쿼리 실행됨");

// 비용이 큰 메시지 구성 전 확인
if (is_enabled(log_level::debug)) {
    log_debug("상세 상태: " + expensive_to_string(state));
}
```

---

### 로깅 매크로

**헤더**: `#include <kcenon/common/logging/log_macros.h>`

**설명**: 로깅을 위한 편리한 전처리기 매크로.

#### 표준 매크로

```cpp
LOG_TRACE(msg)     // trace 메시지 로깅
LOG_DEBUG(msg)     // debug 메시지 로깅
LOG_INFO(msg)      // info 메시지 로깅
LOG_WARNING(msg)   // warning 메시지 로깅
LOG_ERROR(msg)     // error 메시지 로깅
LOG_CRITICAL(msg)  // critical 메시지 로깅
```

#### 명명된 로거 매크로

```cpp
LOG_TRACE_TO(logger_name, msg)
LOG_DEBUG_TO(logger_name, msg)
LOG_INFO_TO(logger_name, msg)
LOG_WARNING_TO(logger_name, msg)
LOG_ERROR_TO(logger_name, msg)
LOG_CRITICAL_TO(logger_name, msg)
```

#### 조건부 로깅

```cpp
// 레벨이 활성화된 경우에만 로깅 (비활성화 시 메시지 구성 회피)
LOG_IF(level, msg)
LOG_IF_TO(level, logger_name, msg)
```

#### 유틸리티 매크로

```cpp
LOG_FLUSH()                          // 기본 로거 플러시
LOG_FLUSH_TO(logger_name)            // 명명된 로거 플러시
LOG_IS_ENABLED(level)                // 레벨 활성화 여부 확인
LOG_IS_ENABLED_FOR(level, logger_name)
```

#### 컴파일 타임 레벨 필터링

헤더 포함 전에 `KCENON_MIN_LOG_LEVEL`을 정의하여 컴파일 타임에 낮은 로그 레벨 비활성화:

```cpp
// 릴리스 빌드에서 trace와 debug 비활성화
#define KCENON_MIN_LOG_LEVEL 2  // 0=trace, 1=debug, 2=info 등
#include <kcenon/common/logging/log_macros.h>

LOG_TRACE("이건 무작동이 됨");  // 컴파일에서 제외
LOG_DEBUG("이것도 마찬가지");   // 컴파일에서 제외
LOG_INFO("이건 로깅됨");        // 활성화
```

---

## 부트스트랩

### SystemBootstrapper

**헤더**: `#include <kcenon/common/bootstrap/system_bootstrapper.h>`

**설명**: 애플리케이션 수준에서 시스템 초기화 및 로거 등록을 위한 플루언트 API.

SystemBootstrapper는 다음을 중앙에서 관리합니다:
- 팩토리 함수를 통한 기본 및 명명된 로거
- 초기화 및 종료 라이프사이클 훅
- RAII 기반 자동 정리

#### 생성자 / 소멸자

```cpp
// 기본 생성자 - 초기화되지 않은 부트스트래퍼 생성
SystemBootstrapper();

// 소멸자 - 초기화된 경우 자동으로 shutdown() 호출
~SystemBootstrapper();

// 복사 불가, 이동 가능
SystemBootstrapper(SystemBootstrapper&& other) noexcept;
SystemBootstrapper& operator=(SystemBootstrapper&& other) noexcept;
```

#### 플루언트 구성 API

```cpp
// 기본 로거용 팩토리 등록
SystemBootstrapper& with_default_logger(LoggerFactory factory);

// 명명된 로거용 팩토리 등록
SystemBootstrapper& with_logger(const std::string& name, LoggerFactory factory);

// 초기화 콜백 등록 (등록 순서대로 호출)
SystemBootstrapper& on_initialize(std::function<void()> callback);

// 종료 콜백 등록 (역순으로 호출 - LIFO)
SystemBootstrapper& on_shutdown(std::function<void()> callback);
```

#### 라이프사이클 관리

```cpp
// 시스템 초기화 (로거 등록, 초기화 콜백 호출)
VoidResult initialize();

// 시스템 종료 (종료 콜백 호출, 로거 삭제)
void shutdown();

// 시스템 초기화 여부 확인
bool is_initialized() const noexcept;

// 초기 상태로 리셋 (필요시 shutdown 호출, 모든 구성 삭제)
void reset();
```

**사용 예시**:
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
        .with_logger("database", []() {
            return std::make_shared<FileLogger>("db.log");
        })
        .with_logger("network", []() {
            return std::make_shared<FileLogger>("network.log");
        })
        .on_initialize([]() {
            LOG_INFO("시스템 초기화됨");
        })
        .on_shutdown([]() {
            LOG_INFO("시스템 종료 중");
        });

    auto result = bootstrapper.initialize();
    if (result.is_err()) {
        std::cerr << "초기화 실패: " << result.error().message;
        return 1;
    }

    // 애플리케이션 로직...
    LOG_INFO("애플리케이션 실행 중");
    LOG_INFO_TO("database", "데이터베이스 연결됨");
    LOG_INFO_TO("network", "서버 시작됨");

    // 소멸자에 의해 shutdown이 자동 호출됨 (RAII)
    return 0;
}
```

**스레드 안전성**:
- 구성 메서드 (`with_*`, `on_*`)는 스레드 안전하지 않음
- `initialize()` 및 `shutdown()`은 상태 보호를 위해 뮤텍스 사용
- 초기화 후 등록된 로거는 GlobalLoggerRegistry를 통해 스레드 안전

---

## 유틸리티

### error_info

**구조체**:
```cpp
struct error_info {
    int code;                           // 에러 코드
    std::string message;                // 사람이 읽을 수 있는 메시지
    std::string module;                 // 소스 모듈 (선택)
    std::optional<std::string> details; // 추가 컨텍스트 (선택)

    // 생성자
    error_info();
    error_info(const std::string& msg);
    error_info(int c, const std::string& msg, const std::string& mod = "");
    error_info(int c, const std::string& msg, const std::string& mod,
               const std::string& det);

    // 강타입 enum 에러 코드 지원
    template<typename Enum, std::enable_if_t<std::is_enum_v<Enum>, int> = 0>
    error_info(Enum c, std::string msg, std::string mod = "",
               std::optional<std::string> det = std::nullopt);
};
```

**사용 예시**:
```cpp
return Result<int>::err(error_info{
    404,
    "찾을 수 없음",
    "config_loader",
    "리소스가 존재하지 않습니다"
});
```

---

### 에러 코드 레지스트리

**헤더**: `#include <kcenon/common/error/error_codes.h>`

시스템별 범위를 가진 표준 에러 코드:

| 시스템 | 코드 범위 | 목적 |
|--------|-----------|------|
| common_system | -1 ~ -99 | 기초 에러 |
| thread_system | -100 ~ -199 | 스레딩 에러 |
| logger_system | -200 ~ -299 | 로깅 에러 |
| monitoring_system | -300 ~ -399 | 모니터링 에러 |
| container_system | -400 ~ -499 | 컨테이너 에러 |
| database_system | -500 ~ -599 | 데이터베이스 에러 |
| network_system | -600 ~ -699 | 네트워크 에러 |

**공통 에러 코드**:
```cpp
namespace kcenon::common::error_codes {
    constexpr int SUCCESS = 0;
    constexpr int INVALID_ARGUMENT = -1;
    constexpr int NOT_FOUND = -2;
    constexpr int PERMISSION_DENIED = -3;
    constexpr int TIMEOUT = -4;
    constexpr int CANCELLED = -5;
    constexpr int NOT_INITIALIZED = -6;
    constexpr int ALREADY_EXISTS = -7;
    constexpr int OUT_OF_MEMORY = -8;
    constexpr int IO_ERROR = -9;
    constexpr int NETWORK_ERROR = -10;
    constexpr int INTERNAL_ERROR = -11;
}
```

---

### 편의 매크로

**헤더**: `#include <kcenon/common/patterns/result.h>`

```cpp
// 표현식이 에러면 조기 반환
COMMON_RETURN_IF_ERROR(expr);

// 값 할당 또는 에러 반환
COMMON_ASSIGN_OR_RETURN(auto value, get_value());

// 조건이 참이면 에러 반환
COMMON_RETURN_ERROR_IF(condition, code, message, module);

// 상세 정보와 함께 에러 반환
COMMON_RETURN_ERROR_IF_WITH_DETAILS(condition, code, message, module, details);
```

---

## 사용 예시

### 기본 사용

```cpp
#include <kcenon/common/patterns/result.h>

using namespace kcenon::common;

Result<int> divide(int a, int b) {
    if (b == 0) {
        return Result<int>::err(error_info{1, "0으로 나눌 수 없습니다"});
    }
    return Result<int>::ok(a / b);
}

int main() {
    auto res = divide(10, 2);

    if (res.is_ok()) {
        std::cout << "결과: " << res.value() << std::endl;  // 5
    } else {
        std::cout << "에러: " << res.error().message << std::endl;
    }

    return 0;
}
```

### 인터페이스 사용

```cpp
#include <kcenon/common/interfaces/logger_interface.h>

class ConsoleLogger : public kcenon::common::interfaces::ILogger {
public:
    VoidResult log(log_level level, const std::string& message) override {
        std::cout << "[" << to_string(level) << "] " << message << std::endl;
        return ok();
    }

    VoidResult flush() override {
        std::cout.flush();
        return ok();
    }

    // ... 나머지 순수 가상 함수 구현
};

int main() {
    ConsoleLogger logger;
    logger.log(log_level::info, "애플리케이션 시작");

    return 0;
}
```

---

## 마이그레이션 가이드

### v1.x에서 v2.0으로

**변경사항**:
- `Result<T>` 기본 생성자 삭제됨 → `Result<T>::ok()` 또는 `Result<T>::err()` 사용
- `result::success()` → `Result<T>::ok()` 또는 `ok()` 헬퍼 함수
- `result::failure()` → `Result<T>::err()` 또는 `make_error<T>()` 헬퍼 함수
- `thread_count()` → `worker_count()` (IExecutor)
- `submit()`/`execute()` 템플릿 → Job 기반 `execute(std::unique_ptr<IJob>&&)`
- `log_level::fatal` → `log_level::critical`
- `log_level::off` 추가됨

**마이그레이션 예시**:
```cpp
// v1.x
auto res = Result<int>();  // 기본 생성
auto res = Result<int>::success(42);
int val = res.get_value();

// v2.0
auto res = Result<int>::uninitialized();  // 또는 명시적 ok()/err() 사용
auto res = Result<int>::ok(42);
int val = res.value();
```

---

**작성일**: 2025-11-21
**버전**: 0.2.3
**관리자**: kcenon@naver.com
