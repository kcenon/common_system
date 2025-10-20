# 정적 분석 기준선 - common_system

> **Language:** [English](STATIC_ANALYSIS_BASELINE.md) | **한국어**

**날짜**: 2025-10-03
**버전**: 1.0.0
**도구 버전**:
- clang-tidy: 18.x
- cppcheck: 2.x

## 개요

이 문서는 common_system의 정적 분석 경고에 대한 기준선을 설정합니다.
목표는 시간 경과에 따른 개선을 추적하고 회귀를 방지하는 것입니다.

## Clang-Tidy 기준선

### 구성
- 활성화된 검사: modernize-*, concurrency-*, performance-*, bugprone-*, cert-*, cppcoreguidelines-*
- 표준: C++20
- 분석 범위: 헤더 전용 라이브러리 (include/kcenon/common/)

### 초기 기준선 (Phase 0)

**총 경고 수**: TBD
실행: `clang-tidy -p build/compile_commands.json include/kcenon/common/**/*.h`

카테고리:
- `modernize-*`: 0 (헤더 전용, C++20 준수)
- `performance-*`: 0 (인터페이스 정의)
- `concurrency-*`: 0 (구현 없음)
- `readability-*`: TBD
- `bugprone-*`: 0

### 주요 억제 항목
- `readability-identifier-length`: 억제됨 (단일 문자 템플릿 매개변수 허용)
- `readability-magic-numbers`: 억제됨 (열거형 값, 오류 코드)
- `cppcoreguidelines-non-private-member-variables-in-classes`: 억제됨 (POD 구조체)

## Cppcheck 기준선

### 구성
- 활성화: 모든 검사
- 표준: C++20
- 플랫폼: native

### 초기 기준선 (Phase 0)

**총 이슈 수**: TBD
실행: `cppcheck --project=.cppcheck --enable=all`

카테고리:
- Error: 0
- Warning: 0
- Style: TBD
- Performance: 0 (헤더 전용)

### 주요 억제 항목
- `passedByValue`: 인터페이스 매개변수에 대해 억제됨
- `noExplicitConstructor`: result.h에 대해 억제됨 (의도적인 암시적 변환)

## 목표

**Phase 1 목표** (2025-11-01까지):
- clang-tidy: 0 오류, < 10 경고
- cppcheck: 0 오류, < 5 경고

**Phase 2 목표** (2025-12-01까지):
- 모든 경고 해결 또는 문서화
- 정적 분석에서 기술 부채 제로

## 분석 실행 방법

### Clang-Tidy
```bash
# 컴파일 명령 생성
cmake -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

# 모든 헤더에 clang-tidy 실행
find include/kcenon/common -name "*.h" | xargs clang-tidy -p build
```

### Cppcheck
```bash
# 프로젝트 구성 사용
cppcheck --project=.cppcheck --enable=all
```

## 변경 사항 추적

경고 증가는 정당한 사유와 함께 여기에 문서화해야 합니다:

| 날짜 | 도구 | 변경 | 사유 | 해결됨 |
|------|------|--------|--------|----------|
| - | - | - | - | - |
