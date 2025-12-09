# C++20 Concepts 가이드

**Language:** [English](CONCEPTS_GUIDE.md) | **한국어**

이 가이드는 Common System 라이브러리에서 컴파일 타임 타입 검증을 위해 C++20 concepts를 사용하는 방법을 설명합니다.

---

## 목차

- [개요](#개요)
- [장점](#장점)
- [빠른 시작](#빠른-시작)
- [사용 가능한 Concepts](#사용-가능한-concepts)
- [사용 예제](#사용-예제)
- [SFINAE에서 마이그레이션](#sfinae에서-마이그레이션)
- [오류 메시지](#오류-메시지)

---

## 개요

C++20 concepts는 컴파일 타임에 템플릿 매개변수를 제약하는 방법을 제공합니다. Common System 라이브러리는 다음을 제공하는 포괄적인 concept 정의를 제공합니다:

- 명확한 오류 메시지로 컴파일 타임에 타입 검증
- 타입 요구사항을 문서로서 명시적으로 표현
- 장황한 SFINAE 기반 제약을 대체
- 자동 완성을 위한 IDE 지원 향상

### 요구사항

- C++20 concepts 지원 컴파일러
- GCC 10+, Clang 10+, MSVC 2022+

---

## 장점

### 이전 (SFINAE)

```cpp
template<typename F,
         typename = std::enable_if_t<
             std::is_invocable_v<F> &&
             std::is_void_v<std::invoke_result_t<F>>>>
void execute_async(F&& func);
```

**오류 메시지:**
```
error: no matching function for call to 'execute_async'
note: candidate template ignored: substitution failure [with F = int]:
      no type named 'type' in 'std::enable_if<false>'
```

### 이후 (Concepts)

```cpp
template<concepts::VoidCallable F>
void execute_async(F&& func);
```

**오류 메시지:**
```
error: constraints not satisfied for 'execute_async' [with F = int]
note: because 'int' does not satisfy 'VoidCallable'
note: because 'std::invocable<int>' evaluated to false
```

---

## 빠른 시작

통합 concepts 헤더를 포함합니다:

```cpp
#include <kcenon/common/concepts/concepts.h>

using namespace kcenon::common::concepts;

// 템플릿 제약에서 concepts 사용
template<Resultable R>
void process(const R& result) {
    if (result.is_ok()) {
        // 성공 처리
    }
}

template<EventHandler<MyEvent> H>
uint64_t subscribe(H&& handler) {
    return bus.subscribe<MyEvent>(std::forward<H>(handler));
}
```

---

## 사용 가능한 Concepts

### Core Concepts (core.h)

| Concept | 설명 |
|---------|------|
| `Resultable` | `is_ok()` 및 `is_err()` 메서드가 있는 타입 |
| `Unwrappable` | 값 추출 지원 타입 (`unwrap()`, `unwrap_or()`) |
| `Mappable` | `map()` 변환 지원 타입 |
| `Chainable` | `and_then()` 체이닝 지원 타입 |
| `MonadicResult` | 모든 모나딕 연산을 지원하는 완전한 Result 유사 타입 |
| `OptionalLike` | 옵션 값 컨테이너 (`has_value()`, `is_some()`, `is_none()`) |
| `ErrorInfo` | `code`, `message`, `module`이 있는 오류 정보 타입 |
| `ValueOrError` | 값 또는 오류를 담는 타입 |

### Callable Concepts (callable.h)

| Concept | 설명 |
|---------|------|
| `Invocable<F, Args...>` | 호출 가능한 타입 |
| `VoidCallable<F, Args...>` | void를 반환하는 호출 가능 타입 |
| `ReturnsResult<F, R, Args...>` | 특정 타입을 반환하는 호출 가능 타입 |
| `NoexceptCallable<F, Args...>` | noexcept 호출 가능 타입 |
| `Predicate<F, Args...>` | bool을 반환하는 호출 가능 타입 |
| `UnaryFunction<F, Arg>` | 단일 인자 호출 가능 타입 |
| `BinaryFunction<F, Arg1, Arg2>` | 두 개의 인자 호출 가능 타입 |
| `JobLike` | IJob 인터페이스를 만족하는 타입 |
| `ExecutorLike` | IExecutor 인터페이스를 만족하는 타입 |
| `TaskFactory<F, T>` | 태스크를 생성하는 호출 가능 타입 |
| `DelayedCallable<F>` | 지연 실행을 위한 호출 가능 타입 |

### Event Concepts (event.h)

| Concept | 설명 |
|---------|------|
| `EventType` | 유효한 이벤트 타입 (class, copy-constructible) |
| `EventHandler<H, E>` | 이벤트 핸들러 호출 가능 타입 |
| `EventFilter<F, E>` | 이벤트 필터 predicate |
| `TimestampedEvent` | timestamp가 있는 이벤트 |
| `NamedEvent` | 모듈 이름이 있는 이벤트 |
| `ErrorEvent` | 메시지와 코드가 있는 오류 이벤트 |
| `MetricEvent` | name, value, unit이 있는 메트릭 이벤트 |
| `ModuleLifecycleEvent` | 모듈 라이프사이클 이벤트 |
| `FullErrorEvent` | 완전한 오류 이벤트 |
| `FullMetricEvent` | 완전한 메트릭 이벤트 |
| `EventBusLike` | 이벤트 버스 인터페이스 타입 |

### Service Concepts (service.h)

| Concept | 설명 |
|---------|------|
| `ServiceInterface` | 유효한 서비스 인터페이스 (polymorphic, virtual destructor) |
| `ServiceImplementation<TImpl, TIface>` | 서비스 구현체 |
| `ServiceFactory<F, T>` | 서비스 팩토리 호출 가능 타입 (컨테이너 포함) |
| `SimpleServiceFactory<F, T>` | 단순 팩토리 호출 가능 타입 (컨테이너 없음) |
| `ServiceContainerLike` | 서비스 컨테이너 타입 |
| `ServiceScopeLike` | 서비스 스코프 타입 |
| `InjectableService` | 자동 주입 가능 서비스 |
| `SharedService` | shared_ptr로 공유 가능한 타입 |
| `ConfigSection` | 설정 섹션 타입 |
| `Validatable` | 자기 검증 타입 |
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
| `ClearableContainer` | clear 가능 컨테이너 |
| `InsertableContainer` | insert 지원 컨테이너 |
| `ErasableContainer` | erase 지원 컨테이너 |
| `RandomAccessContainer` | 임의 접근 컨테이너 (operator[]) |
| `BoundedContainer` | 고정 용량 컨테이너 |
| `ThreadSafeContainer` | 스레드 안전 컨테이너 |
| `PoolableContainer` | 객체 풀 컨테이너 |
| `CircularBuffer` | 순환 버퍼 타입 |

---

## 사용 예제

### Result 처리

```cpp
#include <kcenon/common/concepts/concepts.h>

template<concepts::MonadicResult R>
auto process_chain(const R& result) {
    return result
        .map([](auto& v) { return v * 2; })
        .and_then([](auto v) { return R::ok(v + 1); });
}
```

### Event Bus 구독

```cpp
#include <kcenon/common/patterns/event_bus.h>

struct MyEvent {
    std::string data;
};

// concept 제약이 있는 핸들러로 구독
auto& bus = kcenon::common::get_event_bus();
auto id = bus.subscribe<MyEvent>([](const MyEvent& evt) {
    std::cout << "Received: " << evt.data << std::endl;
});
```

### 서비스 등록

```cpp
#include <kcenon/common/di/service_container.h>

class IMyService {
public:
    virtual ~IMyService() = default;
    virtual void do_work() = 0;
};

class MyServiceImpl : public IMyService {
public:
    void do_work() override { /* ... */ }
};

// concept 제약이 있는 템플릿으로 등록
auto& container = kcenon::common::di::service_container::global();
container.register_type<IMyService, MyServiceImpl>();
```

---

## SFINAE에서 마이그레이션

### 1단계: concepts 헤더 포함

```cpp
#include <kcenon/common/concepts/concepts.h>
```

### 2단계: enable_if를 concepts로 대체

**이전:**
```cpp
template<typename F,
         typename = std::enable_if_t<std::is_invocable_v<F>>>
void execute(F&& func);
```

**이후:**
```cpp
template<concepts::Invocable F>
void execute(F&& func);
```

### 3단계: static_assert를 concepts로 대체

**이전:**
```cpp
template<typename T>
void process(T& container) {
    static_assert(has_begin_v<T>, "T must have begin()");
    static_assert(has_end_v<T>, "T must have end()");
    // ...
}
```

**이후:**
```cpp
template<concepts::Container T>
void process(T& container) {
    // ...
}
```

---

## 오류 메시지

### Concept 위반 예제

**잘못된 Event 타입:**
```cpp
bus.subscribe<int>([](int) {});  // 오류!
// error: constraints not satisfied for 'subscribe'
// note: because 'int' does not satisfy 'EventType'
// note: because 'std::is_class_v<int>' evaluated to false
```

**잘못된 Service 인터페이스:**
```cpp
class NonPolymorphic {};  // 가상 메서드 없음
container.register_type<NonPolymorphic, Impl>();  // 오류!
// error: constraints not satisfied for 'register_type'
// note: because 'NonPolymorphic' does not satisfy 'ServiceInterface'
// note: because 'std::is_polymorphic_v<NonPolymorphic>' evaluated to false
```

---

## 모범 사례

1. **통합 헤더 포함** - 편의를 위해 `concepts/concepts.h` 사용
2. **네임스페이스 별칭 사용** - `namespace concepts = kcenon::common::concepts;`
3. **SFINAE보다 concepts 선호** - Concepts가 더 명확한 오류 메시지 제공
4. **사용자 정의 concepts 문서화** - `@concept` Doxygen 태그 추가
5. **concept 위반 테스트** - 잘못된 타입이 유용한 오류를 생성하는지 확인

---

## 관련 문서

- [API 레퍼런스](../API_REFERENCE_KO.md)
- [Result 패턴 가이드](result_pattern_ko.md)
- [Event Bus 가이드](event_bus_ko.md)
- [의존성 주입 가이드](dependency_injection_ko.md)
