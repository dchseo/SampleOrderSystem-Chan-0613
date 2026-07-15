#pragma once
#include <string>
#include "json/JsonValue.h"

namespace Model
{
    class Sample
    {
    public:
        Sample() = default;
        Sample(std::string sampleId, std::string name, double avgProductionTime, double yieldRate, int initialStock = 0);

        const std::string& GetSampleId() const noexcept;
        const std::string& GetName() const noexcept;
        double GetAvgProductionTime() const noexcept;
        double GetYield() const noexcept;
        int GetStock() const noexcept;

        void IncreaseStock(int amount);
        bool DecreaseStock(int amount);

        // JSON 직렬화 편의 메서드 (DataPersistence PoC) — 필드/JSON 키 매핑은 CLAUDE.md 참고
        json::JsonValue ToJson() const;
        static Sample FromJson(const json::JsonValue& json);

    private:
        std::string sampleId_;
        std::string name_;
        double avgProductionTime_ = 0.0;
        double yield_ = 1.0;
        int stock_ = 0;
    };
}
