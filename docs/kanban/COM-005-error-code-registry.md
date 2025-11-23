# COM-005: Error Code Registry 문서화

**Status**: DONE
**Priority**: HIGH
**Category**: DOC
**Estimated Duration**: 4-5h
**Dependencies**: None

---

## 개요

모든 에러 코드의 중앙 레지스트리를 문서화합니다.

## 현재 상태

**에러 코드 소스**:
- `include/kcenon/common/error/error_codes.h`: 에러 코드 정의

**기존 문서**:
- `docs/guides/ERROR_CODE_GUIDELINES.md`: 가이드라인만 존재
- 실제 에러 코드 목록 문서 없음

## 작업 항목

### 1. 에러 코드 인벤토리
- [ ] error_codes.h 분석
- [ ] 각 에러 코드의 의미, 원인, 해결책 정리

### 2. 문서 작성
```markdown
# docs/ERROR_CODE_REGISTRY.md

## Error Code Ranges
| System | Range | Description |
|--------|-------|-------------|
| common | -1 ~ -99 | 공통 에러 |
| ... | ... | ... |

## Error Codes

### COMMON_ERROR_INVALID_ARGUMENT (-1)
- **Description**: 잘못된 인자가 전달됨
- **Possible Causes**: ...
- **Resolution**: ...
```

### 3. 자동 생성 스크립트
- [ ] 에러 코드 추출 스크립트 (선택)
- [ ] Doxygen 연동 (선택)

## 수락 기준

- [ ] 모든 에러 코드 문서화
- [ ] 각 에러별 원인/해결책 포함
- [ ] 한국어/영어 버전
- [ ] error_codes.h와 동기화
