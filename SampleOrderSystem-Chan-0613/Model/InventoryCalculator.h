#pragma once
#include "Dtos.h"

// 재고/생산량 계산 규칙 (CLAUDE.md "재고 및 생산 라인 처리 규칙 (상세)" 참고).
// Controller가 아니라 Model에 두는 이유: 콘솔 I/O나 Repository 상태에 의존하지 않는
// 순수 계산 규칙이며, OrderController(Phase 2)가 승인/생산 완료 두 시점에서 그대로 재사용한다.
namespace Model
{
    // 부족분 = 주문 수량 - 현재 재고 (재고가 더 많으면 0).
    int CalculateShortage(int orderQuantity, int currentStock);

    // 실 생산량 = ceil(부족분 / 수율). 부족분이 0 이하면 0을 반환한다.
    // 수율은 반드시 0보다 커야 한다 (0이면 std::invalid_argument).
    int CalculateActualProductionQuantity(int shortageQuantity, double yieldRate);

    // 총 생산 시간 = 평균 생산시간 * 실 생산량 (분).
    double CalculateTotalProductionTime(double avgProductionTime, int actualProductionQuantity);

    // 재고 상태 분류: 재고 >= 기준 수량 → 여유, 0 < 재고 < 기준 수량 → 부족, 재고 <= 0 → 고갈.
    // "기준 수량"은 호출 시점에 따라 다르다 — 승인 시점은 그 주문의 수량, 생산 완료 시점은
    // 순가용재고 비교 대상 수량이다 (CLAUDE.md §2 참고).
    InventoryLevel ClassifyInventoryLevel(int stock, int referenceQuantity);
}
