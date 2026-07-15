# PLAN — SampleOrderSystem-Chan-0613 구현 계획

이 문서는 [CLAUDE.md](../CLAUDE.md)에서 정의한 통합 방향을 실제 작업 순서로 풀어낸 것이다. 각 단계는
"이식(그대로 가져오기)"과 "신규 구현(갭 메우기)"을 구분하여, 어떤 코드가 검증된 코드를 재사용하는
것이고 어떤 코드가 이번에 새로 작성해야 하는 것인지 명확히 한다.

## 진행 원칙

- 각 단계 종료 시 **빌드 성공 + 해당 단계 스코프의 수동/자동 검증**을 완료 조건으로 한다.
- PoC에서 이식하는 코드는 **원칙적으로 수정하지 않는다.** 수정이 불가피하면 사유를 커밋 메시지와
  CLAUDE.md에 남긴다.
- 단계마다 최소 1개 이상의 의미 있는 커밋을 남긴다 (Commit 이력 평가 기준).

---

## Phase 0 — 프로젝트 스캐폴딩

**목표**: 4개 PoC를 조립할 빈 골격을 준비한다.

- [ ] `SampleOrderSystem-Chan-0613/` vcxproj에 `Model/`, `View/`, `Controller/`, `Json/`, `data/`,
      `tools/`, `tests/` 폴더 구조 생성 (현재는 빈 vcxproj만 존재)
- [ ] `docs/PRD.md` 작성 — 기능 명세(PDF Chapter 2)를 요구사항 문서 형태로 정리
      (메뉴별 입력/출력, 상태 전이 표, 계산 공식을 표 형태로 재정리)
- [ ] `.gitignore` 정리 (`x64/`, `.vs/`, `*.user` 등 빌드 산출물 제외)

**참고 저장소**: 없음 (신규)

---

## Phase 1 — Model / Repository 계층 이식

**목표**: 검증된 도메인 모델과 JSON Repository를 그대로 가져와 빌드 가능한 상태로 만든다.

- [ ] `Model/Sample.h/.cpp`, `Model/Order.h/.cpp` — [DataPersistence](../../DataPersistence-Chan-0613)에서
      `ToJson`/`FromJson` 포함 버전을 그대로 복사
- [ ] `Model/Dtos.h` — [ConsoleMVC](../../ConsoleMVC-Chan-0613)에서 그대로 복사
      (`OrderApprovalResult`, `InventoryLevel`, `InventoryStatusItem`, `OrderStatusSummary`)
- [ ] `Model/Repository/ISampleRepository.h`, `IOrderRepository.h` — 그대로 복사 (시그니처 변경 금지)
- [ ] `Model/Repository/JsonSampleRepository.h/.cpp`, `JsonOrderRepository.h/.cpp` — DataPersistence에서
      그대로 복사, 파일 경로만 이 저장소의 `data/samples.json`, `data/orders.json`으로 지정
- [ ] `Json/` (JsonLib 전체) — DataPersistence/DataMonitor/DummyDataGenerator에서 공통으로 쓰던 버전을
      그대로 복사 (수정 없이 소비)
- [ ] **신규**: `Model/ProductionLine.h/.cpp` — ConsoleMVC의 `ProductionJob`/`ProductionLine`을 가져오되,
      `ProductionJob`에 `ToJson()`/`FromJson()` 추가 (CLAUDE.md의 "필드 ↔ JSON 키 매핑" 원칙 적용:
      `orderId`, `sampleId`, `shortageQuantity`, `actualQuantity`, `totalProductionTime`)
      **+ 신규 필드 `startTime`(벽시계 타임스탬프)** 추가 — [CLAUDE.md](../CLAUDE.md)의
      "재고 및 생산 라인 처리 규칙 (상세) § 3. 생산 라인 실시간 정산"에서 요구하는 실제 시간
      기반 완료 판정에 필요
- [ ] **신규**: `Model/Repository/IProductionLineRepository.h` — 큐 전체 저장/로드를 표현하는 최소
      인터페이스 정의 (예: `SaveQueue(currentJob, waitingJobs)`, `LoadQueue()`)
- [ ] **신규**: `Model/Repository/JsonProductionLineRepository.h/.cpp` — `data/production_queue.json`에
      write-through로 저장. DataPersistence의 손상 파일 폴백 원칙(빈 큐로 시작, 예외를 잡아 경고 출력)을
      동일하게 적용
- [ ] 검증: 콘솔에서 임시 `main.cpp`로 Sample/Order/ProductionLine 각각 등록 → 재실행 → 데이터 유지
      확인 (DataPersistence PoC와 동일한 round-trip 검증)

**참고 저장소**: ConsoleMVC(도메인/인터페이스), DataPersistence(JSON 구현체/JsonLib)

---

## Phase 2 — Controller 계층 이식 및 재고·생산 라인 로직 완성

**목표**: 상태 전이 골격에 실제 재고 반영, 재고 상태 재평가, 생산 라인 실시간 정산 로직을 채워
기능 명세를 완전히 충족시킨다. 정확한 알고리즘은 [CLAUDE.md](../CLAUDE.md)의
"재고 및 생산 라인 처리 규칙 (상세)"를 그대로 따른다 (아래는 그 요약).

- [ ] `Controller/SampleController.h/.cpp` — ConsoleMVC에서 그대로 복사
- [ ] `Controller/MonitoringController.h/.cpp` — ConsoleMVC에서 그대로 복사하되, `GetInventoryStatus`가
      반환하는 값은 "매 조회 시 재계산"이 아니라 "승인/생산완료 시점에 갱신된 최신 값을 그대로
      반환"하는 형태로 동작을 재확인 (§2)
- [ ] **신규**: `Model::ProductionLine::SettleQueue(now)` (또는 동급의 정산 함수) — 실제 경과 시간
      기준으로 완료된 작업을 FIFO 순서로 순차 처리하는 로직 (§3). 다음을 반복 수행:
      1. `Current` 없고 대기열 있음 → 맨 앞 작업을 `Current`로 승격, `StartTime = now`
      2. `Current`의 완료 예정 시각(`StartTime + totalProductionTime`)이 지났음 → 완료 처리(아래
         재고 반영 로직 호출) 후 다음 작업을 `Current`로 승격, `StartTime = 직전 작업의 완료 예정
         시각` (지금이 아님 — 오프라인 기간도 실제 시간으로 반영, 요구사항 4·5)
      3. 안 지났으면 종료 (진행 중)
- [ ] `Controller/OrderController.h/.cpp` — ConsoleMVC의 시그니처를 유지하되 내부 구현을 확장:
  - `ReserveOrder`: 변경 없음 (`RESERVED` 생성)
  - `ApproveOrder`: **먼저 `SettleQueue(now)`를 호출**해 그 시점까지 이미 끝났어야 할 생산을 반영한
    뒤, 그 결과로 갱신된 `Sample.Stock`만 근거로 판단 (다른 생산 중/대기 중 작업의 미래 산출량은
    고려하지 않음 — 요구사항 6):
    1. 재고 ≥ 주문 수량 → 재고에서 수량 차감, `CONFIRMED`. 재고 상태 재평가: 재고(차감 전) vs
       주문 수량 → 여유
    2. 재고 < 주문 수량 → 재고 전량을 우선 배정(재고 0으로), 부족분 계산 →
       `ProductionJob{shortageQuantity, actualQuantity = ceil(shortageQuantity/yield), totalProductionTime, startTime}`
       생성 후 `ProductionLine`(및 `JsonProductionLineRepository`)에 enqueue, `PRODUCING`. 재고
       상태 재평가: 차감 전 재고가 0이면 고갈, 0<재고<주문수량이면 부족
  - `RejectOrder`: 변경 없음 (`REJECTED`)
  - **완료 처리 로직**(더 이상 사용자가 메뉴에서 수동 트리거하지 않음 — `SettleQueue` 내부에서
    호출되는 헬퍼로 성격 변경, ConsoleMVC의 `CompleteCurrentProduction()` 시그니처는 내부용으로
    재사용):
    1. `actualQuantity` 전체를 재고에 더한다 (수율은 재적용하지 않음 — 요구사항 2)
    2. 그중 해당 주문의 `shortageQuantity`만큼 다시 차감해 배정 → 순증가분(잉여)만 재고에 남음
    3. 주문 상태 `PRODUCING → CONFIRMED`
    4. 재고 상태 재평가: (갱신된 재고) − (같은 시료의 다른 `PRODUCING` 주문들의 부족분 합계) 를
       구해, 방금 완료된 주문의 원래 수량과 비교해 여유/부족/고갈 재분류 (§2)
  - `ReleaseOrder`: `CONFIRMED → RELEASED` 전환 (재고는 승인 시점에 이미 차감되었으므로 출고 시
    추가 차감 없음)
- [ ] 단위 테스트(Phase 6과 연계): 재고 충분/부족 각 분기, 생산 완료 후 재고 갱신, 잉여 재고 처리,
      재고 상태 재평가(§2 두 예시), 실시간 정산(§3, 오프라인 캐치업 포함)을 검증하는 케이스 작성
- [ ] 검증: 콘솔 임시 실행으로 "재고 부족 주문 승인 → (실제 시간 경과 대기 또는 짧은
      `avgProductionTime`으로 테스트) → 생산 라인 조회 시 자동 완료 확인 → 출고"까지 한 사이클을
      수동으로 실행해 상태 전이가 명세와 일치하는지 확인
- [ ] CLAUDE.md의 요구사항 8·9 예시 시나리오를 그대로 재현해 수치가 일치하는지 확인

**참고 저장소**: ConsoleMVC(시그니처/집계 로직), DataMonitor(집계 로직 재사용 사례 참고)

---

## Phase 3 — View 계층 이식 및 메인 메뉴 요약 정보 보강

**목표**: 콘솔 UI를 완성하고, 메인 메뉴에 요구되는 요약 정보를 추가한다.

- [ ] `View/SampleView`, `View/OrderView`, `View/MonitoringView` — ConsoleMVC에서 그대로 복사
      (필요 시 문구만 다듬기)
- [ ] `View/ProductionLineView` — ConsoleMVC에서 복사하되, **"완료 처리" 메뉴 항목은 제거**한다.
      생산 완료는 더 이상 사용자가 트리거하는 동작이 아니라 실제 시간 경과에 따라 자동으로
      정산되기 때문이다(CLAUDE.md §3). 이 화면을 열 때마다 Controller가 먼저 `SettleQueue`를
      호출해 최신 상태를 반영한 뒤, 현재 처리 중인 작업/대기열만 조회 전용으로 표시한다.
- [ ] `View/MainMenuView` — ConsoleMVC 버전을 기반으로, PDF 예시 UI처럼 **요약 정보**(등록 시료 종수,
      총 재고, 전체 주문 건수, 생산 라인 대기 건수)를 상단에 표시하도록 확장. 이 정보는
      `SampleController.ListSamples()` + `MonitoringController` 조회 결과를 조합해 계산 (View 자체는
      로직을 갖지 않고 Controller 결과만 렌더링)
- [ ] 화면 레이아웃은 PDF의 예시 화면을 참고하되 자유롭게 구성 (PDF도 "화면 구성은 자유롭게 결정"이라
      명시)

**참고 저장소**: ConsoleMVC

---

## Phase 4 — main.cpp 조립 (DI)

**목표**: In-Memory 대신 Json* Repository 구현체로 전체 시스템을 조립한다.

- [ ] `main.cpp`에서 `JsonSampleRepository`, `JsonOrderRepository`, `JsonProductionLineRepository` 생성
      (경로: `data/samples.json`, `data/orders.json`, `data/production_queue.json`)
- [ ] `Controller`/`View` 생성 및 메뉴 루프는 ConsoleMVC의 `main.cpp` 구조를 그대로 계승
- [ ] UTF-8 콘솔 코드페이지 설정(`SetConsoleOutputCP`/`SetConsoleCP`) 포함
- [ ] **앱 시작 직후, 메뉴 루프 진입 전에 `SettleQueue(now)`를 1회 호출**해 오프라인 기간 동안
      실제 시간상 이미 끝났어야 할 생산을 즉시 반영한다(CLAUDE.md §3, 요구사항 4·5)
- [ ] 최초 실행(데이터 파일 없음) / 재실행(데이터 파일 존재) / **생산 중 종료 후 재시작(오프라인
      캐치업)** 세 시나리오 모두 수동 검증

**참고 저장소**: ConsoleMVC(main.cpp 구조), DataPersistence(Repository 생성자 패턴)

---

## Phase 5 — 보조 도구 통합 (DataMonitor / DummyDataGenerator)

**목표**: 별도 PoC로 개발했던 "데이터 모니터링 Tool"과 "Dummy 데이터 생성 Tool"을 최종 시스템의 데이터와
연결한다.

- [ ] `tools/DataMonitor/` — [DataMonitor PoC](../../DataMonitor-Chan-0613)의 `View/DashboardView`,
      `main.cpp`(폴링 루프)를 이식. `kSamplesFilePath`/`kOrdersFilePath`를 본 저장소의 `data/` 경로로
      변경. 생산 라인 모니터링은 Phase 1에서 영속화가 추가되었으므로, DataMonitor CLAUDE.md에서
      "아직 영속화되지 않아 제외"했던 생산 라인 현황도 이번에는 포함 (읽기 전용 폴링 패턴 동일 적용)
- [ ] `tools/DummyDataGenerator/` — [DummyDataGenerator PoC](../../DummyDataGenerator-Chan-0613)의
      `Generator/RandomSampleGenerator`, `RandomOrderGenerator`, `main.cpp`를 이식. 동일하게 `data/`
      경로만 변경
- [ ] 두 도구 모두 **별도 실행 파일**로 빌드하여, 메인 애플리케이션과 동시에 실행해도 안전한지
      (읽기 전용 폴링이 쓰기와 충돌하지 않는지) 확인

**참고 저장소**: DataMonitor, DummyDataGenerator

---

## Phase 6 — 테스트 / Harness

**목표**: Agentic Engineering 평가 기준의 "Harness 도입"과 "Test"를 충족한다.

- [ ] 외부 프레임워크 의존 없이(기존 PoC들의 "외부 의존성 없음" 원칙 유지) 간단한 assert 기반 테스트
      러너를 `tests/`에 구성 (예: 실패 시 비정상 종료 + 실패 목록 출력)
- [ ] Model 계산 로직 테스트: 실 생산량(`ceil` 공식, 수율 재적용 없이 100% 산출), 재고 상태 분류
      (여유/부족/고갈 경계값), 상태 전이 허용/금지 케이스
- [ ] Repository 왕복 테스트: 임시 디렉터리에 JSON 파일을 만들고 저장 → 재로드 → 동일성 비교,
      손상된 JSON에 대한 폴백 동작 (`ProductionJob.startTime` 포함)
- [ ] Controller 시나리오 테스트: "재고 부족 주문 승인 → 생산 완료 → 출고" 전체 사이클을 자동화된
      테스트로 재현 (Phase 2 수동 검증을 자동화로 승격)
- [ ] **CLAUDE.md 요구사항 9 재현 테스트**: 재고 50 → 주문A(100) 승인(부족분 50, 재고 0) →
      주문B(100) 승인 시 부족분이 100 전체가 되는지 검증
- [ ] **CLAUDE.md 요구사항 8 재현 테스트**: 동일 시료에 대해 order1(부족분 50, 실생산량 100),
      order2(부족분 50, 실생산량 100)가 순차로 생산 완료된 후 최종 재고가 정확히 100인지, 그
      사이 재고 상태가 예시대로 고갈→여유로 전이하는지 검증
- [ ] **실시간 정산(캐치업) 테스트**: `ProductionJob.startTime`을 인위적으로 과거로 설정한 뒤
      `SettleQueue(now)`를 호출해, 오프라인 기간에 끝났어야 할 작업(대기열에 있던 것 포함, 요구사항
      5)까지 한 번에 정산되는지, 완료 예정 시각이 아직 안 지난 경우 아무 것도 바뀌지 않는지 검증
- [ ] 빌드 스크립트 또는 문서화된 명령으로 "빌드 + 테스트 실행"을 한 번에 수행할 수 있게 정리

**참고 저장소**: 없음 (신규, 단 각 PoC가 수행한 수동 round-trip 검증을 자동화로 승격)

---

## Phase 7 — 마무리 (CleanCode / 문서 / 커밋 정리)

**목표**: 제출 전 품질 기준을 재점검한다.

- [ ] 계층 간 책임 분리 재점검 (Model에 콘솔 I/O 없는지, View에 로직 없는지, Controller가 Model/View를
      올바르게 중개하는지)
- [ ] `README.md` 작성 — 빌드 방법, 실행 방법(메인 앱 + 보조 도구 2종), 폴더 구조 요약
- [ ] `docs/PRD.md`, `CLAUDE.md` 최신화 (Phase 진행 중 변경된 설계 결정 반영)
- [ ] 커밋 이력 리뷰 — 단계별로 의미 있는 커밋이 남아 있는지 확인, 누락 시 정리
- [ ] Repository를 Public으로 설정 확인

---

## 단계별 참고 매핑 요약

| Phase | 주로 참고하는 PoC | 신규 구현 비중 |
|---|---|---|
| 0. 스캐폴딩 | - | 높음 (구조만) |
| 1. Model/Repository | ConsoleMVC, DataPersistence | 중간 (ProductionLine 영속화는 신규) |
| 2. Controller/재고 로직 | ConsoleMVC | 높음 (재고 반영·재평가, 실시간 정산 로직은 신규) |
| 3. View | ConsoleMVC | 낮음 (요약 정보만 추가) |
| 4. main.cpp 조립 | ConsoleMVC, DataPersistence | 낮음 |
| 5. 보조 도구 | DataMonitor, DummyDataGenerator | 낮음 (경로 조정 위주) |
| 6. 테스트/Harness | - | 높음 (신규) |
| 7. 마무리 | 전체 | 낮음 |
