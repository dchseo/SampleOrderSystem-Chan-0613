#pragma once
#include <string>
#include "InventoryLevel.h"
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

        // 재고 상태(여유/부족/고갈) 캐시. 매 조회 시 재계산하지 않고, 주문 승인/생산 완료
        // 시점에만 OrderController가 갱신한다 (CLAUDE.md "재고 및 생산 라인 처리 규칙 (상세) §2").
        // 신규 등록 시료는 재고가 항상 0에서 시작하므로 기본값은 Depleted다.
        InventoryLevel GetInventoryLevel() const noexcept;
        void SetInventoryLevel(InventoryLevel level) noexcept;

        // JSON 직렬화 편의 메서드 (DataPersistence PoC) — 필드/JSON 키 매핑은 CLAUDE.md 참고
        json::JsonValue ToJson() const;
        static Sample FromJson(const json::JsonValue& json);

    private:
        std::string sampleId_;
        std::string name_;
        double avgProductionTime_ = 0.0;
        double yield_ = 1.0;
        int stock_ = 0;
        InventoryLevel inventoryLevel_ = InventoryLevel::Depleted;
    };
}
