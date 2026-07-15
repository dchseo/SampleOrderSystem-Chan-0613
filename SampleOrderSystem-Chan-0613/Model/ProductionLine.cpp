#include "ProductionLine.h"
#include <stdexcept>

namespace Model
{
    json::JsonValue ProductionJob::ToJson() const
    {
        auto json = json::JsonValue::makeObject();
        json.set("orderId", orderId);
        json.set("sampleId", sampleId);
        json.set("shortageQuantity", shortageQuantity);
        json.set("actualQuantity", actualQuantity);
        json.set("totalProductionTime", totalProductionTime);

        const auto epochMs = std::chrono::duration_cast<std::chrono::milliseconds>(
            startTime.time_since_epoch())
                                 .count();
        json.set("startTimeEpochMs", static_cast<int64_t>(epochMs));
        return json;
    }

    ProductionJob ProductionJob::FromJson(const json::JsonValue& json)
    {
        ProductionJob job;
        job.orderId = json["orderId"].asString();
        job.sampleId = json["sampleId"].asString();
        job.shortageQuantity = static_cast<int>(json["shortageQuantity"].asInt());
        job.actualQuantity = static_cast<int>(json["actualQuantity"].asInt());
        job.totalProductionTime = json["totalProductionTime"].asDouble();

        const auto epochMs = json["startTimeEpochMs"].asInt();
        job.startTime = std::chrono::system_clock::time_point(std::chrono::milliseconds(epochMs));
        return job;
    }

    void ProductionLine::Enqueue(const ProductionJob& job)
    {
        queue_.push_back(job);
    }

    bool ProductionLine::HasCurrentJob() const noexcept
    {
        return !queue_.empty();
    }

    const ProductionJob& ProductionLine::CurrentJob() const
    {
        if (queue_.empty())
        {
            throw std::out_of_range("no current production job");
        }
        return queue_.front();
    }

    ProductionJob ProductionLine::CompleteCurrentJob()
    {
        if (queue_.empty())
        {
            throw std::out_of_range("no current production job");
        }
        ProductionJob job = queue_.front();
        queue_.pop_front();
        return job;
    }

    void ProductionLine::SetCurrentJobStartTime(std::chrono::system_clock::time_point startTime)
    {
        if (queue_.empty())
        {
            throw std::out_of_range("no current production job");
        }
        queue_.front().startTime = startTime;
    }

    std::vector<ProductionJob> ProductionLine::GetWaitingJobs() const
    {
        std::vector<ProductionJob> waiting;
        if (queue_.size() > 1)
        {
            waiting.assign(queue_.begin() + 1, queue_.end());
        }
        return waiting;
    }

    std::vector<ProductionJob> ProductionLine::GetAllJobsInOrder() const
    {
        return std::vector<ProductionJob>(queue_.begin(), queue_.end());
    }

    void ProductionLine::ReplaceAll(const std::vector<ProductionJob>& jobsInOrder)
    {
        queue_.assign(jobsInOrder.begin(), jobsInOrder.end());
    }
}
