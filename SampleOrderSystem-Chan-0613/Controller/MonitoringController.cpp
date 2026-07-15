#include "MonitoringController.h"

namespace Controller
{
    MonitoringController::MonitoringController(Model::IOrderRepository& orderRepository, Model::ISampleRepository& sampleRepository)
        : orderRepository_(orderRepository)
        , sampleRepository_(sampleRepository)
    {
    }

    Model::OrderStatusSummary MonitoringController::GetOrderStatusSummary() const
    {
        Model::OrderStatusSummary summary{
            { Model::OrderStatus::Reserved, 0 },
            { Model::OrderStatus::Producing, 0 },
            { Model::OrderStatus::Confirmed, 0 },
            { Model::OrderStatus::Released, 0 },
        };

        for (const auto& order : orderRepository_.GetAll())
        {
            if (order.GetStatus() == Model::OrderStatus::Rejected)
            {
                continue;
            }
            ++summary[order.GetStatus()];
        }
        return summary;
    }

    std::vector<Model::InventoryStatusItem> MonitoringController::GetInventoryStatus() const
    {
        std::vector<Model::InventoryStatusItem> items;
        for (const auto& sample : sampleRepository_.GetAll())
        {
            items.push_back({ sample, sample.GetInventoryLevel() });
        }
        return items;
    }

    Model::MainMenuSummary MonitoringController::GetMainMenuSummary(const Model::ProductionLine& productionLine) const
    {
        Model::MainMenuSummary summary;

        const auto samples = sampleRepository_.GetAll();
        summary.registeredSampleCount = static_cast<int>(samples.size());
        for (const auto& sample : samples)
        {
            summary.totalStock += sample.GetStock();
        }

        summary.totalOrderCount = static_cast<int>(orderRepository_.GetAll().size());
        summary.productionQueueCount = static_cast<int>(productionLine.GetAllJobsInOrder().size());

        return summary;
    }
}
