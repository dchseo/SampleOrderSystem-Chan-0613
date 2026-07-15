#include "SampleController.h"

namespace Controller
{
    SampleController::SampleController(Model::ISampleRepository& sampleRepository)
        : sampleRepository_(sampleRepository)
    {
    }

    bool SampleController::RegisterSample(const std::string& sampleId, const std::string& name, double avgProductionTime, double yieldRate)
    {
        if (sampleRepository_.FindById(sampleId).has_value())
        {
            return false;
        }
        // 수율 정의: 정상 시료 수 / 총 생산 시료 수 (0~1). 0 이하이면 실 생산량 계산에서
        // 0으로 나누기가 발생하므로 등록 단계에서 막는다.
        if (yieldRate <= 0.0 || yieldRate > 1.0)
        {
            return false;
        }
        if (avgProductionTime <= 0.0)
        {
            return false;
        }
        // 재고는 입력받지 않는다 — 신규 시료는 항상 0에서 시작한다 (CLAUDE.md "시스템 규칙" 참고).
        sampleRepository_.Add(Model::Sample(sampleId, name, avgProductionTime, yieldRate, 0));
        return true;
    }

    std::vector<Model::Sample> SampleController::ListSamples() const
    {
        return sampleRepository_.GetAll();
    }

    std::vector<Model::Sample> SampleController::SearchSample(const std::string& keyword) const
    {
        return sampleRepository_.FindByNameContains(keyword);
    }
}
