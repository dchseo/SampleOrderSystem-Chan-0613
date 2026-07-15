#pragma once
#include <chrono>
#include <optional>
#include <string>
#include <vector>
#include "../Model/Dtos.h"
#include "../Model/Order.h"
#include "../Model/ProductionLine.h"
#include "../Model/Repository/IOrderRepository.h"
#include "../Model/Repository/IProductionLineRepository.h"
#include "../Model/Repository/ISampleRepository.h"

namespace Controller
{
    // 시료 주문의 예약/승인/거절/출고와, 승인 시 파생되는 생산 라인 등록 및 실시간 정산을
    // 함께 담당한다. ConsoleMVC PoC 대비 확장 사항(CLAUDE.md "재고 및 생산 라인 처리 규칙 (상세)"
    // 참고):
    //   - 생성자가 IProductionLineRepository&를 추가로 받는다 (생산 큐 영속화를 위해 필요).
    //   - ApproveOrder/생산 완료 로직이 재고 반영·재고 상태 재평가까지 함께 수행한다.
    //   - "생산 완료 처리"는 더 이상 사용자가 트리거하는 공개 동작이 아니라, 실제 시간 기준
    //     자동 정산(SettleProductionQueue)의 내부 단계다.
    class OrderController
    {
    public:
        OrderController(Model::IOrderRepository& orderRepository,
            Model::ISampleRepository& sampleRepository,
            Model::ProductionLine& productionLine,
            Model::IProductionLineRepository& productionLineRepository);

        // 2. 시료 주문
        std::optional<std::string> ReserveOrder(const std::string& sampleId, const std::string& customerName, int quantity);

        // 3. 주문 승인/거절
        std::vector<Model::Order> ListReservedOrders() const;
        Model::OrderApprovalResult ApproveOrder(const std::string& orderId);
        bool RejectOrder(const std::string& orderId);

        // 6. 출고 처리
        std::vector<Model::Order> ListReleasableOrders() const;
        bool ReleaseOrder(const std::string& orderId);

        // 5. 생산 라인 조회
        bool HasCurrentProduction() const;
        const Model::ProductionJob& GetCurrentProduction() const;
        std::vector<Model::ProductionJob> GetProductionQueue() const;

        // 실시간 정산(지연 평가, 백그라운드 프로세스 없음): 저장된 StartTime과 now를 비교해
        // 이미 끝났어야 할 작업을 FIFO 순서로 몰아서 완료 처리한다. 앱 시작 시, 주문 승인
        // 직전, 생산 라인 조회 화면을 열 때 호출한다 (CLAUDE.md §3 참고).
        void SettleProductionQueue(std::chrono::system_clock::time_point now);

    private:
        std::string GenerateOrderId();
        void PersistProductionQueue();
        int SumPendingShortageForSample(const std::string& sampleId) const;
        void ApplyProductionCompletion(const Model::ProductionJob& job);
        // ApproveOrder/RejectOrder/ReleaseOrder가 공통으로 쓰는 "지금 이 상태여야만 유효한
        // 전이" 조회 패턴을 추출한 헬퍼. 상태가 requiredStatus가 아니면 nullopt.
        std::optional<Model::Order> FindOrderRequiringStatus(const std::string& orderId, Model::OrderStatus requiredStatus) const;

        Model::IOrderRepository& orderRepository_;
        Model::ISampleRepository& sampleRepository_;
        Model::ProductionLine& productionLine_;
        Model::IProductionLineRepository& productionLineRepository_;
        int nextOrderSequence_ = 1;
    };
}
