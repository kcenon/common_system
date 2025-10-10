# CI/CD Workflow 실패 분석 및 수정 방안

## 발견된 문제점

### 1. Deprecated GitHub Actions (✅ 수정 완료)

**문제**: `actions/upload-artifact@v3` deprecated  
**영향**: 모든 integration-tests 워크플로우 실패  
**해결**: v4로 업그레이드 완료

```yaml
# 변경 전
uses: actions/upload-artifact@v3

# 변경 후  
uses: actions/upload-artifact@v4
```

### 2. Include Path 문제 (✅ 수정 완료)

**문제**: `#include "framework/system_fixture.h"` 경로 문제  
**원인**: CMake에서 `integration_tests/framework`를 include path로 추가했으나, 파일에서 `framework/` prefix 사용  
**해결**: Include 경로 수정

```cpp
// 변경 전
#include "framework/system_fixture.h"

// 변경 후
#include "system_fixture.h"
```

### 3. 네임스페이스 문제 (⚠️ 미해결 - 근본적인 설계 이슈)

**문제**: `common` 네임스페이스가 전역이지만 `kcenon::common`으로 사용 시도  
**원인**: `result.h`와 `event_bus.h`가 `namespace common`으로 정의됨 (kcenon::common 아님)

```cpp
// result.h, event_bus.h
namespace common {  // NOT namespace kcenon::common
    class Result<T> { ... };
    inline null_event_bus& get_event_bus() { ... };
}
```

**임시 해결방안**:
```cpp
// 테스트 파일에서
using namespace ::common;  // 전역 common 네임스페이스 사용
```

### 4. Result<T> API 불일치 (⚠️ 핵심 문제 - 미해결)

**문제**: 테스트가 `Result<T>::ok()`, `Result<T>::err()` 정적 메서드 사용  
**현실**: Result 클래스에 이러한 정적 메서드 없음

**테스트 코드 예**:
```cpp
auto success = Result<int>::ok(42);  // ❌ 컴파일 에러
auto error = Result<int>::err(error_code{1, "error"});  // ❌ 컴파일 에러
```

**실제 Result 클래스 API**:
```cpp
// result.h에 정의된 전역 함수들
template<typename T>
inline Result<T> ok(T value) { ... }

template<typename T>
inline Result<T> error(int code, const std::string& message, ...) { ... }

// 또는 직접 생성자 사용
Result<int> success(42);  // value 생성자
Result<int> error(error_info{1, "error"});  // error 생성자
```

### 5. error_code vs error_info 타입 불일치 (⚠️ 미해결)

**문제**: 테스트에서 `error_code` 사용, 실제는 `error_info`

```cpp
// 테스트 코드
error_code err{100, "test"};  // ❌ error_code 타입 없음

// 올바른 코드
error_info err{100, "test"};  // ✅ error_info 사용
```

## 권장 해결 방안

### 옵션 A: Result 클래스에 정적 팩토리 메서드 추가 (권장)

`result.h` 수정:

```cpp
template<typename T>
class Result {
    // ... 기존 코드 ...

public:
    // 정적 팩토리 메서드 추가
    static Result<T> ok(T value) {
        return Result<T>(std::move(value));
    }

    static Result<T> err(const error_info& error) {
        return Result<T>(error);
    }

    static Result<T> err(error_info&& error) {
        return Result<T>(std::move(error));
    }
};
```

**장점**:
- 테스트 코드 변경 최소화
- Rust 스타일 API 유지
- 명시적이고 읽기 쉬움

**단점**:
- `result.h` 변경 필요 (공통 헤더 수정)

### 옵션 B: 테스트 코드 전면 수정

모든 테스트를 전역 함수 또는 생성자 사용으로 변경:

```cpp
// 변경 전
auto success = Result<int>::ok(42);
auto error = Result<int>::err(error_info{1, "error"});

// 변경 후 (전역 함수)
auto success = ok(42);  // 타입 추론
auto error = error<int>(1, "error");

// 또는 (직접 생성자)
auto success = Result<int>(42);
auto error = Result<int>(error_info{1, "error"});
```

**장점**:
- 공통 헤더 수정 불필요

**단점**:
- 모든 테스트 파일 대량 수정 필요 (~51개 테스트)
- 타입 추론 실패 시 명시적 타입 지정 필요

### 옵션 C: 통합 테스트 임시 비활성화

빠른 PR 병합을 위해 통합 테스트 빌드 임시 비활성화:

```cmake
# CMakeLists.txt
# option(COMMON_BUILD_INTEGRATION_TESTS "Build integration tests" ON)
option(COMMON_BUILD_INTEGRATION_TESTS "Build integration tests" OFF)  # 임시 비활성화
```

**장점**:
- 즉시 CI 통과 가능
- 다른 워크플로우 영향 없음

**단점**:
- 통합 테스트 미실행

## 즉시 적용 가능한 수정 (옵션 A 권장)

### 1단계: result.h에 정적 메서드 추가

`include/kcenon/common/patterns/result.h` 파일의 `Result` 클래스에 추가:

```cpp
template<typename T>
class Result {
    // ... 기존 private/public 멤버 ...

public:
    // 정적 팩토리 메서드 추가
    static Result<T> ok(T value) {
        return Result<T>(std::move(value));
    }

    static Result<T> ok(const T& value) {
        return Result<T>(value);
    }

    template<typename... Args>
    static Result<T> err(Args&&... args) {
        return Result<T>(error_info{std::forward<Args>(args)...});
    }
};
```

### 2단계: error_code alias 추가 (하위 호환성)

```cpp
namespace common {
    using error_code = error_info;  // Alias for backward compatibility
}
```

### 3단계: 테스트 수정

```bash
# integration_tests 디렉토리에서
sed -i '' 's/using namespace kcenon::common/using namespace ::common/g' **/*.cpp
```

## 예상 소요 시간

- **옵션 A**: 30분 (result.h 수정 + 테스트 네임스페이스 수정)
- **옵션 B**: 2시간 (모든 테스트 수동 수정)
- **옵션 C**: 5분 (CMake 옵션 비활성화)

## 추천: 옵션 A

1. **즉시 적용 가능**
2. **API 개선**: Result<T>::ok/err 정적 메서드가 더 직관적
3. **최소 변경**: 테스트는 네임스페이스만 수정
4. **Rust 스타일 유지**: `Result<T>::ok()` 형태는 Rust에서 차용한 관례

---

**작성일**: 2025-10-10  
**작성자**: kcenon  
**상태**: 분석 완료, 수정 권장사항 제시
