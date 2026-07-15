#include "OrderController.h"
#include <algorithm>
#include <iomanip>
#include <sstream>
#include "../Model/InventoryCalculator.h"

namespace Controller
{
    namespace
    {
        // "ORD-0001" 형식에서 순번을 추출한다. 형식이 다르면 0을 반환한다.
        // 재시작 후에도 이미 존재하는 주문과 번호가 겹치지 않도록 하기 위해 사용한다.
        int ExtractSequenceNumber(const std::string& orderId)
        {
            static const std::string kPrefix = "ORD-";
            if (orderId.size() <= kPrefix.size() || orderId.compare(0, kPrefix.size(), kPrefix) != 0)
            {
                return 0;
            }
            try
            {
                return std::stoi(orderId.substr(kPrefix.size()));
            }
            catch (...)
            {
                return 0;
            }
        }

        // totalProductionTime(분, double)을 system_clock::duration으로 변환한다.
        std::chrono::system_clock::duration MinutesToDuration(double minutes)
        {
            return std::chrono::duration_cast<std::chrono::system_clock::duration>(
                std::chrono::duration<double, std::ratio<60>>(minutes));
        }
    }

    OrderController::OrderController(Model::IOrderRepository& orderRepository,
        Model::ISampleRepository& sampleRepository,
        Model::ProductionLine& productionLine,
        Model::IProductionLineRepository& productionLineRepository)
        : orderRepository_(orderRepository)
        , sampleRepository_(sampleRepository)
        , productionLine_(productionLine)
        , productionLineRepository_(productionLineRepository)
    {
        int maxSequence = 0;
        for (const auto& order : orderRepository_.GetAll())
        {
            maxSequence = std::max(maxSequence, ExtractSequenceNumber(order.GetOrderId()));
        }
        nextOrderSequence_ = maxSequence + 1;
    }

    std::string OrderController::GenerateOrderId()
    {
        std::ostringstream oss;
        oss << "ORD-" << std::setw(4) << std::setfill('0') << nextOrderSequence_++;
        return oss.str();
    }

    void OrderController::PersistProductionQueue()
    {
        productionLineRepository_.SaveQueue(productionLine_.GetAllJobsInOrder());
    }

    int OrderController::SumPendingShortageForSample(const std::string& sampleId) const
    {
        int total = 0;
        for (const auto& job : productionLine_.GetAllJobsInOrder())
        {
            if (job.sampleId == sampleId)
            {
                total += job.shortageQuantity;
            }
        }
        return total;
    }

    std::optional<std::string> OrderController::ReserveOrder(const std::string& sampleId, const std::string& customerName, int quantity)
    {
        if (quantity <= 0 || !sampleRepository_.FindById(sampleId).has_value())
        {
            return std::nullopt;
        }

        const std::string orderId = GenerateOrderId();
        orderRepository_.Add(Model::Order(orderId, sampleId, customerName, quantity));
        return orderId;
    }

    std::vector<Model::Order> OrderController::ListReservedOrders() const
    {
        return orderRepository_.FindByStatus(Model::OrderStatus::Reserved);
    }

    Model::OrderApprovalResult OrderController::ApproveOrder(const std::string& orderId)
    {
        Model::OrderApprovalResult result;

        // 승인 시점의 부족분/재고상태 판단은 "지금 이 순간의 재고"만 근거로 한다 — 그러려면
        // 그 시점까지 실제로 이미 끝났어야 할 생산을 먼저 반영해야 한다 (CLAUDE.md §1, 요구사항 6).
        SettleProductionQueue(std::chrono::system_clock::now());

        auto orderOpt = orderRepository_.FindById(orderId);
        if (!orderOpt.has_value() || orderOpt->GetStatus() != Model::OrderStatus::Reserved)
        {
            result.message = "승인 가능한 주문(RESERVED)이 아닙니다.";
            return result;
        }
        Model::Order order = *orderOpt;

        auto sampleOpt = sampleRepository_.FindById(order.GetSampleId());
        if (!sampleOpt.has_value())
        {
            result.message = "주문한 시료 정보를 찾을 수 없습니다.";
            return result;
        }
        Model::Sample sample = *sampleOpt;
        const int stockBeforeDeduction = sample.GetStock();

        if (stockBeforeDeduction >= order.GetQuantity())
        {
            sample.DecreaseStock(order.GetQuantity());
            sample.SetInventoryLevel(Model::ClassifyInventoryLevel(stockBeforeDeduction, order.GetQuantity()));
            sampleRepository_.Update(sample);

            order.SetStatus(Model::OrderStatus::Confirmed);
            orderRepository_.Update(order);

            result.success = true;
            result.resultingStatus = Model::OrderStatus::Confirmed;
            result.message = "재고가 충분하여 즉시 출고 대기(CONFIRMED) 상태로 전환되었습니다.";
            return result;
        }

        const int shortage = Model::CalculateShortage(order.GetQuantity(), stockBeforeDeduction);
        const int actualQuantity = Model::CalculateActualProductionQuantity(shortage, sample.GetYield());
        const double totalProductionTime = Model::CalculateTotalProductionTime(sample.GetAvgProductionTime(), actualQuantity);

        // 가용 재고 전량을 이 주문에 우선 배정하고 재고를 0으로 차감한다.
        sample.DecreaseStock(stockBeforeDeduction);
        sample.SetInventoryLevel(Model::ClassifyInventoryLevel(stockBeforeDeduction, order.GetQuantity()));
        sampleRepository_.Update(sample);

        Model::ProductionJob job;
        job.orderId = order.GetOrderId();
        job.sampleId = sample.GetSampleId();
        job.shortageQuantity = shortage;
        job.actualQuantity = actualQuantity;
        job.totalProductionTime = totalProductionTime;
        if (!productionLine_.HasCurrentJob())
        {
            // 생산 라인이 유휴 상태였으므로 이 작업이 즉시 Current가 된다.
            job.startTime = std::chrono::system_clock::now();
        }
        // 대기열이 비어있지 않다면 startTime은 아직 정하지 않는다 — 이 작업이 Current로
        // 승격되는 순간(SettleProductionQueue)에 직전 작업의 완료 예정 시각으로 기록된다.
        productionLine_.Enqueue(job);
        PersistProductionQueue();

        order.SetStatus(Model::OrderStatus::Producing);
        orderRepository_.Update(order);

        result.success = true;
        result.resultingStatus = Model::OrderStatus::Producing;
        result.shortageQuantity = shortage;
        result.actualProductionQuantity = actualQuantity;
        result.totalProductionTime = totalProductionTime;
        result.message = "재고 부족으로 생산 라인(PRODUCING)에 등록되었습니다.";
        return result;
    }

    bool OrderController::RejectOrder(const std::string& orderId)
    {
        auto orderOpt = orderRepository_.FindById(orderId);
        if (!orderOpt.has_value() || orderOpt->GetStatus() != Model::OrderStatus::Reserved)
        {
            return false;
        }
        Model::Order order = *orderOpt;
        order.SetStatus(Model::OrderStatus::Rejected);
        orderRepository_.Update(order);
        return true;
    }

    std::vector<Model::Order> OrderController::ListReleasableOrders() const
    {
        return orderRepository_.FindByStatus(Model::OrderStatus::Confirmed);
    }

    bool OrderController::ReleaseOrder(const std::string& orderId)
    {
        auto orderOpt = orderRepository_.FindById(orderId);
        if (!orderOpt.has_value() || orderOpt->GetStatus() != Model::OrderStatus::Confirmed)
        {
            return false;
        }
        Model::Order order = *orderOpt;
        order.SetStatus(Model::OrderStatus::Released);
        orderRepository_.Update(order);
        return true;
    }

    bool OrderController::HasCurrentProduction() const
    {
        return productionLine_.HasCurrentJob();
    }

    const Model::ProductionJob& OrderController::GetCurrentProduction() const
    {
        return productionLine_.CurrentJob();
    }

    std::vector<Model::ProductionJob> OrderController::GetProductionQueue() const
    {
        return productionLine_.GetWaitingJobs();
    }

    void OrderController::ApplyProductionCompletion(const Model::ProductionJob& job)
    {
        auto sampleOpt = sampleRepository_.FindById(job.sampleId);
        auto orderOpt = orderRepository_.FindById(job.orderId);
        if (!sampleOpt.has_value() || !orderOpt.has_value())
        {
            // 정상 흐름에서는 발생하지 않아야 하는 방어적 케이스 (참조 무결성 깨짐).
            return;
        }

        Model::Sample sample = *sampleOpt;
        Model::Order order = *orderOpt;

        // 실 생산량 전체를 재고에 더한 뒤, 이 주문의 부족분만큼만 다시 차감해 배정한다.
        // 수율은 실 생산량을 계산할 때만 쓰고 여기서 다시 적용하지 않는다 — 계산된 실
        // 생산량은 항상 100% 그대로 만들어진다 (요구사항 2).
        sample.IncreaseStock(job.actualQuantity);
        sample.DecreaseStock(job.shortageQuantity);

        // 재고 상태 재평가: 같은 시료에 대해 아직 생산 중인 다른 주문들의 부족분 합계를
        // 뺀 순가용재고를, 방금 완료된 주문의 원래 수량과 비교해 재분류한다 (CLAUDE.md §2).
        // 이 시점에는 job이 이미 대기열에서 pop된 상태이므로, GetAllJobsInOrder()의 결과가
        // 곧 "다른" 대기 중인 작업들이다.
        const int otherPendingShortage = SumPendingShortageForSample(job.sampleId);
        const int netAvailable = sample.GetStock() - otherPendingShortage;
        sample.SetInventoryLevel(Model::ClassifyInventoryLevel(netAvailable, order.GetQuantity()));

        sampleRepository_.Update(sample);

        order.SetStatus(Model::OrderStatus::Confirmed);
        orderRepository_.Update(order);
    }

    void OrderController::SettleProductionQueue(std::chrono::system_clock::time_point now)
    {
        while (productionLine_.HasCurrentJob())
        {
            const Model::ProductionJob current = productionLine_.CurrentJob();
            const auto completionTime = current.startTime + MinutesToDuration(current.totalProductionTime);

            if (now < completionTime)
            {
                break; // 아직 완료 예정 시각이 지나지 않음 — 진행 중.
            }

            const Model::ProductionJob completed = productionLine_.CompleteCurrentJob();
            ApplyProductionCompletion(completed);

            if (productionLine_.HasCurrentJob())
            {
                // 대기열에 들어간 작업은 그 시점에 계산된 실 생산량으로 무조건 끝까지
                // 생산한다 — 재계산/취소 없이 그대로 승격만 한다 (요구사항 7).
                // 새 Current의 시작 시각은 "지금"이 아니라 직전 작업의 완료 예정 시각이다 —
                // 오프라인 기간이 길었어도 실제 경과 시간을 그대로 이어서 반영하기 위함이다
                // (요구사항 4·5).
                productionLine_.SetCurrentJobStartTime(completionTime);
            }

            PersistProductionQueue();
        }
    }
}
