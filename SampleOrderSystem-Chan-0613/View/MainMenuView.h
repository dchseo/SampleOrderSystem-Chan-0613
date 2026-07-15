#pragma once
#include "../Model/Dtos.h"

namespace View
{
    enum class MainMenuOption
    {
        SampleManagement = 1,
        PlaceOrder = 2,
        ApproveOrRejectOrder = 3,
        Monitoring = 4,
        ProductionLine = 5,
        ReleaseOrder = 6,
        Exit = 0,
        Invalid = -1
    };

    class MainMenuView
    {
    public:
        // ConsoleMVC PoC 대비 확장 — 상단에 전체 시료 요약 정보를 표시한다(PDF 예시 UI 참고).
        // 요약 계산은 Controller(MonitoringController::GetMainMenuSummary) 책임이며, View는
        // 전달받은 값을 그대로 렌더링만 한다.
        void ShowMenu(const Model::MainMenuSummary& summary) const;
        MainMenuOption PromptMenuChoice() const;
        void ShowInvalidChoiceMessage() const;
    };
}
