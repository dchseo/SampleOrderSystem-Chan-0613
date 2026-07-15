#include "../OrderControllerFixture.h"
#include "../TestFramework.h"

using TestSupport::OrderControllerFixture;

TEST(Unit, OrderController_FullCycle_ShortageApproval_SettlementThenRelease)
{
    // "재고 부족 주문 승인 -> 생산 완료 -> 출고" 전체 사이클 (Phase 2 목표 시나리오).
    OrderControllerFixture fx("full_cycle");
    fx.sampleRepo.Add(Model::Sample("S-001", "실리콘 웨이퍼-8인치", 0.5, 0.5, 50));

    const auto orderId = fx.orderController.ReserveOrder("S-001", "삼성전자 파운드리", 100);
    ASSERT_TRUE(orderId.has_value());

    const auto approval = fx.orderController.ApproveOrder(*orderId);
    ASSERT_TRUE(approval.success);
    ASSERT_TRUE(approval.resultingStatus == Model::OrderStatus::Producing);
    ASSERT_EQ(50, approval.shortageQuantity);
    ASSERT_EQ(100, approval.actualProductionQuantity); // ceil(50 / 0.5) = 100

    // 승인 시점에 가용 재고 전량이 이 주문에 배정되어 재고는 0이 된다.
    ASSERT_EQ(0, fx.sampleRepo.FindById("S-001")->GetStock());
    ASSERT_TRUE(fx.sampleRepo.FindById("S-001")->GetInventoryLevel() == Model::InventoryLevel::Low);

    // 아직 완료 예정 시각 이전이면 정산해도 상태가 바뀌지 않는다.
    fx.orderController.SettleProductionQueue(std::chrono::system_clock::now());
    ASSERT_TRUE(fx.orderRepo.FindById(*orderId)->GetStatus() == Model::OrderStatus::Producing);

    // 시작 시각을 과거로 돌려 "이미 완료됐어야 하는" 상황을 흉내낸다.
    fx.productionLine.SetCurrentJobStartTime(std::chrono::system_clock::now() - std::chrono::hours(2));
    fx.orderController.SettleProductionQueue(std::chrono::system_clock::now());

    ASSERT_TRUE(fx.orderRepo.FindById(*orderId)->GetStatus() == Model::OrderStatus::Confirmed);
    // 재고 = 실생산량(100) - 부족분(50) = 잉여 50
    ASSERT_EQ(50, fx.sampleRepo.FindById("S-001")->GetStock());
    ASSERT_FALSE(fx.orderController.HasCurrentProduction());

    ASSERT_TRUE(fx.orderController.ReleaseOrder(*orderId));
    ASSERT_TRUE(fx.orderRepo.FindById(*orderId)->GetStatus() == Model::OrderStatus::Released);
}

TEST(Unit, OrderController_ApproveOrder_ConfirmsImmediatelyWhenStockSufficient)
{
    OrderControllerFixture fx("sufficient_stock");
    fx.sampleRepo.Add(Model::Sample("S-001", "포토레지스트-PR7", 0.2, 0.95, 500));

    const auto orderId = fx.orderController.ReserveOrder("S-001", "DB하이텍", 200);
    const auto approval = fx.orderController.ApproveOrder(*orderId);

    ASSERT_TRUE(approval.success);
    ASSERT_TRUE(approval.resultingStatus == Model::OrderStatus::Confirmed);
    ASSERT_EQ(300, fx.sampleRepo.FindById("S-001")->GetStock());
    ASSERT_TRUE(fx.sampleRepo.FindById("S-001")->GetInventoryLevel() == Model::InventoryLevel::Sufficient);
    ASSERT_FALSE(fx.orderController.HasCurrentProduction());
}

TEST(Unit, OrderController_Requirement9_SecondOrderTakesFullShortageAfterStockAlreadyClaimed)
{
    // CLAUDE.md 요구사항 9: 재고 50, 주문A(100) 승인 -> 재고 0, 부족분 50.
    // 이어서 주문B(100)가 승인되면 이 시점 재고는 이미 0이므로 부족분은 100 전체가 된다.
    OrderControllerFixture fx("requirement9");
    fx.sampleRepo.Add(Model::Sample("S-001", "SiC 파워기판-6인치", 0.8, 1.0, 50));

    const auto orderA = fx.orderController.ReserveOrder("S-001", "A", 100);
    const auto approvalA = fx.orderController.ApproveOrder(*orderA);
    ASSERT_EQ(50, approvalA.shortageQuantity);
    ASSERT_EQ(0, fx.sampleRepo.FindById("S-001")->GetStock());

    const auto orderB = fx.orderController.ReserveOrder("S-001", "B", 100);
    const auto approvalB = fx.orderController.ApproveOrder(*orderB);
    ASSERT_EQ(100, approvalB.shortageQuantity);
}

TEST(Unit, OrderController_Requirement9_ThirdOrderAlsoTakesFullShortageWhileStockStillClaimed)
{
    // 재고 선점 경합을 3건까지 연장 — 두 번째, 세 번째 주문 모두 재고 0에서 전체를 부족분으로 갖는다.
    OrderControllerFixture fx("requirement9_extended");
    fx.sampleRepo.Add(Model::Sample("S-001", "산화막 웨이퍼-SiO2", 0.6, 1.0, 30));

    const auto orderA = fx.orderController.ReserveOrder("S-001", "A", 80);
    fx.orderController.ApproveOrder(*orderA); // shortage 50, stock -> 0

    const auto orderB = fx.orderController.ReserveOrder("S-001", "B", 120);
    const auto approvalB = fx.orderController.ApproveOrder(*orderB);
    ASSERT_EQ(120, approvalB.shortageQuantity);

    const auto orderC = fx.orderController.ReserveOrder("S-001", "C", 90);
    const auto approvalC = fx.orderController.ApproveOrder(*orderC);
    ASSERT_EQ(90, approvalC.shortageQuantity);
}

TEST(Unit, OrderController_Requirement8_SequentialCompletionYieldsSurplusAndInventoryTransition)
{
    // CLAUDE.md 요구사항 8: 동일 시료에 order1(부족분 50, 실생산량 100),
    // order2(부족분 50, 실생산량 100)가 순차 완료 -> 최종 재고 100, 그 사이 고갈->여유 전이.
    OrderControllerFixture fx("requirement8");
    fx.sampleRepo.Add(Model::Sample("S-001", "GaN 에피택셜-4인치", 1.0, 0.5, 0));

    const auto orderA = fx.orderController.ReserveOrder("S-001", "A", 50);
    fx.orderController.ApproveOrder(*orderA); // shortage 50, actualQty 100

    const auto orderB = fx.orderController.ReserveOrder("S-001", "B", 50);
    fx.orderController.ApproveOrder(*orderB); // 재고 여전히 0 -> shortage 50, actualQty 100

    const auto now = std::chrono::system_clock::now();
    fx.productionLine.SetCurrentJobStartTime(now - std::chrono::hours(3));
    fx.orderController.SettleProductionQueue(now);

    ASSERT_TRUE(fx.orderRepo.FindById(*orderA)->GetStatus() == Model::OrderStatus::Confirmed);
    ASSERT_TRUE(fx.orderRepo.FindById(*orderB)->GetStatus() == Model::OrderStatus::Producing);
    ASSERT_EQ(50, fx.sampleRepo.FindById("S-001")->GetStock());
    ASSERT_TRUE(fx.sampleRepo.FindById("S-001")->GetInventoryLevel() == Model::InventoryLevel::Depleted);

    fx.productionLine.SetCurrentJobStartTime(now - std::chrono::hours(3));
    fx.orderController.SettleProductionQueue(now);

    ASSERT_TRUE(fx.orderRepo.FindById(*orderB)->GetStatus() == Model::OrderStatus::Confirmed);
    ASSERT_EQ(100, fx.sampleRepo.FindById("S-001")->GetStock());
    ASSERT_TRUE(fx.sampleRepo.FindById("S-001")->GetInventoryLevel() == Model::InventoryLevel::Sufficient);
}

TEST(Unit, OrderController_SettleProductionQueue_CatchesUpMultipleJobsAfterLongOffline)
{
    // 실시간 정산(캐치업): 대기열에 있던 작업까지 한 번의 호출로 몰아서 완료되는지 확인
    // (요구사항 5 — 대기 중인 주문도 정산 시점까지 실제 시간이 지났다면 생산이 끝난 것으로 본다).
    OrderControllerFixture fx("catchup");
    fx.sampleRepo.Add(Model::Sample("S-001", "테스트 시료", 0.1, 1.0, 0));

    const auto orderA = fx.orderController.ReserveOrder("S-001", "A", 10);
    fx.orderController.ApproveOrder(*orderA); // actualQty 10, totalProductionTime 1분

    const auto orderB = fx.orderController.ReserveOrder("S-001", "B", 10);
    fx.orderController.ApproveOrder(*orderB); // actualQty 10, totalProductionTime 1분

    // 매우 긴 오프라인 기간을 흉내낸다 — order1의 시작 시각을 10시간 전으로.
    fx.productionLine.SetCurrentJobStartTime(std::chrono::system_clock::now() - std::chrono::hours(10));

    // 한 번의 정산 호출로 order1, order2 모두 끝나야 한다 (각 1분짜리 작업이므로).
    fx.orderController.SettleProductionQueue(std::chrono::system_clock::now());

    ASSERT_TRUE(fx.orderRepo.FindById(*orderA)->GetStatus() == Model::OrderStatus::Confirmed);
    ASSERT_TRUE(fx.orderRepo.FindById(*orderB)->GetStatus() == Model::OrderStatus::Confirmed);
    ASSERT_FALSE(fx.orderController.HasCurrentProduction());
}

TEST(Unit, OrderController_ReserveOrder_GeneratesNonCollidingIdsAfterRestart)
{
    // 재시작(새 OrderController 인스턴스) 후에도 기존 주문과 번호가 겹치지 않아야 한다.
    OrderControllerFixture fx("restart_ids");
    fx.sampleRepo.Add(Model::Sample("S-001", "테스트 시료", 0.5, 1.0, 0));

    const auto firstId = fx.orderController.ReserveOrder("S-001", "A", 10);
    ASSERT_TRUE(firstId.has_value());

    // 같은 Repository를 바라보는 새 OrderController를 만들어 "재시작"을 흉내낸다.
    Controller::OrderController restarted(fx.orderRepo, fx.sampleRepo, fx.productionLine, fx.productionLineRepo);
    const auto secondId = restarted.ReserveOrder("S-001", "B", 10);

    ASSERT_TRUE(secondId.has_value());
    ASSERT_FALSE(*firstId == *secondId);
}
