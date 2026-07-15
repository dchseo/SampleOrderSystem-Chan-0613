#include "OrderView.h"
#include <iostream>
#include <limits>
#include "ConsoleFormat.h"

namespace View
{
    void OrderView::ShowReservationMenu() const
    {
        std::cout << "\n--- [2] 시료 주문 ---\n";
    }

    OrderReservationInput OrderView::PromptOrderReservation() const
    {
        OrderReservationInput input;
        std::cout << "시료 ID > ";
        std::getline(std::cin, input.sampleId);
        std::cout << "고객명 > ";
        std::getline(std::cin, input.customerName);
        std::cout << "주문 수량 > ";
        std::cin >> input.quantity;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        return input;
    }

    void OrderView::ShowReservationResult(bool success, const std::string& orderId) const
    {
        if (success)
        {
            std::cout << "예약 접수 완료. 주문번호: " << orderId << " (상태: RESERVED)\n";
        }
        else
        {
            std::cout << "예약 접수 실패: 존재하지 않는 시료 ID이거나 수량이 올바르지 않습니다.\n";
        }
    }

    void OrderView::ShowApprovalMenu() const
    {
        std::cout << "\n[1] 승인   [2] 거절   [0] 뒤로\n";
        std::cout << "선택 > ";
    }

    void OrderView::ShowReservedOrders(const std::vector<Model::Order>& orders) const
    {
        std::cout << "\n--- [3] 주문 승인/거절 : 승인 대기 중인 예약 목록 (RESERVED) ---\n";
        if (orders.empty())
        {
            std::cout << "대기 중인 주문이 없습니다.\n";
            return;
        }
        std::cout << PadRight("주문번호", 14)
            << PadRight("시료ID", 14)
            << PadRight("고객명", 16)
            << "수량" << '\n';
        for (const auto& order : orders)
        {
            std::cout << PadRight(order.GetOrderId(), 14)
                << PadRight(order.GetSampleId(), 14)
                << PadRight(order.GetCustomerName(), 16)
                << order.GetQuantity() << '\n';
        }
    }

    std::string OrderView::PromptOrderId(const std::string& label) const
    {
        std::cout << label << " > ";
        std::string orderId;
        std::getline(std::cin, orderId);
        return orderId;
    }

    void OrderView::ShowApprovalResult(const std::string& orderId, const Model::OrderApprovalResult& result) const
    {
        if (!result.success)
        {
            std::cout << "승인 실패 (" << orderId << "): " << result.message << '\n';
            return;
        }

        std::cout << result.message << '\n';
        std::cout << "주문번호 " << orderId << " 상태 변경 -> " << Model::ToString(result.resultingStatus) << '\n';
        if (result.resultingStatus == Model::OrderStatus::Producing)
        {
            std::cout << "부족분: " << result.shortageQuantity << " ea, 실생산량: " << result.actualProductionQuantity
                << " ea, 총 생산시간: " << result.totalProductionTime << " min\n";
        }
    }

    void OrderView::ShowRejectionResult(bool success, const std::string& orderId) const
    {
        if (success)
        {
            std::cout << "주문 거절 완료. 주문번호 " << orderId << " 상태 변경 -> REJECTED\n";
        }
        else
        {
            std::cout << "거절 실패: 승인 대기(RESERVED) 상태의 주문이 아닙니다. (" << orderId << ")\n";
        }
    }

    void OrderView::ShowReleasableOrders(const std::vector<Model::Order>& orders) const
    {
        std::cout << "\n--- [6] 출고 처리 : 출고 가능 주문 (CONFIRMED) ---\n";
        if (orders.empty())
        {
            std::cout << "출고 가능한 주문이 없습니다.\n";
            return;
        }
        std::cout << PadRight("주문번호", 14)
            << PadRight("시료ID", 14)
            << PadRight("고객명", 16)
            << "수량" << '\n';
        for (const auto& order : orders)
        {
            std::cout << PadRight(order.GetOrderId(), 14)
                << PadRight(order.GetSampleId(), 14)
                << PadRight(order.GetCustomerName(), 16)
                << order.GetQuantity() << '\n';
        }
    }

    void OrderView::ShowReleaseResult(bool success, const std::string& orderId) const
    {
        if (success)
        {
            std::cout << "출고 처리 완료. 주문번호 " << orderId << " 상태 변경 -> RELEASED\n";
        }
        else
        {
            std::cout << "출고 실패: 출고 대기(CONFIRMED) 상태의 주문이 아닙니다. (" << orderId << ")\n";
        }
    }
}
