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
     독립적으로 재점검한다.
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

## Phase 2 — Controller 계층 이식 및 재고·생산 라인 로직 완성

**목표**: 상태 전이 골격에 실제 재고 반영, 재고 상태 재평가, 생산 라인 실시간 정산 로직을 채워
기능 명세를 완전히 충족시킨다. 정확한 알고리즘은 [CLAUDE.md](../CLAUDE.md)의
"재고 및 생산 라인 처리 규칙 (상세)"를 그대로 따른다 (아래는 그 요약).

- [ ] `Controller/SampleController.h/.cpp` — ConsoleMVC에서 그대로 복사
- [ ] `Controller/MonitoringController.h/.cpp` — ConsoleMVC에서 그대로 복사하되, `GetInventoryStatus`가
      반환하는 값은 "매 조회 시 재계산"이 아니라 "승인/생산완료 시점에 갱신된 최신 값을 그대로
      반환"하는 형태로 동작을 재확인 (§2)
- [ ] **신규**: `Model::ProductionLine::SettleQueue(now)` (또는 동급의 정산 함수) — 실제 경과 시간
      기준으로 완료된 작업을 FIFO 순서로 순차 처리하는 로직 (§3). **별도 스레드/타이머 없이 지연
      평가(lazy evaluation)로 구현한다** — 백그라운드에서 계속 시간을 흘려보내며 생산을 진행시키는
      것이 아니라, 아래 트리거 시점에 호출될 때만 저장된 `StartTime`과 인자로 받은 `now`(현재
      시각)를 비교해 이미 끝났어야 할 작업을 그 순간에 몰아서 정산한다. 앱이 꺼져 있는 동안에는
      아무 계산도 일어나지 않는다. 다음을 반복 수행:
      1. `Current` 없고 대기열 있음 → 맨 앞 작업을 `Current`로 승격, `StartTime = now`
      2. `Current`의 완료 예정 시각(`StartTime + totalProductionTime`)이 지났음 → 완료 처리(아래
         재고 반영 로직 호출) 후 다음 작업을 `Current`로 승격, `StartTime = 직전 작업의 완료 예정
         시각` (지금이 아님 — 오프라인 기간도 실제 시간으로 반영, 요구사항 4·5)
      3. 안 지났으면 종료 (진행 중)
      호출할 때마다 `IProductionLineRepository`를 통해 갱신된 큐 상태(`StartTime` 포함)를 즉시
      다시 저장해, 다음 호출(다음 실행/다음 트리거) 때도 정확히 이어서 계산할 수 있게 한다.
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
- [ ] 검증: 콘솔 임시 실행으로 "재고 부족 주문 승인 → (실제 시간 경과 대기 또는 짧은
      `avgProductionTime`으로 테스트) → 생산 라인 조회 시 자동 완료 확인 → 출고"까지 한 사이클을
      수동으로 실행해 상태 전이가 명세와 일치하는지 확인

**Unit Test (이 Phase에서 바로 작성)**
- [ ] Controller 시나리오 테스트: 위 수동 검증("재고 부족 주문 승인 → 생산 완료 → 출고" 사이클)을
      자동화된 테스트로 승격, 재고 충분/부족 각 분기와 잉여 재고 처리 포함
- [ ] **CLAUDE.md 요구사항 9 재현 테스트**: 재고 50 → 주문A(100) 승인(부족분 50, 재고 0) →
      주문B(100) 승인 시 부족분이 100 전체가 되는지 검증
- [ ] **CLAUDE.md 요구사항 8 재현 테스트**: 동일 시료에 대해 order1(부족분 50, 실생산량 100),
      order2(부족분 50, 실생산량 100)가 순차로 생산 완료된 후 최종 재고가 정확히 100인지, 그
      사이 재고 상태가 예시대로 고갈→여유로 전이하는지 검증 (§2)
- [ ] **실시간 정산(캐치업) 테스트**: `ProductionJob.startTime`을 인위적으로 과거로 설정한 뒤
      `SettleQueue(now)`를 호출해, 오프라인 기간에 끝났어야 할 작업(대기열에 있던 것 포함, 요구사항
      5)까지 한 번에 정산되는지, 완료 예정 시각이 아직 안 지난 경우 아무 것도 바뀌지 않는지 검증

**적대적 테스트 (이 Phase에서 바로 작성)**
- [ ] 상태 전이 위반: 이미 `CONFIRMED`/`PRODUCING`/`REJECTED`/`RELEASED`인 주문을 다시 승인·거절
      시도, `CONFIRMED`가 아닌 주문(`RESERVED`/`PRODUCING`/`REJECTED`)을 출고 시도
- [ ] 존재하지 않는 참조: 존재하지 않는 `SampleId`로 `ReserveOrder` 호출, 존재하지 않는 `OrderId`로
      `ApproveOrder`/`RejectOrder`/`ReleaseOrder` 호출
- [ ] 경계·비정상 입력값: 주문 수량 0 또는 음수, 수율 0(0으로 나누기 방지 확인), 평균 생산시간 0
- [ ] 동시성·경합: 동일 시료에 대해 재고보다 큰 주문을 연속 승인해 재고 선점 경합을 유발하는
      시나리오(요구사항 9)가 정상적으로 처리되는지 재확인(Unit Test의 요구사항 9 테스트와 별개로,
      3건 이상 연속 승인 등 더 공격적인 순서로 반복)
- [ ] 실시간 정산 경계값: `StartTime`이 미래 시각(시계 역행 등 비정상 상태)이거나, 완료 예정 시각과
      `now`가 정확히 같은 경계 시각일 때 `SettleQueue`가 예외 없이 일관되게 처리하는지 검증

**참고 저장소**: ConsoleMVC(시그니처/집계 로직), DataMonitor(집계 로직 재사용 사례 참고)

---

## Phase 3 — View 계층 이식 및 메인 메뉴 요약 정보 보강

**목표**: 콘솔 UI를 완성하고, 메인 메뉴에 요구되는 요약 정보를 추가한다.

- [ ] `View/SampleView` — ConsoleMVC에서 그대로 복사. 시료 등록 화면은 시료 ID/이름/평균
      생산시간/수율 4개 입력만 받는다 — **재고는 입력받지 않으며 항상 0으로 시작**한다(CLAUDE.md
      "시스템 규칙" 참고)
- [ ] `View/OrderView` — ConsoleMVC에서 그대로 복사 (필요 시 문구만 다듬기)
- [ ] `View/MonitoringView` — ConsoleMVC에서 복사하되, **PDF 예시 UI(페이지 19)의 "잔여율"(%) 게이지
      바는 구현하지 않는다.** 시료별 재고 수량과 여유/부족/고갈 라벨만 표시한다(CLAUDE.md "시스템
      규칙" 참고)
- [ ] `View/ProductionLineView` — ConsoleMVC에서 복사하되, **"완료 처리" 메뉴 항목은 제거**한다.
      생산 완료는 더 이상 사용자가 트리거하는 동작이 아니라 실제 시간 경과에 따라 자동으로
      정산되기 때문이다(CLAUDE.md §3). 이 화면을 열 때마다 Controller가 먼저 `SettleQueue`를
      호출해 최신 상태를 반영한 뒤, 현재 처리 중인 작업/대기열만 조회 전용으로 표시한다. **(권장)**
      `Current` 작업의 진행률(%)과 완료 예정 시각을, 저장된 `StartTime`으로부터 추가 계산 없이 함께
      표시한다(PDF 예시 UI 페이지 21, CLAUDE.md §3 "권장" 참고) — 필수는 아니다.
- [ ] `View/MainMenuView` — ConsoleMVC 버전을 기반으로, PDF 예시 UI처럼 **요약 정보**(등록 시료 종수,
      총 재고, 전체 주문 건수, 생산 라인 대기 건수)를 상단에 표시하도록 확장. 이 정보는
      `SampleController.ListSamples()` + `MonitoringController` 조회 결과를 조합해 계산 (View 자체는
      로직을 갖지 않고 Controller 결과만 렌더링). **메뉴 항목은 시료 관리/시료 주문/주문 승인·거절/
      모니터링/생산 라인 조회/출고 처리 6개 + 종료**로 구성한다(CLAUDE.md "시스템 규칙" 참고 — PDF
      기능 명세 표는 5개 카테고리로 설명하지만 예시 UI는 6개 메뉴로 분리되어 있음)
- [ ] 화면 레이아웃은 PDF의 예시 화면을 참고하되 자유롭게 구성 (PDF도 "화면 구성은 자유롭게 결정"이라
      명시)

**적대적 테스트 (이 Phase에서 바로 작성)**
- [ ] 잘못된 메뉴 입력값: 존재하지 않는 메뉴 번호, 숫자가 아닌 입력, 빈 입력이 View에서 크래시 없이
      "잘못된 선택" 안내 후 메뉴로 복귀하는지 검증 (View는 로직을 갖지 않으므로 입력 파싱/가드 로직만
      대상으로 최소한으로 확인)

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
