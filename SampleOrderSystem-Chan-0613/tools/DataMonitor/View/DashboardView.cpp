#include "DashboardView.h"
#include "../Model/Order.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace View
{
    namespace
    {
        constexpr std::array<Model::OrderStatus, 4> kDisplayOrder{
            Model::OrderStatus::Reserved,
            Model::OrderStatus::Producing,
            Model::OrderStatus::Confirmed,
            Model::OrderStatus::Released,
        };

        std::string CurrentTimestamp()
        {
            const auto now = std::chrono::system_clock::now();
            const std::time_t nowTime = std::chrono::system_clock::to_time_t(now);
            std::tm localTime{};
            localtime_s(&localTime, &nowTime);

            std::ostringstream oss;
            oss << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S");
            return oss.str();
        }

        std::string FormatClockTime(std::chrono::system_clock::time_point tp)
        {
            const std::time_t time = std::chrono::system_clock::to_time_t(tp);
            std::tm localTime{};
            localtime_s(&localTime, &time);
            std::ostringstream oss;
            oss << std::put_time(&localTime, "%H:%M");
            return oss.str();
        }

        std::string LevelLabel(Model::InventoryLevel level)
        {
            switch (level)
            {
            case Model::InventoryLevel::Sufficient: return "여유";
            case Model::InventoryLevel::Low: return "부족";
            case Model::InventoryLevel::Depleted: return "고갈";
            }
            return "?";
        }

        // std::setw는 바이트 수 기준이라 한글(콘솔 표시폭 2칸)이 섞이면 열이 어긋난다.
        // UTF-8 선행 바이트만 세어 표시폭을 근사 계산한 뒤 직접 공백을 채운다.
        int DisplayWidth(const std::string& text)
        {
            int width = 0;
            for (size_t i = 0; i < text.size(); )
            {
                const unsigned char c = static_cast<unsigned char>(text[i]);
                if (c < 0x80) { width += 1; i += 1; }
                else if ((c & 0xE0) == 0xC0) { width += 2; i += 2; }
                else if ((c & 0xF0) == 0xE0) { width += 2; i += 3; }
                else if ((c & 0xF8) == 0xF0) { width += 2; i += 4; }
                else { width += 1; i += 1; }
            }
            return width;
        }

        std::string PadRight(const std::string& text, int targetWidth)
        {
            const int width = DisplayWidth(text);
            if (width >= targetWidth)
            {
                return text + " ";
            }
            return text + std::string(targetWidth - width, ' ');
        }
    }

    DashboardView::DashboardView(std::string samplesFilePath, std::string ordersFilePath,
        std::string productionQueueFilePath, int refreshIntervalSeconds)
        : samplesFilePath_(std::move(samplesFilePath))
        , ordersFilePath_(std::move(ordersFilePath))
        , productionQueueFilePath_(std::move(productionQueueFilePath))
        , refreshIntervalSeconds_(refreshIntervalSeconds)
    {
    }

    void DashboardView::Render(const DashboardSnapshot& snapshot, long long refreshCount) const
    {
        // ANSI 이스케이프로 화면을 지우고 커서를 맨 위로 옮긴다 (main에서 VT 모드를 활성화해 둔다).
        std::cout << "\x1B[2J\x1B[H";

        std::cout << "==========================================================\n";
        std::cout << " DataMonitor  반도체 시료 생산주문관리 모니터링\n";
        std::cout << "==========================================================\n";
        std::cout << " 갱신 시각   : " << CurrentTimestamp() << " (제" << refreshCount << "회 새로고침, "
                   << refreshIntervalSeconds_ << "초 주기)\n";
        std::cout << " 데이터 소스 : " << samplesFilePath_ << ", " << ordersFilePath_ << ",\n"
                   << "               " << productionQueueFilePath_ << "\n";
        std::cout << "----------------------------------------------------------\n";

        std::cout << " [주문 현황] REJECTED 제외\n";
        for (const auto status : kDisplayOrder)
        {
            const auto it = snapshot.orderStatusSummary.find(status);
            const int count = (it != snapshot.orderStatusSummary.end()) ? it->second : 0;
            std::cout << "   " << PadRight(Model::ToString(status), 12) << count << "건\n";
        }
        std::cout << "----------------------------------------------------------\n";

        std::cout << " [재고 현황]\n";
        if (snapshot.inventoryStatus.empty())
        {
            std::cout << "   등록된 시료가 없습니다.\n";
        }
        else
        {
            std::cout << "   " << PadRight("ID", 10) << PadRight("시료명", 20)
                       << PadRight("재고", 10) << "상태\n";
            for (const auto& item : snapshot.inventoryStatus)
            {
                std::cout << "   " << PadRight(item.sample.GetSampleId(), 10)
                           << PadRight(item.sample.GetName(), 20)
                           << PadRight(std::to_string(item.sample.GetStock()) + " ea", 10)
                           << LevelLabel(item.level) << "\n";
            }
        }
        std::cout << "----------------------------------------------------------\n";

        std::cout << " [생산 라인] (읽기 전용 — 표시 시각 기준 진행률/완료 예정은 참고용)\n";
        if (!snapshot.currentJob.has_value())
        {
            std::cout << "   현재 생산 중인 항목이 없습니다.\n";
        }
        else
        {
            const auto& job = *snapshot.currentJob;
            const auto now = std::chrono::system_clock::now();
            const double elapsedMinutes = std::chrono::duration_cast<std::chrono::duration<double, std::ratio<60>>>(
                now - job.startTime).count();
            double progressPercent = job.totalProductionTime > 0.0
                ? (elapsedMinutes / job.totalProductionTime) * 100.0
                : 100.0;
            progressPercent = std::min(100.0, std::max(0.0, progressPercent));

            std::cout << "   현재 처리 중: 주문번호 " << job.orderId << " 시료 " << job.sampleId
                       << " (부족분 " << job.shortageQuantity << " -> 실생산량 " << job.actualQuantity << ")\n";
            std::cout << "   진행 " << static_cast<int>(progressPercent) << "%   완료 예정 "
                       << FormatClockTime(job.CompletionTime()) << "\n";
        }
        std::cout << "   대기 중: " << snapshot.waitingJobs.size() << "건";
        if (!snapshot.waitingJobs.empty())
        {
            std::cout << " (";
            for (size_t i = 0; i < snapshot.waitingJobs.size(); ++i)
            {
                std::cout << snapshot.waitingJobs[i].orderId;
                if (i + 1 < snapshot.waitingJobs.size())
                {
                    std::cout << ", ";
                }
            }
            std::cout << ")";
        }
        std::cout << "\n";
        std::cout << "----------------------------------------------------------\n";
        std::cout << " 종료하려면 Q 를 누르세요.\n";
    }

    void DashboardView::ShowExitMessage() const
    {
        std::cout << "\nDataMonitor를 종료합니다.\n";
    }
}
