#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <chrono>
#include <filesystem>
#include <iostream>
#include <limits>
#include <string>

#include "Model/ProductionLine.h"
#include "Model/Repository/JsonOrderRepository.h"
#include "Model/Repository/JsonProductionLineRepository.h"
#include "Model/Repository/JsonSampleRepository.h"

#include "Controller/MonitoringController.h"
#include "Controller/OrderController.h"
#include "Controller/SampleController.h"

#include "View/MainMenuView.h"
#include "View/MonitoringView.h"
#include "View/OrderView.h"
#include "View/ProductionLineView.h"
#include "View/SampleView.h"

#include "tests/TestFramework.h"

namespace
{
    // 실행 파일의 작업 디렉터리를 기준으로 한 상대 경로 — Visual Studio 디버거는 기본적으로
    // 프로젝트 폴더를 작업 디렉터리로 사용하므로, 이 폴더의 data/ 하위에 그대로 생성/갱신된다.
    constexpr const char* kDataDirectory = "data";
    constexpr const char* kSamplesFilePath = "data/samples.json";
    constexpr const char* kOrdersFilePath = "data/orders.json";
    constexpr const char* kProductionQueueFilePath = "data/production_queue.json";

    int ReadChoice()
    {
        int choice = 0;
        if (!(std::cin >> choice))
        {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            return -1;
        }
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        return choice;
    }

    void RunSampleManagement(Controller::SampleController& controller, View::SampleView& view)
    {
        bool back = false;
        while (!back)
        {
            view.ShowMenu();
            switch (ReadChoice())
            {
            case 1:
            {
                const auto input = view.PromptSampleRegistration();
                const bool success = controller.RegisterSample(input.sampleId, input.name, input.avgProductionTime, input.yieldRate);
                view.ShowRegistrationResult(success, input.sampleId);
                break;
            }
            case 2:
                view.ShowSampleList(controller.ListSamples());
                break;
            case 3:
            {
                const auto keyword = view.PromptSearchKeyword();
                view.ShowSampleList(controller.SearchSample(keyword));
                break;
            }
            case 0:
                back = true;
                break;
            default:
                break;
            }
        }
    }

    void RunOrderReservation(Controller::OrderController& controller, View::OrderView& view)
    {
        view.ShowReservationMenu();
        const auto input = view.PromptOrderReservation();
        const auto orderId = controller.ReserveOrder(input.sampleId, input.customerName, input.quantity);
        view.ShowReservationResult(orderId.has_value(), orderId.value_or(""));
    }

    void RunOrderApproval(Controller::OrderController& controller, View::OrderView& view)
    {
        bool back = false;
        while (!back)
        {
            view.ShowReservedOrders(controller.ListReservedOrders());
            view.ShowApprovalMenu();

            switch (ReadChoice())
            {
            case 1:
            {
                const auto orderId = view.PromptOrderId("승인할 주문번호");
                const auto result = controller.ApproveOrder(orderId);
                view.ShowApprovalResult(orderId, result);
                break;
            }
            case 2:
            {
                const auto orderId = view.PromptOrderId("거절할 주문번호");
                const bool success = controller.RejectOrder(orderId);
                view.ShowRejectionResult(success, orderId);
                break;
            }
            case 0:
                back = true;
                break;
            default:
                back = true;
                break;
            }
        }
    }

    void RunMonitoring(Controller::MonitoringController& controller, View::MonitoringView& view)
    {
        bool back = false;
        while (!back)
        {
            view.ShowMenu();
            switch (ReadChoice())
            {
            case 1:
                view.ShowOrderStatusSummary(controller.GetOrderStatusSummary());
                break;
            case 2:
                view.ShowInventoryStatus(controller.GetInventoryStatus());
                break;
            case 0:
                back = true;
                break;
            default:
                back = true;
                break;
            }
        }
    }

    void RunProductionLine(Controller::OrderController& controller, View::ProductionLineView& view)
    {
        // 생산 라인 조회 화면을 열 때마다 실시간 정산을 먼저 반영한다
        // (CLAUDE.md "재고 및 생산 라인 처리 규칙 (상세) §3" 트리거 시점 중 하나).
        controller.SettleProductionQueue(std::chrono::system_clock::now());

        bool back = false;
        while (!back)
        {
            view.ShowMenu();
            switch (ReadChoice())
            {
            case 1:
                if (controller.HasCurrentProduction())
                {
                    view.ShowCurrentProduction(controller.GetCurrentProduction(), std::chrono::system_clock::now());
                }
                else
                {
                    view.ShowNoActiveProductionMessage();
                }
                view.ShowProductionQueue(controller.GetProductionQueue());
                break;
            case 0:
                back = true;
                break;
            default:
                back = true;
                break;
            }
        }
    }

    void RunReleaseProcessing(Controller::OrderController& controller, View::OrderView& view)
    {
        const auto releasable = controller.ListReleasableOrders();
        view.ShowReleasableOrders(releasable);
        if (releasable.empty())
        {
            return;
        }

        const auto orderId = view.PromptOrderId("출고 처리할 주문번호");
        const bool success = controller.ReleaseOrder(orderId);
        view.ShowReleaseResult(success, orderId);
    }

    int RunApplication()
    {
        // Repository들이 data/*.json에 write-through로 저장을 시도하기 전에 폴더 자체가
        // 반드시 존재해야 한다 — JsonDocument::saveToFile은 상위 폴더가 없으면 실패를
        // false로만 반환하고 예외를 던지지 않으므로, 이 폴더가 없는 채로 실행하면(예: 빌드
        // 출력 폴더처럼 data/.gitkeep이 딸려오지 않는 위치) 등록/승인이 화면에는 성공한
        // 것처럼 보이지만 실제로는 아무것도 저장되지 않고, 재시작 시 초기화된 것처럼
        // 보이는 문제가 생긴다.
        std::filesystem::create_directories(kDataDirectory);

        Model::JsonSampleRepository sampleRepository(kSamplesFilePath);
        Model::JsonOrderRepository orderRepository(kOrdersFilePath);
        Model::JsonProductionLineRepository productionLineRepository(kProductionQueueFilePath);

        Model::ProductionLine productionLine;
        productionLine.ReplaceAll(productionLineRepository.LoadQueue());

        Controller::SampleController sampleController(sampleRepository);
        Controller::OrderController orderController(orderRepository, sampleRepository, productionLine, productionLineRepository);
        Controller::MonitoringController monitoringController(orderRepository, sampleRepository);

        // 앱 시작 직후, 메뉴 루프 진입 전에 실시간 정산을 1회 수행한다 — 오프라인 기간 동안 이미
        // 끝났어야 할 생산을 즉시 반영한다 (CLAUDE.md §3, 요구사항 4·5). 백그라운드 프로세스를
        // 띄우는 것이 아니라, 로드된 큐 상태를 지금 시각과 비교해 한 번에 정산하는 동기 호출이다.
        orderController.SettleProductionQueue(std::chrono::system_clock::now());

        View::MainMenuView mainMenuView;
        View::SampleView sampleView;
        View::OrderView orderView;
        View::MonitoringView monitoringView;
        View::ProductionLineView productionLineView;

        bool running = true;
        while (running)
        {
            const auto summary = monitoringController.GetMainMenuSummary(productionLine);
            mainMenuView.ShowMenu(summary);
            switch (mainMenuView.PromptMenuChoice())
            {
            case View::MainMenuOption::SampleManagement:
                RunSampleManagement(sampleController, sampleView);
                break;
            case View::MainMenuOption::PlaceOrder:
                RunOrderReservation(orderController, orderView);
                break;
            case View::MainMenuOption::ApproveOrRejectOrder:
                RunOrderApproval(orderController, orderView);
                break;
            case View::MainMenuOption::Monitoring:
                RunMonitoring(monitoringController, monitoringView);
                break;
            case View::MainMenuOption::ProductionLine:
                RunProductionLine(orderController, productionLineView);
                break;
            case View::MainMenuOption::ReleaseOrder:
                RunReleaseProcessing(orderController, orderView);
                break;
            case View::MainMenuOption::Exit:
                running = false;
                break;
            default:
                mainMenuView.ShowInvalidChoiceMessage();
                break;
            }
        }

        return 0;
    }
}

// "--test" 인자를 받으면 등록된 모든 테스트(Unit + 적대적)를 실행하고 실패 개수를 종료 코드로
// 반환한다. 그 외에는 Json Repository를 조립(DI)해 메인 메뉴 루프를 실행한다.
int main(int argc, char** argv)
{
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    if (argc > 1 && std::string(argv[1]) == "--test")
    {
        const int failed = Testing::TestRegistry::Instance().RunAll();
        return failed == 0 ? 0 : 1;
    }

    return RunApplication();
}
