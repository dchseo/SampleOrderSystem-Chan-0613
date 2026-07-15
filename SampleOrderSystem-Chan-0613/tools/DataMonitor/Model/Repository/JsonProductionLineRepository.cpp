#include "JsonProductionLineRepository.h"
#include "json/JsonDocument.h"
#include "json/JsonException.h"
#include <iostream>

namespace Model
{
    JsonProductionLineRepository::JsonProductionLineRepository(std::string filePath)
        : filePath_(std::move(filePath))
    {
    }

    void JsonProductionLineRepository::SaveQueue(const std::vector<ProductionJob>& jobsInOrder)
    {
        auto root = json::JsonValue::makeArray();
        for (const auto& job : jobsInOrder)
        {
            root.push_back(job.ToJson());
        }

        json::JsonDocument document(std::move(root));
        document.saveToFile(filePath_, {.pretty = true});
    }

    std::vector<ProductionJob> JsonProductionLineRepository::LoadQueue() const
    {
        std::vector<ProductionJob> jobs;

        json::JsonDocument document;
        std::string error;
        if (!json::JsonDocument::tryLoadFromFile(filePath_, document, error))
        {
            // 파일이 없거나(최초 실행) 손상된 경우 빈 큐로 시작한다.
            return jobs;
        }

        try
        {
            for (const auto& entry : document.root().asArray())
            {
                jobs.push_back(ProductionJob::FromJson(entry));
            }
        }
        catch (const json::JsonException& ex)
        {
            std::cerr << "[JsonProductionLineRepository] " << filePath_
                      << " 파싱 실패, 빈 큐로 시작합니다: " << ex.what() << "\n";
            jobs.clear();
        }

        return jobs;
    }
}
