#include <filesystem>
#include <string>

#include "../../Model/Order.h"
#include "../../Model/ProductionLine.h"
#include "../../Model/Repository/JsonOrderRepository.h"
#include "../../Model/Repository/JsonProductionLineRepository.h"
#include "../../Model/Repository/JsonSampleRepository.h"
#include "../../Model/Sample.h"
#include "../TestFramework.h"

namespace
{
    // 테스트마다 겹치지 않는 임시 파일 경로를 만든다 (Date.now류 API는 쓸 수 없으므로
    // 테스트 이름을 파일명에 그대로 사용해 충돌을 피한다).
    std::string TempFilePath(const std::string& fileName)
    {
        return (std::filesystem::temp_directory_path() / ("SampleOrderSystem_" + fileName)).string();
    }
}

TEST(Unit, JsonSampleRepository_RoundTripsAcrossInstances)
{
    const std::string path = TempFilePath("sample_roundtrip.json");
    std::filesystem::remove(path);

    {
        Model::JsonSampleRepository repo(path);
        repo.Add(Model::Sample("S-001", "실리콘 웨이퍼-8인치", 0.5, 0.92, 480));
        repo.Add(Model::Sample("S-002", "GaN 에피택셜-4인치", 0.3, 0.78, 220));
    }

    // 새 인스턴스(재실행을 흉내)로 다시 열어 데이터가 유지되는지 확인한다.
    Model::JsonSampleRepository reloaded(path);
    const auto all = reloaded.GetAll();
    ASSERT_EQ(static_cast<size_t>(2), all.size());

    const auto found = reloaded.FindById("S-001");
    ASSERT_TRUE(found.has_value());
    ASSERT_EQ(std::string("실리콘 웨이퍼-8인치"), found->GetName());
    ASSERT_EQ(480, found->GetStock());

    std::filesystem::remove(path);
}

TEST(Unit, JsonOrderRepository_RoundTripsAcrossInstances)
{
    const std::string path = TempFilePath("order_roundtrip.json");
    std::filesystem::remove(path);

    {
        Model::JsonOrderRepository repo(path);
        Model::Order order("ORD-0001", "S-001", "삼성전자 파운드리", 200);
        order.SetStatus(Model::OrderStatus::Producing);
        repo.Add(order);
    }

    Model::JsonOrderRepository reloaded(path);
    const auto found = reloaded.FindById("ORD-0001");
    ASSERT_TRUE(found.has_value());
    ASSERT_EQ(std::string("S-001"), found->GetSampleId());
    ASSERT_EQ(200, found->GetQuantity());
    ASSERT_TRUE(found->GetStatus() == Model::OrderStatus::Producing);

    std::filesystem::remove(path);
}

TEST(Unit, JsonProductionLineRepository_RoundTripsQueueOrderAndStartTime)
{
    const std::string path = TempFilePath("production_queue_roundtrip.json");
    std::filesystem::remove(path);

    Model::ProductionJob current;
    current.orderId = "ORD-0001";
    current.sampleId = "S-001";
    current.shortageQuantity = 50;
    current.actualQuantity = 100;
    current.totalProductionTime = 50.0;
    current.startTime = std::chrono::system_clock::now();

    Model::ProductionJob waiting;
    waiting.orderId = "ORD-0002";
    waiting.sampleId = "S-002";
    waiting.shortageQuantity = 80;
    waiting.actualQuantity = 114;
    waiting.totalProductionTime = 34.2;
    waiting.startTime = std::chrono::system_clock::time_point{}; // 아직 시작 전이므로 epoch(0)

    {
        Model::JsonProductionLineRepository repo(path);
        repo.SaveQueue({current, waiting});
    }

    Model::JsonProductionLineRepository reloaded(path);
    const auto jobs = reloaded.LoadQueue();
    ASSERT_EQ(static_cast<size_t>(2), jobs.size());

    // 순서(맨 앞이 Current Job)가 보존되어야 한다.
    ASSERT_EQ(std::string("ORD-0001"), jobs[0].orderId);
    ASSERT_EQ(std::string("ORD-0002"), jobs[1].orderId);

    // startTime이 밀리초 단위까지 왕복되는지 확인한다.
    const auto expectedMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                                 current.startTime.time_since_epoch())
                                 .count();
    const auto actualMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                               jobs[0].startTime.time_since_epoch())
                               .count();
    ASSERT_EQ(expectedMs, actualMs);

    std::filesystem::remove(path);
}

TEST(Unit, ProductionLine_ReplaceAllAndGetAllJobsInOrderRoundTrip)
{
    Model::ProductionJob first;
    first.orderId = "ORD-A";
    Model::ProductionJob second;
    second.orderId = "ORD-B";

    Model::ProductionLine line;
    line.ReplaceAll({first, second});

    const auto jobs = line.GetAllJobsInOrder();
    ASSERT_EQ(static_cast<size_t>(2), jobs.size());
    ASSERT_EQ(std::string("ORD-A"), jobs[0].orderId);
    ASSERT_EQ(std::string("ORD-B"), jobs[1].orderId);

    ASSERT_TRUE(line.HasCurrentJob());
    ASSERT_EQ(std::string("ORD-A"), line.CurrentJob().orderId);

    const auto completed = line.CompleteCurrentJob();
    ASSERT_EQ(std::string("ORD-A"), completed.orderId);
    ASSERT_EQ(std::string("ORD-B"), line.CurrentJob().orderId);
}
