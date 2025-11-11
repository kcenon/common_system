# Error Handling 가이드라인

> **Language:** [English](ERROR_HANDLING.md) | **한국어**


## 목차

- [개요](#개요)
- [원칙](#원칙)
  - [1. 예외 없는 에러 처리](#1-예외-없는-에러-처리)
  - [2. 중앙 집중식 에러 코드](#2-중앙-집중식-에러-코드)
  - [3. 타입 안전한 에러 전파](#3-타입-안전한-에러-전파)
- [Result<T> 사용법](#resultt-사용법)
  - [기본 사용법](#기본-사용법)
  - [팩토리 함수](#팩토리-함수)
  - [Result 검사](#result-검사)
  - [Result Unwrapping](#result-unwrapping)
- [모나딕 연산](#모나딕-연산)
  - [map - 성공 값 변환](#map---성공-값-변환)
  - [and_then - 작업 체이닝](#and_then---작업-체이닝)
  - [or_else - 에러 처리](#or_else---에러-처리)
- [에러 전파 패턴](#에러-전파-패턴)
  - [패턴 1: 조기 반환](#패턴-1-조기-반환)
  - [패턴 2: RETURN_IF_ERROR 매크로](#패턴-2-return_if_error-매크로)
  - [패턴 3: ASSIGN_OR_RETURN 매크로](#패턴-3-assign_or_return-매크로)
  - [패턴 4: 모나딕 체이닝](#패턴-4-모나딕-체이닝)
- [에러 코드 사용법](#에러-코드-사용법)
  - [에러 코드 정의](#에러-코드-정의)
  - [에러 코드 사용](#에러-코드-사용)
  - [에러 메시지 가져오기](#에러-메시지-가져오기)
- [모범 사례](#모범-사례)
  - [1. 모듈 경계에서 Result<T> 사용](#1-모듈-경계에서-resultt-사용)
  - [2. 에러에 컨텍스트 제공](#2-에러에-컨텍스트-제공)
  - [3. 적절한 에러 코드 사용](#3-적절한-에러-코드-사용)
  - [4. 에러 조건 문서화](#4-에러-조건-문서화)
  - [5. 에러를 적절하게 처리](#5-에러를-적절하게-처리)
- [예외 변환](#예외-변환)
- [마이그레이션 가이드](#마이그레이션-가이드)
  - [1단계: 에러 코드 추가](#1단계-에러-코드-추가)
  - [2단계: 함수 시그니처 업데이트](#2단계-함수-시그니처-업데이트)
  - [3단계: 예외 교체](#3단계-예외-교체)
  - [4단계: 호출자 업데이트](#4단계-호출자-업데이트)
- [테스팅](#테스팅)
  - [성공 케이스 테스팅](#성공-케이스-테스팅)
  - [에러 케이스 테스팅](#에러-케이스-테스팅)
  - [에러 전파 테스팅](#에러-전파-테스팅)
- [성능 고려사항](#성능-고려사항)
  - [Result<T> 성능](#resultt-성능)
  - [최적화 팁](#최적화-팁)
- [일반적인 패턴](#일반적인-패턴)
  - [패턴: 리소스 획득](#패턴-리소스-획득)
  - [패턴: 검증 체인](#패턴-검증-체인)
  - [패턴: 에러 복구](#패턴-에러-복구)
- [참고 자료](#참고-자료)

**버전**: 1.0
**최종 업데이트**: 2025-10-09

---

## 개요

이 문서는 `Result<T>` 패턴을 사용하여 모든 시스템에 걸친 포괄적인 에러 처리 가이드라인을 제공합니다. 목표는 명확한 에러 전파와 함께 예외 없는, 타입 안전한 에러 처리를 제공하는 것입니다.

---

## 원칙

### 1. 예외 없는 에러 처리

모듈 경계에서 모든 에러 발생 가능 작업에 `Result<T>`를 사용합니다.

**이유는?**
- 함수 시그니처에서 명시적 에러 처리
- 컴파일러가 강제하는 에러 검사
- 더 나은 성능 (예외 unwinding 없음)
- 명확한 에러 전파 경로

### 2. 중앙 집중식 에러 코드

모든 에러 코드는 시스템별 지정된 범위와 함께 `error/error_codes.h`에 정의됩니다.

**에러 코드 범위**:
```cpp
0:            성공
-1 to -99:    공통 에러
-100 to -199: thread_system
-200 to -299: logger_system
-300 to -399: monitoring_system
-400 to -499: container_system
-500 to -599: database_system
-600 to -699: network_system
```

### 3. 타입 안전한 에러 전파

에러 처리 로직 구성을 위해 모나딕 연산(`map`, `and_then`, `or_else`)을 사용합니다.

---

## Result<T> 사용법

### 기본 사용법

```cpp
#include <kcenon/common/patterns/result.h>
#include <kcenon/common/error/error_codes.h>

using namespace common;

// 실패할 수 있는 함수
Result<int> divide(int a, int b) {
    if (b == 0) {
        return error<int>(
            error::codes::common::invalid_argument,
            "Division by zero",
            "math"
        );
    }
    return ok(a / b);
}

// result 사용하기
void example() {
    auto result = divide(10, 2);

    if (result.is_ok()) {
        std::cout << "Result: " << result.value() << std::endl;
    } else {
        std::cout << "Error: " << result.error().message << std::endl;
    }
}
```

### 팩토리 함수

```cpp
// 성공
Result<int> success = ok(42);
Result<void> void_success = ok();  // void 작업용

// 에러
Result<int> failure = error<int>(
    error::codes::common::not_found,
    "Resource not found",
    "resource_manager"
);

// 세부 정보 포함
Result<int> detailed_error = error<int>(
    error::codes::common::io_error,
    "File operation failed",
    "file_system",
    "Permission denied for /etc/config"
);
```

### Result 검사

```cpp
Result<int> result = some_operation();

// 방법 1: is_ok / is_err
if (result.is_ok()) {
    // 안전하게 값 접근
    int value = result.value();
}

// 방법 2: 헬퍼 함수
if (is_ok(result)) {
    int value = get_value(result);
}

if (is_error(result)) {
    const auto& err = get_error(result);
    log_error(err.message);
}
```

### Result Unwrapping

```cpp
// unwrap() - 에러 시 throw (신중하게 사용)
try {
    int value = result.unwrap();
} catch (const std::runtime_error& e) {
    // 예외 처리
}

// unwrap_or() - 기본값 제공
int value = result.unwrap_or(0);

// get_if_ok() - 포인터 또는 nullptr 반환
if (const int* value = get_if_ok(result)) {
    std::cout << *value << std::endl;
}
```

---

## 모나딕 연산

### map - 성공 값 변환

성공한 Result 내부의 값을 변환합니다.

```cpp
Result<int> calculate() {
    return ok(10);
}

// int를 string으로 변환
Result<std::string> result = calculate()
    .map([](int x) { return std::to_string(x * 2); });

// result는 "20"을 포함
```

### and_then - 작업 체이닝

Result를 반환하는 작업들을 체이닝합니다 (flatMap/bind).

```cpp
Result<int> parse_int(const std::string& str) {
    try {
        return ok(std::stoi(str));
    } catch (...) {
        return error<int>(
            error::codes::common::invalid_argument,
            "Invalid integer format",
            "parser"
        );
    }
}

Result<int> validate_positive(int value) {
    if (value > 0) {
        return ok(value);
    }
    return error<int>(
        error::codes::common::invalid_argument,
        "Value must be positive",
        "validator"
    );
}

// 작업 체이닝
Result<std::string> input = ok(std::string("42"));
Result<int> result = input
    .and_then(parse_int)
    .and_then(validate_positive);
```

### or_else - 에러 처리

에러 발생 시 대안 동작을 제공합니다.

```cpp
Result<int> result = risky_operation()
    .or_else([](const error_info& err) -> Result<int> {
        log_error(err.message);
        return ok(0);  // 기본값
    });
```

---

## 에러 전파 패턴

### 패턴 1: 조기 반환

```cpp
Result<int> complex_operation() {
    auto step1 = first_step();
    if (step1.is_err()) {
        return error<int>(step1.error());
    }

    auto step2 = second_step(step1.value());
    if (step2.is_err()) {
        return error<int>(step2.error());
    }

    return ok(step2.value() * 2);
}
```

### 패턴 2: RETURN_IF_ERROR 매크로

```cpp
Result<int> complex_operation() {
    RETURN_IF_ERROR(first_step());
    RETURN_IF_ERROR(second_step());

    return ok(42);
}
```

### 패턴 3: ASSIGN_OR_RETURN 매크로

```cpp
Result<int> complex_operation() {
    ASSIGN_OR_RETURN(auto value1, first_step());
    ASSIGN_OR_RETURN(auto value2, second_step(value1));

    return ok(value1 + value2);
}
```

### 패턴 4: 모나딕 체이닝

```cpp
Result<int> complex_operation() {
    return first_step()
        .and_then(second_step)
        .and_then(third_step)
        .map([](auto x) { return x * 2; });
}
```

---

## 에러 코드 사용법

### 에러 코드 정의

에러 코드는 `error/error_codes.h`에서 중앙 집중식으로 정의됩니다:

```cpp
namespace common {
namespace error {
namespace codes {

namespace my_system {
    constexpr int base = -700;  // 새 시스템 범위

    constexpr int operation_failed = base + 0;
    constexpr int invalid_state = base + 1;
    // ...
}

} // namespace codes
} // namespace error
} // namespace common
```

### 에러 코드 사용

```cpp
#include <kcenon/common/error/error_codes.h>

using namespace common::error;

Result<void> start_server(int port) {
    if (port < 1024) {
        return error<std::monostate>(
            codes::network_system::bind_failed,
            "Cannot bind to privileged port",
            "server"
        );
    }

    // 서버 시작...
    return ok();
}
```

### 에러 메시지 가져오기

```cpp
int code = error::codes::logger_system::file_open_failed;

// 메시지 가져오기
std::string_view msg = error::get_error_message(code);
// "Failed to open log file"

// 카테고리 가져오기
std::string_view category = error::get_category_name(code);
// "LoggerSystem"
```

---

## 모범 사례

### 1. 모듈 경계에서 Result<T> 사용

```cpp
// 좋은 예: 공개 API가 Result 반환
class DatabaseManager {
public:
    Result<Connection> connect(const std::string& url);
    Result<QueryResult> execute(const std::string& query);
};

// 내부 함수는 예외 사용 가능
private:
    void internal_helper() {
        // throw 가능 - 경계에서 catch하여 변환
    }
```

### 2. 에러에 컨텍스트 제공

```cpp
// 나쁜 예: 일반적인 에러
return error<int>(
    error::codes::common::io_error,
    "Error",
    "system"
);

// 좋은 예: 컨텍스트가 포함된 구체적인 에러
return error<int>(
    error::codes::database_system::connection_failed,
    "Failed to connect to PostgreSQL",
    "connection_pool",
    "Host: localhost:5432, Database: myapp"
);
```

### 3. 적절한 에러 코드 사용

```cpp
// 나쁜 예: 잘못된 카테고리 사용
return error<int>(
    error::codes::logger_system::file_open_failed,  // 잘못됨!
    "Database connection failed",
    "database"
);

// 좋은 예: 올바른 카테고리 사용
return error<int>(
    error::codes::database_system::connection_failed,
    "Database connection failed",
    "database"
);
```

### 4. 에러 조건 문서화

```cpp
/**
 * @brief 데이터베이스 연결 열기
 * @param url 연결 URL
 * @return 성공 시 Result<Connection>
 *
 * 에러 조건:
 * - database_system::connection_failed: 데이터베이스 서버에 연결할 수 없음
 * - database_system::connection_timeout: 연결 시도 시간 초과
 * - common::invalid_argument: 잘못된 연결 URL 형식
 */
Result<Connection> connect(const std::string& url);
```

### 5. 에러를 적절하게 처리

```cpp
auto result = operation();

result
    .map([](auto value) {
        // 성공 처리
        process(value);
        return value;
    })
    .or_else([](const error_info& err) {
        // 에러 로깅
        logger->error("Operation failed: {}", err.message);

        // 모니터링 알림
        if (err.code == error::codes::common::out_of_memory) {
            monitor->alert("Critical: Out of memory");
        }

        return ok(default_value());
    });
```

---

## 예외 변환

예외 기반 코드와 인터페이싱하기 위해:

```cpp
#include <kcenon/common/patterns/result.h>

// 예외를 Result로 변환
Result<int> safe_operation() {
    return try_catch<int>([&]() {
        return risky_function_that_throws();
    }, "operation_name");
}

// void 작업용
VoidResult safe_void_operation() {
    return try_catch_void([&]() {
        risky_void_function();
    }, "operation_name");
}
```

---

## 마이그레이션 가이드

### 1단계: 에러 코드 추가

`error/error_codes.h`에 시스템별 에러 코드 추가:

```cpp
namespace my_system {
    constexpr int base = -800;
    constexpr int specific_error = base + 0;
}
```

### 2단계: 함수 시그니처 업데이트

```cpp
// 이전
void process_data(const Data& data);

// 이후
Result<void> process_data(const Data& data);
```

### 3단계: 예외 교체

```cpp
// 이전
void connect(const std::string& url) {
    if (url.empty()) {
        throw std::invalid_argument("Empty URL");
    }
}

// 이후
Result<void> connect(const std::string& url) {
    if (url.empty()) {
        return error<std::monostate>(
            error::codes::common::invalid_argument,
            "Empty URL",
            "connect"
        );
    }
    return ok();
}
```

### 4단계: 호출자 업데이트

```cpp
// 이전
try {
    connect(url);
} catch (const std::exception& e) {
    handle_error(e.what());
}

// 이후
auto result = connect(url);
if (result.is_err()) {
    handle_error(result.error().message);
}
```

---

## 테스팅

### 성공 케이스 테스팅

```cpp
TEST(MyTest, SuccessfulOperation) {
    auto result = my_function(valid_input);

    ASSERT_TRUE(result.is_ok());
    EXPECT_EQ(42, result.value());
}
```

### 에러 케이스 테스팅

```cpp
TEST(MyTest, ErrorHandling) {
    auto result = my_function(invalid_input);

    ASSERT_TRUE(result.is_err());
    EXPECT_EQ(
        error::codes::common::invalid_argument,
        result.error().code
    );
    EXPECT_EQ("Invalid input", result.error().message);
}
```

### 에러 전파 테스팅

```cpp
TEST(MyTest, ErrorPropagation) {
    // 에러가 올바르게 전파되는지 테스트
    auto result = complex_operation();

    ASSERT_TRUE(result.is_err());
    EXPECT_EQ("Step 1 failed", result.error().message);
    EXPECT_EQ("step1", result.error().module);
}
```

---

## 성능 고려사항

### Result<T> 성능

- **크기**: `sizeof(Result<T>) ≈ sizeof(T) + sizeof(error_info) + alignment`
- **복사 비용**: T 또는 error_info 복사와 동일
- **이동 비용**: T 또는 error_info 이동과 동일 (일반적으로 저렴)
- **인라인**: 최적화가 활성화된 상태에서 대부분의 작업이 잘 인라인됨

### 최적화 팁

1. **이동 시맨틱 사용**:
   ```cpp
   return ok(std::move(large_object));
   ```

2. **불필요한 복사 피하기**:
   ```cpp
   // 좋음: const 참조로 전달
   Result<void> process(const Result<Data>& input);

   // 좋음: 소비 시 이동
   Result<void> process(Result<Data>&& input);
   ```

3. **큰 값에는 참조 사용**:
   ```cpp
   if (result.is_ok()) {
       const auto& value = result.value();  // 복사 없음
   }
   ```

---

## 일반적인 패턴

### 패턴: 리소스 획득

```cpp
class FileHandle {
public:
    static Result<FileHandle> open(const std::string& path) {
        FILE* file = fopen(path.c_str(), "r");
        if (!file) {
            return error<FileHandle>(
                error::codes::logger_system::file_open_failed,
                "Cannot open file",
                "FileHandle",
                path
            );
        }
        return ok(FileHandle(file));
    }

private:
    explicit FileHandle(FILE* f) : file_(f) {}
    std::unique_ptr<FILE, decltype(&fclose)> file_;
};
```

### 패턴: 검증 체인

```cpp
Result<User> create_user(const std::string& name, int age) {
    return validate_name(name)
        .and_then([age](auto) { return validate_age(age); })
        .and_then([name, age](auto) {
            return User::create(name, age);
        });
}
```

### 패턴: 에러 복구

```cpp
Result<Config> load_config() {
    return load_from_file("config.json")
        .or_else([](const error_info&) {
            // 폴백 시도
            return load_from_file("config.default.json");
        })
        .or_else([](const error_info&) {
            // 하드코딩된 기본값 사용
            return ok(Config::default_config());
        });
}
```

---

## 참고 자료

- [Result<T> 구현](../include/kcenon/common/patterns/result.h)
- [에러 코드 레지스트리](../include/kcenon/common/error/error_codes.h)
- [RAII 가이드라인](RAII_GUIDELINES.md)
- [스마트 포인터 가이드라인](SMART_POINTER_GUIDELINES.md)

---

*Last Updated: 2025-10-20*
