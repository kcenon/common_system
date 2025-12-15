# Common System 문서

> **Language:** [English](README.md) | **한국어**

common_system 문서에 오신 것을 환영합니다. 이 디렉토리에는 common system을 사용하고 통합하기 위한 포괄적인 가이드, 참조 자료 및 모범 사례가 포함되어 있습니다.

## 📚 목차

### 🏗️ 아키텍처 및 설계

- **[아키텍처](ARCHITECTURE_KO.md)** / **[English](ARCHITECTURE.md)**
  - 완전한 시스템 아키텍처 개요
  - 레이어 아키텍처 및 모듈 의존성
  - 통합 패턴 및 모범 사례

- **[아키텍처 이슈](ARCHITECTURE_ISSUES_KO.md)** / **[English](ARCHITECTURE_ISSUES.md)**
  - 알려진 아키텍처 이슈 및 추적
  - 해결 단계 및 우선순위
  - 위험 평가 및 완화

- **[구조](STRUCTURE.md)**
  - 프로젝트 디렉토리 레이아웃
  - 네임스페이스 구성
  - 컴포넌트 아키텍처

- **[현재 상태](CURRENT_STATE_KO.md)** / **[English](CURRENT_STATE.md)**
  - 시스템 베이스라인 및 상태
  - 빌드 구성
  - 알려진 이슈 및 다음 단계

### ⚠️ 에러 처리

- **[에러 처리 가이드라인](ERROR_HANDLING_KO.md)** / **[English](ERROR_HANDLING.md)**
  - Result<T> 패턴 사용법
  - 모나딕 연산 (map, and_then, or_else)
  - 에러 전파 패턴
  - 모범 사례 및 테스팅

- **[에러 코드 가이드라인](ERROR_CODE_GUIDELINES.md)**
  - 에러 코드 범위 및 할당
  - 새로운 에러 코드 추가
  - 컴파일 타임 검증
  - 레지스트리 관리

### 🔗 통합 및 마이그레이션

- **[통합 가이드](INTEGRATION_KO.md)** / **[English](INTEGRATION.md)**
  - 시스템 통합 예제
  - 빠른 시작 가이드
  - 완전한 통합 패턴
  - 문제 해결

- **[통합 정책](INTEGRATION_POLICY_KO.md)** / **[English](INTEGRATION_POLICY.md)**
  - 공식 통합 정책
  - 통합 계층 (필수, 선택)
  - CMake 구성 패턴
  - 의존성 관리

- **[마이그레이션 가이드](MIGRATION_KO.md)** / **[English](MIGRATION.md)**
  - common_system 통합으로 마이그레이션
  - Result<T> 패턴 적용
  - 표준 인터페이스 마이그레이션
  - 버전별 가이드

- **[네임스페이스 마이그레이션](NAMESPACE_MIGRATION.md)**
  - 네임스페이스 마이그레이션 전략
  - 하위 호환성
  - 권장 사용 패턴

- **[IExecutor 마이그레이션 가이드](IEXECUTOR_MIGRATION_GUIDE.md)**
  - 함수 기반에서 잡 기반 API로 마이그레이션
  - 지원 중단 일정
  - 코드 예제 및 패턴

### 🔍 개발 가이드라인

- **[RAII 가이드라인](RAII_GUIDELINES_KO.md)** / **[English](RAII_GUIDELINES.md)**
  - Resource Acquisition Is Initialization 패턴
  - 생명주기 관리
  - 모범 사례

- **[스마트 포인터 가이드라인](SMART_POINTER_GUIDELINES_KO.md)** / **[English](SMART_POINTER_GUIDELINES.md)**
  - std::unique_ptr 및 std::shared_ptr 사용법
  - 소유권 의미
  - 성능 고려사항

- **[싱글톤 패턴 가이드라인](SINGLETON_GUIDELINES_KO.md)** / **[English](SINGLETON_GUIDELINES.md)**
  - SDOF(Static Destruction Order Fiasco) 방지
  - Meyer's 싱글톤 vs 의도적 누수 패턴
  - 시스템 간 싱글톤 관리

### 📊 분석 및 메트릭

- **[성능 베이스라인](BASELINE.md)**
  - Result<T> 패턴 성능
  - 인터페이스 추상화 오버헤드
  - 컴파일 타임 메트릭
  - 메모리 사용량

- **[정적 분석 베이스라인](STATIC_ANALYSIS_BASELINE_KO.md)** / **[English](STATIC_ANALYSIS_BASELINE.md)**
  - clang-tidy 베이스라인
  - cppcheck 베이스라인
  - 목표 설정 및 추적

- **[의존성 매트릭스](DEPENDENCY_MATRIX_KO.md)** / **[English](DEPENDENCY_MATRIX.md)**
  - 시스템 의존성 관계
  - 모듈 통합 상태
  - 의존성 그래프

### 📈 개선 사항

- **[개선 사항](IMPROVEMENTS_KO.md)** / **[English](IMPROVEMENTS.md)**
  - 완료된 개선 사항
  - 진행 중인 개선 사항
  - 향후 로드맵

## 📖 빠른 링크

### 시작하기
1. [아키텍처](ARCHITECTURE_KO.md)에서 시스템 개요 파악
2. [통합 가이드](INTEGRATION_KO.md)에서 실용적인 예제 확인
3. [에러 처리 가이드라인](ERROR_HANDLING_KO.md)에서 모범 사례 학습

### 개발자용
- [구조](STRUCTURE.md) - 프로젝트 구성 이해
- [에러 코드 가이드라인](ERROR_CODE_GUIDELINES.md) - 새로운 에러 코드 추가
- [RAII 가이드라인](RAII_GUIDELINES_KO.md) - 리소스 관리 패턴

### 통합용
- [통합 정책](INTEGRATION_POLICY_KO.md) - 공식 통합 요구사항
- [마이그레이션 가이드](MIGRATION_KO.md) - 기존 코드 마이그레이션
- [IExecutor 마이그레이션 가이드](IEXECUTOR_MIGRATION_GUIDE.md) - executor 사용법 업데이트

## 🔄 문서 업데이트

모든 문서는 코드 변경사항과 함께 적극적으로 유지 관리되고 업데이트됩니다. 불일치를 발견한 경우:

1. `include/kcenon/common/`의 해당 소스 코드 확인
2. `tests/` 및 `integration_tests/`의 최신 테스트 케이스 참조
3. GitHub Issues를 통해 문제 보고

## 📝 기여하기

문서를 추가하거나 업데이트할 때:

1. 영어 및 한국어 버전을 동기화 유지
2. 실용적인 코드 예제 포함
3. 새 문서 추가 시 이 인덱스 업데이트
4. 기존 형식 및 구조 준수

---

**버전:** 1.0.0
**마지막 업데이트:** 2025-11-09
**관리자:** kcenon
