#pragma once
#include <vector>
#include "../Model/Dtos.h"

namespace View
{
    class MonitoringView
    {
    public:
        void ShowMenu() const;
        void ShowOrderStatusSummary(const Model::OrderStatusSummary& summary) const;
        void ShowInventoryStatus(const std::vector<Model::InventoryStatusItem>& items) const;
    };
}
