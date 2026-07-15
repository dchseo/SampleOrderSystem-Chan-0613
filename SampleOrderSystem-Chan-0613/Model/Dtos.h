#pragma once
#include <map>
#include <string>
#include "InventoryLevel.h"
#include "Order.h"
#include "Sample.h"

// Controller와 View가 공통으로 주고받는 결과/조회용 값 타입.
// Model 네임스페이스에 두어 View가 Controller 헤더를 include하지 않도록 한다.
namespace Model
{
    struct OrderApprovalResult
    {
        bool success = false;
        OrderStatus resultingStatus = OrderStatus::Rejected;
        int shortageQuantity = 0;
        int actualProductionQuantity = 0;
        double totalProductionTime = 0.0;
        std::string message;
    };

    struct InventoryStatusItem
    {
        Sample sample;
        InventoryLevel level = InventoryLevel::Sufficient;
    };

    using OrderStatusSummary = std::map<OrderStatus, int>;

    // 메인 메뉴 상단에 표시하는 전체 시료 요약 정보 (PDF 예시 UI 참고). MonitoringController가
    // 계산해 View에 그대로 전달한다 — View는 렌더링만 담당한다.
    struct MainMenuSummary
    {
        int registeredSampleCount = 0;
        int totalStock = 0;
        int totalOrderCount = 0;
        int productionQueueCount = 0; // 현재 생산 중 + 대기 중인 작업 수 합계
    };
}
