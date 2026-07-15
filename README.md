# SampleOrderSystem-Chan-0613

반도체 시료 생산주문관리 시스템 — 최종 프로젝트

## 과제 목적

가상의 반도체 회사 "S-Semi"는 다양한 종류의 반도체 시료(Sample)를 생산하여 연구소, 팹리스 업체,
대학 연구실 등에 납품한다. 주문량이 급증하면서 엑셀/메모장 기반 관리로는 주문 처리 여부, 공정
완료 시점, 재고 대비 불필요한 추가 공정 여부를 파악하기 어려워졌다. 이를 해소하기 위해 시료 등록
→ 주문 접수 → 승인/거절 → (재고 부족 시) 생산 → 출고에 이르는 전체 흐름을 관리하는 **콘솔 기반
반도체 시료 생산주문관리 시스템**을 구현한다.

## 시스템 개요

- **콘솔 기반 애플리케이션**: 담당자가 메뉴 번호를 직접 입력해 시료 관리, 주문 접수, 주문 승인/거절,
  모니터링, 생산 라인 조회, 출고 처리를 수행한다.
- **주문 상태 흐름**: `RESERVED`(접수) → 승인 시 재고 비교 → `CONFIRMED`(재고 충분, 출고 대기) 또는
  `PRODUCING`(재고 부족, 생산 중) → 생산 완료 시 `CONFIRMED` → 출고 시 `RELEASED`. 거절 시 즉시
  `REJECTED`(모니터링 집계 제외).
- **생산 라인**: 시료를 하나씩 생산하는 단일 라인, 부족분에 대해서만 **FIFO(선입선출)** 로 생산.
  실 생산량 = `ceil(부족분 / 수율)`, 총 생산 시간 = 평균 생산시간 × 실 생산량.
- **재고 상태**: 여유 / 부족 / 고갈(0) 3단계로 분류하여 모니터링에 표시.
- **아키텍처**: C++20 기반 콘솔 MVC (Model / View / Controller 계층 분리), Repository 인터페이스를
  통한 데이터 접근.
- **데이터 영속성**: 자체 구현 JSON 라이브러리(`JsonLib`)를 이용해 `data/*.json` 파일로 저장하며,
  애플리케이션을 재시작해도 데이터가 유지된다.
- **보조 도구**: 저장된 데이터를 콘솔에서 실시간으로 확인하는 모니터링 도구, 테스트용 더미 데이터를
  생성해 실제 데이터 파일에 주입하는 도구를 함께 제공한다.

자세한 기능 명세, 설계 결정, 단계별 구현 계획은 [CLAUDE.md](CLAUDE.md), [docs/PLAN.md](docs/PLAN.md)를
참고한다.

## 사전 검토한 PoC 저장소

본 프로젝트는 아래 4개 PoC(Proof of Concept) 저장소에서 검증한 설계와 코드를 그대로 계승·통합한다.

| PoC | 저장소 | 역할 |
|---|---|---|
| MVC 스켈레톤 코드 | [ConsoleMVC-Chan-0613](../ConsoleMVC-Chan-0613) | Model/View/Controller 계층 구조, Repository 인터페이스, 상태 전이·계산 규칙 확정 |
| 데이터 영속성 처리 | [DataPersistence-Chan-0613](../DataPersistence-Chan-0613) | JSON 파일 기반 Repository 구현체(`JsonSampleRepository`/`JsonOrderRepository`), 자체 JSON 라이브러리(`JsonLib`) |
| 데이터 모니터링 Tool | [DataMonitor-Chan-0613](../DataMonitor-Chan-0613) | 저장된 데이터를 읽기 전용으로 폴링하여 콘솔에 실시간 표시하는 대시보드 |
| Dummy 데이터 생성 Tool | [DummyDataGenerator-Chan-0613](../DummyDataGenerator-Chan-0613) | 테스트용 시료/주문 더미 데이터를 생성해 Repository에 실제로 주입하는 도구 |

## Commit 컨벤션

커밋 메시지는 `[prefix] 내용` 형식을 따르며, 작업 단위로 의미 있게 나누어 커밋한다.

| Prefix | 용도 |
|---|---|
| `[feat]` | 기능 추가, 변경 및 업데이트 |
| `[fix]` | 버그 수정 |
| `[refactor]` | 동작 변화 없는 코드 구조 개선(리팩토링) |
| `[docs]` | `CLAUDE.md`, `README.md`, `docs/PLAN.md` 등 문서 추가/수정 |
| `[test]` | 테스트 코드 추가/수정 (테스트 대상 코드 자체는 변경하지 않음) |
| `[chore]` | 빌드 설정, 프로젝트 구조, `.gitignore` 등 기타 잡무성 변경 |
| `[style]` | 포맷팅, 세미콜론/공백 등 동작에 영향 없는 코드 스타일 수정 |
| `[perf]` | 동작은 동일하되 성능을 개선하는 변경 |

### 커밋 메시지 템플릿

`git commit` (메시지 없이) 실행 시 아래 템플릿이 편집기에 자동으로 채워지도록 설정되어 있다
(설정 방법은 아래 "템플릿/훅 적용" 참고). 주석(`#`)으로 시작하는 줄은 커밋에 반영되지 않는다.

```
[prefix] 한 줄 요약 (무엇을, 어디에)

# 왜 이 변경이 필요했는지 (배경/문제)
#
# 무엇을 했는지 (핵심 변경 내용, 여러 개면 목록으로)
#
# 어디서 가져왔는지 / 무엇이 신규인지 (이식 vs 신규 구현 구분)
#
# 영향 범위 (다른 계층/PoC와의 통합 지점이 깨질 수 있는 변경이면 명시)
```

예시:

```
[feat] JsonProductionLineRepository 추가

ConsoleMVC/DataPersistence PoC에서 ProductionLine이 In-Memory로만 존재해
재시작 시 생산 큐가 유실되는 문제가 있었음.

- IProductionLineRepository 인터페이스와 JSON 기반 구현체 신규 작성
  (DataPersistence의 write-through 저장 규약을 그대로 계승)
- data/production_queue.json 파일 포맷 신규 도입
- OrderController 생성자 시그니처에 영향 없음 (기존 통합 지점 유지)
```

### 템플릿/훅 적용 방법

제목 줄의 prefix 형식은 커밋마다 사람이 기억해서 지키기보다, 저장소에 함께 커밋된
`scripts/hooks/commit-msg` 훅으로 강제한다. 클론 후 한 번만 아래 명령을 실행하면 이후 모든 커밋에
자동 적용된다.

```bash
git config commit.template .gitmessage   # git commit 시 위 템플릿 자동 표시
git config core.hooksPath scripts/hooks  # 커밋 메시지 형식 검증 훅 활성화
```

`core.hooksPath`는 로컬 저장소 설정이라 클론할 때마다(팀원 각자) 최초 1회 실행해야 하며, 실행하지
않으면 템플릿/검증 없이도 커밋은 가능하다. 훅이 활성화된 상태에서 첫 줄이
`[feat]`/`[fix]`/`[refactor]`/`[docs]`/`[test]`/`[chore]`/`[style]`/`[perf]` 중 하나로 시작하지
않으면 커밋이 거부된다. 다만 본문(왜/무엇을/영향 범위)의 실제 내용까지는 자동으로 검증하지
못하므로, 그 부분은 리뷰 시 사람이 확인한다.
