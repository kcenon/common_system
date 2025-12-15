# 싱글톤 패턴 가이드라인

이 가이드는 kcenon 에코시스템 전체에서 Static Destruction Order Fiasco (SDOF) 문제를 방지하기 위한 표준화된 싱글톤 패턴 가이드라인을 수립합니다.

## 배경

kcenon 에코시스템의 여러 시스템은 다양한 매니저와 로거에 싱글톤 패턴을 사용합니다. 정적 소멸 중 이러한 싱글톤들 간의 상호 작용은 반복적인 문제를 일으켰습니다:

- network_system#301: SDOF로 인한 CI 실패
- network_system#302, #304: io_context_thread_manager SDOF
- thread_system#293, #295: thread_logger 및 thread_pool SDOF

## 문제 설명

여러 시스템이 통합될 때(예: network_system과 thread_system), 싱글톤들이 예측할 수 없는 순서로 소멸되어 다음과 같은 문제가 발생할 수 있습니다:

- `free(): invalid pointer` 크래시
- 소멸된 객체에 대한 접근
- 프로세스 종료 중 정의되지 않은 동작

## 싱글톤 패턴

### 1. Meyer's 싱글톤 (순수 데이터용 기본값)

싱글톤이 의존성이 없고 다른 객체의 소멸 중에 접근되지 않을 때 사용합니다.

```cpp
class PureDataSingleton {
public:
    static PureDataSingleton& instance() {
        static PureDataSingleton instance;
        return instance;
    }

private:
    PureDataSingleton() = default;
    ~PureDataSingleton() = default;
};
```

**사용 시기:**
- 의존성이 없는 순수 데이터 싱글톤
- 초기화 중에만 읽히는 설정 관리자
- 마지막에 소멸되는 것이 보장된 싱글톤

### 2. 의도적 누수 패턴 (인프라에 필수)

다른 객체의 소멸 중에 접근될 수 있는 싱글톤에 사용합니다.

```cpp
class InfrastructureSingleton {
public:
    static InfrastructureSingleton& instance() {
        // SDOF를 피하기 위해 의도적으로 누수
        static InfrastructureSingleton* instance = new InfrastructureSingleton();
        return *instance;
    }

    // 선택사항: 리소스 정리를 위한 명시적 shutdown
    static void shutdown() {
        // ... 정리 로직 (로깅, 플러시 등)
        // 참고: 인스턴스는 절대 삭제되지 않음
    }

private:
    InfrastructureSingleton() = default;
    ~InfrastructureSingleton() = default;  // 절대 호출되지 않음
};
```

**사용 시기:**
- 로거 및 로깅 인프라
- 스레드 풀 및 비동기 매니저
- 다른 객체의 소멸 중에 접근될 수 있는 모든 싱글톤
- 시스템 간 리소스를 조정하는 글로벌 매니저

## common_system 감사 결과

| 싱글톤 | 위치 | 현재 패턴 | SDOF 위험 | 권장사항 |
|--------|------|----------|----------|---------|
| `simple_event_bus::instance()` | event_bus.h:424 | Meyer's 싱글톤 | ⚠️ 중간 | 소멸 중 사용 시 의도적 누수 고려 |
| `GlobalLoggerRegistry::instance()` | global_logger_registry.h:335 | Meyer's 싱글톤 | ⚠️ 중간 | 시스템 간 로깅에 의도적 누수 고려 |
| `GlobalLoggerRegistry::null_logger()` | global_logger_registry.h:340 | Meyer's 싱글톤 | ✅ 낮음 | 허용 (순수 데이터, 의존성 없음) |
| `patterns::Singleton<T>` | forward.h:21 | 전방 선언만 | N/A | 구현 없음 |

### 상세 분석

#### simple_event_bus

```cpp
// 현재 구현 (event_bus.h:424-427)
static simple_event_bus& instance() {
    static simple_event_bus instance;
    return instance;
}
```

**위험 분석:**
- 이벤트 버스는 종료 중 정리 알림에 사용될 수 있음
- 구독자가 소멸자에서 이벤트를 로깅하면 SDOF가 발생할 수 있음
- 시스템 간 이벤트가 필요한 경우 의도적 누수로 마이그레이션 고려

#### GlobalLoggerRegistry

```cpp
// 현재 구현 (global_logger_registry.h:335-338)
static GlobalLoggerRegistry& instance() {
    static GlobalLoggerRegistry instance;
    return instance;
}
```

**위험 분석:**
- 로거 레지스트리는 최종 로깅을 위해 종료 중에 접근될 가능성이 높음
- 다른 시스템 소멸자가 로깅 함수를 호출할 수 있음
- 의도적 누수 패턴의 강력한 후보

## 정적 소멸 안전 체크리스트

새 싱글톤의 경우 확인:

- [ ] 소멸자에서 로깅 호출 없음
- [ ] 소멸자에서 다른 싱글톤 접근 없음
- [ ] 소멸자에서 스레드 작업 없음
- [ ] 위의 항목이 필요한 경우 의도적 누수 고려

## 문서화 요구사항

각 싱글톤은 다음을 문서화해야 합니다:

1. **패턴 선택**: 이 패턴을 선택한 이유
2. **메모리 영향**: 누수 크기 (의도적 누수 패턴의 경우)
3. **종료 절차**: 깔끔하게 종료하는 방법 (해당되는 경우)

### 문서화 예시

```cpp
/**
 * @brief 싱글톤 인스턴스를 가져옵니다.
 *
 * 패턴: 의도적 누수
 * 이유: 이 로거는 다른 시스템 컴포넌트의 최종 로깅을 위해
 *       정적 소멸 중에 접근될 수 있습니다.
 * 메모리: ~200 바이트 누수 (프로세스 수명 싱글톤으로 허용)
 * 종료: 우아한 정리를 위해 main() 반환 전에 shutdown() 호출.
 *
 * @return 싱글톤 인스턴스에 대한 참조
 */
static MyLogger& instance();
```

## 마이그레이션 가이드

### Meyer's 싱글톤에서 의도적 누수로

```cpp
// 이전 (Meyer's 싱글톤)
class MySingleton {
public:
    static MySingleton& instance() {
        static MySingleton instance;
        return instance;
    }
private:
    MySingleton() = default;
    ~MySingleton() { /* 정리 */ }
};

// 이후 (의도적 누수)
class MySingleton {
public:
    static MySingleton& instance() {
        static MySingleton* instance = new MySingleton();
        return *instance;
    }

    static void shutdown() {
        // 정리 로직을 여기로 이동
        // main() 반환 전에 명시적으로 호출
    }

private:
    MySingleton() = default;
    ~MySingleton() = default;  // 절대 호출되지 않음
};
```

### 종료 지원 추가

```cpp
// main.cpp 또는 애플리케이션 진입점에서
int main() {
    // 애플리케이션 코드...

    // 반환 전에 역 의존성 순서로 shutdown 호출
    MyDependentSingleton::shutdown();
    MySingleton::shutdown();

    return 0;
}
```

## 코드 리뷰 체크리스트

싱글톤 구현 검토 시:

1. **패턴 선택**
   - [ ] 사용 사례에 올바른 패턴이 사용되었는가?
   - [ ] 패턴 선택이 문서화되었는가?

2. **스레드 안전성**
   - [ ] 싱글톤이 생성에 대해 스레드 안전한가?
   - [ ] 싱글톤이 접근에 대해 스레드 안전한가?

3. **소멸 안전성**
   - [ ] 소멸자가 다른 싱글톤에 접근하는가?
   - [ ] 소멸자가 로깅을 수행하는가?
   - [ ] 소멸자가 스레드 작업을 포함하는가?

4. **문서화**
   - [ ] 코드에 패턴이 문서화되었는가?
   - [ ] 종료 절차가 문서화되었는가 (해당되는 경우)?

## 관련 이슈

### thread_system
- #293: thread_logger 의도적 누수 (CLOSED)
- #295: thread_pool SDOF 방지

### network_system
- #301: CI 표준화 (SDOF에 의해 차단됨)
- #302: io_context_thread_manager 안전 소멸
- #304: io_context_thread_manager 의도적 누수

## 참고자료

- [C++ Core Guidelines I.3: Avoid singletons](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#i3-avoid-singletons)
- [Static Initialization Order Fiasco](https://isocpp.org/wiki/faq/ctors#static-init-order)
- [Intentional Leak Pattern](https://google.github.io/styleguide/cppguide.html#Static_and_Global_Variables) (Google C++ Style Guide)

## 요약 표

| 패턴 | 스레드 안전 | 메모리 | SDOF 안전 | 사용 사례 |
|------|-----------|--------|----------|----------|
| Meyer's 싱글톤 | 예 (C++11) | 자동 해제 | 아니오 | 순수 데이터, 의존성 없음 |
| 의도적 누수 | 예 | 누수됨 | 예 | 인프라, 시스템 간 |

## 버전 기록

- **1.0.0** (2025-12-15): common_system 감사 결과를 포함한 초기 릴리스
