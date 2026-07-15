# PLAN — SampleOrderSystem-Chan-0613 구현 계획

이 문서는 [CLAUDE.md](../CLAUDE.md)에서 정의한 통합 방향을 실제 작업 순서로 풀어낸 것이다. 각 단계는
"이식(그대로 가져오기)"과 "신규 구현(갭 메우기)"을 구분하여, 어떤 코드가 검증된 코드를 재사용하는
것이고 어떤 코드가 이번에 새로 작성해야 하는 것인지 명확히 한다.

## 진행 원칙

- 각 단계 종료 시 **빌드 성공 + 해당 단계 스코프의 수동/자동 검증**을 완료 조건으로 한다.
- PoC에서 이식하는 코드는 **원칙적으로 수정하지 않는다.** 수정이 불가피하면 사유를 커밋 메시지와
  CLAUDE.md에 남긴다.
- 단계마다 최소 1개 이상의 의미 있는 커밋을 남긴다 (Commit 이력 평가 기준).
- **테스트(Unit Test + 적대적 테스트)는 몰아서 마지막에 작성하지 않고, 그 기능을 구현한 Phase 안에서
  바로 작성한다.** 예를 들어 Phase 1에서 Repository를 만들면 그 Phase 안에서 Repository 왕복
  테스트를, Phase 2에서 재고 반영 로직을 만들면 그 Phase 안에서 관련 Unit Test와 적대적 테스트를
  함께 작성한다. 테스트 하네스(러너) 자체는 Phase 0에서 미리 준비해, 이후 Phase들이 곧바로 테스트를
  추가할 수 있게 한다. **Phase 6("테스트 회귀·정리")은 새 테스트를 처음 작성하는 단계가 아니라, 그때
  까지 각 Phase에서 쌓인 테스트를 모아 한 번에 빌드+실행하고 빠진 부분이 없는지 점검하는 단계다.**
- **Sub-agent 활용 방침**: Phase 0~4, 6~7은 이전 Phase의 인터페이스/파일에 강하게 의존하고 동일한
  `.vcxproj`를 계속 같이 수정하므로, 여러 sub-agent로 병렬 분산하지 않고 **메인 대화에서 Phase
  순서대로 직접 구현**한다 (병렬화 이득보다 파일 충돌·조율 비용이 더 큼). Sub-agent는 아래 두
  경우에만 사용한다.
  1. **각 Phase 완료 직후 code-review 서브에이전트로 검증**: 계층 분리(Model/View/Controller 역할
     원칙), 재고·생산 라인 처리 규칙(상세 섹션), 커밋 컨벤션 준수 여부를 그 Phase의 메인 구현과
     독립적으로 재점검한다. **테스트 전담 에이전트를 별도로 두지 않는다** — 테스트는 그 기능을
     구현한 Phase 안에서 메인 대화가 바로 작성하고(위 원칙 참고), 별도 에이전트로 분리하면 방금
     구현한 인터페이스/재고 규칙 컨텍스트를 다시 넘겨줘야 하는 핸드오프 비용만 늘고 같은
     `.vcxproj`를 다루므로 병렬 이득도 없다. 대신 **이 code-review 서브에이전트의 점검 항목에
     테스트 품질 검증을 명시적으로 포함**한다 — 통과 여부가 아니라 "이 테스트가 실제로 실패할 수
     있는 케이스를 검증하는가", "경계값(0, 음수, 정확히 일치하는 값 등)을 빠뜨리지 않았는가",
     "적대적 테스트가 진짜로 공격적인가(단순히 정상 케이스를 재확인하는 수준에 그치지 않는가)"를
     구현자 본인이 아닌 독립된 시각에서 재확인한다.
  2. **Phase 5(DataMonitor/DummyDataGenerator 도구 이식)**: 메인 애플리케이션 로직과 파일이
     겹치지 않는 독립적인 작업이므로, 별도 sub-agent에 위임해도 충돌 위험이 적다.

---

## Phase 0 — 프로젝트 스캐폴딩 ✅ 완료

**목표**: 4개 PoC를 조립할 빈 골격을 준비한다.

- [x] `SampleOrderSystem-Chan-0613/` vcxproj에 `Model/`, `View/`, `Controller/`, `Json/`, `data/`,
      `tools/`, `tests/unit/`, `tests/adversarial/` 폴더 구조 생성. 아직 내용이 없는 폴더
      (`Model`, `View`, `Controller`, `Json`, `tools`, `data`)에는 `.gitkeep`을 두어 Git에 빈
      폴더 구조가 보이도록 함
- [x] `docs/PRD.md` 작성 — 기능 명세(PDF Chapter 1·2)를 요구사항 문서 형태로 정리 (용어 정의,
      메뉴별 입력/출력, 상태 전이 표, 계산 공식, 비기능 요구사항)
- [x] `.gitignore` 정리 — 기존 VisualStudio 표준 무시 규칙(`x64/`, `.vs/`, `*.user` 등)은 이미
      충분했음. `SampleOrderSystem-Chan-0613/data/*.json`(런타임 생성 데이터)만 추가로 무시하고
      `data/.gitkeep`은 예외 처리 (DataPersistence PoC와 동일 규약)
- [x] **MSBuild 등록 습관화**: `main.cpp`, `tests/TestFramework.h/.cpp`,
      `tests/unit/FrameworkSelfTest.cpp`, `tests/adversarial/FrameworkSelfTest.cpp`를
      `.vcxproj`/`.vcxproj.filters`에 등록. `/p:PlatformToolset=v143`으로 실제 MSBuild 빌드를
      돌려 컴파일 대상에 포함되었는지 검증함(아래 "빌드 검증" 참고)
- [x] 테스트 하네스(러너) 최소 골격을 `tests/`에 구성 — `TestFramework.h/.cpp`에 `TEST(suite, name)`
      매크로와 `ASSERT_TRUE`/`ASSERT_FALSE`/`ASSERT_EQ`/`ASSERT_THROWS` 매크로, `TestRegistry`를
      구현. **"콘솔 인자로 테스트 모드 진입"** 방식을 채택 — 별도 vcxproj 타겟 대신, `main.cpp`가
      `--test` 인자를 받으면 `Testing::TestRegistry::Instance().RunAll()`을 실행하고 실패 개수를
      종료 코드로 반환한다 (실패 0건 → 0, 그 외 → 1). `tests/unit/FrameworkSelfTest.cpp`,
      `tests/adversarial/FrameworkSelfTest.cpp`에 하네스 자체를 검증하는 self-test 5건을 작성해
      `SampleOrderSystem-Chan-0613.exe --test`로 전부 통과 확인함

### 빌드 검증

- `MSBuild.exe SampleOrderSystem-Chan-0613.slnx /p:Configuration=Debug /p:Platform=x64
  /p:PlatformToolset=v143` 로 빌드 성공 확인 (로컬에 v145 툴셋 없어 v143으로 오버라이드 — 앞선
  PoC들과 동일)
- 빌드 중 **서로 다른 폴더의 동일 파일명(`tests/unit/FrameworkSelfTest.cpp` vs
  `tests/adversarial/FrameworkSelfTest.cpp`)이 오브젝트 파일 이름 충돌을 일으키는 것을 실제로
  발견**(`MSB8027` 경고 + `LNK4042`, 한쪽 파일의 테스트가 조용히 링크에서 빠짐). 모든 `ClCompile`
  설정에 `<ObjectFileName>$(IntDir)%(RelativeDir)</ObjectFileName>`를 추가해 해결하고
  [CLAUDE.md](../CLAUDE.md) "기술 스택"에 이 실무적 함정을 기록함
- `SampleOrderSystem-Chan-0613.exe --test` 실행 → 5/5 테스트 통과, 종료 코드 0 확인
- `SampleOrderSystem-Chan-0613.exe` (인자 없음) 실행 → Phase 0 임시 안내 메시지가 한글 깨짐 없이
  출력되는 것 확인 (UTF-8 콘솔 코드페이지 설정 검증)

**참고 저장소**: 없음 (신규)

---

## Phase 1 — Model / Repository 계층 이식 ✅ 완료

**목표**: 검증된 도메인 모델과 JSON Repository를 그대로 가져와 빌드 가능한 상태로 만든다.

- [x] `Model/Sample.h/.cpp`, `Model/Order.h/.cpp` — [DataPersistence](../../DataPersistence-Chan-0613)에서
      `ToJson`/`FromJson` 포함 버전을 파일 그대로 복사(`cp`)해 바이트 단위로 동일하게 이식
- [x] `Model/Dtos.h` — [ConsoleMVC](../../ConsoleMVC-Chan-0613)에서 그대로 복사
      (`OrderApprovalResult`, `InventoryLevel`, `InventoryStatusItem`, `OrderStatusSummary`)
- [x] `Model/Repository/ISampleRepository.h`, `IOrderRepository.h` — 그대로 복사 (시그니처 변경 없음)
- [x] `Model/Repository/JsonSampleRepository.h/.cpp`, `JsonOrderRepository.h/.cpp` — DataPersistence에서
      그대로 복사. 파일 경로는 이 저장소의 `data/samples.json`, `data/orders.json`을 Phase 4(main.cpp
      조립)에서 지정하기로 하고, 이 Phase에서는 생성자가 임의 경로를 받도록만 확인(테스트에서는 임시
      디렉터리 경로 사용)
- [x] `Json/` (JsonLib 전체, 16개 파일) — DataPersistence에서 그대로 복사 (수정 없이 소비)
- [x] **신규**: `Model/ProductionLine.h/.cpp` — ConsoleMVC의 `ProductionJob`/`ProductionLine`을 가져오되,
      `ProductionJob`에 `ToJson()`/`FromJson()` 추가 (필드: `orderId`, `sampleId`, `shortageQuantity`,
      `actualQuantity`, `totalProductionTime`, **+ 신규 `startTime`**(`std::chrono::system_clock::time_point`,
      JSON에는 `startTimeEpochMs` 정수로 직렬화)). `ProductionLine`에도 영속화를 위한 벌크 접근자
      `GetAllJobsInOrder()`/`ReplaceAll()`을 신규 추가 (기존 `Enqueue`/`CurrentJob`/`CompleteCurrentJob`/
      `GetWaitingJobs`는 변경 없음)
- [x] **신규**: `Model/Repository/IProductionLineRepository.h` — `SaveQueue(jobsInOrder)`/`LoadQueue()`로
      큐 전체(맨 앞이 Current Job)를 스냅샷 단위로 다루는 인터페이스
- [x] **신규**: `Model/Repository/JsonProductionLineRepository.h/.cpp` — `data/production_queue.json`에
      write-through로 저장. Sample/Order Repository와 달리 생성자에서 자동 로드하지 않고, 로드 시점은
      호출부가 결정하도록 설계(주석에 근거 명시). 손상 파일 폴백 원칙은 DataPersistence와 동일
- [x] **신규**: `Model/InventoryCalculator.h/.cpp` — 애초 계획에는 없었으나, 이 Phase의 "Model 계산 로직
      테스트"(실 생산량 `ceil` 공식, 재고 상태 분류) 항목을 테스트하려면 그 계산 자체가 Model 계층의
      순수 함수로 존재해야 한다고 판단해 추가함: `CalculateShortage`, `CalculateActualProductionQuantity`
      (ceil, 수율 0이면 예외), `CalculateTotalProductionTime`, `ClassifyInventoryLevel`(여유/부족/고갈).
      Phase 2에서 `OrderController`가 이 함수들을 그대로 재사용한다.
- [x] 검증: 콘솔 임시 확인 대신, 아래 Unit Test(Repository 왕복 테스트)로 자동화하여 대체

**Unit Test (이 Phase에서 바로 작성) — 20건**
- [x] Repository 왕복 테스트: `tests/unit/JsonRepositoryRoundTripTest.cpp` — `JsonSampleRepository`/
      `JsonOrderRepository`/`JsonProductionLineRepository` 각각 임시 파일에 저장 → 새 인스턴스로 재로드
      → 값 동일성 확인. `ProductionJob.startTime`이 밀리초 단위까지 정확히 왕복되는지, 큐 순서(Current가
      맨 앞)가 보존되는지도 함께 확인. `ProductionLine::ReplaceAll`/`GetAllJobsInOrder`도 함께 검증
- [x] Model 계산 로직 테스트: `tests/unit/InventoryCalculatorTest.cpp` — `CalculateShortage`(재고
      충분/부족/정확히 일치), `CalculateActualProductionQuantity`(ceil 공식, 나누어떨어지지 않는 경우,
      부족분 0, 수율 0일 때 예외), `CalculateTotalProductionTime`, `ClassifyInventoryLevel`(여유/부족/
      고갈 3구간) 검증

**적대적 테스트 (이 Phase에서 바로 작성) — 6건**
- [x] `tests/adversarial/JsonRepositoryFallbackTest.cpp` — `samples.json`/`orders.json`/
      `production_queue.json`이 없거나(최초 실행) 문법이 손상된 경우, 세 Repository 모두 크래시 없이
      빈 컬렉션으로 폴백하고, 폴백 이후에도 `Add`/`SaveQueue`로 정상 복구되는지 검증

### 빌드 검증

- `MSBuild.exe ... /p:PlatformToolset=v143` 빌드 성공 (경고 없음)
- `SampleOrderSystem-Chan-0613.exe --test` 실행 → **27/27 테스트 통과**, 종료 코드 0

**참고 저장소**: ConsoleMVC(도메인/인터페이스), DataPersistence(JSON 구현체/JsonLib)

---

## Phase 2 — Controller 계층 이식 및 재고·생산 라인 로직 완성 ✅ 완료

**목표**: 상태 전이 골격에 실제 재고 반영, 재고 상태 재평가, 생산 라인 실시간 정산 로직을 채워
기능 명세를 완전히 충족시킨다. 정확한 알고리즘은 [CLAUDE.md](../CLAUDE.md)의
"재고 및 생산 라인 처리 규칙 (상세)"를 그대로 따른다.

- [x] `Controller/SampleController.h/.cpp` — ConsoleMVC 구조를 계승하되 `RegisterSample`에 검증
      추가(계획에 없던 확장): 수율이 `(0, 1]` 범위를 벗어나거나 평균 생산시간이 0 이하면 등록을
      거부한다. 그렇지 않으면 `Model::CalculateActualProductionQuantity`가 나중에 0으로 나누기
      예외를 던지게 되므로, 경계에서 미리 막는 편이 사용자 경험상 낫다고 판단함
- [x] `Controller/MonitoringController.h/.cpp` — `GetInventoryStatus`가 각 `Sample`에 캐시된
      `GetInventoryLevel()`을 그대로 반환하도록 재작성 (ConsoleMVC의 고정 임계값 50 하드코딩
      방식은 사용하지 않음 — §2 정의와 맞지 않아 폐기)
- [x] **신규**: `Controller::OrderController::SettleProductionQueue(now)` — 계획에는
      `Model::ProductionLine`에 두는 것으로 되어 있었으나, 재고/주문 Repository에 접근해야 해서
      Controller 책임으로 옮김(ProductionLine은 순수 큐 자료구조로 유지). 별도 스레드 없이 지연
      평가로 구현: `Current` 작업의 완료 예정 시각과 `now`를 비교해 지났으면 완료 처리 후 다음
      작업을 승격하고, 새 `Current`의 `StartTime`은 "지금"이 아니라 직전 작업의 완료 예정 시각으로
      설정한다(`ProductionLine::SetCurrentJobStartTime` 신규 추가). 호출마다
      `IProductionLineRepository::SaveQueue`로 즉시 재저장
- [x] `Controller/OrderController.h/.cpp` — 생성자가 `IProductionLineRepository&`를 추가로 받도록
      **시그니처를 확장**(ConsoleMVC 대비 변경 — 생산 큐 영속화에 필요, CLAUDE.md에 명시된 대로
      "완전 무변경 계승"이 아님을 인지하고 진행):
  - `ReserveOrder`: 변경 없음. 단, 재시작 후 주문 번호가 기존 데이터와 겹치지 않도록 생성자에서
    기존 주문의 최대 순번을 스캔해 `nextOrderSequence_`를 이어서 채번하도록 함(계획에 없던 수정 —
    영속화된 Repository를 그대로 재사용하는 이 구조에서는 실제로 필요한 보정이었음)
  - `ApproveOrder`: 먼저 `SettleProductionQueue(now)` 호출 → 차감 전 재고 vs 주문 수량으로 여유/
    부족/고갈 재평가 → 재고 충분이면 즉시 차감 후 `CONFIRMED`, 부족이면 가용 재고 전량 배정(재고
    0) 후 `Model::CalculateShortage`/`CalculateActualProductionQuantity`/
    `CalculateTotalProductionTime`(Phase 1의 `InventoryCalculator` 그대로 재사용)으로 `ProductionJob`
    생성, `PRODUCING`
  - `RejectOrder`: 변경 없음
  - 완료 처리(`ApplyProductionCompletion`, 비공개 헬퍼로 전환): `actualQuantity` 전체를 재고에
    더한 뒤 **`shortageQuantity`만큼만** 다시 차감해 배정(ConsoleMVC 원본은 `order.GetQuantity()`
    전체를 차감했는데, 이러면 잉여 재고가 전혀 남지 않아 CLAUDE.md 예시와 어긋남 — 원본을 그대로
    베끼지 않고 CLAUDE.md 규칙대로 고쳐 구현함). 이후 같은 시료의 다른 `PRODUCING` 주문 부족분
    합계를 뺀 순가용재고로 재고 상태 재분류
  - `ReleaseOrder`: 변경 없음
- [x] 검증: 콘솔 임시 실행 대신, 아래 Unit Test(전체 사이클 자동화 테스트)로 대체

**Unit Test (이 Phase에서 바로 작성) — `tests/unit/OrderControllerScenarioTest.cpp`, 8건**
- [x] 전체 사이클: 재고 부족 승인 → (StartTime을 과거로 조작해 완료 시각 경과를 흉내) →
      `SettleProductionQueue` → `CONFIRMED` → `ReleaseOrder` → `RELEASED`, 잉여 재고 확인
- [x] 재고 충분 시 즉시 `CONFIRMED` 분기
- [x] **요구사항 9 재현**(2건): 재고 50 → 주문A(100) 승인(부족분 50, 재고 0) → 주문B(100) 승인 시
      부족분 100 전체. 3건 연속 승인으로 확장한 버전도 추가
- [x] **요구사항 8 재현**: 동일 시료 order1/order2 순차 완료, 중간 상태(고갈) → 최종 상태(여유),
      최종 재고 100 확인
- [x] **실시간 정산(캐치업)**: 매우 긴 오프라인(10시간)을 흉내낸 뒤 한 번의 `SettleProductionQueue`
      호출로 대기 중이던 2개 작업이 모두 완료되는지 확인
- [x] 재시작 후 주문 번호 비충돌: 새 `OrderController` 인스턴스를 만들어도 기존 주문과 ID가
      겹치지 않는지 확인

**적대적 테스트 (이 Phase에서 바로 작성) — `tests/adversarial/OrderControllerAdversarialTest.cpp`, 16건**
- [x] 상태 전이 위반(6건): `CONFIRMED`/`PRODUCING`/`REJECTED`인 주문 재승인·재거절 거부,
      `RESERVED`/`PRODUCING`/`REJECTED`/이미 `RELEASED`인 주문 출고 거부
- [x] 존재하지 않는 참조(4건): 알 수 없는 `SampleId`로 예약, 알 수 없는 `OrderId`로 승인/거절/출고
- [x] 경계·비정상 입력값(5건): 주문 수량 0/음수, 수율 0/음수/1 초과, 평균 생산시간 0/음수,
      중복 `SampleId` 등록 거부
- [x] 실시간 정산 경계값(2건): `StartTime`이 미래 시각이면 완료되지 않음, 완료 예정 시각과 `now`가
      정확히 같으면 완료됨(경계 포함 판정 확인)

### 발견 및 수정한 버그 (전/후 별도 커밋)

- **`ClassifyInventoryLevel`이 재고 0을 무조건 고갈로 판정하지 않던 문제**: PDF 명세("고갈 : 수량이
  0인 상태")를 다시 확인하는 과정에서, `referenceQuantity`가 0 이하인 경계 조건에서 재고 0을 "여유"로
  잘못 판정할 수 있음을 발견함. 회귀 테스트를 먼저 추가해 실패를 확인한 뒤(`[test]` 커밋), 판정
  순서를 바꿔 고갈 검사를 최우선으로 수행하도록 수정(`[fix]` 커밋)했다. 실사용 경로(주문 수량은
  항상 양수)에는 영향이 없었다.

### 빌드 검증

- `MSBuild.exe ... /p:PlatformToolset=v143` 빌드 성공 (경고 없음)
- `SampleOrderSystem-Chan-0613.exe --test` 실행 → **51/51 테스트 통과**, 종료 코드 0

**참고 저장소**: ConsoleMVC(시그니처/집계 로직 기반, 단 완료 처리 로직과 생성자 시그니처는 의도적으로
확장/수정함)

---

## Phase 3 — View 계층 이식 및 메인 메뉴 요약 정보 보강 ✅ 완료

**목표**: 콘솔 UI를 완성하고, 메인 메뉴에 요구되는 요약 정보를 추가한다.

- [x] `View/SampleView`, `View/OrderView`, `View/MonitoringView` — ConsoleMVC에서 파일 그대로(`cp`)
      이식, 수정 없음. `MonitoringView`는 원래부터 "잔여율" 게이지를 구현하지 않고 있어 별도 조치
      불필요했음(확인만 함). `SampleView`도 재고 입력 필드가 없는 4개 입력(ID/이름/평균생산시간/
      수율)만 받는 상태로 이미 맞았음
- [x] `View/ProductionLineView` — "완료 처리" 메뉴 항목과 `ShowCompletionResult`를 제거. **(권장)**
      진행률/완료 예정 시각 표시를 실제로 구현함: `ShowCurrentProduction(job, now)`이 저장된
      `startTime`/`totalProductionTime`으로 진행률(%)과 완료 예정 시각(`localtime_s` 기반 `HH:MM`)을
      계산해 표시. 이 계산에 필요한 `ProductionJob::CompletionTime()`을 Model에 신규 추가하고,
      Phase 2에서 `OrderController`가 직접 계산하던 동일 로직을 이 메서드 재사용으로 리팩터링해
      중복을 제거함(계획에 없던 정리)
- [x] `View/MainMenuView` — `Model::MainMenuSummary`(신규 DTO: 등록 시료 종수/총 재고/전체 주문
      건수/생산 라인 대기 건수)를 받아 상단에 표시하도록 확장. 계산 책임은
      `MonitoringController::GetMainMenuSummary(productionLine)`(신규)에 둠 — View는 렌더링만.
      메뉴는 원래부터 6개 항목 + 종료로 되어 있어 별도 조치 불필요했음(확인만 함)
- [x] 화면 레이아웃은 PDF 예시를 참고해 자유롭게 구성

**적대적 테스트 (이 Phase에서 바로 작성) — `tests/adversarial/MainMenuViewAdversarialTest.cpp`, 3건**
- [x] 잘못된 메뉴 입력값: 숫자가 아닌 입력 후에도 스트림이 정상 상태로 복구되어 다음 입력을 이어서
      읽는지, 범위 밖 메뉴 번호, 빈 입력(즉시 EOF) 모두 크래시 없이 `Invalid`를 반환하는지 검증.
      `std::cin`의 streambuf를 임시로 바꿔치기해 검증하며, 테스트 실패 시에도 RAII로 반드시
      원상복구되도록 함(다른 테스트에 영향 주지 않기 위함)

### 빌드 검증

- `MSBuild.exe ... /p:PlatformToolset=v143` 빌드 성공 (경고 없음)
- `SampleOrderSystem-Chan-0613.exe --test` 실행 → **54/54 테스트 통과**, 종료 코드 0

**참고 저장소**: ConsoleMVC

---

## Phase 4 — main.cpp 조립 (DI)

**목표**: In-Memory 대신 Json* Repository 구현체로 전체 시스템을 조립한다.

- [ ] `main.cpp`에서 `JsonSampleRepository`, `JsonOrderRepository`, `JsonProductionLineRepository` 생성
      (경로: `data/samples.json`, `data/orders.json`, `data/production_queue.json`)
- [ ] `Controller`/`View` 생성 및 메뉴 루프는 ConsoleMVC의 `main.cpp` 구조를 그대로 계승
- [ ] UTF-8 콘솔 코드페이지 설정(`SetConsoleOutputCP`/`SetConsoleCP`) 포함
- [ ] **앱 시작 직후, 메뉴 루프 진입 전에 `SettleQueue(now)`를 1회 호출**해 오프라인 기간 동안
      실제 시간상 이미 끝났어야 할 생산을 즉시 반영한다(CLAUDE.md §3, 요구사항 4·5). 이 호출은
      백그라운드 프로세스를 띄우는 것이 아니라, 로드된 `production_queue.json`의 `startTime`을
      현재 시각과 비교해 한 번에 정산하는 동기 호출이다.
- [ ] 최초 실행(데이터 파일 없음) / 재실행(데이터 파일 존재) / **생산 중 종료 후 재시작(오프라인
      캐치업)** 세 시나리오 모두 수동 검증

**적대적 테스트 (이 Phase에서 바로 작성)**
- [ ] 손상/누락된 영속 데이터로 전체 앱 기동: `data/samples.json`/`orders.json`/
      `production_queue.json` 중 일부 또는 전부가 없거나 손상된 상태로 조립된 앱(`main.cpp` 경로)을
      시작했을 때, 개별 Repository 단위 테스트(Phase 1)와 별개로 **조립된 전체 시스템**이 크래시
      없이 정상 메뉴까지 도달하는지 검증

**참고 저장소**: ConsoleMVC(main.cpp 구조), DataPersistence(Repository 생성자 패턴)

---

## Phase 5 — 보조 도구 통합 (DataMonitor / DummyDataGenerator)

**목표**: 별도 PoC로 개발했던 "데이터 모니터링 Tool"과 "Dummy 데이터 생성 Tool"을 최종 시스템의 데이터와
연결한다. (진행 원칙 참고 — 메인 애플리케이션과 파일이 겹치지 않으므로 sub-agent 위임 대상 후보)

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

## Phase 6 — 테스트 회귀 실행 및 커버리지 점검

**목표**: 새 테스트를 여기서 처음 작성하지 않는다. Phase 0~5에서 각 기능을 구현하며 그 자리에서
바로 작성해 둔 Unit Test/적대적 테스트([CLAUDE.md](../CLAUDE.md) 체크리스트의 "Test" 항목,
"진행 원칙" 참고)를 모아 **한 번에 회귀 실행**하고, 커버리지 공백이 없는지 최종 점검한다.

- [ ] 전체 테스트 스위트(`tests/unit/`, `tests/adversarial/`)를 한 번에 실행해 Phase 0~5 사이 다른
      기능 추가로 인한 회귀(regression)가 없는지 확인
- [ ] **커버리지 점검**: 아래 항목이 어느 Phase에서든 실제로 테스트되었는지 체크리스트로 재확인하고,
      빠진 것이 있으면 이 단계에서 보완한다.
      - Model 계산(ceil 공식, 재고 상태 분류) — Phase 1
      - Repository 왕복 + 손상/누락 파일 폴백 — Phase 1
      - Controller 시나리오, 요구사항 8·9 재현, 실시간 정산 캐치업 — Phase 2
      - 상태 전이 위반, 존재하지 않는 참조, 경계 입력값, 재고 선점 경합, 실시간 정산 경계값 — Phase 2
      - 잘못된 메뉴 입력 처리 — Phase 3
      - 손상/누락된 영속 데이터로 전체 앱 기동 — Phase 4
      - 보조 도구 동시 실행 시 충돌 없음 — Phase 5
- [ ] 빌드 스크립트 또는 문서화된 명령으로 "빌드 + 테스트 전체 실행"을 한 번에 수행할 수 있게 정리
      (Phase 0에서 마련한 하네스를 최종 형태로 굳힘)

**참고 저장소**: 없음 (신규, 단 각 Phase에서 이미 작성된 테스트를 모아 회귀 실행)

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
| 0. 스캐폴딩 | - | 높음 (구조 + 테스트 하네스 골격) |
| 1. Model/Repository | ConsoleMVC, DataPersistence | 중간 (ProductionLine 영속화는 신규, Unit/적대적 테스트 포함) |
| 2. Controller/재고 로직 | ConsoleMVC | 높음 (재고 반영·재평가, 실시간 정산 로직 + Unit/적대적 테스트 대부분) |
| 3. View | ConsoleMVC | 낮음 (요약 정보만 추가, 입력 검증 테스트 소량) |
| 4. main.cpp 조립 | ConsoleMVC, DataPersistence | 낮음 (조립 + 통합 적대적 테스트 1건) |
| 5. 보조 도구 | DataMonitor, DummyDataGenerator | 낮음 (경로 조정 위주) |
| 6. 테스트 회귀·정리 | - | 낮음 (신규 작성 없음 — 회귀 실행/커버리지 점검만) |
| 7. 마무리 | 전체 | 낮음 |
