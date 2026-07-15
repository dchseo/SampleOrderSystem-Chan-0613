#include "MonitoringView.h"
#include <iomanip>
#include <iostream>
#include "ConsoleFormat.h"

namespace View
{
    void MonitoringView::ShowMenu() const
    {
        std::cout << "\n--- [4] 모니터링 ---\n";
        std::cout << "[1] 주문량 확인   [2] 재고량 확인   [0] 뒤로\n";
        std::cout << "선택 > ";
    }

    void MonitoringView::ShowOrderStatusSummary(const Model::OrderStatusSummary& summary) const
    {
        std::cout << "\n상태별 주문 현황 (REJECTED 제외)\n";
        for (const auto& [status, count] : summary)
        {
            std::cout << std::left << std::setw(12) << Model::ToString(status) << count << "건\n";
        }
    }

    void MonitoringView::ShowInventoryStatus(const std::vector<Model::InventoryStatusItem>& items) const
    {
        std::cout << "\n시료별 재고 현황\n";
        std::cout << PadRight("시료명", 18)
            << PadRight("재고", 8)
            << "상태" << '\n';

        for (const auto& item : items)
        {
            std::string levelText;
            switch (item.level)
            {
            case Model::InventoryLevel::Sufficient: levelText = "여유"; break;
            case Model::InventoryLevel::Low: levelText = "부족"; break;
            case Model::InventoryLevel::Depleted: levelText = "고갈"; break;
            }
            std::cout << PadRight(item.sample.GetName(), 18)
                << PadRight(std::to_string(item.sample.GetStock()), 8)
                << levelText << '\n';
        }
    }
}
