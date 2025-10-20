# Common System - 개선 계획

> **Language:** [English](IMPROVEMENTS.md) | **한국어**

## 현재 상태

**버전:** 1.0.0
**최종 검토:** 2025-01-20
**전체 점수:** 4.0/5

### 강점
- 우수한 Result<T> 패턴 구현 (Rust 스타일)
- 깔끔한 인터페이스 정의
- 강력한 타입 안정성
- 포괄적인 오류 코드 시스템

### 개선이 필요한 영역
- 예외 매핑이 RTTI에 의존
- 일부 예제에서 네임스페이스 오염
- C++20 기능 활용 제한적

---

## 치명적 이슈

common_system 코어에서 치명적 이슈는 발견되지 않았습니다.

---

## 고우선순위 개선사항

### 1. Exception Mapper - RTTI 의존성 제거

**위치:** `include/kcenon/common/patterns/result.h:558-593`

**현재 문제:**
```cpp
static error_info map_exception(const std::exception& e, const std::string& module = "") {
    // dynamic_cast 사용 - RTTI 필요
    if (dynamic_cast<const std::bad_alloc*>(&e)) { ... }
    if (dynamic_cast<const std::system_error*>(&e)) { ... }
    // ...
}
```

**문제점:**
- `-fno-rtti` 컴파일러 플래그와 호환 불가
- dynamic_cast로 인한 성능 오버헤드
- 임베디드 시스템에 부적합

**제안된 해결책:**
```cpp
// 옵션 1: Type-erased exception wrapper
class typed_exception_wrapper {
    std::exception_ptr ptr_;
    std::type_index type_;
    std::function<error_info(const std::exception&)> mapper_;

public:
    template<typename E>
    typed_exception_wrapper(const E& e)
        : ptr_(std::make_exception_ptr(e))
        , type_(typeid(E))
        , mapper_([](const std::exception& ex) -> error_info {
            return map_specific_exception(static_cast<const E&>(ex));
        }) {}

    error_info map_to_error_info() const;
};

// 옵션 2: 컴파일 타임 디스패치
template<typename E>
error_info map_exception_impl(const E& e, std::string_view module) {
    if constexpr (std::is_same_v<E, std::bad_alloc>) {
        return {error_codes::OUT_OF_MEMORY, e.what(), std::string(module)};
    } else if constexpr (std::is_base_of_v<std::system_error, E>) {
        return {e.code().value(), e.what(), std::string(module)};
    } else if constexpr (std::is_base_of_v<std::invalid_argument, E>) {
        return {error_codes::INVALID_ARGUMENT, e.what(), std::string(module)};
    }
    // ... 추가 조건 ...
    return {error_codes::INTERNAL_ERROR, e.what(), std::string(module)};
}

// 사용법
try {
    // ...
} catch (const std::bad_alloc& e) {
    return Result<T>(map_exception_impl(e, "module_name"));
} catch (const std::system_error& e) {
    return Result<T>(map_exception_impl(e, "module_name"));
} catch (const std::exception& e) {
    return Result<T>(map_exception_impl(e, "module_name"));
}
```

**우선순위:** P1
**작업량:** 2-3일
**영향:** 높음 (RTTI 없는 빌드 가능)

---

### 2. 타입 제약을 위한 C++20 Concepts 추가

**위치:** `include/kcenon/common/patterns/result.h`

**현재 문제:**
템플릿 메서드에 명시적 제약이 없어 불명확한 오류 메시지가 발생합니다.

**제안된 해결책:**
```cpp
// concepts 헤더 추가
#if __cplusplus >= 202002L
#include <concepts>

namespace common::concepts {

template<typename T>
concept Mappable = requires(T t) {
    typename std::invoke_result_t<T>;
};

template<typename F, typename T>
concept UnaryFunction = std::invocable<F, T>;

template<typename F, typename T>
concept ReturnsResult = requires(F f, T t) {
    { f(t) } -> std::same_as<Result<typename std::invoke_result_t<F, T>::value_type>>;
};

} // namespace common::concepts

// Result<T>에서 사용
template<typename F>
    requires common::concepts::UnaryFunction<F, T>
auto map(F&& func) const -> Result<std::invoke_result_t<F, T>> {
    using ReturnType = std::invoke_result_t<F, T>;

    if (is_ok()) {
        return Result<ReturnType>(func(std::get<T>(value_)));
    } else {
        return Result<ReturnType>(std::get<error_info>(value_));
    }
}

template<typename F>
    requires common::concepts::ReturnsResult<F, T>
auto and_then(F&& func) const -> std::invoke_result_t<F, T> {
    using ReturnType = std::invoke_result_t<F, T>;

    if (is_ok()) {
        return func(std::get<T>(value_));
    } else {
        return ReturnType(std::get<error_info>(value_));
    }
}
#endif // C++20
```

**우선순위:** P2
**작업량:** 1-2일
**영향:** 중간 (더 나은 컴파일 타임 오류, 미래 대비)

---

## 중우선순위 개선사항

### 3. 더 나은 오류 메시지를 위한 Result<T>::expect() 추가

**제안된 추가사항:**
```cpp
// Result<T> 클래스에서
template<typename T>
class Result {
    // ... 기존 코드 ...

    /**
     * @brief 사용자 정의 패닉 메시지로 언래핑
     * @param msg 사용자 정의 오류 메시지
     * @throws std::runtime_error 오류 시 사용자 메시지와 함께
     */
    const T& expect(std::string_view msg) const {
        if (is_err()) {
            const auto& err = std::get<error_info>(value_);
            std::string full_msg = std::string(msg) + ": " + err.message;
            throw std::runtime_error(full_msg);
        }
        return std::get<T>(value_);
    }

    T& expect(std::string_view msg) {
        if (is_err()) {
            const auto& err = std::get<error_info>(value_);
            std::string full_msg = std::string(msg) + ": " + err.message;
            throw std::runtime_error(full_msg);
        }
        return std::get<T>(value_);
    }
};

// 사용법
auto config = load_config().expect("Failed to load configuration");
// "Called unwrap on error" 대신 명확한 메시지
```

**우선순위:** P3
**작업량:** 0.5일
**영향:** 낮음 (개발자 경험 개선)

---

### 4. Result<T> Monadic 연산자 추가

**제안된 추가사항:**
```cpp
// or_else를 위한 operator| 추가
template<typename T, typename F>
Result<T> operator|(Result<T>&& result, F&& alternative) {
    return result.or_else(std::forward<F>(alternative));
}

// and_then을 위한 operator& 추가
template<typename T, typename F>
auto operator&(Result<T>&& result, F&& func)
    -> decltype(result.and_then(std::forward<F>(func))) {
    return result.and_then(std::forward<F>(func));
}

// 사용법 - 파이프라인 스타일
auto result = load_file("config.json")
    & parse_json
    & validate_schema
    | [] { return load_default_config(); };
```

**우선순위:** P3
**작업량:** 1일
**영향:** 낮음 (문법적 편의, 일부에게는 가독성 저하 가능)

---

### 5. 오류 컨텍스트 전파 개선

**현재 문제:**
여러 레이어를 거치면서 오류 컨텍스트가 손실될 수 있습니다.

**제안된 해결책:**
```cpp
struct error_info {
    int code;
    std::string message;
    std::string module;
    std::optional<std::string> details;

    // 새로운 기능: 오류 체인 지원
    std::vector<error_info> causes;  // 원인 체인

    /**
     * @brief 오류 체인에 원인 추가
     */
    error_info& caused_by(const error_info& cause) {
        causes.push_back(cause);
        return *this;
    }

    /**
     * @brief 전체 오류 체인을 문자열로 반환
     */
    std::string full_chain() const {
        std::string result = message;
        for (const auto& cause : causes) {
            result += "\n  Caused by: " + cause.message;
        }
        return result;
    }
};

// 사용법
Result<Config> load_config() {
    auto result = read_file("config.json");
    if (!result) {
        return error_info{-1, "Failed to load config", "config_loader"}
            .caused_by(result.error());
    }
    // ...
}
```

**우선순위:** P3
**작업량:** 2일
**영향:** 중간 (더 나은 디버깅)

---

## 저우선순위 개선사항

### 6. 편리한 변환 함수 추가

**제안된 추가사항:**
```cpp
namespace common {

// Optional<T>를 Result<T>로 변환
template<typename T>
Result<T> from_optional(const Optional<T>& opt,
                       const error_info& error_on_none) {
    if (opt.is_some()) {
        return Result<T>::ok(opt.value());
    }
    return Result<T>::err(error_on_none);
}

// std::optional을 Result<T>로 변환
template<typename T>
Result<T> from_std_optional(const std::optional<T>& opt,
                            const error_info& error_on_none) {
    if (opt.has_value()) {
        return Result<T>::ok(*opt);
    }
    return Result<T>::err(error_on_none);
}

// Result<T>를 std::optional로 변환
template<typename T>
std::optional<T> to_optional(const Result<T>& result) {
    if (result.is_ok()) {
        return std::make_optional(result.value());
    }
    return std::nullopt;
}

} // namespace common
```

**우선순위:** P4
**작업량:** 0.5일
**영향:** 낮음 (편의성)

---

## 테스트 권장사항

### 필수 테스트 커버리지

1. **Exception Mapper 테스트:**
   ```cpp
   TEST(ExceptionMapper, MapsStdExceptions) {
       auto err = exception_mapper::map_exception(
           std::bad_alloc(), "test_module");
       EXPECT_EQ(err.code, error_codes::OUT_OF_MEMORY);
   }

   TEST(ExceptionMapper, WorksWithoutRTTI) {
       // -fno-rtti로 컴파일
       auto err = map_exception_impl(std::bad_alloc(), "test");
       EXPECT_EQ(err.code, error_codes::OUT_OF_MEMORY);
   }
   ```

2. **Result<T> Monadic 연산:**
   ```cpp
   TEST(Result, MapPreservesError) {
       Result<int> err = error_info{-1, "error"};
       auto mapped = err.map([](int x) { return x * 2; });
       EXPECT_TRUE(mapped.is_err());
       EXPECT_EQ(mapped.error().code, -1);
   }

   TEST(Result, AndThenChainsResults) {
       auto result = ok<int>(5)
           .and_then([](int x) { return ok<int>(x * 2); })
           .and_then([](int x) { return ok<int>(x + 1); });
       EXPECT_EQ(result.value(), 11);
   }
   ```

---

## 구현 타임라인

| 단계 | 작업 | 기간 | 의존성 |
|------|------|------|--------|
| Phase 1 | Exception mapper에서 RTTI 제거 | 2-3일 | 없음 |
| Phase 2 | C++20 concepts 추가 | 1-2일 | C++20 컴파일러 |
| Phase 3 | expect() 메서드 추가 | 0.5일 | 없음 |
| Phase 4 | 오류 체인 지원 | 2일 | 없음 |
| Phase 5 | Monadic 연산자 & 변환 함수 | 1.5일 | 없음 |

**총 예상 작업량:** 7-9일

---

## 호환성 매트릭스

| 컴파일러 | 버전 | 상태 | 참고 |
|---------|------|------|------|
| GCC | 10+ | ✅ 완전 지원 | C++17/20 지원 |
| GCC | 9 | ⚠️ 제한적 | C++17만 지원 |
| Clang | 12+ | ✅ 완전 지원 | C++17/20 지원 |
| Clang | 10-11 | ⚠️ 제한적 | C++17만 지원 |
| MSVC | 2019+ | ✅ 완전 지원 | C++17/20 지원 |
| Apple Clang | 13+ | ✅ 완전 지원 | C++17/20 지원 |

**최소 요구사항:**
- C++17 (현재)
- C++20 (향후 기능에 권장)

---

## 참고 자료

- [Result<T> 설계 문서](./docs/result_design.md)
- [오류 처리 가이드라인](./docs/error_handling.md)
- [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/)
- [Rust Result<T,E> 문서](https://doc.rust-lang.org/std/result/)
