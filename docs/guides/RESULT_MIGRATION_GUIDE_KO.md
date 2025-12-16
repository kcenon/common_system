# Result\<T\> 마이그레이션 가이드

> **Language:** [English](RESULT_MIGRATION_GUIDE.md) | **한국어**

## 목차

- [개요](#개요)
- [문제 정의](#문제-정의)
- [표준 API 레퍼런스](#표준-api-레퍼런스)
  - [핵심 타입](#핵심-타입)
  - [팩토리 함수](#팩토리-함수)
  - [멤버 함수](#멤버-함수)
  - [모나딕 연산](#모나딕-연산)
  - [헬퍼 함수](#헬퍼-함수)
- [마이그레이션 단계](#마이그레이션-단계)
  - [Phase 1: Deprecated 별칭 추가](#phase-1-deprecated-별칭-추가)
  - [Phase 2: 내부 사용 업데이트](#phase-2-내부-사용-업데이트)
  - [Phase 3: Deprecated 타입 제거](#phase-3-deprecated-타입-제거)
- [시스템별 마이그레이션](#시스템별-마이그레이션)
  - [thread_system](#thread_system)
  - [database_system](#database_system)
  - [network_system](#network_system)
  - [monitoring_system](#monitoring_system)
- [하위 호환성](#하위-호환성)
- [코드 예제](#코드-예제)
- [마이그레이션 테스트](#마이그레이션-테스트)
- [FAQ](#faq)
- [참고 자료](#참고-자료)

**버전**: 1.0
**최종 업데이트**: 2025-12-16

---

## 개요

이 가이드는 모든 KCENON 생태계 시스템을 `common_system`의 표준화된 `Result<T>` 타입으로 마이그레이션하기 위한 포괄적인 지침을 제공합니다. 목표는 모든 시스템 간의 타입 일관성과 원활한 상호 운용성을 보장하는 것입니다.

**대상 독자**: KCENON 생태계 시스템을 유지보수하거나 기여하는 개발자

---

## 문제 정의

현재 생태계 전반에 걸쳐 여러 `Result<T>` 구현이 존재합니다:

| 시스템 | 파일 | 타입 이름 | 주요 메서드 |
|--------|------|-----------|-------------|
| **common_system** | `patterns/result.h` | `Result<T>` | `is_ok()`, `value()`, `unwrap()` |
| thread_system | `core/error_handling.h` | `result<T>` | `is_success()`, `value()` |
| database_system | `core/result.h` | `result<T>` | `is_ok()`, `get_error()` |
| network_system | `utils/result_types.h` | Custom result | 다양함 |
| monitoring_system | `core/result_types.h` | Custom result | 다양함 |

**여러 구현의 문제점**:

1. **타입 이름 불일치**: `Result` vs `result` (C++에서 대소문자 구분)
2. **메서드 이름 차이**: `is_ok` vs `is_success`, `value` vs `unwrap`
3. **타입 변환 필요**: 시스템 간 결과 전달 시 변환 필요
4. **컴파일 타임 안전성 저하**: 다른 타입으로 인해 템플릿 추론 실패

---

## 표준 API 레퍼런스

표준 `Result<T>` 타입은 다음에 정의되어 있습니다:

```cpp
#include <kcenon/common/patterns/result.h>
```

네임스페이스: `kcenon::common`

### 핵심 타입

```cpp
namespace kcenon::common {

// 메인 Result 타입
template<typename T>
class Result;

// 에러 정보 구조체
struct error_info {
    int code;
    std::string message;
    std::string module;
    std::optional<std::string> details;
};

// 하위 호환성을 위한 별칭
using error_code = error_info;

// void 연산을 위한 Result
using VoidResult = Result<std::monostate>;

}
```

### 팩토리 함수

```cpp
// 성공 결과 생성
template<typename T>
Result<T> ok(T value);

// void 성공 결과 생성
VoidResult ok();

// 에러 결과 생성
template<typename T>
Result<T> make_error(int code, const std::string& message,
                     const std::string& module = "");

// 상세 정보가 있는 에러 결과 생성
template<typename T>
Result<T> make_error(int code, const std::string& message,
                     const std::string& module, const std::string& details);

// error_info로부터 에러 결과 생성
template<typename T>
Result<T> make_error(const error_info& err);
```

**정적 팩토리 메서드** (Result 클래스):

```cpp
// 성공 결과 생성
Result<T>::ok(value);

// 에러 결과 생성
Result<T>::err(error_info);
Result<T>::err(code, message, module);

// 명시적으로 초기화되지 않은 결과 생성 (신중하게 사용)
Result<T>::uninitialized();
```

### 멤버 함수

```cpp
template<typename T>
class Result {
public:
    // 상태 확인
    bool is_ok() const;      // 값을 포함하면 true 반환
    bool is_err() const;     // 에러를 포함하면 true 반환

    // 값 접근
    const T& value() const;  // 값 참조 반환 (에러면 정의되지 않음)
    T& value();              // 가변 값 참조 반환
    const T& unwrap() const; // 값 반환 또는 에러시 예외 발생
    T& unwrap();             // 가변 값 반환 또는 에러시 예외 발생
    T unwrap_or(T default_value) const;  // 값 또는 기본값 반환
    T value_or(T default_value) const;   // C++23 std::expected 호환

    // 에러 접근
    const error_info& error() const;  // 에러 참조 반환
};
```

### 모나딕 연산

```cpp
// 성공 값 변환
template<typename F>
auto map(F&& func) const -> Result<decltype(func(value))>;

// Result를 반환하는 연산 체이닝
template<typename F>
auto and_then(F&& func) const -> decltype(func(value));

// 에러 시 대안 제공
template<typename F>
Result<T> or_else(F&& func) const;
```

### 헬퍼 함수

```cpp
// 자유 함수
template<typename T> bool is_ok(const Result<T>& result);
template<typename T> bool is_error(const Result<T>& result);
template<typename T> const T& get_value(const Result<T>& result);
template<typename T> const error_info& get_error(const Result<T>& result);
template<typename T> T value_or(const Result<T>& result, T default_value);
template<typename T> const T* get_if_ok(const Result<T>& result);
template<typename T> const error_info* get_if_error(const Result<T>& result);
```

---

## 마이그레이션 단계

### Phase 1: Deprecated 별칭 추가

표준 타입을 가리키는 하위 호환 별칭을 추가하고 기존 타입을 deprecated로 표시합니다.

**thread_system 예제**:

```cpp
// thread_system/core/error_handling.h

// 표준 Result 타입 임포트
#include <kcenon/common/patterns/result.h>

namespace kcenon::thread {

// Deprecated 별칭 - 다음 메이저 버전에서 제거됨
template<typename T>
using result [[deprecated("kcenon::common::Result<T>를 대신 사용하세요")]]
    = kcenon::common::Result<T>;

// 메서드 호환성 래퍼 (필요한 경우)
template<typename T>
[[deprecated("is_ok()를 대신 사용하세요")]]
inline bool is_success(const common::Result<T>& r) {
    return r.is_ok();
}

} // namespace kcenon::thread
```

### Phase 2: 내부 사용 업데이트

모든 내부 사용을 `kcenon::common::Result<T>`로 교체합니다.

**변경 전**:
```cpp
namespace kcenon::thread {

result<void> thread_pool::submit(std::function<void()> task) {
    if (!running_) {
        return result<void>::error(-1, "Pool not running");
    }
    // ...
    return result<void>::ok();
}

} // namespace kcenon::thread
```

**변경 후**:
```cpp
#include <kcenon/common/patterns/result.h>

namespace kcenon::thread {

using common::Result;
using common::VoidResult;
using common::ok;
using common::make_error;
using common::error_codes::INVALID_STATE;

VoidResult thread_pool::submit(std::function<void()> task) {
    if (!running_) {
        return make_error<std::monostate>(INVALID_STATE,
            "Pool not running", "thread_pool");
    }
    // ...
    return ok();
}

} // namespace kcenon::thread
```

### Phase 3: Deprecated 타입 제거

Deprecation 기간(일반적으로 하나의 메이저 버전) 후, deprecated 별칭을 제거하고 모든 사용자가 표준 타입을 직접 사용하도록 합니다.

**타임라인 권장**:
- **패치 릴리스 (x.y.z)**: `[[deprecated]]`와 함께 별칭 추가
- **마이너 릴리스 (x.Y.0)**: 런타임에 deprecation 경고 로깅
- **메이저 릴리스 (X.0.0)**: deprecated 타입 제거

---

## 시스템별 마이그레이션

### thread_system

**현재 위치**: `core/error_handling.h`
**현재 타입**: `is_success()` 메서드를 가진 `result<T>`

**마이그레이션 체크리스트**:
- [ ] `#include <kcenon/common/patterns/result.h>` 추가
- [ ] deprecated 별칭 생성: `template<typename T> using result = common::Result<T>;`
- [ ] `is_success()` 호환성 래퍼 추가
- [ ] 모든 내부 `result<T>`를 `common::Result<T>`로 업데이트
- [ ] 팩토리 호출 업데이트: `result::ok()` → `common::ok()`
- [ ] 팩토리 호출 업데이트: `result::error()` → `common::make_error<T>()`
- [ ] 테스트 업데이트

### database_system

**현재 위치**: `core/result.h`
**현재 타입**: `get_error()` 메서드를 가진 `result<T>`

**마이그레이션 체크리스트**:
- [ ] `#include <kcenon/common/patterns/result.h>` 추가
- [ ] deprecated 별칭 생성
- [ ] `get_error()` 호환성 확인 (표준은 `error()` 사용)
- [ ] 모든 내부 사용 업데이트
- [ ] 테스트 업데이트

### network_system

**현재 위치**: `utils/result_types.h`
**현재 타입**: 커스텀 result 타입

**마이그레이션 체크리스트**:
- [ ] 현재 API 차이점 분석
- [ ] `#include <kcenon/common/patterns/result.h>` 추가
- [ ] 모든 커스텀 타입에 대해 deprecated 별칭 생성
- [ ] 커스텀 메서드용 호환성 래퍼 생성
- [ ] 모든 내부 사용 업데이트
- [ ] 테스트 업데이트

### monitoring_system

**현재 위치**: `core/result_types.h`
**현재 타입**: 커스텀 result 타입

**마이그레이션 체크리스트**:
- [ ] 현재 API 차이점 분석
- [ ] `#include <kcenon/common/patterns/result.h>` 추가
- [ ] deprecated 별칭 생성
- [ ] 모든 내부 사용 업데이트
- [ ] 테스트 업데이트

---

## 하위 호환성

마이그레이션 중 하위 호환성을 유지하려면 다음 패턴을 사용하세요:

### 1. 네임스페이스 별칭

```cpp
namespace kcenon::your_system {
    // common 타입을 쉽게 접근할 수 있도록
    using common::Result;
    using common::VoidResult;
    using common::error_info;
    using common::ok;
    using common::make_error;
}
```

### 2. 메서드 래퍼

시스템에서 다른 메서드 이름을 사용했다면:

```cpp
namespace kcenon::your_system::compat {

template<typename T>
[[deprecated("is_ok()를 대신 사용하세요")]]
inline bool is_success(const common::Result<T>& r) {
    return r.is_ok();
}

template<typename T>
[[deprecated("error()를 대신 사용하세요")]]
inline const common::error_info& get_error(const common::Result<T>& r) {
    return r.error();
}

} // namespace kcenon::your_system::compat
```

### 3. 매크로 마이그레이션

커스텀 매크로가 있었다면, 표준 매크로에 매핑하세요:

```cpp
// 기존 매크로
#define THREAD_RETURN_IF_ERROR(expr) /* ... */

// Deprecation 매핑
#define THREAD_RETURN_IF_ERROR(expr) \
    _Pragma("message(\"THREAD_RETURN_IF_ERROR is deprecated, use COMMON_RETURN_IF_ERROR\")") \
    COMMON_RETURN_IF_ERROR(expr)
```

---

## 코드 예제

### 예제 1: 기본 마이그레이션

**변경 전** (thread_system 스타일):
```cpp
result<int> parse_number(const std::string& s) {
    try {
        return result<int>::ok(std::stoi(s));
    } catch (...) {
        return result<int>::error(-1, "Parse failed");
    }
}

void example() {
    auto r = parse_number("42");
    if (r.is_success()) {
        std::cout << r.value() << "\n";
    }
}
```

**변경 후** (common_system 스타일):
```cpp
#include <kcenon/common/patterns/result.h>

using namespace kcenon::common;

Result<int> parse_number(const std::string& s) {
    return try_catch<int>([&]() {
        return std::stoi(s);
    }, "parser");
}

void example() {
    auto r = parse_number("42");
    if (r.is_ok()) {
        std::cout << r.value() << "\n";
    }
}
```

### 예제 2: 시스템 간 상호 운용성

```cpp
#include <kcenon/common/patterns/result.h>
#include <kcenon/database/database.h>
#include <kcenon/network/client.h>

using namespace kcenon::common;

// 모든 시스템이 이제 동일한 Result<T> 타입을 반환
Result<UserData> fetch_user(int user_id) {
    // 데이터베이스 쿼리 - common::Result 반환
    auto db_result = database::query_user(user_id);
    if (db_result.is_err()) {
        return db_result;  // 변환 필요 없음!
    }

    // 네트워크 요청 - 마찬가지로 common::Result 반환
    auto net_result = network::fetch_profile(db_result.value().profile_url);
    if (net_result.is_err()) {
        return net_result;  // 원활한 에러 전파
    }

    return ok(UserData{db_result.value(), net_result.value()});
}
```

### 예제 3: 모나딕 연산 사용

```cpp
Result<ProcessedData> process_pipeline(const RawData& input) {
    return validate(input)
        .and_then([](const ValidData& v) { return transform(v); })
        .and_then([](const TransformedData& t) { return enrich(t); })
        .map([](const EnrichedData& e) { return ProcessedData(e); })
        .or_else([](const error_info& err) -> Result<ProcessedData> {
            log_error("Pipeline failed: {}", err.message);
            return make_error<ProcessedData>(err);
        });
}
```

---

## 마이그레이션 테스트

### 단위 테스트 업데이트

**변경 전**:
```cpp
TEST(ThreadPool, SubmitTask) {
    auto result = pool.submit(task);
    EXPECT_TRUE(result.is_success());
}
```

**변경 후**:
```cpp
TEST(ThreadPool, SubmitTask) {
    auto result = pool.submit(task);
    EXPECT_TRUE(result.is_ok());  // 메서드 이름 변경
}
```

### 컴파일 검증

마이그레이션 후 deprecation 경고가 나타나는지 확인:

```bash
# 경고를 활성화하여 빌드
cmake -DCMAKE_CXX_FLAGS="-Wall -Wdeprecated" ..
make 2>&1 | grep -i deprecated
```

### 통합 테스트

시스템 간 상호 운용성 테스트:

```cpp
TEST(CrossSystem, ResultCompatibility) {
    // 다른 시스템의 Result가 호환되어야 함
    common::Result<int> common_result = common::ok(42);

    // 상호 교환적으로 사용 가능
    EXPECT_TRUE(common_result.is_ok());
    EXPECT_EQ(common_result.value(), 42);
}
```

---

## FAQ

### Q: 왜 common_system의 Result<T>로 표준화하나요?

**A**: common_system의 `Result<T>` 구현은:
- 완전한 Rust 스타일 API 제공 (`map`, `and_then`, `or_else`)
- C++23 `std::expected` 호환 (`value_or`)
- 종합적인 에러 정보 (`error_info`)
- 헬퍼 매크로 및 함수 포함
- 충분히 테스트되고 문서화됨

### Q: 성능에 대해서는?

**A**: 표준 `Result<T>`는 내부적으로 `std::optional<T>`를 사용하며, 값 저장에 대한 오버헤드가 없습니다. 에러 경로는 `std::optional<error_info>`로 인해 약간의 오버헤드가 있을 수 있지만, 실제로는 무시할 수 있는 수준입니다.

### Q: 시스템의 커스텀 메서드를 계속 사용할 수 있나요?

**A**: 네, 마이그레이션 기간 동안 가능합니다. 표준 메서드에 매핑되는 deprecated 래퍼를 생성하세요. 다음 메이저 버전에서 제거할 계획을 세우세요.

### Q: type-erased results는 어떻게 처리하나요?

**A**: void 연산에는 `VoidResult` (`Result<std::monostate>`)를 사용하거나, type-erased 값에는 `Result<std::any>`를 고려하세요 (적절한 문서화 필요).

### Q: 커스텀 에러 타입이 필요하면 어떻게 하나요?

**A**: `error_info` 구조체에는 추가 컨텍스트를 위한 `details` 필드 (`std::optional<std::string>`)가 있습니다. 구조화된 커스텀 데이터의 경우, details 필드에 직렬화하거나 모듈에서 error_info를 확장하세요 (상호 운용성을 위해 권장되지 않음).

---

## 참고 자료

- [에러 처리 가이드라인](ERROR_HANDLING_KO.md) - 포괄적인 에러 처리 패턴
- [API 레퍼런스](../API_REFERENCE_KO.md) - 전체 API 문서
- [에러 코드 레지스트리](../ERROR_CODE_REGISTRY_KO.md) - 중앙화된 에러 코드
- [모범 사례](BEST_PRACTICES.md) - 일반 모범 사례

---

*이 마이그레이션 가이드에 대한 질문이나 문제가 있으면 common_system 레포지토리에 이슈를 열어주세요.*
