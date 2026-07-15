#pragma once
#include <string>
#include <vector>
#include "../Model/Repository/ISampleRepository.h"

namespace Controller
{
    class SampleController
    {
    public:
        explicit SampleController(Model::ISampleRepository& sampleRepository);

        // 등록 실패 사유: 이미 존재하는 SampleId, 수율이 (0, 1] 범위 밖, 평균 생산시간이 0 이하.
        // (ConsoleMVC PoC 대비 확장 — CLAUDE.md "재고 및 생산 라인 처리 규칙" 참고)
        bool RegisterSample(const std::string& sampleId, const std::string& name, double avgProductionTime, double yieldRate);
        std::vector<Model::Sample> ListSamples() const;
        std::vector<Model::Sample> SearchSample(const std::string& keyword) const;

    private:
        Model::ISampleRepository& sampleRepository_;
    };
}
