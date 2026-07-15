#include "RandomSampleGenerator.h"
#include "IdSequence.h"
#include <array>

namespace Generator
{
    namespace
    {
        // 기능 명세서(반도체 시료 생산주문관리) 예시 화면에 등장하는 시료명을 후보로 사용한다.
        constexpr std::array<const char*, 8> kSampleNames = {
            "실리콘 웨이퍼-8인치",
            "GaN 에피택셜-4인치",
            "SiC 파워기판-6인치",
            "포토레지스트-PR7",
            "산화막 웨이퍼-SiO2",
            "GaAs 에피택셜-4인치",
            "InP 파워기판-6인치",
            "질화막 웨이퍼-Si3N4",
        };

        constexpr double kMinProductionTime = 0.1; // min/ea
        constexpr double kMaxProductionTime = 1.5;
        constexpr double kMinYield = 0.70;          // 기능 명세: 수율 = 정상 시료 / 총 생산 시료
        constexpr double kMaxYield = 0.99;
        constexpr int kMinInitialStock = 0;
        constexpr int kMaxInitialStock = 500;
    }

    RandomSampleGenerator::RandomSampleGenerator(const std::vector<std::string>& existingSampleIds)
        : nextNumber_(NextSequenceNumber(existingSampleIds, "S-"))
        , engine_(std::random_device{}())
        , namePicker_(0, kSampleNames.size() - 1)
        , productionTimeRange_(kMinProductionTime, kMaxProductionTime)
        , yieldRange_(kMinYield, kMaxYield)
        , stockRange_(kMinInitialStock, kMaxInitialStock)
    {
    }

    Model::Sample RandomSampleGenerator::Generate()
    {
        const std::string sampleId = FormatId("S-", nextNumber_++, 3);
        const std::string name = kSampleNames[namePicker_(engine_)];
        const double avgProductionTime = productionTimeRange_(engine_);
        const double yieldRate = yieldRange_(engine_);
        const int initialStock = stockRange_(engine_);

        Model::Sample sample(sampleId, name, avgProductionTime, yieldRate, initialStock);

        // SampleOrderSystem-Chan-0613 본 시스템은 Sample에 재고 상태(여유/부족/고갈) 캐시를
        // 추가했다(DummyDataGenerator PoC 원본에는 없던 필드). 정상 흐름에서는 OrderController가
        // 주문 승인/생산 완료 시점에 갱신하지만, 더미 생성은 그 흐름을 거치지 않고 재고를 직접
        // 주입하므로 여기서 재고 유무만으로 최소한의 합리적인 값을 대신 채워 둔다 — 그렇지 않으면
        // 재고가 있는 더미 시료도 DataMonitor 대시보드에 "고갈"로 잘못 표시된다.
        sample.SetInventoryLevel(initialStock > 0 ? Model::InventoryLevel::Sufficient : Model::InventoryLevel::Depleted);

        return sample;
    }
}
