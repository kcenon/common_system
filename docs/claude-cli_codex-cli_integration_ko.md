# claude-cli ↔ codex-cli 오케스트레이션 통합 (로컬, 최소 변경)

## 1. 목적 (Objectives)
- claude-cli가 다음을 수행할 수 있도록 한다:
  - 코드 중심 또는 저장소 조작이 필요한 상황을 감지한다.
  - 해당 작업을 서브프로세스로 실행되는 codex-cli에 위임한다.
  - codex-cli가 스트리밍하는 이벤트와 최종 결과를 수신한다.
  - 결과를 바탕으로 대화와 후속 처리를 claude-cli에서 이어간다.

## 2. 제약 및 가정 (Constraints and Assumptions)
- 네트워크가 제한된 환경을 고려하여 로컬 통합(로컬 프로세스, 파이프) 선호.
- 기존 CLI에 대한 변경은 최소화(포크/대규모 변형 지양, 브리지 방식 선호).
- 표준입출력 기반의 단순한 JSON Lines(JSONL) 프로토콜 사용.
- 데이터 무결성 준수: 실제 실행 상태를 반영하고 가공된/더미 출력을 금지.

## 3. 상위 아키텍처 (High-Level Architecture)
- 옵션 A(권장): claude-cli 내부 “codex” 도구 플러그인 브리지
  - claude-cli 도구 레지스트리에 “codex” 도구 추가.
  - codex-cli를 자식 프로세스로 실행(spawn)하고 stdin/stdout으로 JSONL 통신.
  - codex 이벤트를 claude의 이벤트 버스로 스트리밍.
  - 장점: 데몬 없이 간단, 롤백 용이, 운영 부담 낮음.
  - 단점: codex-cli의 CLI 표면/프로토콜 변화에 결합도 존재.

- 옵션 B: 외부 얇은 코디네이터(브리지) 프로세스
  - 양쪽 CLI가 호출하는 소형 중개 바이너리(예: Node/Python).
  - 장점: 양자 분리, 프로토콜 독립적 진화.
  - 단점: 배포·모니터링 대상이 하나 추가됨.

- 옵션 C(권장하지 않음): 로컬 HTTP 래퍼
  - codex-cli 앞에 로컬 HTTP 서버를 두어 HTTP로 호출.
  - 장점: 친숙한 인터페이스, 멀티클라이언트 용이.
  - 단점: 네트워크 표면 확대, 본 환경에 불필요한 복잡성.

## 4. 메시지 프로토콜 (JSON Lines)
- 전송: 개행 단위의 JSON 객체 1개/라인(JSONL).
- 공통 필드(모든 메시지):
  - `id`: 문자열(상관관계 식별자)
  - `type`: `"request" | "event" | "result" | "error"`
  - `ts`: ISO-8601 타임스탬프

- Request (claude → codex):
  - `action`: 문자열(예: `"analyze_repo"`, `"apply_patch"`, `"run_tests"`)
  - `content`: 문자열(자연어 지시 또는 diff 등 페이로드)
  - `context`: `{ cwd?: string, env?: object, timeouts?: { hard_ms, idle_ms }, limits?: { output_bytes } }`
  - `attachments?`: `[{ kind: "file" | "patch" | "refs", path?: string, data_base64?: string }]`

- Event (codex → claude):
  - `event`: `"log" | "chunk" | "status"`
  - `data`: 문자열(부분 로그/출력) 또는 객체(상태)

- Result (codex → claude):
  - `status`: `"ok"`
  - `output`: `{ text?: string, files?: [{ path, status }], metrics?: object }`

- Error (codex → claude):
  - `status`: `"error"`
  - `error`: `{ code: string, message: string, details?: object, recoverable?: boolean }`

## 5. 제어 흐름 (Control Flow)
- 트리거(라우팅):
  - 다음 조건일 때 claude 정책이 요청을 “codex” 도구로 라우팅:
    - 저장소/파일 수정, 파일 쓰기 동반 코드 생성, 테스트/빌드 실행, diff 생성, 샌드박스 실행 등.
    - 명시적 지시 태그(예: `#ask-codex`).

- 실행 단계:
  1) claude가 Request(JSONL)를 codex stdin으로 1줄 전송.
  2) codex는 Event(JSONL)를 스트리밍(상태/로그/부분 출력).
  3) codex는 최종 Result(JSONL) 또는 Error(JSONL)를 출력하고 종료.
  4) claude는 이벤트를 집계·대화 상태를 갱신하고 후속 조치를 결정.

- 타임아웃:
  - `idle_ms`(무출력 공백)과 `hard_ms`(총 한도)를 claude가 집행.
  - 순차적 종료: 경고 → `SIGINT` → `SIGKILL`, 종료 사유를 이벤트로 공지.

## 6. 라우팅과 컨텍스트 (Routing and Context)
- 컨텍스트 패킹:
  - 경로와 `cwd`는 로컬 기준으로 유지, 메시지에 비밀정보 삽입 금지.
  - 대용량 파일은 본문으로 싣지 말고 참조(`refs`)를 사용하여 codex가 직접 읽도록.

- 스트리밍:
  - codex는 `chunk` 이벤트로 stdout/stderr 일부를 전송.
  - claude는 사용자 노출 또는 버퍼링 모드를 선택적으로 적용.

## 7. 오류 처리와 복구 (Error Handling and Recovery)
- 범주:
  - 샌드박스/권한: 승격 요청 또는 대체 경로 제안.
  - 도구 오용: `error.code = "BAD_REQUEST"`로 반환.
  - 타임아웃: 부분 출력 보존, 범위 축소 또는 한도 상향 제안.
  - 충돌(패치 실패): 헝크 결과와 수동 해결 지침 포함.

- 재시도:
  - 멱등 작업은 1회 자동 재시도(백오프) 가능.
  - 비멱등 작업은 사용자 확인 필요.

- 폴백:
  - 부분 성과 요약, 수동 진행을 위한 구체적 다음 단계 제시.

## 8. 보안과 안전 (Security and Safety)
- 프롬프트 주입 방어:
  - 저장소의 원문을 실행 지시로 그대로 전달하지 않음(역할 태깅 필수).
  - 시스템/도구/사용자 역할 분리, 파일 내 모델 지시문 무력화/정규화.

- 권한:
  - 쓰기/승격이 수반되는 액션은 명시적 플래그 필요, 모든 쓰기 경로 로깅.

- 로깅:
  - 민감한 환경변수 마스킹, 보관되는 대화 기록 길이 상한 적용.

## 9. 구현 스케치 (Implementation Sketch)
- claude 도구 “codex” (TypeScript 유사 의사 코드):

```ts
async function codexTool(input) {
  const child = spawn('codex-cli', ['--jsonl'], { cwd: input.context?.cwd });
  const id = genId();
  child.stdin.write(JSON.stringify({
    id, type: 'request', ts: nowIso(),
    action: input.action, content: input.content,
    context: input.context, attachments: input.attachments
  }) + '\n');

  for await (const line of readLines(child.stdout)) {
    const msg = JSON.parse(line);
    if (msg.type === 'event') emitEvent(msg.event, msg.data);
    else if (msg.type === 'result') return msg.output;
    else if (msg.type === 'error') throw new ToolError(msg.error);
  }
}
```

- codex-cli `--jsonl` 모드(언어 무관 공통 로직):
  - 1개의 요청 라인을 읽어 파싱.
  - `action`을 내부 실행기로 매핑.
  - 진행 로그/상태를 이벤트로 스트리밍.
  - 최종 결과/오류를 출력하고 0/1 코드로 종료.

## 10. 예시 교환 (Examples)
- Request:

```json
{"id":"a1","type":"request","ts":"2025-01-12T12:00:00Z","action":"apply_patch","content":"<diff>","context":{"cwd":"/workspace","timeouts":{"hard_ms":120000}}}
```

- Event:

```json
{"id":"a1","type":"event","ts":"2025-01-12T12:00:01Z","event":"status","data":{"phase":"patching","file_count":3}}
```

- Result:

```json
{"id":"a1","type":"result","ts":"2025-01-12T12:00:04Z","status":"ok","output":{"text":"Patch applied","files":[{"path":"src/a.ts","status":"modified"}]}}
```

## 11. 최소 변경 계획 (Minimal Changes Plan)
- claude-cli:
  - 도구 레지스트리에 “codex” 추가.
  - JSONL 서브프로세스 래퍼, 타임아웃, 스트리밍 브리지 구현.
  - 라우팅 휴리스틱과 명시적 트리거 태그(`#ask-codex`) 추가.

- codex-cli:
  - 단일 요청을 읽고 실행·스트리밍·최종 JSON을 출력하는 `--jsonl` 모드 추가(또는 기존 기능 확인).

- 공통:
  - 메시지 스키마(TypeScript/JSON) 정의.
  - 기본값/한도 구성(타임아웃, 최대 출력 바이트, 기본 `cwd`).

## 12. 테스트 전략 (Testing Strategy)
- 단위(Unit):
  - 메시지 코덱 인코드/디코드 라운드트립, 타임아웃 처리, stderr/stdout 멀티플렉싱.

- 통합(Integration):
  - 임시 워크스페이스에서 `apply_patch` 종단 간 실행, 파일 변경 및 이벤트 순서 검증.

- 시나리오(Scenario):
  - analyze → patch → run tests 다단계 흐름, 1개 의도적 오류 포함하여 복구 경로 검증.

- 성능(Performance):
  - 대형 diff 스트리밍 및 백프레셔 동작 확인.

## 13. 롤아웃 (Rollout)
- 1단계: 개발자 프리뷰(피처 플래그 뒤에 배포).
- 2단계: 내부 사용자 확대, 기본 텔레메트리(건수, 소요시간, 에러코드) 추가.
- 3단계: 기본값으로 전환, 기존 경로 폴백 유지.

## 14. 운영 (Operations)
- 가시성(Observability):
  - 메시지별 `id`, `action`, 지속시간, 종료 코드 등 구조화 로그.

- 한도(Limits):
  - 출력 길이 상한 및 `truncated: true` 마커, 이어서 볼 수 있는 “next-steps” 제공.

- 정리(Cleanup):
  - 실행 후 임시 파일/디렉터리 정리 보장.

