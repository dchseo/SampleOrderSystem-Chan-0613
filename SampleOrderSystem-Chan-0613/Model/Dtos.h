#pragma once
#include <map>
#include <string>
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

    enum class InventoryLevel
    {
        Sufficient, // 여유
        Low,        // 부족
        Depleted    // 고갈
    };

    struct InventoryStatusItem
    {
        Sample sample;
        InventoryLevel level = InventoryLevel::Sufficient;
    };

    using OrderStatusSummary = std::map<OrderStatus, int>;
}
