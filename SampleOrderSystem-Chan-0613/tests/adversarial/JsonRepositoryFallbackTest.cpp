#include <filesystem>
#include <fstream>
#include <string>

#include "../../Model/Repository/JsonOrderRepository.h"
#include "../../Model/Repository/JsonProductionLineRepository.h"
#include "../../Model/Repository/JsonSampleRepository.h"
#include "../TestFramework.h"

// 손상/누락된 JSON 파일에 대해 각 Repository가 크래시 없이 빈 컬렉션으로 폴백하는지 검증한다
// (CLAUDE.md "재고 및 생산 라인 처리 규칙" 및 DataPersistence PoC의 손상 파일 폴백 원칙 참고).

namespace
{
    std::string TempFilePath(const std::string& fileName)
    {
        return (std::filesystem::temp_directory_path() / ("SampleOrderSystem_Adversarial_" + fileName)).string();
    }

    void WriteRawText(const std::string& path, const std::string& content)
    {
        std::ofstream out(path, std::ios::binary | std::ios::trunc);
        out << content;
    }
}

TEST(Adversarial, JsonSampleRepository_MissingFileStartsEmpty)
{
    const std::string path = TempFilePath("sample_missing.json");
    std::filesystem::remove(path);

    Model::JsonSampleRepository repo(path);
    ASSERT_TRUE(repo.GetAll().empty());
}

TEST(Adversarial, JsonSampleRepository_CorruptedFileFallsBackToEmpty)
{
    const std::string path = TempFilePath("sample_corrupted.json");
    WriteRawText(path, "{ this is not valid JSON !!!");

    Model::JsonSampleRepository repo(path);
    ASSERT_TRUE(repo.GetAll().empty());

    // 폴백 이후에도 정상적으로 계속 사용할 수 있어야 한다 (크래시하지 않고 살아있는 상태).
    repo.Add(Model::Sample("S-999", "폴백 이후 등록", 1.0, 1.0, 0));
    ASSERT_EQ(static_cast<size_t>(1), repo.GetAll().size());

    std::filesystem::remove(path);
}

TEST(Adversarial, JsonOrderRepository_MissingFileStartsEmpty)
{
    const std::string path = TempFilePath("order_missing.json");
    std::filesystem::remove(path);

    Model::JsonOrderRepository repo(path);
    ASSERT_TRUE(repo.GetAll().empty());
}

TEST(Adversarial, JsonOrderRepository_CorruptedFileFallsBackToEmpty)
{
    const std::string path = TempFilePath("order_corrupted.json");
    WriteRawText(path, "[ { \"orderId\": \"ORD-0001\", ");

    Model::JsonOrderRepository repo(path);
    ASSERT_TRUE(repo.GetAll().empty());

    std::filesystem::remove(path);
}

TEST(Adversarial, JsonProductionLineRepository_MissingFileReturnsEmptyQueue)
{
    const std::string path = TempFilePath("production_queue_missing.json");
    std::filesystem::remove(path);

    Model::JsonProductionLineRepository repo(path);
    ASSERT_TRUE(repo.LoadQueue().empty());
}

TEST(Adversarial, JsonProductionLineRepository_CorruptedFileReturnsEmptyQueue)
{
    const std::string path = TempFilePath("production_queue_corrupted.json");
    WriteRawText(path, "not even close to json");

    Model::JsonProductionLineRepository repo(path);
    ASSERT_TRUE(repo.LoadQueue().empty());

    // 폴백 이후 SaveQueue가 정상적으로 파일을 새로 써서 복구할 수 있어야 한다.
    Model::ProductionJob job;
    job.orderId = "ORD-RECOVER";
    repo.SaveQueue({job});

    const auto jobs = repo.LoadQueue();
    ASSERT_EQ(static_cast<size_t>(1), jobs.size());
    ASSERT_EQ(std::string("ORD-RECOVER"), jobs[0].orderId);

    std::filesystem::remove(path);
}
