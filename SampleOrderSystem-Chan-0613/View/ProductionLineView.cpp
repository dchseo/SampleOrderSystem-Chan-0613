#include "ProductionLineView.h"
#include <algorithm>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>
#include "ConsoleFormat.h"

namespace View
{
    namespace
    {
        std::string FormatClockTime(std::chrono::system_clock::time_point tp)
        {
            const std::time_t time = std::chrono::system_clock::to_time_t(tp);
            std::tm localTime{};
            localtime_s(&localTime, &time);
            std::ostringstream oss;
            oss << std::put_time(&localTime, "%H:%M");
            return oss.str();
        }
    }

    void ProductionLineView::ShowMenu() const
    {
        std::cout << "\n--- [5] 생산 라인 조회 (FIFO) ---\n";
        std::cout << "[1] 현재 생산 및 대기열 조회   [0] 뒤로\n";
        std::cout << "선택 > ";
    }

    void ProductionLineView::ShowCurrentProduction(const Model::ProductionJob& job, std::chrono::system_clock::time_point now) const
    {
        std::cout << "\n현재 처리 중\n";
        std::cout << "주문번호 " << job.orderId << "  시료 " << job.sampleId << '\n';
        std::cout << "부족분 " << job.shortageQuantity << " ea -> 실생산량 " << job.actualQuantity
            << " ea (총 생산시간 " << job.totalProductionTime << " min)\n";

        const double elapsedMinutes = std::chrono::duration_cast<std::chrono::duration<double, std::ratio<60>>>(
            now - job.startTime).count();
        double progressPercent = job.totalProductionTime > 0.0
            ? (elapsedMinutes / job.totalProductionTime) * 100.0
            : 100.0;
        progressPercent = std::clamp(progressPercent, 0.0, 100.0);

        std::cout << "진행 " << static_cast<int>(progressPercent) << "%   완료 예정 "
            << FormatClockTime(job.CompletionTime()) << '\n';
    }

    void ProductionLineView::ShowNoActiveProductionMessage() const
    {
        std::cout << "\n현재 생산 중인 항목이 없습니다.\n";
    }

    void ProductionLineView::ShowProductionQueue(const std::vector<Model::ProductionJob>& queue) const
    {
        std::cout << "\n대기 중인 주문 (FIFO 순)\n";
        if (queue.empty())
        {
            std::cout << "대기 중인 생산 작업이 없습니다.\n";
            return;
        }
        std::cout << PadRight("순서", 8)
            << PadRight("주문번호", 14)
            << PadRight("시료ID", 14)
            << PadRight("부족분", 10)
            << "실생산량" << '\n';

        int order = 1;
        for (const auto& job : queue)
        {
            std::cout << PadRight(std::to_string(order++), 8)
                << PadRight(job.orderId, 14)
                << PadRight(job.sampleId, 14)
                << PadRight(std::to_string(job.shortageQuantity), 10)
                << job.actualQuantity << '\n';
        }
    }
}
