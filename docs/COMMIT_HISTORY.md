# 커밋 이력

전체 커밋을 시간 순(오래된 것부터)으로 정리한 표다. Phase 7 "커밋 이력 리뷰" 항목의 결과물이며,
각 Phase가 `[feat]`(구현) + `[docs]`(그 Phase 완료를 문서에 반영)로 짝을 이루는 패턴과, 테스트
과정에서 문제가 발견되었을 때 `[test]`(재현, RED) → `[fix]`(수정, GREEN)로 분리한 지점을 한눈에
볼 수 있다. 이 문서 자체를 추가하는 커밋(및 이후 커밋)은 작성 시점 특성상 표에 포함하지 않는다.

| # | 커밋 해시 | 제목 | 반영 내용 |
|---|---|---|---|
| 1 | `43d63f5` | .gitattributes 및 .gitignore을(를) 추가합니다. | 저장소 초기 설정 — 줄바꿈 정규화 규칙, VS 표준 무시 규칙(`x64/`, `.vs/`, `*.user` 등) 추가 |
| 2 | `2563589` | 초기 커밋. | 저장소 최초 생성 |
| 3 | `fd68ab3` | [docs] CLAUDE.md에서 과제 진행 규칙 삭제, 기술 명세 위주로 정리 | CLAUDE.md를 과제 진행 절차가 아닌 시스템/기술 명세 중심 문서로 재정리 |
| 4 | `9469f37` | [docs] README.md 추가 (과제 목적, 시스템 개요, PoC 링크, Commit 컨벤션) | README 최초 작성 — 과제 배경, 4개 PoC 저장소 링크, 커밋 prefix 컨벤션 정의 |
| 5 | `c26ac92` | [docs] 재고·생산 라인 처리 규칙 9종 상세화 (CLAUDE.md, PLAN.md) | 재고 반영 시점, 실 생산량 `ceil` 공식, 재고 상태 재평가 시점 등 핵심 알고리즘을 구체적인 예시(요구사항 1~9)로 확정 |
| 6 | `28baa89` | [docs] 생산 라인 정산이 지연 평가 방식임과 상태 영속화 필수 조건 명시 | `SettleProductionQueue`가 백그라운드 타이머 없이 지연 평가로 동작함과, `StartTime` 영속화가 왜 필수인지 명문화 |
| 7 | `0de3927` | [docs] PDF 명세 재검토로 발견한 문서 정합성 갭 보완 | PDF 원문과 CLAUDE.md 사이 불일치 지점 재검토 후 보완 |
| 8 | `cc66ab7` | [docs] Test 체크리스트에 Unit/적대적 테스트 도입 및 Phase별 분산 배치 | 테스트를 마지막에 몰아 쓰지 않고 기능 구현 Phase 안에서 바로 작성하도록 원칙 확정 |
| 9 | `d25455b` | [docs] 구현 진행 방식 확정 — Sub-agent는 code-review와 Phase 5에만 사용 | 핵심 Phase는 메인 대화에서 순차 진행하고, sub-agent는 Phase별 code-review 검증과 독립적인 Phase 5(도구 이식)에만 위임하기로 결정 |
| 10 | `a3dd8f6` | [feat] Phase 0 — 프로젝트 폴더 스캐폴딩 및 테스트 하네스 구성 | `Model/View/Controller/Json/data/tools/tests` 폴더 골격 생성, `TEST`/`ASSERT_*` 매크로 기반 최소 테스트 하네스와 `--test` 모드 도입 |
| 11 | `27e2602` | [docs] Phase 0 완료 반영 — PRD.md 작성, CLAUDE.md/PLAN.md 갱신 | `docs/PRD.md` 최초 작성(요구사항 정리), Phase 0 빌드 검증 결과 기록 |
| 12 | `002ae19` | [docs] README에 문서 구성(PRD/CLAUDE/PLAN 역할 구분) 섹션 추가 | PRD/CLAUDE/PLAN 세 문서의 역할(무엇을/어떻게/어떤 순서로)을 README에서 구분해 안내 |
| 13 | `4dfdbab` | [feat] Phase 1 — Model/Repository 계층 이식 및 생산 라인 영속화 신규 구현 | ConsoleMVC/DataPersistence의 도메인 모델·Repository를 이식하고, 신규로 `IProductionLineRepository`/`JsonProductionLineRepository` 및 `InventoryCalculator` 구현 |
| 14 | `6b1dbda` | [docs] Phase 1 완료 반영 — CLAUDE.md/PLAN.md 갱신 | Phase 1 빌드/테스트 검증 결과와 아키텍처 변경 사항 문서화 |
| 15 | `9bfc46e` | [docs] 테스트 전담 sub-agent 미도입 결정 및 code-review 점검항목 확장 | 테스트를 별도 에이전트로 분리하지 않기로 확정하고, code-review 점검 항목에 "테스트가 실제로 실패할 수 있는 케이스를 검증하는지" 등을 명시 |
| 16 | `7cd06d0` | [test] ClassifyInventoryLevel의 고갈 우선순위 회귀 테스트 추가 (현재 실패) | 재고 0일 때 무조건 "고갈"이어야 하는데 그렇지 않은 버그를 재현하는 테스트를 먼저 추가 (RED) |
| 17 | `69dff5d` | [fix] ClassifyInventoryLevel이 재고 0을 무조건 고갈로 판정하도록 수정 | 재고 0 검사를 참조 수량 비교보다 먼저 하도록 분류 순서 수정 (GREEN) |
| 18 | `aa85d56` | [feat] Phase 2 — Controller 계층 이식 및 재고·생산 라인 로직 완성 | `OrderController`/`SampleController`/`MonitoringController` 구현, 재고 반영·재평가·실시간 정산 로직 완성 (이 저장소의 핵심 신규 구현) |
| 19 | `543dad2` | [docs] Phase 2 완료 반영 — CLAUDE.md/PLAN.md 갱신 | Phase 2 빌드/테스트 검증 결과, ConsoleMVC 대비 완료 처리 로직 수정 사유 문서화 |
| 20 | `419cbc0` | [feat] Phase 3 — View 계층 이식 및 메인 메뉴 요약 정보 보강 | ConsoleMVC의 View 이식, 메인 메뉴에 요약 정보(등록 시료 수/총 재고/주문 건수/생산 대기 건수) 표시 추가 |
| 21 | `582df10` | [docs] Phase 3 완료 반영 — CLAUDE.md/PLAN.md 갱신 | Phase 3 빌드/테스트 검증 결과 문서화 |
| 22 | `699227a` | [feat] Phase 4 — main.cpp DI 조립으로 실제 시스템 완성 | Repository/Controller/View를 `main.cpp`에서 조립(DI)해 실행 가능한 콘솔 앱 완성, 시작 시 실시간 정산 캐치업 반영 |
| 23 | `40ab52d` | [docs] Phase 4 완료 반영 — CLAUDE.md/PLAN.md 갱신 | Phase 4 빌드/테스트 검증 및 수동 시나리오(신규 실행/재시작/오프라인 캐치업) 검증 결과 문서화 |
| 24 | `83ed3a7` | [test] 서로 다른 시료 교차 시나리오에 대한 재고/생산 큐 검증 추가 | 수율·평균생산시간이 다른 3개 시료가 예약/승인 순서가 어긋난 상태로 뒤섞일 때 전역 FIFO와 시료별 재고 격리가 유지되는지 검증하는 테스트 추가 |
| 25 | `3d75a88` | [docs] 다중 시료 교차 검증 결과를 CLAUDE.md/PLAN.md에 반영 | 재고/생산 라인 처리 규칙이 시료에 무관하게 일반화됨을 CLAUDE.md에 명시하고 검증 테스트를 근거로 인용 |
| 26 | `67e3446` | [feat] Phase 5 — DataMonitor/DummyDataGenerator 도구를 별도 프로젝트로 통합 | 두 보조 도구를 현재 스키마(InventoryLevel/startTime 포함) 기준으로 재이식해 별도 `.vcxproj`로 솔루션에 추가 |
| 27 | `41d7ed7` | [docs] Phase 5 완료 반영 — CLAUDE.md/PLAN.md 갱신 | Phase 5 빌드 검증, 도구 간 동시 실행 수동 검증 결과, 원본 PoC 대비 재이식 사유 문서화 |
| 28 | `6841502` | [docs] Phase 6 완료 — 전체 테스트 회귀 실행 및 커버리지 점검 결과 기록 | 3개 실행 파일 전체 빌드 + 56/56 회귀 테스트 통과 확인, 커버리지 체크리스트 8개 항목을 실제 테스트 파일과 대조해 공백 없음을 확인 |
| 29 | `4a7ad34` | [refactor] OrderController: 상태 검증 조회 패턴을 FindOrderRequiringStatus로 추출 | `ApproveOrder`/`RejectOrder`/`ReleaseOrder`의 중복된 조회+상태검증 블록을 헬퍼로 추출 (behavior-preserving, 56/56 유지) |
| 30 | `d815b1e` | [docs] Phase 7 — Clean Code/SOLID 트레이드오프 검토 문서 추가 | `docs/CLEAN_CODE_REVIEW.md` 신규 작성 — 계층 분리 재점검 결과와 SRP/ISP/OCP 등 SOLID 트레이드오프 결정 근거 정리 |
| 31 | `1760c14` | [docs] Phase 7 — README 빌드/실행/폴더 구조 섹션 추가, CLAUDE.md 참고 문서 갱신 | README에 빌드 방법/실행 방법(메인 앱 + 보조 도구 2종)/폴더 구조 요약 섹션 추가, CLAUDE.md 참고 문서 링크 갱신 |
