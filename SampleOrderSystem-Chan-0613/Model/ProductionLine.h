#pragma once
#include <chrono>
#include <deque>
#include <string>
#include <vector>
#include "json/JsonValue.h"

namespace Model
{
    // 생산 라인 큐에 등록되는 단일 생산 작업 (부족분에 대한 생산).
    // startTime은 이 작업이 Current(생산 중)가 된 실제 벽시계 시각이며, 실시간 정산
    // (CLAUDE.md "재고 및 생산 라인 처리 규칙 (상세) §3")의 기준이 된다.
    struct ProductionJob
    {
        std::string orderId;
        std::string sampleId;
        int shortageQuantity = 0;      // 부족분 = 주문 수량 - 재고
        int actualQuantity = 0;        // 실 생산량 = ceil(부족분 / 수율)
        double totalProductionTime = 0.0; // 평균 생산시간 * 실 생산량 (분)
        std::chrono::system_clock::time_point startTime{};

        // JSON 직렬화 편의 메서드. startTime은 epoch 밀리초 정수로 저장한다.
        json::JsonValue ToJson() const;
        static ProductionJob FromJson(const json::JsonValue& json);
    };

    // 단일 생산 라인, FIFO 큐(선입선출)로 부족분을 순차 생산한다.
    class ProductionLine
    {
    public:
        void Enqueue(const ProductionJob& job);
        bool HasCurrentJob() const noexcept;
        const ProductionJob& CurrentJob() const;
        ProductionJob CompleteCurrentJob();
        std::vector<ProductionJob> GetWaitingJobs() const;

        // JsonProductionLineRepository의 영속화(저장/로드)를 위한 벌크 접근.
        // 순서는 큐 순서와 동일하다 (맨 앞이 Current Job).
        std::vector<ProductionJob> GetAllJobsInOrder() const;
        void ReplaceAll(const std::vector<ProductionJob>& jobsInOrder);

    private:
        std::deque<ProductionJob> queue_;
    };
}
