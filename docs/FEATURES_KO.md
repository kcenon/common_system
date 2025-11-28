# Common System - 상세 기능

**언어:** [English](FEATURES.md) | **한국어**

이 문서는 Common System 프로젝트에서 사용 가능한 모든 기능에 대한 포괄적인 세부 정보를 제공합니다.

---

## 목차

- [핵심 장점 및 이점](#핵심-장점-및-이점)
- [핵심 컴포넌트](#핵심-컴포넌트)
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

- `BUILD_WITH_THREAD_SYSTEM`: thread_system 통합 활성화
- `BUILD_WITH_CONTAINER_SYSTEM`: container_system 통합 활성화
- `BUILD_WITH_LOGGER_SYSTEM`: logger_system 통합 활성화
- `BUILD_WITH_MONITORING_SYSTEM`: monitoring_system 통합 활성화
- `BUILD_WITH_NETWORK_SYSTEM`: network_system 통합 활성화
- `DATABASE_USE_COMMON_SYSTEM`: database_system에서 Result<T> 래퍼 활성화

**CMake 사용:**

```cmake
# 통합 활성화
set(BUILD_WITH_THREAD_SYSTEM ON)
set(BUILD_WITH_LOGGER_SYSTEM ON)
set(DATABASE_USE_COMMON_SYSTEM ON)

# 모듈 추가
add_subdirectory(common_system)
add_subdirectory(thread_system)
add_subdirectory(logger_system)
add_subdirectory(database_system)

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

**최종 업데이트**: 2025-11-28
**버전**: 1.0

---

Made with ❤️ by 🍀☀🌕🌥 🌊
