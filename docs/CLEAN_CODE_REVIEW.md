# Clean Code / SOLID 트레이드오프 검토 (Phase 7)

Phase 7 "마무리" 단계에서 계층 간 책임 분리(Model/View/Controller 원칙)와 OOP/SOLID 원칙을
기준으로 전체 코드를 재점검한 기록이다. 발견된 항목마다 "고쳤다"와 "의도적으로 그대로 뒀다"를
구분하고, 그대로 둔 항목은 반드시 트레이드오프 근거를 남긴다 — 원칙을 기계적으로 적용하기보다,
이 프로젝트의 규모·이미 검증된 테스트 자산·CLAUDE.md에 명시된 계승 계약을 함께 고려했다.

## 1. 계층 분리(Model/View/Controller) 재점검 결과

- **View → 비즈니스 로직 유출**: 없음. `View/*.cpp` 전체를 확인한 결과, 재고 비교·상태 전이
  가능 여부 판단·수량 계산 등은 전부 Controller/Model에 있고 View는 Controller가 반환한 결과를
  그대로 표시만 한다. `ProductionLineView`의 진행률(%) 계산과 `MonitoringView`의
  `InventoryLevel`→한글 라벨 매핑은 CLAUDE.md가 View 책임으로 명시적으로 허용한 표시 포맷팅이라
  위반이 아니다.
- **Model → 콘솔 I/O 유출**: `JsonSampleRepository.cpp`/`JsonOrderRepository.cpp`/
  `JsonProductionLineRepository.cpp`가 손상된 파일을 만나면 `std::cerr`로 진단 로그를 직접
  출력한다. 원칙적으로는 "Model은 콘솔 I/O 금지"에 어긋나지만 **의도적으로 그대로 둔다** —
  아래 4번 항목에서 근거를 설명한다.

## 2. 실제로 반영한 리팩토링

### OrderController의 상태 검증 조회 패턴 중복 제거

`ApproveOrder`/`RejectOrder`/`ReleaseOrder` 세 메서드가 각각
"`FindById`로 주문을 찾고, 지금 상태가 기대 상태(RESERVED/CONFIRMED)가 아니면 실패 처리"하는
동일한 블록을 인라인으로 중복 작성하고 있었다. 이를 private 헬퍼
`FindOrderRequiringStatus(orderId, requiredStatus)`로 추출했다 (커밋 `4a7ad34`).

- **판단 근거**: 공개 시그니처·반환값·부작용을 전혀 바꾸지 않는 순수 behavior-preserving
  리팩토링이라 위험이 없고, 3곳의 중복을 1곳으로 줄여 "상태 검증 실패 처리 방식"이 바뀔 때
  고쳐야 할 곳이 하나로 줄어든다(유지보수성 개선). 리팩토링 직후 전체 회귀 테스트 56/56이
  그대로 통과해 동작 동일성을 확인했다.

## 3. 검토했지만 "그대로 둔" 트레이드오프

### 3-1. `OrderController`의 SRP 긴장 — 주문 생명주기 + 생산 라인 정산을 한 클래스가 담당

`OrderController`는 예약/승인/거절/출고(주문 생명주기)뿐 아니라 생산 라인 등록·실시간 정산·
재고 반영(`ApplyProductionCompletion`, `SettleProductionQueue`)까지 함께 담당한다. 엄밀한
SRP 기준으로는 "주문 상태를 어떻게 바꾸는가"와 "생산이 언제 끝나고 재고에 어떻게 반영되는가"는
서로 다른 변경 이유(reason to change)로 볼 수 있어, `OrderLifecycleController` +
`ProductionSettlementService` 같은 분리도 가능하다.

- **결정: 분리하지 않고 그대로 둔다.**
- **이유**: 두 책임이 CLAUDE.md의 재고/생산 라인 처리 규칙상 **원자적으로 얽혀 있다** — 주문
  승인(`ApproveOrder`)은 그 순간 반드시 최신 정산 결과(`SettleProductionQueue`)를 먼저 반영한
  뒤 재고를 비교해야 하고(요구사항 6), 생산 완료 시 재고 반영은 반드시 해당 주문의 상태 전이와
  같은 트랜잭션 안에서 함께 일어나야 한다(요구사항 3). 클래스를 나누면 이 원자성을 지키기 위해
  두 클래스가 서로를 호출하거나 상위 오케스트레이터가 순서를 강제해야 하는데, 이는 결합도를
  줄이지 못한 채 간접 호출 계층만 늘리는 결과가 된다. 반대로 지금 구조는 "주문 하나의 생명주기가
  어떻게 흘러가는가"라는 하나의 응집된 질문에 답하는 클래스로 볼 수 있어, 실질적 응집도
  (cohesion)가 높다. Phase 2~6에서 이미 이 경계를 기준으로 다수의 시나리오/적대적 테스트가
  작성되어 검증된 상태라, 마무리 단계에서 구조를 바꾸는 리스크가 순도(purity)로 얻는 이득보다
  크다고 판단했다.

### 3-2. Repository 인터페이스의 ISP — 읽기 전용 소비자도 쓰기 메서드까지 강제됨

`MonitoringController`는 `IOrderRepository`/`ISampleRepository`의 `GetAll`/`FindByStatus`
등 조회 메서드만 쓰고 `Add`/`Update`는 전혀 호출하지 않는다. 엄밀한 ISP 기준으로는
`IOrderReader`(조회 전용)와 `IOrderWriter`(쓰기 전용)로 인터페이스를 분리하고
`IOrderRepository : IOrderReader, IOrderWriter`로 합성하는 편이 더 좁고 깨끗한 인터페이스다.

- **결정: 분리하지 않고 그대로 둔다.**
- **이유**: 첫째, `ISampleRepository`/`IOrderRepository`는 CLAUDE.md
  "상위 설계 계승" 섹션에 DataPersistence PoC로부터 **변경 없이 그대로 재사용**하도록 명시된
  계약이다 — 인터페이스를 쪼개면 이 계승 계약과 `JsonSampleRepository`/`JsonOrderRepository`의
  "한 줄도 수정하지 않는다"는 원칙이 흔들린다. 둘째, 메서드가 5개뿐인 작은 인터페이스라 실질적
  고통(원치 않는 메서드를 억지로 구현/모킹해야 하는 부담)이 없다 — `MonitoringController`가
  `Add`/`Update`를 "안 쓰는 것"과 "억지로 구현해야 하는 것"은 다르며, 여기서는 전자에 해당한다.
  ISP는 인터페이스를 구현하는 쪽의 고통을 줄이기 위한 원칙인데, 이 프로젝트엔 그 고통이 없으므로
  분리의 실익보다 계승 계약을 지키는 안정성이 더 크다고 판단했다.

### 3-3. Model 계층의 `std::cerr` 로깅 — 계층 순수성 위반이지만 그대로 둠

`JsonSampleRepository.cpp`/`JsonOrderRepository.cpp`는 DataPersistence PoC에서 완성되어
"한 줄도 수정하지 않고 그대로 이식"하기로 CLAUDE.md에 명시된 파일이다. `JsonProductionLineRepository.cpp`는
이 저장소에서 신규 작성했지만, 손상/부재 파일에 대한 폴백 규약(write-through, 실패 시 빈
목록으로 시작 + 진단 로그)을 앞의 두 파일과 **의도적으로 동일하게** 맞췄다(CLAUDE.md
"1. 생산 라인의 JSON 영속화" 갭 항목 참고).

- **결정: 세 파일 모두 그대로 둔다.**
- **이유**: `JsonSampleRepository`/`JsonOrderRepository`는 계승 계약상 수정 자체가 금지되어
  있다. `JsonProductionLineRepository`만 로거 인터페이스로 바꾸는 것은 기술적으로는 가능하지만,
  그렇게 하면 세 Repository 구현체 간 폴백 로깅 방식이 서로 달라져 "동일한 규약을 따른다"는
  일관성이 오히려 깨진다. 또한 이 출력은 `std::cin`/`std::cout` 기반 사용자 대면 메뉴 흐름과
  무관한 진단용 `stderr` 로그이며 View가 이를 소비하지 않으므로, 계층 오염의 실질적 피해(View
  로직에 섞이거나 테스트 가능성을 해치는 것)는 없다. 순수한 계층 분리 원칙보다, 이미 검증된
  이식 코드의 안정성과 세 구현체 간 동작 일관성을 우선했다.

### 3-4. `Order::SetStatus` — 원시 setter (빈약한 도메인 모델) vs 자가 검증 도메인 모델

`Order`는 `SetStatus(OrderStatus)`라는 원시 setter만 노출하고, "이 전이가 유효한가"는 전부
`OrderController`가 판단한다(3-1에서 다룬 `FindOrderRequiringStatus` 등). 더 객체지향적인
설계라면 `Order` 스스로 `TransitionTo(OrderStatus)` 같은 메서드를 갖고 잘못된 전이 시 예외를
던지는 "Tell, Don't Ask" 스타일의 자가 검증 도메인 모델이 될 수 있다.

- **결정: 원시 setter를 유지한다.**
- **이유**: `Order`는 CLAUDE.md에 ConsoleMVC PoC로부터 계승된 도메인 모델로 명시되어 있고,
  이미 Phase 2~6에서 작성된 다수의 시나리오/적대적 테스트가 테스트 픽스처를 구성할 때
  `order.SetStatus(...)`를 직접 호출해 특정 상태를 즉시 만들어낸다(예: 이미 CONFIRMED인 주문을
  준비해두고 재승인을 시도하는 적대적 테스트). `Order`에 전이 검증 로직을 넣으면 이 테스트들의
  픽스처 구성 방식까지 함께 손봐야 해서 변경 범위가 커지고, 전이 규칙 자체는 이미
  `OrderController`에 명확하게 구현되어 15건 이상의 적대적 테스트로 잘 검증되어 있다. 순수
  객체지향 스타일로 한 단계 더 다가가는 이득보다, 마무리 단계에서 검증된 테스트 자산을 건드리지
  않는 안정성을 우선했다.

### 3-5. `main.cpp`의 메뉴 switch 3중 반복 — OCP 관점

메뉴 항목을 추가하려면 `View::MainMenuOption` enum, `MainMenuView::PromptMenuChoice`의
switch, `main.cpp`의 `RunApplication` switch까지 세 곳을 함께 고쳐야 한다. 커맨드 패턴 등으로
추상화하면 새 메뉴 추가 시 수정 지점을 줄일 수 있다.

- **결정: 그대로 둔다.**
- **이유**: 메뉴가 6개로 고정되어 있고 이 프로젝트 스코프에서 메뉴 항목이 늘어날 계획이
  없다. 콘솔 메뉴 앱에서 이 정도 반복은 흔한 관용구이며, 커맨드 패턴 도입은 간접 계층만
  늘리고 가독성을 해칠 뿐 실질적 확장성 이득이 없다고 판단했다.

## 4. 요약

| # | 항목 | 원칙 | 결정 | 핵심 근거 |
|---|---|---|---|---|
| 1 | OrderController 조회+검증 중복 | DRY | **리팩토링함** | 순수 behavior-preserving, 회귀 없음 |
| 2 | OrderController 다중 책임 | SRP | 유지 | 재고/생산 규칙이 원자적으로 얽혀 있어 분리 시 결합만 늘어남 |
| 3 | Repository 인터페이스 크기 | ISP | 유지 | 계승 계약(CLAUDE.md) + 실질적 구현 고통 없음 |
| 4 | Model의 stderr 로깅 | 계층 분리 | 유지 | 2개는 계승 계약상 수정 금지, 1개는 일관성 위해 동일하게 맞춤 |
| 5 | Order::SetStatus 원시 setter | 캡슐화(Tell,Don't Ask) | 유지 | 전이 규칙은 Controller에서 이미 검증됨, 테스트 픽스처 영향 회피 |
| 6 | main.cpp 메뉴 switch 반복 | OCP | 유지 | 메뉴 6개 고정, 추상화 실익 낮음 |
