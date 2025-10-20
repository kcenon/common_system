# 오류 처리 가이드라인

> **Language:** [English](ERRORS.md) | **한국어**

## 목차

- [개요](#개요)
- [권장 패턴: Result<T>](#권장-패턴-resultt)
  - [기본 Result 타입 정의](#기본-result-타입-정의)
- [사용 예제](#사용-예제)
  - [Result를 반환하는 함수](#result를-반환하는-함수)
  - [Result 처리](#result-처리)
- [마이그레이션 전략](#마이그레이션-전략)
  - [Phase 1: 가이드라인 (현재)](#phase-1-가이드라인-현재)
  - [Phase 2: 점진적 채택](#phase-2-점진적-채택)
  - [Phase 3: 표준화](#phase-3-표준화)
- [모듈별 적응](#모듈별-적응)
  - [thread_system](#thread_system)
  - [logger_system](#logger_system)
  - [network_system](#network_system)
  - [database_system](#database_system)
- [오류 코드 범위](#오류-코드-범위)
- [공통 오류 코드](#공통-오류-코드)
- [모범 사례](#모범-사례)
- [성능 고려사항](#성능-고려사항)
- [향후 개선 사항](#향후-개선-사항)

## 개요

이 문서는 일관성과 유지보수성을 보장하기 위해 모든 시스템 모듈에 대한 표준화된 오류 처리 패턴을 정의합니다.

## 권장 패턴: Result<T>

모듈 경계(API)에서 오류 처리를 위해 Result 타입을 사용하고, 구현 세부 사항에는 내부적으로 예외를 허용하는 것을 권장합니다.

### 기본 Result 타입 정의

```cpp
#include <variant>
#include <optional>
#include <string>

namespace common {

/**
 * @struct error_info
 * @brief 표준 오류 정보
 */
struct error_info {
    int code;
    std::string message;
    std::string module;
    std::optional<std::string> details;

    error_info(int c, const std::string& msg, const std::string& mod = "")
        : code(c), message(msg), module(mod) {}
};

/**
 * @brief 오류 처리를 위한 Result 타입
 *
 * std::expected (C++23) 또는 Rust의 Result<T,E>와 유사
 */
template<typename T>
using Result = std::variant<T, error_info>;

/**
 * @brief result가 값을 포함하는지 확인
 */
template<typename T>
bool is_ok(const Result<T>& result) {
    return std::holds_alternative<T>(result);
}

/**
 * @brief result가 오류를 포함하는지 확인
 */
template<typename T>
bool is_error(const Result<T>& result) {
    return std::holds_alternative<error_info>(result);
}

/**
 * @brief result에서 값 가져오기 (오류면 예외 발생)
 */
template<typename T>
const T& get_value(const Result<T>& result) {
    return std::get<T>(result);
}

/**
 * @brief result에서 오류 가져오기 (값이면 예외 발생)
 */
template<typename T>
const error_info& get_error(const Result<T>& result) {
    return std::get<error_info>(result);
}

/**
 * @brief 값 또는 기본값 가져오기
 */
template<typename T>
T value_or(const Result<T>& result, T default_value) {
    if (is_ok(result)) {
        return get_value(result);
    }
    return default_value;
}

} // namespace common
```

## 사용 예제

### Result를 반환하는 함수

```cpp
#include <common/result.h>

common::Result<std::string> read_config(const std::string& path) {
    if (path.empty()) {
        return common::error_info{
            -1, "Path cannot be empty", "config_reader"
        };
    }

    try {
        // 내부 구현에서 예외 사용 가능
        auto content = internal_read_file(path);
        return content;  // 성공 케이스
    } catch (const std::exception& e) {
        // 경계에서 예외를 Result로 변환
        return common::error_info{
            -2, e.what(), "config_reader"
        };
    }
}
```

### Result 처리

```cpp
// 방법 1: 헬퍼 함수 사용
auto result = read_config("app.conf");
if (common::is_ok(result)) {
    const auto& config = common::get_value(result);
    process_config(config);
} else {
    const auto& error = common::get_error(result);
    log_error(error.code, error.message);
}

// 방법 2: std::visit 사용
std::visit([](auto&& arg) {
    using T = std::decay_t<decltype(arg)>;
    if constexpr (std::is_same_v<T, std::string>) {
        // 성공 처리
        process_config(arg);
    } else if constexpr (std::is_same_v<T, common::error_info>) {
        // 오류 처리
        log_error(arg.code, arg.message);
    }
}, result);

// 방법 3: value_or 사용
auto config = common::value_or(result, std::string("default.conf"));
```

## 마이그레이션 전략

### Phase 1: 가이드라인 (현재)
- Result 패턴 문서화
- 헬퍼 함수 및 예제 제공
- 변경 사항 없음

### Phase 2: 점진적 채택
- 새 API는 공개 인터페이스에 Result<T> 사용
- 내부 구현에서 예외 계속 사용 가능
- 모듈 경계에 어댑터 추가

### Phase 3: 표준화
- 폐기 공지와 함께 점진적으로 기존 API 마이그레이션
- 오버로드를 통해 하위 호환성 유지
- 릴리스 노트에 마이그레이션 문서화

## 모듈별 적응

### thread_system
```cpp
// 현재 (예외 기반)
std::future<void> submit(std::function<void()> task);

// 미래 (Result 기반)
common::Result<std::future<void>> submit_safe(std::function<void()> task);
```

### logger_system
```cpp
// 현재
void log(LogLevel level, const std::string& message);

// 미래
common::Result<void> log_safe(LogLevel level, const std::string& message);
```

### network_system
```cpp
// 현재
void send_message(const Message& msg);

// 미래
common::Result<void> send_message_safe(const Message& msg);
```

### database_system
```cpp
// 현재
QueryResult execute_query(const std::string& sql);

// 미래
common::Result<QueryResult> execute_query_safe(const std::string& sql);
```

## 오류 코드 범위

충돌을 피하기 위해 각 모듈은 특정 오류 코드 범위를 사용해야 합니다:

| 모듈 | 오류 코드 범위 | 설명 |
|-------------------|------------------|------------------------------|
| Common            | -1 to -99        | 공통/일반 오류 |
| thread_system     | -100 to -199     | 스레딩 오류 |
| logger_system     | -200 to -299     | 로깅 오류 |
| monitoring_system | -300 to -399     | 모니터링 오류 |
| container_system  | -400 to -499     | 컨테이너/직렬화 오류 |
| database_system   | -500 to -599     | 데이터베이스 오류 |
| network_system    | -600 to -699     | 네트워크 오류 |

## 공통 오류 코드

```cpp
namespace common {
namespace error_codes {
    constexpr int SUCCESS = 0;
    constexpr int INVALID_ARGUMENT = -1;
    constexpr int NOT_FOUND = -2;
    constexpr int PERMISSION_DENIED = -3;
    constexpr int TIMEOUT = -4;
    constexpr int CANCELLED = -5;
    constexpr int NOT_INITIALIZED = -6;
    constexpr int ALREADY_EXISTS = -7;
    constexpr int OUT_OF_MEMORY = -8;
    constexpr int INTERNAL_ERROR = -99;
}
}
```

## 모범 사례

1. **API 경계에서 Result<T> 사용** - 공개 함수는 Result 반환
2. **내부적으로 예외 허용** - 구현 세부 사항에서 예외 사용 가능
3. **경계에서 변환** - 모듈 인터페이스에서 예외를 Result로 변환
4. **명확한 오류 메시지 제공** - 컨텍스트와 제안된 수정 포함
5. **적절하게 오류 로깅** - 오류에 대한 구조화된 로깅 사용
6. **오류 조건 문서화** - 함수 문서에 가능한 오류 코드 나열
7. **오류 경로 테스트** - 단위 테스트에 오류 케이스 포함

## 성능 고려사항

- Result<T>는 최소한의 오버헤드 (std::variant와 유사)
- 오류 정보에 대한 동적 할당 없음
- 오류가 드문 경우 분기 예측 친화적
- 성능 중요 경로에는 `noexcept` 고려

## 향후 개선 사항

C++23의 `std::expected`가 널리 사용 가능해지면:
```cpp
template<typename T>
using Result = std::expected<T, error_info>;
```

이는 모나딕 연산으로 더 나은 사용성을 제공합니다:
```cpp
auto result = read_config("app.conf")
    .and_then(parse_config)
    .or_else(load_default_config);
```

---

*Last Updated: 2025-10-20*
