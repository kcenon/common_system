# common_system API 레퍼런스

> **버전**: 2.0
> **최종 업데이트**: 2025-11-21
> **상태**: 프로덕션 레디 (Tier 0)

## 목차

1. [네임스페이스](#네임스페이스)
2. [Result<T> 패턴 (권장)](#resultt-패턴-권장)
3. [인터페이스](#인터페이스)
4. [유틸리티](#유틸리티)

---

## 네임스페이스

### `kcenon::common`

common_system의 모든 공개 API는 이 네임스페이스에 포함됩니다.

**포함 항목**:
- `result<T>` - 결과 타입
- `result_void` - void용 결과
- 모든 인터페이스 (`IExecutor`, `ILogger`, `IMonitor`, `IDatabase`)

---

## Result<T> 패턴 (권장)

### `result<T>`

**헤더**: `#include <kcenon/common/patterns/result.h>`

**설명**: 성공 값 또는 에러를 나타내는 타입 안전 컨테이너

#### 생성자

```cpp
// 성공 값으로 생성
static auto ok(T value) -> result<T>;

// 에러로 생성
static auto err(error_info error) -> result<T>;
```

#### 핵심 메서드

##### `is_ok()` / `is_error()`

```cpp
auto is_ok() const -> bool;
auto is_error() const -> bool;
```

**설명**: Result 상태 확인

**예시**:
```cpp
result<int> res = result<int>::ok(42);
if (res.is_ok()) {
    // 성공 처리
}
```

##### `value()`

```cpp
auto value() const -> const T&;
auto value() -> T&;
```

**설명**: 성공 값 접근 (에러 상태에서 호출 시 예외 발생)

**예외**:
- `std::runtime_error`: 에러 상태에서 호출 시 발생

**예시**:
```cpp
auto res = result<int>::ok(42);
int val = res.value();  // 42
```

##### `error()`

```cpp
auto error() const -> const error_info&;
```

**설명**: 에러 정보 접근

**예시**:
```cpp
auto res = result<int>::err(error_info{1, "실패"});
auto err = res.error();
std::cout << err.message << std::endl;
```

---

### `result_void`

**헤더**: `#include <kcenon/common/patterns/result.h>`

**설명**: void 반환용 결과 (성공/실패만 표현)

#### 사용 예시

```cpp
auto process() -> result_void {
    if (/* 성공 조건 */) {
        return result_void::ok();
    }
    return result_void::err(error_info{1, "처리 실패"});
}

auto res = process();
if (res.is_ok()) {
    // 성공 처리
}
```

---

## 인터페이스

### `IExecutor`

**헤더**: `#include <kcenon/common/interfaces/i_executor.h>`

**설명**: 태스크 실행자 인터페이스

#### 순수 가상 함수

```cpp
virtual auto execute(std::function<void()> task) -> result_void = 0;
virtual auto shutdown() -> result_void = 0;
```

**구현 예시**:
```cpp
class MyExecutor : public kcenon::common::IExecutor {
public:
    auto execute(std::function<void()> task) -> result_void override {
        // 태스크 실행 구현
        task();
        return result_void::ok();
    }

    auto shutdown() -> result_void override {
        // 종료 처리
        return result_void::ok();
    }
};
```

---

### `ILogger`

**헤더**: `#include <kcenon/common/interfaces/i_logger.h>`

**설명**: 로거 인터페이스

#### 순수 가상 함수

```cpp
virtual auto log(log_level level, const std::string& message) -> result_void = 0;
virtual auto flush() -> result_void = 0;
```

**로그 레벨**:
```cpp
enum class log_level {
    trace,
    debug,
    info,
    warning,
    error,
    fatal
};
```

---

### `IMonitor`

**헤더**: `#include <kcenon/common/interfaces/i_monitor.h>`

**설명**: 모니터링 인터페이스

#### 순수 가상 함수

```cpp
virtual auto record_metric(const std::string& name, double value) -> result_void = 0;
virtual auto start_timer(const std::string& name) -> result_void = 0;
virtual auto stop_timer(const std::string& name) -> result_void = 0;
```

---

### `IDatabase`

**헤더**: `#include <kcenon/common/interfaces/i_database.h>`

**설명**: 데이터베이스 인터페이스

#### 순수 가상 함수

```cpp
virtual auto connect(const connection_info& info) -> result_void = 0;
virtual auto execute(const std::string& query) -> result<query_result> = 0;
virtual auto disconnect() -> result_void = 0;
```

---

## 유틸리티

### error_info

**구조체**:
```cpp
struct error_info {
    int code;
    std::string message;
    std::string details;
};
```

**사용 예시**:
```cpp
return result<int>::err(error_info{
    .code = 404,
    .message = "찾을 수 없음",
    .details = "리소스가 존재하지 않습니다"
});
```

---

## 사용 예시

### 기본 사용

```cpp
#include <kcenon/common/patterns/result.h>

using namespace kcenon::common;

auto divide(int a, int b) -> result<int> {
    if (b == 0) {
        return result<int>::err(error_info{1, "0으로 나눌 수 없습니다"});
    }
    return result<int>::ok(a / b);
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
#include <kcenon/common/interfaces/i_logger.h>

class ConsoleLogger : public kcenon::common::ILogger {
public:
    auto log(log_level level, const std::string& message) -> result_void override {
        std::cout << "[" << to_string(level) << "] " << message << std::endl;
        return result_void::ok();
    }

    auto flush() -> result_void override {
        std::cout.flush();
        return result_void::ok();
    }
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
- `Result<T>` → `result<T>` (소문자)
- `result::success()` → `result<T>::ok()`
- `result::failure()` → `result<T>::err()`
- `get_value()` → `value()`
- `get_error()` → `error()`

**마이그레이션 예시**:
```cpp
// v1.x
auto res = Result<int>::success(42);
int val = res.get_value();

// v2.0
auto res = result<int>::ok(42);
int val = res.value();
```

---

**작성일**: 2025-11-21
**버전**: 2.0
**관리자**: kcenon@naver.com
