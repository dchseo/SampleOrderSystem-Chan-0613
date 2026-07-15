#pragma once
#include <vector>
#include "../ProductionLine.h"

namespace Model
{
    // ProductionLine의 큐 전체(Current Job + 대기열, 순서 보존)를 단일 단위로 저장/조회하는
    // 인터페이스. Sample/Order Repository와 달리 개별 항목 CRUD가 아니라 "큐 스냅샷"을
    // 통째로 다루는 이유는, 생산 라인이 시스템 전체에 하나뿐인 단일 자원이기 때문이다
    // (CLAUDE.md "재고 및 생산 라인 처리 규칙 (상세) §3" 참고).
    class IProductionLineRepository
    {
    public:
        virtual ~IProductionLineRepository() = default;

        // jobsInOrder[0]이 Current Job, 나머지가 대기열 순서(FIFO)다.
        virtual void SaveQueue(const std::vector<ProductionJob>& jobsInOrder) = 0;
        virtual std::vector<ProductionJob> LoadQueue() const = 0;
    };
}
