// DataMonitor — 데이터 모니터링 콘솔 도구 (DataMonitor-Chan-0613 PoC 이식).
//
// SampleOrderSystem 본 애플리케이션이 쓰는 ../../data/samples.json, orders.json,
// production_queue.json을 JsonSampleRepository/JsonOrderRepository/
// JsonProductionLineRepository(변경 없이 소비)로 읽어 MonitoringController로 집계한 뒤,
// 콘솔 화면에 일정 주기로 다시 그린다.
//
// 이 도구는 읽기 전용이며 Repository에 아무것도 쓰지 않는다. "실시간성"은 매 주기마다
// Repository를 새로 생성해 파일을 다시 읽는 방식으로 얻는다 — 본 애플리케이션이나
// DummyDataGenerator가 같은 JSON 파일을 갱신하면 다음 새로고침에 반영된다.
//
// DataMonitor-Chan-0613 PoC 대비 변경 사항:
//   - 생산 라인(ProductionLine) 현황을 새로 포함한다 — 원본 PoC 시점에는 아직
//     production_queue.json이 없어 제외했으나, 본 시스템에서는 영속화되어 있다.
//   - "잔여율"(%) 게이지 바는 표시하지 않는다 — 본 시스템의 메인 애플리케이션
//     모니터링 화면과 동일한 스코프 결정(CLAUDE.md 참고)을 그대로 따른다.

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <conio.h>

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <thread>

#include "Controller/MonitoringController.h"
#include "Model/Repository/JsonOrderRepository.h"
#include "Model/Repository/JsonProductionLineRepository.h"
#include "Model/Repository/JsonSampleRepository.h"
#include "View/DashboardView.h"

namespace
{
    constexpr const char* kSamplesFilePath = "../../data/samples.json";
    constexpr const char* kOrdersFilePath = "../../data/orders.json";
    constexpr const char* kProductionQueueFilePath = "../../data/production_queue.json";
    constexpr int kDefaultRefreshIntervalSeconds = 2;

    void EnableAnsiEscapes()
    {
        const HANDLE outHandle = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD mode = 0;
        if (GetConsoleMode(outHandle, &mode))
        {
            SetConsoleMode(outHandle, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
        }
    }

    // Q/Esc가 눌렸는지 논블로킹으로 확인한다. 눌린 키가 없으면 즉시 false를 반환한다.
    bool QuitKeyPressed()
    {
        if (!_kbhit())
        {
            return false;
        }
        const int ch = _getch();
        return ch == 'q' || ch == 'Q' || ch == 27; // 27 = ESC
    }

    View::DashboardSnapshot TakeSnapshot()
    {
        Model::JsonSampleRepository sampleRepository(kSamplesFilePath);
        Model::JsonOrderRepository orderRepository(kOrdersFilePath);
        Model::JsonProductionLineRepository productionLineRepository(kProductionQueueFilePath);
        Controller::MonitoringController monitoringController(orderRepository, sampleRepository);

        View::DashboardSnapshot snapshot;
        snapshot.orderStatusSummary = monitoringController.GetOrderStatusSummary();
        snapshot.inventoryStatus = monitoringController.GetInventoryStatus();

        const auto jobs = productionLineRepository.LoadQueue();
        if (!jobs.empty())
        {
            snapshot.currentJob = jobs.front();
            snapshot.waitingJobs.assign(jobs.begin() + 1, jobs.end());
        }

        return snapshot;
    }
}

int main(int argc, char* argv[])
{
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    EnableAnsiEscapes();

    // 인자: [새로고침 주기(초)] [최대 반복 횟수 (0=Q/Esc를 누를 때까지 무한 반복, 자동화 테스트용)]
    int refreshIntervalSeconds = kDefaultRefreshIntervalSeconds;
    long long maxIterations = 0;
    if (argc > 1)
    {
        refreshIntervalSeconds = std::max(1, std::atoi(argv[1]));
    }
    if (argc > 2)
    {
        maxIterations = std::atoll(argv[2]);
    }

    View::DashboardView dashboardView(kSamplesFilePath, kOrdersFilePath, kProductionQueueFilePath, refreshIntervalSeconds);

    long long refreshCount = 0;
    bool running = true;
    while (running)
    {
        ++refreshCount;
        dashboardView.Render(TakeSnapshot(), refreshCount);

        if (maxIterations > 0 && refreshCount >= maxIterations)
        {
            break;
        }

        const auto waitUntil = std::chrono::steady_clock::now() + std::chrono::seconds(refreshIntervalSeconds);
        while (std::chrono::steady_clock::now() < waitUntil)
        {
            if (QuitKeyPressed())
            {
                running = false;
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    dashboardView.ShowExitMessage();
    return 0;
}
