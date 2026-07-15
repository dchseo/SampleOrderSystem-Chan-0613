#include "../OrderControllerFixture.h"
#include "../TestFramework.h"

using TestSupport::OrderControllerFixture;

// 서로 다른 시료(수율/평균생산시간이 각기 다름) + 예약 순서와 다른 승인 순서로 생산 큐를
// 채워, 생산 라인이 "시료 종류와 무관하게 승인된 순서" 그대로 FIFO 처리되는지, 재고/재고
// 상태 재평가가 시료별로 정확히 분리되는지 확인한다. 기존 테스트는 픽스처마다 시료를 하나만
// 등록해 이 교차 오염 가능성을 검증하지 못했다.
TEST(Unit, OrderController_MultiSample_InterleavedApprovalPreservesGlobalFifoAndPerSampleAccounting)
{
    OrderControllerFixture fx("multi_sample");
    fx.sampleRepo.Add(Model::Sample("S-A", "시료 A", 1.0, 0.9, 0)); // 평균생산시간 1.0, 수율 0.9
    fx.sampleRepo.Add(Model::Sample("S-B", "시료 B", 3.0, 0.4, 0)); // 평균생산시간 3.0, 수율 0.4
    fx.sampleRepo.Add(Model::Sample("S-C", "시료 C", 0.5, 0.7, 0)); // 평균생산시간 0.5, 수율 0.7

    // 예약은 B1 -> A1 -> C1 -> A2 -> B2 순서로 한다.
    const auto orderB1 = fx.orderController.ReserveOrder("S-B", "고객1", 20);
    const auto orderA1 = fx.orderController.ReserveOrder("S-A", "고객2", 50);
    const auto orderC1 = fx.orderController.ReserveOrder("S-C", "고객3", 10);
    const auto orderA2 = fx.orderController.ReserveOrder("S-A", "고객4", 30);
    const auto orderB2 = fx.orderController.ReserveOrder("S-B", "고객5", 15);

    // 승인은 예약 순서와 다르게 A1 -> C1 -> B1 -> A2 -> B2 순서로 뒤섞는다 — 생산 큐는
    // "승인된 순서"대로 채워져야 한다(시료 종류와 무관).
    const auto approvalA1 = fx.orderController.ApproveOrder(*orderA1);
    const auto approvalC1 = fx.orderController.ApproveOrder(*orderC1);
    const auto approvalB1 = fx.orderController.ApproveOrder(*orderB1);
    const auto approvalA2 = fx.orderController.ApproveOrder(*orderA2);
    const auto approvalB2 = fx.orderController.ApproveOrder(*orderB2);

    ASSERT_EQ(56, approvalA1.actualProductionQuantity); // ceil(50 / 0.9)
    ASSERT_EQ(15, approvalC1.actualProductionQuantity); // ceil(10 / 0.7)
    ASSERT_EQ(50, approvalB1.actualProductionQuantity); // ceil(20 / 0.4)
    ASSERT_EQ(34, approvalA2.actualProductionQuantity); // ceil(30 / 0.9), 재고 여전히 0 (요구사항 9)
    ASSERT_EQ(38, approvalB2.actualProductionQuantity); // ceil(15 / 0.4), 재고 여전히 0 (요구사항 9)

    // A1은 승인 시점에 큐가 비어 있어 실제 시각으로 시작됐다. 테스트가 결정론적으로
    // 검증할 수 있도록 기준 시각을 우리가 통제하는 값으로 덮어쓴다.
    const auto startBase = std::chrono::system_clock::now();
    fx.productionLine.SetCurrentJobStartTime(startBase);

    // 체크포인트 1: 큐 순서 [A1, C1, B1, A2, B2] 중 A1(56분)만 끝나고, C1(승격 후 +7.5=63.5분)은
    // 아직 끝나지 않은 시점.
    fx.orderController.SettleProductionQueue(startBase + std::chrono::minutes(57));

    ASSERT_TRUE(fx.orderRepo.FindById(*orderA1)->GetStatus() == Model::OrderStatus::Confirmed);
    ASSERT_TRUE(fx.orderRepo.FindById(*orderC1)->GetStatus() == Model::OrderStatus::Producing);
    ASSERT_TRUE(fx.orderRepo.FindById(*orderB1)->GetStatus() == Model::OrderStatus::Producing);
    ASSERT_TRUE(fx.orderRepo.FindById(*orderA2)->GetStatus() == Model::OrderStatus::Producing);
    ASSERT_TRUE(fx.orderController.HasCurrentProduction());
    // 다음 Current는 시료 종류와 무관하게 "승인된 순서"상 다음인 C1이어야 한다(전역 FIFO).
    ASSERT_TRUE(fx.orderController.GetCurrentProduction().orderId == *orderC1);

    ASSERT_EQ(6, fx.sampleRepo.FindById("S-A")->GetStock());   // 0 + 56 - 50
    ASSERT_EQ(0, fx.sampleRepo.FindById("S-B")->GetStock());   // 아직 미완료
    ASSERT_EQ(0, fx.sampleRepo.FindById("S-C")->GetStock());   // 아직 미완료
    // A2(부족분 30)가 아직 큐에 남아 있으므로 순가용재고 = 6 - 30 <= 0 -> 고갈.
    ASSERT_TRUE(fx.sampleRepo.FindById("S-A")->GetInventoryLevel() == Model::InventoryLevel::Depleted);

    // 체크포인트 2: [C1, B1]까지 끝나고(63.5, 213.5) [A2](247.5)는 아직인 시점.
    fx.orderController.SettleProductionQueue(startBase + std::chrono::minutes(220));

    ASSERT_TRUE(fx.orderRepo.FindById(*orderC1)->GetStatus() == Model::OrderStatus::Confirmed);
    ASSERT_TRUE(fx.orderRepo.FindById(*orderB1)->GetStatus() == Model::OrderStatus::Confirmed);
    ASSERT_TRUE(fx.orderRepo.FindById(*orderA2)->GetStatus() == Model::OrderStatus::Producing);
    ASSERT_TRUE(fx.orderController.GetCurrentProduction().orderId == *orderA2);

    ASSERT_EQ(6, fx.sampleRepo.FindById("S-A")->GetStock());    // A2 아직 미완료라 그대로
    ASSERT_EQ(30, fx.sampleRepo.FindById("S-B")->GetStock());   // 0 + 50 - 20
    ASSERT_EQ(5, fx.sampleRepo.FindById("S-C")->GetStock());    // 0 + 15 - 10
    // B2(부족분 15)가 아직 큐에 남아 있으므로 순가용재고 = 30 - 15 = 15 < B1 수량(20) -> 부족.
    ASSERT_TRUE(fx.sampleRepo.FindById("S-B")->GetInventoryLevel() == Model::InventoryLevel::Low);
    // C는 다른 C 작업이 없으므로 순가용재고 = 5, C1 수량(10)보다 작음 -> 부족.
    ASSERT_TRUE(fx.sampleRepo.FindById("S-C")->GetInventoryLevel() == Model::InventoryLevel::Low);

    // 체크포인트 3: 모두(361.5분까지) 끝난 시점.
    fx.orderController.SettleProductionQueue(startBase + std::chrono::minutes(362));

    ASSERT_TRUE(fx.orderRepo.FindById(*orderA2)->GetStatus() == Model::OrderStatus::Confirmed);
    ASSERT_TRUE(fx.orderRepo.FindById(*orderB2)->GetStatus() == Model::OrderStatus::Confirmed);
    ASSERT_FALSE(fx.orderController.HasCurrentProduction());

    ASSERT_EQ(10, fx.sampleRepo.FindById("S-A")->GetStock());  // 6 + 34 - 30
    ASSERT_EQ(53, fx.sampleRepo.FindById("S-B")->GetStock());  // 30 + 38 - 15
    ASSERT_EQ(5, fx.sampleRepo.FindById("S-C")->GetStock());   // 변화 없음

    // 각 시료의 마지막 완료 이벤트 기준 재평가: 더 이상 같은 시료의 대기 작업이 없으므로
    // 순가용재고 = 원 재고 그대로.
    ASSERT_TRUE(fx.sampleRepo.FindById("S-A")->GetInventoryLevel() == Model::InventoryLevel::Low);       // 10 < A2 수량(30)
    ASSERT_TRUE(fx.sampleRepo.FindById("S-B")->GetInventoryLevel() == Model::InventoryLevel::Sufficient); // 53 >= B2 수량(15)
    ASSERT_TRUE(fx.sampleRepo.FindById("S-C")->GetInventoryLevel() == Model::InventoryLevel::Low);        // 변화 없음(체크포인트 2와 동일)
}
