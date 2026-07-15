# SampleOrderSystem-Chan-0613

## 프로젝트 목적

가상의 반도체 회사 "S-Semi"를 위한 **반도체 시료 생산주문관리 시스템** 최종 프로젝트.
엑셀/메모장으로 주문을 관리하며 발생하던 문제(주문 처리 여부 불명, 공정 완료 시점 불명, 불필요한
추가 공정)를 해소하기 위해, 시료 등록 → 주문 접수 → 승인/거절 → (필요시) 생산 → 출고까지의 전
흐름을 콘솔 기반 시스템으로 관리한다.

이 저장소는 앞서 완료한 4개 PoC(Proof of Concept) 저장소의 산출물을 **그대로 계승·통합**하여
기능 명세의 세부 로직을 완성하는 최종본이다.

```
ConsoleMVC        → 계층 구조 / 인터페이스 확정
DataPersistence    → Repository 인터페이스의 실제 구현체를 JSON 파일 기반으로 제공
DataMonitor        → Model(Repository)을 읽어 콘솔에 실시간 표시
DummyDataGenerator → Repository에 테스트 데이터 주입
SampleOrderSystem  → 위 4가지를 조립한 최종본, 기능 명세의 세부 로직을 완성        (본 저장소)
```

| PoC 항목 | 저장소 | 이 저장소에서의 역할 |
|---|---|---|
| MVC 스켈레톤 코드 | [ConsoleMVC-Chan-0613](../ConsoleMVC-Chan-0613) | Model/View/Controller 구조, Repository 인터페이스, 상태 전이·계산 규칙을 그대로 계승 |
| 데이터 영속성 처리 (JSON) | [DataPersistence-Chan-0613](../DataPersistence-Chan-0613) | `JsonSampleRepository`/`JsonOrderRepository`, `JsonLib`을 변경 없이 그대로 이식 |
| 데이터 모니터링 Tool | [DataMonitor-Chan-0613](../DataMonitor-Chan-0613) | 실시간 폴링 대시보드를 별도 실행 파일로 이식, `data/` 경로만 본 시스템 것으로 맞춤 |
| Dummy 데이터 생성 Tool | [DummyDataGenerator-Chan-0613](../DummyDataGenerator-Chan-0613) | `Generator/` 계층을 별도 실행 파일로 이식, 동일한 `data/` 폴더에 시딩 |
| 반도체 시료 생산주문관리 시스템 | **SampleOrderSystem-Chan-0613 (본 저장소)** | 위 4가지의 통합 + 미완결 로직(생산 라인 영속화, 재고 반영, 출고) 완성 |

## 참고 문서

- [docs/PLAN.md](docs/PLAN.md) — 단계별(Phase) 구현 계획. 각 Phase가 어떤 PoC를 참고/이식하는지,
  무엇이 신규 구현인지 정리되어 있다. 작업을 시작하기 전에 먼저 확인한다.

## 반드시 지켜야 하는 사항 (시스템 및 기술 명세)

### 시스템 규칙 (기능 명세)
- **콘솔 기반 애플리케이션**이며, 담당자가 메뉴 번호로 직접 명령을 입력하는 방식으로 동작한다.
- **생산 라인**: 시료를 하나씩 생산하는 단일 설비 흐름이며, **주문이 들어온 시료에 대해서만** 생산한다.
  생산 큐는 **FIFO(선입선출)** 로 처리한다.
- **주문 상태 전이**는 아래 흐름을 반드시 따른다.
  - `RESERVED`(주문 접수) → 승인 시 재고 비교
    - 재고 충분 → `CONFIRMED`(출고 대기)
    - 재고 부족 → `PRODUCING`(생산 라인 등록, 부족분만 생산) → 생산 완료 시 `CONFIRMED`로 전환
  - `CONFIRMED` → 출고 처리 → `RELEASED`(출고 완료, 명세서 표기는 "RELEASE"이나 문법상 과거분사
    `RELEASED`를 상태 이름으로 사용 — ConsoleMVC PoC에서 확정한 표기를 그대로 계승)
  - 거절 시 즉시 `REJECTED`로 전환. **`REJECTED`는 정상 흐름 밖의 상태이며 모니터링 집계에서 제외**한다.
- **생산량 계산 규칙**은 반드시 아래 공식을 그대로 사용한다.
  - 부족분 = 주문 수량 − 현재 재고
  - 실 생산량 = `ceil(부족분 / 수율)` — 이 값은 수율 손실을 미리 보정한 목표 생산량이며,
    **시뮬레이션 중에 수율을 다시 적용해 산출량을 깎지 않는다.** 계산된 실 생산량은 항상 100%
    그대로 만들어진다 (부족분 50, 수율 0.5 → 실 생산량 100이 그대로 생산되고, 그중 50만 주문에
    배정, 나머지 50은 재고로 남음). 상세 규칙은 "재고 및 생산 라인 처리 규칙 (상세)" 참고.
  - 총 생산 시간 = 평균 생산시간 × 실 생산량 (분 단위, **실제(현실) 시간** 기준으로 진행됨)
  - 수율 = 정상 시료 수 / 총 생산 시료 수 (0~1)
- **재고 상태 분류**: 여유(충분) / 부족(주문대비 재고 부족) / 고갈(재고 0) 3단계로 표기하되,
  이 상태는 매 조회 시 새로 정의하지 않고 **주문 승인 시점**과 **생산 완료 시점**, 이 두 이벤트에서만
  재평가한다 (다른 시점에는 재고 자체가 바뀌지 않으므로 상태도 바뀌지 않는다). 정확한 비교 기준은
  "재고 및 생산 라인 처리 규칙 (상세)" 참고. **PDF 예시 UI(페이지 19)의 "잔여율"(%) 게이지 바는
  구현하지 않는다** — 여유/부족/고갈 라벨과 현재 재고 수량만 표시하면 충분하며, 잔여율의 분모(무엇
  대비 퍼센트인지)가 명세상 명확히 정의되어 있지 않으므로 스코프에서 제외한다.
- **메인 메뉴 6개 항목 + 종료**를 모두 제공한다: 시료 관리(등록/조회/검색), 시료 주문(예약), 주문
  승인/거절, 모니터링(주문량/재고량), 생산 라인 조회, 출고 처리. (PDF 기능 명세 표는 "주문
  (접수/승인/거절)"을 한 카테고리로 묶어 5개 항목으로 설명하지만, 예시 UI(페이지 11)는 시료 주문과
  주문 승인/거절을 별도 메뉴로 분리해 `[1]~[6]` 6개 항목 + `[0]` 종료로 제시한다 — 이 저장소는
  예시 UI를 기준으로 6개 항목 + 종료로 구현한다.) 화면 구성 자체는 자유롭게 결정한다.
- **시스템에 등록된 시료만 주문 가능**하다 (존재하지 않는 `SampleId`로 주문 생성 불가).
- **시료 등록 시 재고는 입력받지 않는다.** PDF의 시료 등록 속성 목록(시료 ID, 이름, 평균 생산시간,
  수율)에 재고가 없으므로, 신규 등록되는 시료의 초기 재고는 항상 0이며, 이후 재고는 오직 "생산
  완료 시점"(재고 및 생산 라인 처리 규칙 §1)을 통해서만 증가한다. `SampleController::RegisterSample`
  시그니처에 재고 파라미터를 두지 않는다.

### 프로젝트 개발 평가 주안점 체크리스트 — 작업 전반에서 지속적으로 반영
- [ ] **문서 관리**: `CLAUDE.md`, `docs/PRD.md`, `docs/PLAN.md` 등 설계·의사결정 문서를 최신 상태로 유지한다.
- [ ] **Harness 도입**: 자동화된 빌드/테스트/검증 체계를 마련한다. 하네스(러너) 골격은
      [docs/PLAN.md](docs/PLAN.md) Phase 0에서 가장 먼저 준비해, 이후 각 Phase가 곧바로 테스트를
      추가할 수 있게 한다.
- [ ] **구현 진행 방식(Sub-agent 활용)**: Phase 0~4, 6~7의 핵심 구현은 이전 Phase의 인터페이스/파일에
      강하게 의존하고 같은 `.vcxproj`를 계속 같이 수정하므로 병렬 sub-agent로 나누지 않고 메인
      대화에서 순차 진행한다. Sub-agent는 "각 Phase 완료 직후 code-review로 검증"과 "Phase 5(보조
      도구 이식, 메인 앱과 파일이 겹치지 않음)"에만 사용한다. **테스트 전담 에이전트는 별도로 두지
      않는다** — 테스트는 그 기능을 구현한 Phase 안에서 메인 대화가 바로 작성하고, 대신
      code-review 서브에이전트의 점검 항목에 "테스트가 실제로 실패할 수 있는 케이스를 검증하는지,
      경계값을 빠뜨리지 않았는지, 적대적 테스트가 진짜로 공격적인지"를 명시적으로 포함해 독립된
      시각에서 재확인한다. 상세는 [docs/PLAN.md](docs/PLAN.md) "진행 원칙" 참고.
- [ ] **Test**: 아래 두 종류를 모두 작성한다. **몰아서 마지막에 작성하지 않고, 해당 기능을 구현한
      Phase 안에서 바로 작성한다** (예: Repository 완성 직후 왕복 테스트, 재고 로직 완성 직후 관련
      Unit/적대적 테스트) — 구체적인 배치는 [docs/PLAN.md](docs/PLAN.md)의 Phase별 섹션 참고.
  - **Unit Test**: Model 계산 로직(실 생산량 `ceil` 공식, 재고 상태 분류), 상태 전이, Repository
    왕복(round-trip)을 각각 독립적으로 검증하는 단위 테스트.
  - **적대적 테스트(Adversarial Test)**: 정상 입력이 아니라 "일부러 깨뜨리려는" 입력·순서로 시스템의
    가정을 검증하는 테스트. 예: 이미 승인된 주문을 다시 승인/거절 시도, 존재하지 않는 `SampleId`/
    `OrderId`로 주문·승인·출고 시도, `RESERVED`가 아닌 주문을 승인, `CONFIRMED`가 아닌 주문을 출고,
    손상된 JSON 파일 로드, 음수/0 수량 주문, 동일 시료에 대한 동시다발적 주문 승인으로 재고 선점
    경합(요구사항 9) 유발 등. 정상 흐름 테스트만으로는 놓치기 쉬운 경계·예외 케이스를 잡아낸다.
- [ ] **CleanCode**: 계층 간 책임 분리(Model은 콘솔 I/O 금지, View는 로직 금지 등) 원칙을 유지한다.
- [ ] **Commit 이력**: 작업 단위로 의미 있는 커밋을 남긴다.
- [ ] **Repository는 Public으로 생성**하고, 이름은 `SampleOrderSystem-Chan-0613` 규칙을 따른다(이미 반영됨).

## 상위 설계 계승 — 변경 없이 그대로 재사용하는 것

- **도메인 모델**: `Model::Sample` (`SampleId`, `Name`, `AvgProductionTime`, `Yield`, `Stock`),
  `Model::Order` (`OrderId`, `SampleId`, `CustomerName`, `Quantity`, `OrderStatus`),
  `Model::ProductionLine`/`ProductionJob` (ConsoleMVC 확정, DataPersistence의 `ToJson`/`FromJson` 포함).
  단, `ProductionJob`은 실시간 정산(아래 "재고 및 생산 라인 처리 규칙 (상세)" 참고)을 위해 이
  저장소에서 `StartTime` 필드를 새로 추가하므로, 이 항목만은 "완전 무변경 계승"이 아니라 "기존
  필드는 그대로 두고 신규 필드를 추가하는 확장"이다.
- **Repository 인터페이스**: `Model::ISampleRepository`, `Model::IOrderRepository`
  (`Add`/`Update`/`FindById`/`FindByNameContains`|`FindByStatus`/`GetAll`)
- **Repository 구현체**: `Model::JsonSampleRepository`, `Model::JsonOrderRepository` (DataPersistence
  완성본, write-through 저장, 손상/부재 파일에 대한 폴백 포함) — 한 줄도 수정하지 않고 그대로 이식한다.
- **JSON 라이브러리**: `Json/`(JsonLib) — Poc_JSON → DataPersistence → DataMonitor/DummyDataGenerator를
  거쳐 검증된 상태로 그대로 이식한다.
- **Controller**: `SampleController`, `OrderController`, `MonitoringController` — 상태 전이 로직은
  ConsoleMVC 확정본을 기반으로 하되, 아래 갭을 메우기 위해 `OrderController`는 시그니처까지 확장한다
  (생성자가 `Model::IProductionLineRepository&`를 추가로 받음 — 생산 큐 영속화에 필요, Phase 2에서
  실제로 반영됨). **완료 처리 로직은 ConsoleMVC 원본을 그대로 베끼지 않는다** — 원본은 재고 반영 시
  `order.GetQuantity()` 전체를 차감했으나, 이는 승인 시점에 이미 일부 재고가 소진된 사실을 반영하지
  못해 잉여 재고가 남지 않는 오류가 있다. 이 저장소는 CLAUDE.md에 정의된 대로 `shortageQuantity`만
  차감하도록 고쳐 구현한다.
- **필드 ↔ JSON 키 매핑(camelCase), 파일 단위 규약**: `data/samples.json`, `data/orders.json`
  (DataPersistence와 동일)

## PoC 대비 이 저장소에서 새로 메워야 하는 갭

4개 PoC는 각자의 스코프 안에서 의도적으로 다음을 다루지 않았다. 최종 시스템은 기능 명세를
완전히 충족해야 하므로, 아래 항목을 이 저장소에서 새로 구현한다.

1. **생산 라인(ProductionLine)의 JSON 영속화** — DataPersistence/DataMonitor/DummyDataGenerator
   CLAUDE.md에 공통으로 명시된 대로, `ProductionLine`(생산 큐)은 지금까지 In-Memory에만 존재했다.
   이 저장소에서는 `IProductionLineRepository` 인터페이스와 `JsonProductionLineRepository` 구현체를
   신규로 추가하여 `data/production_queue.json`에 큐 상태(대기 중인 `ProductionJob` 목록, 현재 처리
   중인 작업)를 저장한다. 파일 구조/저장 시점(write-through) 원칙은 DataPersistence의 기존 규약을
   그대로 따른다. `ProductionJob`에 새로 추가되는 `StartTime`(벽시계 타임스탬프)도 이 JSON에 함께
   저장해야 앱 재시작 후에도 실시간 정산(요구사항 4·5)이 가능하다.
2. **재고 반영 로직의 완성** — ConsoleMVC PoC는 상태 전이 골격만 갖추고 있었다. 최종본에서는
   `OrderController`가 재고 반영, 재고 상태 재평가, 생산 라인 실시간 정산을 명시적으로 처리해야
   한다. 정확한 알고리즘은 "재고 및 생산 라인 처리 규칙 (상세)" 섹션을 따른다.
3. **생산 라인의 실제 시간(real-time) 기반 진행** — ConsoleMVC PoC는 생산 완료를 사용자가 메뉴에서
   수동으로 트리거하는 방식(`CompleteCurrentProduction()`)으로 설계했다. 최종본에서는 생산이 앱
   실행 여부와 무관하게 실제 시간에 따라 진행되어야 하므로(요구사항 3~5), `ProductionJob`에 시작
   시각(`StartTime`) 필드를 추가하고, 앱 시작 시/주문 승인 직전/생산 라인 조회 시마다 경과 시간
   기준으로 완료된 작업을 자동 정산하는 로직을 새로 구현한다. 상세는 아래 섹션 참고.
4. **DataMonitor/DummyDataGenerator의 실행 파일 통합** — 두 PoC는 원래 별도 프로세스(별도 `.exe`)로
   설계되어 있다. 최종 시스템에서도 메인 애플리케이션과는 별도의 실행 파일로 유지하되, 동일한
   `data/` 폴더를 가리키도록 경로만 맞춘다 (메인 앱 실행 중에도 안전하게 읽기 전용으로 병행 실행 가능).

## 재고 및 생산 라인 처리 규칙 (상세)

기능 명세만으로는 재고 반영 시점과 생산 완료 판정 방식이 모호할 수 있어 아래와 같이 구체적으로
확정한다. `Controller`/`Model` 구현은 반드시 이 규칙을 그대로 따른다. (일부 항목은 원 명세의
표현이 다소 함축적이어서, 아래처럼 구체적인 계산식으로 해석하여 확정했다 — 이후 변경이 필요하면
이 섹션과 코드를 함께 갱신한다.)

아래 규칙은 특정 시료 하나만을 전제하지 않는다 — 생산 라인은 시스템 전체에 **단일 공유 큐**이므로,
서로 다른 시료(수율·평균생산시간이 각기 다름)의 작업이 이 큐에 섞여 있어도 "승인된 순서" 그대로
전역 FIFO로 처리되고, `SumPendingShortageForSample`(§2)은 시료 ID로 정확히 필터링되어 다른 시료의
부족분과 섞이지 않는다. 이는 `tests/unit/OrderControllerMultiSampleTest.cpp`(예약 순서와 승인
순서가 다른 3개 시료 교차 시나리오)로 실제 검증되어 있다.

### 1. 재고 수량(`Sample.Stock`) 반영 시점

- **주문 승인 시점**: 재고와 주문 수량을 비교해 즉시 반영한다.
  - 재고 ≥ 주문 수량 → 주문 수량만큼 즉시 차감, `CONFIRMED`
  - 재고 < 주문 수량 → 가용 재고 전량을 이 주문에 배정하고 재고를 0으로 차감, 부족분(주문 수량 −
    배정 전 재고)만큼 생산 라인에 등록, `PRODUCING`
  - **이 시점의 부족분/실 생산량 계산은 오직 "지금 이 순간의 `Sample.Stock`"만 근거로 한다. 현재
    생산 중이거나 대기열에 있는 다른 작업이 앞으로 만들어낼 물량은 전혀 고려하지 않는다**
    (요구사항 6). 대신 승인 처리 직전에 반드시 아래 "3. 생산 라인 실시간 정산"을 먼저 수행해, 그
    시점까지 실제로 이미 끝났어야 할 생산을 재고에 먼저 반영한 뒤 비교한다.
  - 예(요구사항 9): 재고 50, 주문A(수량 100) 승인 → 재고 50 전량을 A에 배정(재고 0), 부족분 50
    생산 등록. 이어서 A의 생산이 아직 끝나지 않은 상태에서 주문B(수량 100)가 들어와 승인되면, 이
    시점의 재고는 이미 0이므로 B의 부족분은 100 전체가 된다 (A가 재고를 "선점"한 결과).

- **생산 완료 시점** (실시간 정산 중 한 작업이 완료 판정될 때마다):
  1. 그 작업의 실 생산량(`actualQuantity`) 전체를 재고에 더한다.
  2. 그중 해당 주문의 부족분(`shortageQuantity`)만큼을 다시 차감해 주문에 배정한다.
  3. 순증가분(잉여 = `actualQuantity − shortageQuantity`)만 최종적으로 재고에 남는다.
  4. 해당 주문 상태를 `PRODUCING → CONFIRMED`로 전환한다.
  - 재고 반영은 **주문(= 생산 작업) 단위**로, 그 주문의 생산이 완전히 끝난 시점에만 발생한다
    (부분 반영 없음, 요구사항 3).
  - 예(요구사항 2): 부족분 50, 수율 0.5 → 실 생산량 100. 완료 시 재고에 100을 더하고, 주문에
    필요한 50을 다시 차감해 배정 → 잉여 50이 재고에 남는다.

### 2. 재고 상태(여유/부족/고갈) 재평가 시점

값이 실제로 바뀔 수 있는 위 두 이벤트가 발생할 때만 재평가한다 (다른 시점에는 재고 자체가
변하지 않으므로 상태도 변하지 않는다).

- **주문 승인 시점**: 그 주문의 수량과, 차감 전 재고를 비교한다.
  - 재고 ≥ 주문 수량 → 여유 / 0 < 재고 < 주문 수량 → 부족 / 재고 == 0 → 고갈
- **생산 완료 시점**: 위 "1.의 재고 반영"을 마친 직후, 같은 시료에 대해 **아직 생산 중인
  (`PRODUCING`) 다른 주문들의 부족분 합계**를 구해 재고에서 제외한 값(= 다른 주문들에 이미
  내정된 몫을 뺀 "순가용 재고")을, 방금 완료된 주문의 원래 수량과 같은 기준으로 다시 비교해
  여유/부족/고갈을 판단한다.
  - 예(요구사항 8 상황 연장): A-order1(수량 50, 부족분 50, 실생산량 100)과 A-order2(수량 50,
    부족분 50, 실생산량 100)가 각각 생산 중/대기 중이라 하자. order1이 먼저 완료되면 재고에 100을
    더하고 50을 order1에 배정 → 재고 50. 이 시점에 order2가 아직 `PRODUCING`이므로, 순가용 재고 =
    50 − (order2의 부족분 50) = 0 → order1 완료 시점의 재분류는 "고갈". 이후 order2도 완료되면
    재고에 100을 더하고 50을 배정 → 재고 100. 이제 생산 중인 다른 주문이 없으므로 순가용 재고
    100 → "여유"로 재분류된다.

### 3. 생산 라인 실시간(real-time) 정산

생산은 **애플리케이션 실행 여부와 무관하게 실제(현실) 시간 기준으로 진행**된다고 간주한다
(요구사항 3-b, 4, 5). 이를 위해 `ProductionJob`에 시작 시각(`StartTime`, 벽시계 타임스탬프)을
새 필드로 추가하고 JSON에 함께 영속화한다.

- **백그라운드 프로세스/타이머를 두지 않는다 — 지연 평가(lazy evaluation) 방식이다.** 별도
  스레드나 타이머가 계속 시간을 흘려보내며 생산을 "실행"하는 것이 아니라, 각 `ProductionJob`의
  완료 예정 시각(`StartTime + 총 생산 시간`)만 계산해 저장해 두고, 아래 트리거 시점에 "지금(now)
  vs 완료 예정 시각"을 비교하는 방식으로 동작한다. 앱이 실행되지 않는 동안에는 어떤 계산도
  일어나지 않으며, 앱이 종료되어 있던 기간 동안의 경과 시간은 다음 트리거 시점에 한꺼번에
  "따라잡기(catch-up)" 방식으로 정산된다.
- **정산에 필요한 상태(`StartTime` 포함 `ProductionJob` 전체, 대기열 순서)는 반드시 영속화되어야
  한다.** `JsonProductionLineRepository`가 `data/production_queue.json`에 write-through로 즉시
  저장하므로, 앱이 종료되어도 각 작업의 시작 시각과 대기 순서는 파일에 그대로 남는다. 앱이
  재시작되면 이 파일에서 큐 상태를 그대로 로드한 뒤, 트리거 시점에 `SettleProductionQueue`가
  로드된 `StartTime`과 현재 시각을 비교해 오프라인 기간 동안 이미 끝났어야 할 작업들을 정산한다.
  이 영속화가 없으면(예: `StartTime`을 메모리에만 들고 있으면) 앱 재시작 시 시작 시각을 알 수
  없어 "실제 시간만큼 생산이 진행되었다"는 요구사항(4·5)을 충족할 수 없다.
- 완료 예정 시각 = `StartTime + 총 생산 시간(평균생산시간 × 실생산량, 분)`
- 생산 라인은 **단일 라인**이므로 한 번에 하나의 작업만 `Current`로 처리하고, 나머지는 FIFO
  대기열에 머문다.
- 아래 정산 함수(`SettleProductionQueue`)를 다음 시점마다 호출해, 실제 경과 시간 기준으로 이미
  끝났어야 할 작업들을 순서대로 처리한다: **앱 시작 시**, **주문 승인으로 새 작업을 큐에 넣기
  직전**, **생산 라인 조회 화면을 열 때**.
  1. `Current` 작업이 없고 대기열에 작업이 있으면, 맨 앞 작업을 `Current`로 승격한다. 이때
     `StartTime`은 "지금"으로 기록한다 (생산 라인이 유휴 상태였으므로).
  2. `Current` 작업의 완료 예정 시각이 이미 지났으면: 위 "1. 재고 반영 — 생산 완료 시점"과
     "2. 재고 상태 재평가"를 수행해 그 작업을 완료 처리하고, 대기열에 다음 작업이 있으면 그
     작업을 `Current`로 승격한다. **이때 새 `Current`의 `StartTime`은 "지금"이 아니라 직전 작업의
     완료 예정 시각으로 기록한다** — 오프라인 기간이 길었어도 실제 경과 시간을 그대로 이어서
     반영하기 위함이다. 이후 다시 1번부터 반복한다 (오프라인 기간이 충분히 길면 한 번의 정산으로
     여러 작업이 한꺼번에 완료 처리될 수 있다, 요구사항 4·5).
  3. `Current` 작업의 완료 예정 시각이 아직 지나지 않았으면 아무 것도 하지 않는다 (진행 중).
- **대기열에 들어간 작업은 그 시점에 계산된 실 생산량으로 무조건 끝까지 생산한다** — 이후 재고
  상황이 바뀌어도(예: 다른 작업의 완료로 재고가 생겨도) 재계산하거나 취소하지 않는다
  (요구사항 7).
- 이 정산 방식 덕분에 "생산 완료 처리"는 더 이상 사용자가 메뉴에서 수동으로 트리거하는 동작이
  아니다. `View`의 "생산 라인 조회" 메뉴는 조회할 때마다 먼저 `SettleProductionQueue`를 호출해
  최신 상태를 반영한 뒤 화면에 표시하기만 한다 (ConsoleMVC의
  `OrderController::CompleteCurrentProduction()`은 사용자 트리거용 공개 메서드에서, 정산 로직
  내부에서 호출되는 헬퍼로 성격이 바뀐다).
- **(권장, 필수 아님) 진행률/완료 예정 시각 표시**: PDF 예시 UI(페이지 21)는 현재 생산 중인 작업에
  대해 진행률(%)과 완료 예정 시각을 함께 보여준다. `Current` 작업의 `StartTime`과 완료 예정 시각을
  이미 갖고 있으므로 `진행률 = (now − StartTime) / 총생산시간`, `완료 예정 시각 = StartTime + 총
  생산시간`으로 추가 계산 없이 표시할 수 있다. PDF는 "표기할 정보 수준은 자율적으로 결정"이라고
  명시하므로 필수 구현 대상은 아니지만, 이미 갖고 있는 데이터로 저비용에 구현 가능하므로 View에
  반영하는 것을 권장한다. (참고: 이 진행률은 위에서 제외한 모니터링 화면의 "잔여율"과는 다른
  개념이다 — 이건 생산 작업 하나의 시간 경과율, 잔여율은 재고 수량의 비율이다.)

## 아키텍처

```
SampleOrderSystem-Chan-0613/
├── Model/
│   ├── Sample.h / .cpp                      # + Phase 2: InventoryLevel 캐시 필드(§2) 추가
│   ├── Order.h / .cpp                       # OrderStatus enum
│   ├── ProductionLine.h / .cpp              # ProductionJob(startTime), 벌크 접근자
│   │                                        # + Phase 2: SetCurrentJobStartTime 추가
│   │                                        # + Phase 3: ProductionJob::CompletionTime() 추가
│   │                                        # (Controller/View가 공유하는 완료 시각 계산 헬퍼)
│   ├── Dtos.h                               # OrderApprovalResult, InventoryStatusItem 등
│   ├── InventoryLevel.h / .cpp              # 신규(Phase 2) — Dtos.h/Sample.h 순환 include 방지용 분리
│   ├── InventoryCalculator.h / .cpp         # 신규(Phase 1) — 부족분/실생산량/재고상태 순수 계산 함수
│   └── Repository/
│       ├── ISampleRepository.h / IOrderRepository.h / IProductionLineRepository.h
│       └── JsonSampleRepository / JsonOrderRepository / JsonProductionLineRepository (신규)
├── Json/                                    # JsonLib (수정 없이 소비)
├── Controller/
│   ├── SampleController.h / .cpp            # RegisterSample에 수율/평균생산시간 검증 추가(Phase 2)
│   ├── OrderController.h / .cpp             # 예약/승인/거절/출고 + 재고 반영 + SettleProductionQueue
│   │                                        # (생성자가 IProductionLineRepository&도 받음 — ConsoleMVC
│   │                                        # 대비 시그니처 확장, Phase 2)
│   └── MonitoringController.h / .cpp        # GetInventoryStatus는 Sample의 캐시값을 그대로 반환
├── View/
│   ├── MainMenuView.h / .cpp                # 요약 정보(Model::MainMenuSummary) 표시
│   ├── SampleView / OrderView / MonitoringView       # ConsoleMVC에서 변경 없이 이식
│   └── ProductionLineView.h / .cpp          # "완료 처리" 메뉴 제거, 진행률/완료 예정 시각 표시 추가
├── data/
│   ├── samples.json / orders.json / production_queue.json
├── tools/
│   ├── DataMonitor/                         # DataMonitor PoC 이식 (읽기 전용 폴링 대시보드)
│   └── DummyDataGenerator/                  # DummyDataGenerator PoC 이식 (더미 데이터 시딩)
├── tests/
│   ├── TestFramework.h / .cpp                # 최소 assert 기반 테스트 러너 (Phase 0에서 구성)
│   │                                         # TEST(suite, name), ASSERT_TRUE/EQ/THROWS 매크로 제공
│   ├── unit/                                 # Unit Test (Phase별로 그 자리에서 추가, PLAN.md 참고)
│   └── adversarial/                          # 적대적 테스트 (Phase별로 그 자리에서 추가)
├── main.cpp                                 # Json Repository 구현체 조립(DI) + 메뉴 루프 (Phase 4)
│                                             # "--test" 인자로 테스트 실행 분기는 Phase 0부터 유지
└── docs/
    ├── PRD.md
    └── PLAN.md
```

### 역할 원칙 (ConsoleMVC 계승)

- **Model**: 도메인 데이터와 비즈니스 규칙(재고 계산, 상태 전이, 생산량 계산)만 담당. 콘솔 I/O 직접
  호출 금지. Repository는 인터페이스 의존 + 구현체 분리.
- **View**: 콘솔 입출력만 담당. 비즈니스 로직·상태 판단 포함 금지.
- **Controller**: View 입력을 받아 Repository를 통해 Model을 조작하고 결과를 View에 전달. Model과
  View는 서로 직접 참조하지 않고 반드시 Controller를 경유한다.
- **main.cpp**: Repository 구현체(Json*) 조립과 데이터 폴더 경로 결정만 담당하는 유일한 지점.
- 순환 의존 금지: `Model → View/Controller` 방향의 include는 금지.

## 기술 스택

- 언어: C++20 (`stdcpp20`)
- 빌드: Visual Studio (MSBuild, vcxproj), PlatformToolset v145 (로컬에 없으면
  `/p:PlatformToolset=v143`으로 오버라이드 가능 — 앞선 PoC들에서 확인됨)
- **MSBuild는 CMake처럼 폴더를 스캔해 소스 파일을 자동으로 인식하지 않는다.** 다른 PoC에서 파일을
  폴더에 복사해 넣는 것만으로는 컴파일 대상에 포함되지 않으며, 반드시 `.vcxproj`의 `ClInclude`/
  `ClCompile` `ItemGroup`과 `.vcxproj.filters`에 명시적으로 추가해야 한다. Phase 0에서
  `main.cpp`/`tests/*`를 등록하며 이 습관을 확립했다 — 이후 Phase에서 파일을 추가할 때마다 계속
  빠뜨리지 않는다 ([docs/PLAN.md](docs/PLAN.md) 각 Phase 체크리스트 참고).
- **서로 다른 폴더에 같은 이름의 `.cpp` 파일이 있으면 기본 설정에서 오브젝트 파일 이름이 충돌한다**
  (`MSB8027` 경고, 링크 시 한쪽이 조용히 무시됨 — Phase 0에서 `tests/unit/FrameworkSelfTest.cpp`와
  `tests/adversarial/FrameworkSelfTest.cpp`를 추가하며 실제로 겪음). 이 프로젝트는 모든 `ClCompile`
  설정에 `<ObjectFileName>$(IntDir)%(RelativeDir)</ObjectFileName>`를 지정해 폴더별로 오브젝트
  파일을 분리함으로써 해결했다 — 새 파일을 추가할 때 이름이 겹쳐도 안전하다. 빌드 시 이 경고
  (`둘 이상의 파일이 같은 위치에 출력을 생성합니다`)가 다시 나타나면 신규 파일 경로를 점검한다.
- 대상: Windows Console Application (Win32 / x64, Debug / Release)
- 외부 의존성 없음 — JSON 처리는 자체 구현 `JsonLib`만 사용, 난수는 표준 라이브러리 `<random>`만 사용
- 소스 파일은 BOM 없는 UTF-8이므로, 한글 리터럴이 포함된 모든 `ClCompile` 항목에 `/utf-8`을 지정한다.
  콘솔 코드페이지도 `SetConsoleOutputCP(CP_UTF8)`/`SetConsoleCP(CP_UTF8)`로 맞춘다.
- 로컬 빌드 확인 명령(PlatformToolset이 v145가 없는 환경): `MSBuild.exe
  SampleOrderSystem-Chan-0613.slnx /p:Configuration=Debug /p:Platform=x64 /p:PlatformToolset=v143`

## 커밋/문서 관리

- Agentic Engineering 평가 기준(문서 관리, Harness, Test, CleanCode, Commit 이력)을 고려하여 작업
  단위로 의미 있는 커밋을 남긴다.
- 이 파일(CLAUDE.md)은 스코프가 바뀌거나 구조가 변경될 때 함께 갱신한다.
- `docs/PRD.md`에 기능 명세를 기준으로 한 요구사항 정의를, `docs/PLAN.md`에 단계별 구현 계획을 둔다.
- 이식한 코드(Model, Repository, JsonLib)를 수정할 경우, 통합 지점이 깨질 수 있으므로 변경 사유를
  커밋 메시지에 명시하고 필요 시 원본 PoC 저장소 쪽 CLAUDE.md에도 반영한다.
