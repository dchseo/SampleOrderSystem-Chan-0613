#include "../OrderControllerFixture.h"
#include "../TestFramework.h"

using TestSupport::OrderControllerFixture;

// 상태 전이 위반 ---------------------------------------------------------

TEST(Adversarial, ApproveOrder_RejectsAlreadyConfirmedOrder)
{
    OrderControllerFixture fx("reapprove_confirmed");
    fx.sampleRepo.Add(Model::Sample("S-001", "테스트", 0.5, 1.0, 100));
    const auto orderId = fx.orderController.ReserveOrder("S-001", "A", 10);
    fx.orderController.ApproveOrder(*orderId); // -> CONFIRMED (재고 충분)

    const auto secondApproval = fx.orderController.ApproveOrder(*orderId);
    ASSERT_FALSE(secondApproval.success);
}

TEST(Adversarial, ApproveOrder_RejectsAlreadyProducingOrder)
{
    OrderControllerFixture fx("reapprove_producing");
    fx.sampleRepo.Add(Model::Sample("S-001", "테스트", 0.5, 1.0, 0));
    const auto orderId = fx.orderController.ReserveOrder("S-001", "A", 10);
    fx.orderController.ApproveOrder(*orderId); // -> PRODUCING (재고 부족)

    const auto secondApproval = fx.orderController.ApproveOrder(*orderId);
    ASSERT_FALSE(secondApproval.success);
}

TEST(Adversarial, ApproveOrder_RejectsAlreadyRejectedOrder)
{
    OrderControllerFixture fx("reapprove_rejected");
    fx.sampleRepo.Add(Model::Sample("S-001", "테스트", 0.5, 1.0, 0));
    const auto orderId = fx.orderController.ReserveOrder("S-001", "A", 10);
    ASSERT_TRUE(fx.orderController.RejectOrder(*orderId));

    const auto approvalAfterReject = fx.orderController.ApproveOrder(*orderId);
    ASSERT_FALSE(approvalAfterReject.success);
}

TEST(Adversarial, RejectOrder_RejectsAlreadyConfirmedOrder)
{
    OrderControllerFixture fx("reject_confirmed");
    fx.sampleRepo.Add(Model::Sample("S-001", "테스트", 0.5, 1.0, 100));
    const auto orderId = fx.orderController.ReserveOrder("S-001", "A", 10);
    fx.orderController.ApproveOrder(*orderId); // -> CONFIRMED

    ASSERT_FALSE(fx.orderController.RejectOrder(*orderId));
}

TEST(Adversarial, ReleaseOrder_RejectsNonConfirmedOrders)
{
    OrderControllerFixture fx("release_non_confirmed");
    fx.sampleRepo.Add(Model::Sample("S-001", "테스트", 0.5, 1.0, 0));

    const auto reservedOrder = fx.orderController.ReserveOrder("S-001", "A", 10);
    ASSERT_FALSE(fx.orderController.ReleaseOrder(*reservedOrder)); // 아직 RESERVED

    fx.orderController.ApproveOrder(*reservedOrder); // 재고 0 -> PRODUCING
    ASSERT_FALSE(fx.orderController.ReleaseOrder(*reservedOrder)); // 아직 PRODUCING

    const auto rejectedOrder = fx.orderController.ReserveOrder("S-001", "B", 10);
    fx.orderController.RejectOrder(*rejectedOrder);
    ASSERT_FALSE(fx.orderController.ReleaseOrder(*rejectedOrder)); // REJECTED
}

TEST(Adversarial, ReleaseOrder_RejectsAlreadyReleasedOrder)
{
    OrderControllerFixture fx("rerelease");
    fx.sampleRepo.Add(Model::Sample("S-001", "테스트", 0.5, 1.0, 100));
    const auto orderId = fx.orderController.ReserveOrder("S-001", "A", 10);
    fx.orderController.ApproveOrder(*orderId);
    ASSERT_TRUE(fx.orderController.ReleaseOrder(*orderId));

    ASSERT_FALSE(fx.orderController.ReleaseOrder(*orderId)); // 이미 RELEASED
}

// 존재하지 않는 참조 -----------------------------------------------------

TEST(Adversarial, ReserveOrder_RejectsUnknownSampleId)
{
    OrderControllerFixture fx("unknown_sample");
    const auto orderId = fx.orderController.ReserveOrder("NO-SUCH-SAMPLE", "A", 10);
    ASSERT_FALSE(orderId.has_value());
}

TEST(Adversarial, ApproveOrder_RejectsUnknownOrderId)
{
    OrderControllerFixture fx("unknown_order_approve");
    const auto approval = fx.orderController.ApproveOrder("ORD-9999");
    ASSERT_FALSE(approval.success);
}

TEST(Adversarial, RejectOrder_RejectsUnknownOrderId)
{
    OrderControllerFixture fx("unknown_order_reject");
    ASSERT_FALSE(fx.orderController.RejectOrder("ORD-9999"));
}

TEST(Adversarial, ReleaseOrder_RejectsUnknownOrderId)
{
    OrderControllerFixture fx("unknown_order_release");
    ASSERT_FALSE(fx.orderController.ReleaseOrder("ORD-9999"));
}

// 경계·비정상 입력값 -----------------------------------------------------

TEST(Adversarial, ReserveOrder_RejectsZeroOrNegativeQuantity)
{
    OrderControllerFixture fx("invalid_quantity");
    fx.sampleRepo.Add(Model::Sample("S-001", "테스트", 0.5, 1.0, 100));

    ASSERT_FALSE(fx.orderController.ReserveOrder("S-001", "A", 0).has_value());
    ASSERT_FALSE(fx.orderController.ReserveOrder("S-001", "A", -5).has_value());
}

TEST(Adversarial, RegisterSample_RejectsZeroOrNegativeYield)
{
    OrderControllerFixture fx("invalid_yield");
    ASSERT_FALSE(fx.sampleController.RegisterSample("S-001", "테스트", 0.5, 0.0));
    ASSERT_FALSE(fx.sampleController.RegisterSample("S-002", "테스트", 0.5, -0.1));
    ASSERT_FALSE(fx.sampleController.RegisterSample("S-003", "테스트", 0.5, 1.1)); // 수율은 0~1 범위
}

TEST(Adversarial, RegisterSample_RejectsZeroOrNegativeAvgProductionTime)
{
    OrderControllerFixture fx("invalid_avg_time");
    ASSERT_FALSE(fx.sampleController.RegisterSample("S-001", "테스트", 0.0, 0.9));
    ASSERT_FALSE(fx.sampleController.RegisterSample("S-002", "테스트", -1.0, 0.9));
}

TEST(Adversarial, RegisterSample_RejectsDuplicateSampleId)
{
    OrderControllerFixture fx("duplicate_sample");
    ASSERT_TRUE(fx.sampleController.RegisterSample("S-001", "테스트", 0.5, 0.9));
    ASSERT_FALSE(fx.sampleController.RegisterSample("S-001", "다른 이름", 0.3, 0.8));
}

// 실시간 정산 경계값 -----------------------------------------------------

TEST(Adversarial, SettleProductionQueue_DoesNothingWhenStartTimeIsInFuture)
{
    // 시계 역행 등으로 StartTime이 미래로 설정된 비정상 상태에서도 완료 처리되면 안 된다.
    OrderControllerFixture fx("future_start_time");
    fx.sampleRepo.Add(Model::Sample("S-001", "테스트", 1.0, 1.0, 0));
    const auto orderId = fx.orderController.ReserveOrder("S-001", "A", 10);
    fx.orderController.ApproveOrder(*orderId);

    const auto now = std::chrono::system_clock::now();
    fx.productionLine.SetCurrentJobStartTime(now + std::chrono::hours(1)); // 미래 시각
    fx.orderController.SettleProductionQueue(now);

    ASSERT_TRUE(fx.orderRepo.FindById(*orderId)->GetStatus() == Model::OrderStatus::Producing);
    ASSERT_TRUE(fx.orderController.HasCurrentProduction());
}

TEST(Adversarial, SettleProductionQueue_CompletesWhenNowExactlyEqualsCompletionTime)
{
    // 완료 예정 시각과 now가 정확히 같은 경계 시각에도 "지난 것"으로 처리되어야 한다.
    OrderControllerFixture fx("exact_boundary_time");
    fx.sampleRepo.Add(Model::Sample("S-001", "테스트", 10.0, 1.0, 0)); // 평균생산시간 10분
    const auto orderId = fx.orderController.ReserveOrder("S-001", "A", 10);
    fx.orderController.ApproveOrder(*orderId); // actualQty=10, totalProductionTime=100분

    const auto start = std::chrono::system_clock::now() - std::chrono::hours(1);
    fx.productionLine.SetCurrentJobStartTime(start);
    const auto exactCompletionTime = start + std::chrono::minutes(100);

    fx.orderController.SettleProductionQueue(exactCompletionTime);

    ASSERT_TRUE(fx.orderRepo.FindById(*orderId)->GetStatus() == Model::OrderStatus::Confirmed);
}
