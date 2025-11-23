# COM-009: clang-tidy 규칙 강화

**Status**: DONE
**Priority**: MEDIUM
**Category**: REFACTOR
**Estimated Duration**: 4-5h
**Dependencies**: None

---

## 개요

clang-tidy 규칙을 확장하고 CI에서 강제 적용합니다.

## 현재 상태

**기존 설정**:
- `.clang-tidy` 파일 존재
- `static-analysis.yml` 워크플로우 존재

**검토 필요**:
- 현재 활성화된 체크 목록
- 누락된 중요 체크

## 작업 항목

### 1. 현재 규칙 분석
```bash
# .clang-tidy 내용 분석
cat .clang-tidy
```

### 2. 규칙 확장 제안

| Check | 설명 | 우선순위 |
|-------|------|----------|
| modernize-* | C++17/20 현대화 | HIGH |
| performance-* | 성능 관련 | HIGH |
| bugprone-* | 버그 방지 | HIGH |
| readability-* | 가독성 | MEDIUM |
| cppcoreguidelines-* | 코어 가이드라인 | LOW |

### 3. 점진적 적용
```yaml
# Phase 1: 필수
Checks: >
  bugprone-*,
  performance-*,
  modernize-use-override,
  modernize-use-nullptr

# Phase 2: 권장
Checks: >
  readability-identifier-naming,
  modernize-use-auto
```

### 4. CI 강화
- [ ] 경고를 에러로 처리 옵션
- [ ] PR 코멘트로 결과 리포트

## 수락 기준

- [ ] 최소 15개 체크 활성화
- [ ] 현재 코드에서 0 경고
- [ ] CI에서 강제 검사
- [ ] 새 PR에서 위반 시 실패
