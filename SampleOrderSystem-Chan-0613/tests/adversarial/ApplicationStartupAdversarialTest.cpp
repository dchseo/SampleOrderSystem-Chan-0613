#include <filesystem>
#include <fstream>

#include "../OrderControllerFixture.h"
#include "../TestFramework.h"

// main.cpp::RunApplication()의 조립 절차(Repository 생성 -> ProductionLine 로드 -> Controller
// 생성 -> 초기 SettleProductionQueue)를 손상/누락된 파일로 그대로 재현해, "조립된 전체 시스템"이
// 크래시 없이 정상 상태(빈 컬렉션)로 기동되는지 확인한다. 개별 Repository 단위의 폴백 동작은
// Phase 1의 tests/adversarial/JsonRepositoryFallbackTest.cpp에서 이미 검증했다 — 이 테스트는
// main.cpp이 실제로 조립하는 전체 조합을 대상으로 한다는 점이 다르다.

TEST(Adversarial, ApplicationStartup_CorruptedOrMissingDataFiles_DoesNotCrash)
{
    const std::string samplesPath = TestSupport::TempFilePath("startup_samples.json");
    const std::string ordersPath = TestSupport::TempFilePath("startup_orders.json");
    const std::string queuePath = TestSupport::TempFilePath("startup_queue.json");

    // samples.json: 문법이 손상된 파일
    {
        std::ofstream out(samplesPath, std::ios::binary | std::ios::trunc);
        out << "{ this is not valid JSON !!!";
    }
    // orders.json: 중간에 잘린 파일
    {
        std::ofstream out(ordersPath, std::ios::binary | std::ios::trunc);
        out << "[ { \"orderId\": \"ORD-0001\", ";
    }
    // production_queue.json: 파일 자체가 없는 경우(최초 실행)
    std::filesystem::remove(queuePath);

    Model::JsonSampleRepository sampleRepository(samplesPath);
    Model::JsonOrderRepository orderRepository(ordersPath);
    Model::JsonProductionLineRepository productionLineRepository(queuePath);

    Model::ProductionLine productionLine;
    productionLine.ReplaceAll(productionLineRepository.LoadQueue());

    Controller::SampleController sampleController(sampleRepository);
    Controller::OrderController orderController(orderRepository, sampleRepository, productionLine, productionLineRepository);
    Controller::MonitoringController monitoringController(orderRepository, sampleRepository);

    // 앱 시작 시 호출하는 초기 실시간 정산도 빈 큐에 대해 안전하게 아무 일도 하지 않아야 한다.
    orderController.SettleProductionQueue(std::chrono::system_clock::now());

    ASSERT_TRUE(sampleController.ListSamples().empty());
    ASSERT_TRUE(orderController.ListReservedOrders().empty());
    ASSERT_FALSE(orderController.HasCurrentProduction());

    const auto summary = monitoringController.GetMainMenuSummary(productionLine);
    ASSERT_EQ(0, summary.registeredSampleCount);
    ASSERT_EQ(0, summary.totalStock);
    ASSERT_EQ(0, summary.totalOrderCount);
    ASSERT_EQ(0, summary.productionQueueCount);

    // 폴백 이후에도 정상적으로 계속 사용할 수 있어야 한다(등록 -> 조회 왕복).
    ASSERT_TRUE(sampleController.RegisterSample("S-001", "복구 확인용", 0.5, 0.9));
    ASSERT_EQ(static_cast<size_t>(1), sampleController.ListSamples().size());

    std::filesystem::remove(samplesPath);
    std::filesystem::remove(ordersPath);
    std::filesystem::remove(queuePath);
}
