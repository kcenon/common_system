# Deprecated APIs

이 문서는 Common System 라이브러리의 모든 deprecated API, 대체 API, 마이그레이션 가이드를 제공합니다.

> **Language:** [English](DEPRECATION.md) | **한국어**

---

## 개요

Common System 라이브러리는 시맨틱 버저닝을 따릅니다. Deprecated API는 `[[deprecated]]` 속성으로 표시되며 다음 메이저 버전에서 제거될 예정입니다.

### Deprecation 타임라인

| 단계 | 버전 | 설명 |
|------|------|------|
| Deprecation | v2.0.0 | API에 deprecated 표시 및 컴파일러 경고 |
| 유예 기간 | v2.x | Deprecated API 계속 동작 |
| 제거 | v3.0.0 | Deprecated API 제거 |

---

## 현재 Deprecated API

현재 deprecated된 API가 없습니다.

---

## v3.0.0에서 제거된 API

### 1. 레거시 로거 메서드 (File/Line/Function 파라미터)

**파일:** `include/kcenon/common/interfaces/logger_interface.h`

**제거 버전:** v3.0.0 (Issue #217)

**이전 선언:**
```cpp
[[deprecated("Use log(log_level, std::string_view, const source_location&) instead.")]]
virtual VoidResult log(log_level level,
                       const std::string& message,
                       const std::string& file,
                       int line,
                       const std::string& function) = 0;
```

**제거 사유:**
- C++20 `source_location` 기반 API로 대체됨
- 타입 안전성: `source_location`은 컴파일 타임 검증 제공
- 자동 캡처: 호출 위치에서 소스 위치가 자동으로 캡처됨
- 깔끔한 API: 파라미터 수 감소로 사용성 향상

**대체 API:**
```cpp
virtual VoidResult log(log_level level,
                       std::string_view message,
                       const source_location& loc = source_location::current());
```

**마이그레이션 가이드:**

<details>
<summary>Before (제거됨)</summary>

```cpp
// 수동 파라미터로 직접 호출
logger->log(log_level::info, "작업 완료", __FILE__, __LINE__, __func__);

// 커스텀 래퍼 함수
void my_log(ILogger* logger, log_level level, const std::string& msg) {
    logger->log(level, msg, __FILE__, __LINE__, __func__);
}
```
</details>

<details>
<summary>After (현재)</summary>

```cpp
// 직접 호출 - source_location 자동 캡처
logger->log(log_level::info, "작업 완료");

// 편의 함수 사용 (권장)
#include <kcenon/common/logging/log_functions.h>
log_info("작업 완료");

// 매크로 사용 (권장)
#include <kcenon/common/logging/log_macros.h>
LOG_INFO("작업 완료");
```
</details>

**구현 마이그레이션:**

커스텀 `ILogger` 구현이 있다면 다음과 같이 업데이트하세요:

<details>
<summary>Before (제거된 구현)</summary>

```cpp
class MyLogger : public ILogger {
public:
    VoidResult log(log_level level,
                   const std::string& message,
                   const std::string& file,
                   int line,
                   const std::string& function) override {
        // file/line/function으로 로깅
        std::cout << file << ":" << line << " [" << function << "] " << message;
        return VoidResult::ok();
    }
};
```
</details>

<details>
<summary>After (현재 구현)</summary>

```cpp
class MyLogger : public ILogger {
public:
    // source_location 기반 메서드 구현
    VoidResult log(log_level level,
                   std::string_view message,
                   const source_location& loc = source_location::current()) override {
        std::cout << loc.file_name() << ":" << loc.line()
                  << " [" << loc.function_name() << "] " << message;
        return VoidResult::ok();
    }
};
```
</details>

---

## v2.0.0에서 제거된 API

다음 API들은 이전 버전에서 deprecated되었고 v2.0.0에서 제거되었습니다:

### 1. Result::is_uninitialized()

**제거 버전:** v2.0.0

**사유:** 기본 생성자가 이제 Result를 에러 상태로 초기화하므로 "uninitialized" 개념이 불필요해짐.

**마이그레이션:**
```cpp
// Before
if (result.is_uninitialized()) { ... }

// After - 특정 에러 상태 확인
if (!result.is_ok()) { ... }
```

### 2. THREAD_LOG_* 매크로 (Deprecated로 재추가됨)

**상태:** v2.x에서 하위 호환성을 위해 재추가됨, deprecated, v3.0.0에서 제거 예정

**참고:** 이 매크로들은 v2.0.0에서 제거되었지만 마이그레이션을 돕기 위해 재추가되었습니다.
현재 표준 LOG_* 매크로로 리디렉션되지만 deprecated로 표시되어 있습니다.

**마이그레이션:**
```cpp
// Before (deprecated)
THREAD_LOG_INFO("메시지");

// After (권장)
LOG_INFO("메시지");
```

---

## 컴파일러 경고 억제

마이그레이션 중 deprecated API를 사용해야 하는 경우 경고를 억제할 수 있습니다:

### GCC/Clang
```cpp
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
// deprecated API 사용
#pragma GCC diagnostic pop
```

### MSVC
```cpp
#pragma warning(push)
#pragma warning(disable: 4996)
// deprecated API 사용
#pragma warning(pop)
```

---

## CI에서 Deprecation 경고 활성화

Deprecated API 사용을 조기에 발견하려면 빌드에서 deprecation 경고를 활성화하세요:

### CMake
```cmake
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(${TARGET} PRIVATE -Wdeprecated-declarations)
elseif(MSVC)
    target_compile_options(${TARGET} PRIVATE /W4)
endif()
```

### 커맨드 라인
```bash
# GCC/Clang
cmake -DCMAKE_CXX_FLAGS="-Wdeprecated-declarations" ..

# deprecated 사용 스캔
make 2>&1 | grep -i deprecated
```

---

## 다운스트림 시스템 알림

모든 의존 시스템에 deprecated API 및 v3.0.0 제거 예정에 대해 알림을 완료했습니다:

| 시스템 | 리포지토리 | 알림 이슈 |
|--------|------------|-----------|
| thread_system | [kcenon/thread_system](https://github.com/kcenon/thread_system) | [#331](https://github.com/kcenon/thread_system/issues/331) |
| logger_system | [kcenon/logger_system](https://github.com/kcenon/logger_system) | [#248](https://github.com/kcenon/logger_system/issues/248) |
| monitoring_system | [kcenon/monitoring_system](https://github.com/kcenon/monitoring_system) | [#269](https://github.com/kcenon/monitoring_system/issues/269) |
| pacs_system | [kcenon/pacs_system](https://github.com/kcenon/pacs_system) | [#399](https://github.com/kcenon/pacs_system/issues/399) |
| database_system | [kcenon/database_system](https://github.com/kcenon/database_system) | [#276](https://github.com/kcenon/database_system/issues/276) |

전체 deprecation 계획 추적은 [#213](https://github.com/kcenon/common_system/issues/213)을 참조하세요.

---

## 관련 문서

- [CHANGELOG](CHANGELOG_KO.md) - deprecation 공지가 포함된 버전 히스토리
- [API Reference](API_REFERENCE_KO.md) - 전체 API 문서
- [Migration Guide](advanced/MIGRATION_KO.md) - 일반 마이그레이션 가이드
- [Logging Best Practices](guides/LOGGING_BEST_PRACTICES.md) - 권장 로깅 패턴

---

## 질문이 있으신가요?

Deprecated API 마이그레이션에 대한 질문이 있으시면:
1. 위의 관련 문서를 확인하세요
2. 기존 [GitHub Issues](https://github.com/kcenon/common_system/issues) 검색
3. `question` 레이블로 새 이슈 생성
