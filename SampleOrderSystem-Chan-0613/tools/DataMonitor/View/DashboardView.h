#pragma once
#include <chrono>
#include <optional>
#include <string>
#include <vector>
#include "../Model/Dtos.h"
#include "../Model/ProductionLine.h"

// 콘솔 화면 출력만 담당한다 (비즈니스 로직 없음). ConsoleMVC-Chan-0613의 View 계층 원칙을 따른다.
namespace View
{
    struct DashboardSnapshot
    {
        Model::OrderStatusSummary orderStatusSummary;
        std::vector<Model::InventoryStatusItem> inventoryStatus;

        // 생산 라인 현황(DataMonitor-Chan-0613 PoC 이식 당시에는 ProductionLine이 아직
        // 영속화되지 않아 제외됐던 부분 — 본 시스템에서는 production_queue.json이 생겨
        // 이번에 새로 포함한다). 큐 맨 앞이 currentJob이며, 없으면 std::nullopt.
        std::optional<Model::ProductionJob> currentJob;
        std::vector<Model::ProductionJob> waitingJobs;
    };

    class DashboardView
    {
    public:
        DashboardView(std::string samplesFilePath, std::string ordersFilePath,
            std::string productionQueueFilePath, int refreshIntervalSeconds);

        // 매 새로고침 주기마다 화면을 지우고 스냅샷을 다시 그린다.
        void Render(const DashboardSnapshot& snapshot, long long refreshCount) const;
        void ShowExitMessage() const;

    private:
        std::string samplesFilePath_;
        std::string ordersFilePath_;
        std::string productionQueueFilePath_;
        int refreshIntervalSeconds_;
    };
}
