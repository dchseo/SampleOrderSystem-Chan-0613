#pragma once
#include <vector>
#include "../Model/Dtos.h"
#include "../Model/Repository/IOrderRepository.h"
#include "../Model/Repository/ISampleRepository.h"

namespace Controller
{
    class MonitoringController
    {
    public:
        MonitoringController(Model::IOrderRepository& orderRepository, Model::ISampleRepository& sampleRepository);

        // 상태별(RESERVED/PRODUCING/CONFIRMED/RELEASED) 주문 수. REJECTED는 집계 제외.
        Model::OrderStatusSummary GetOrderStatusSummary() const;

        // 시료별 현재 재고 및 여유/부족/고갈 상태. 이 값은 매 조회 시 재계산하지 않고,
        // OrderController가 주문 승인/생산 완료 시점에 Sample에 캐시해 둔 값을 그대로
        // 반환한다 (CLAUDE.md "재고 및 생산 라인 처리 규칙 (상세) §2" 참고).
        std::vector<Model::InventoryStatusItem> GetInventoryStatus() const;

    private:
        Model::IOrderRepository& orderRepository_;
        Model::ISampleRepository& sampleRepository_;
    };
}
