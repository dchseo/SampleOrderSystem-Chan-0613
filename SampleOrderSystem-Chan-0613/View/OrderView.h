#pragma once
#include <string>
#include <vector>
#include "../Model/Dtos.h"
#include "../Model/Order.h"

namespace View
{
    struct OrderReservationInput
    {
        std::string sampleId;
        std::string customerName;
        int quantity = 0;
    };

    class OrderView
    {
    public:
        // 2. 시료 주문
        void ShowReservationMenu() const;
        OrderReservationInput PromptOrderReservation() const;
        void ShowReservationResult(bool success, const std::string& orderId) const;

        // 3. 주문 승인/거절
        void ShowApprovalMenu() const;
        void ShowReservedOrders(const std::vector<Model::Order>& orders) const;
        std::string PromptOrderId(const std::string& label) const;
        void ShowApprovalResult(const std::string& orderId, const Model::OrderApprovalResult& result) const;
        void ShowRejectionResult(bool success, const std::string& orderId) const;

        // 6. 출고 처리
        void ShowReleasableOrders(const std::vector<Model::Order>& orders) const;
        void ShowReleaseResult(bool success, const std::string& orderId) const;
    };
}
