#pragma once
#include <string>
#include "IProductionLineRepository.h"

namespace Model
{
    // IProductionLineRepository의 JSON 파일 기반 구현체.
    // 저장 정책: write-through — SaveQueue 호출 시마다 즉시 파일 전체를 다시 저장한다.
    // Sample/Order Repository와 달리 생성자에서 자동으로 로드하지 않는다 — 생산 라인은
    // ProductionLine이라는 별도 객체가 소유하는 단일 자원이므로, "언제 로드해서 ProductionLine에
    // 반영할지"는 호출부(Controller/main.cpp)가 명시적으로 결정한다 (예: 앱 시작 시 1회
    // LoadQueue() 결과를 ProductionLine::ReplaceAll()에 전달).
    class JsonProductionLineRepository : public IProductionLineRepository
    {
    public:
        explicit JsonProductionLineRepository(std::string filePath);

        void SaveQueue(const std::vector<ProductionJob>& jobsInOrder) override;
        std::vector<ProductionJob> LoadQueue() const override;

    private:
        std::string filePath_;
    };
}
