#include "MainMenuView.h"
#include <iostream>
#include <limits>

namespace View
{
    void MainMenuView::ShowMenu(const Model::MainMenuSummary& summary) const
    {
        std::cout << "\n================= 반도체 시료 생산주문관리 시스템 =================\n";
        std::cout << "등록 시료 " << summary.registeredSampleCount << "종   "
            << "총 재고 " << summary.totalStock << " ea   "
            << "전체 주문 " << summary.totalOrderCount << "건   "
            << "생산라인 " << summary.productionQueueCount << "건 대기\n";
        std::cout << "---------------------------------------------------------------\n";
        std::cout << "[1] 시료 관리        [2] 시료 주문\n";
        std::cout << "[3] 주문 승인/거절   [4] 모니터링\n";
        std::cout << "[5] 생산 라인 조회   [6] 출고 처리\n";
        std::cout << "[0] 종료\n";
        std::cout << "선택 > ";
    }

    MainMenuOption MainMenuView::PromptMenuChoice() const
    {
        int choice = 0;
        if (!(std::cin >> choice))
        {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            return MainMenuOption::Invalid;
        }
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        switch (choice)
        {
        case 1: return MainMenuOption::SampleManagement;
        case 2: return MainMenuOption::PlaceOrder;
        case 3: return MainMenuOption::ApproveOrRejectOrder;
        case 4: return MainMenuOption::Monitoring;
        case 5: return MainMenuOption::ProductionLine;
        case 6: return MainMenuOption::ReleaseOrder;
        case 0: return MainMenuOption::Exit;
        default: return MainMenuOption::Invalid;
        }
    }

    void MainMenuView::ShowInvalidChoiceMessage() const
    {
        std::cout << "잘못된 선택입니다. 다시 입력해 주세요.\n";
    }
}
