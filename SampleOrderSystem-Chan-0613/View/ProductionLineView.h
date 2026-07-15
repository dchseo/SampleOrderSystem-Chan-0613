#pragma once
#include <chrono>
#include <vector>
#include "../Model/ProductionLine.h"

namespace View
{
    // ConsoleMVC PoC 대비 변경: "완료 처리" 메뉴를 제거했다. 생산 완료는 더 이상 사용자가
    // 트리거하는 동작이 아니라 실제 시간 경과에 따라 자동으로 정산되기 때문이다
    // (CLAUDE.md "재고 및 생산 라인 처리 규칙 (상세) §3"). 이 조회 화면을 열기 전에
    // Controller::OrderController::SettleProductionQueue를 먼저 호출해 최신 상태를 반영해야 한다.
    class ProductionLineView
    {
    public:
        void ShowMenu() const;

        // (권장) Current 작업의 진행률(%)과 완료 예정 시각을 함께 표시한다 — 저장된 startTime과
        // totalProductionTime으로 추가 계산 없이 구할 수 있다(PDF 예시 UI 페이지 21 참고).
        void ShowCurrentProduction(const Model::ProductionJob& job, std::chrono::system_clock::time_point now) const;
        void ShowNoActiveProductionMessage() const;
        void ShowProductionQueue(const std::vector<Model::ProductionJob>& queue) const;
    };
}
