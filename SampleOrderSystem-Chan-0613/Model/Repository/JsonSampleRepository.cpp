#include "JsonSampleRepository.h"
#include "json/JsonDocument.h"
#include "json/JsonException.h"
#include <iostream>

namespace Model
{
    JsonSampleRepository::JsonSampleRepository(std::string filePath)
        : filePath_(std::move(filePath))
    {
        Load();
    }

    void JsonSampleRepository::Load()
    {
        json::JsonDocument document;
        std::string error;
        if (!json::JsonDocument::tryLoadFromFile(filePath_, document, error))
        {
            // 파일이 없거나(최초 실행) 손상된 경우 빈 컬렉션으로 시작한다.
            return;
        }

        try
        {
            for (const auto& entry : document.root().asArray())
            {
                const Sample sample = Sample::FromJson(entry);
                samplesById_.emplace(sample.GetSampleId(), sample);
            }
        }
        catch (const json::JsonException& ex)
        {
            std::cerr << "[JsonSampleRepository] " << filePath_ << " 파싱 실패, 빈 목록으로 시작합니다: "
                       << ex.what() << "\n";
            samplesById_.clear();
        }
    }

    void JsonSampleRepository::Persist() const
    {
        auto root = json::JsonValue::makeArray();
        for (const auto& [id, sample] : samplesById_)
        {
            root.push_back(sample.ToJson());
        }

        json::JsonDocument document(std::move(root));
        document.saveToFile(filePath_, { .pretty = true });
    }

    void JsonSampleRepository::Add(const Sample& sample)
    {
        samplesById_[sample.GetSampleId()] = sample;
        Persist();
    }

    bool JsonSampleRepository::Update(const Sample& sample)
    {
        const auto it = samplesById_.find(sample.GetSampleId());
        if (it == samplesById_.end())
        {
            return false;
        }
        it->second = sample;
        Persist();
        return true;
    }

    std::optional<Sample> JsonSampleRepository::FindById(const std::string& sampleId) const
    {
        const auto it = samplesById_.find(sampleId);
        if (it == samplesById_.end())
        {
            return std::nullopt;
        }
        return it->second;
    }

    std::vector<Sample> JsonSampleRepository::FindByNameContains(const std::string& keyword) const
    {
        std::vector<Sample> result;
        for (const auto& [id, sample] : samplesById_)
        {
            if (sample.GetName().find(keyword) != std::string::npos)
            {
                result.push_back(sample);
            }
        }
        return result;
    }

    std::vector<Sample> JsonSampleRepository::GetAll() const
    {
        std::vector<Sample> result;
        result.reserve(samplesById_.size());
        for (const auto& [id, sample] : samplesById_)
        {
            result.push_back(sample);
        }
        return result;
    }

    bool JsonSampleRepository::Remove(const std::string& sampleId)
    {
        const auto erased = samplesById_.erase(sampleId);
        if (erased == 0)
        {
            return false;
        }
        Persist();
        return true;
    }
}
