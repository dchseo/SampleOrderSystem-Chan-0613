#include "JsonOrderRepository.h"
#include "json/JsonDocument.h"
#include "json/JsonException.h"
#include <iostream>

namespace Model
{
    JsonOrderRepository::JsonOrderRepository(std::string filePath)
        : filePath_(std::move(filePath))
    {
        Load();
    }

    void JsonOrderRepository::Load()
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
                const Order order = Order::FromJson(entry);
                ordersById_.emplace(order.GetOrderId(), order);
            }
        }
        catch (const json::JsonException& ex)
        {
            std::cerr << "[JsonOrderRepository] " << filePath_ << " 파싱 실패, 빈 목록으로 시작합니다: "
                       << ex.what() << "\n";
            ordersById_.clear();
        }
    }

    void JsonOrderRepository::Persist() const
    {
        auto root = json::JsonValue::makeArray();
        for (const auto& [id, order] : ordersById_)
        {
            root.push_back(order.ToJson());
        }

        json::JsonDocument document(std::move(root));
        document.saveToFile(filePath_, { .pretty = true });
    }

    void JsonOrderRepository::Add(const Order& order)
    {
        ordersById_[order.GetOrderId()] = order;
        Persist();
    }

    bool JsonOrderRepository::Update(const Order& order)
    {
        const auto it = ordersById_.find(order.GetOrderId());
        if (it == ordersById_.end())
        {
            return false;
        }
        it->second = order;
        Persist();
        return true;
    }

    std::optional<Order> JsonOrderRepository::FindById(const std::string& orderId) const
    {
        const auto it = ordersById_.find(orderId);
        if (it == ordersById_.end())
        {
            return std::nullopt;
        }
        return it->second;
    }

    std::vector<Order> JsonOrderRepository::FindByStatus(OrderStatus status) const
    {
        std::vector<Order> result;
        for (const auto& [id, order] : ordersById_)
        {
            if (order.GetStatus() == status)
            {
                result.push_back(order);
            }
        }
        return result;
    }

    std::vector<Order> JsonOrderRepository::GetAll() const
    {
        std::vector<Order> result;
        result.reserve(ordersById_.size());
        for (const auto& [id, order] : ordersById_)
        {
            result.push_back(order);
        }
        return result;
    }

    bool JsonOrderRepository::Remove(const std::string& orderId)
    {
        const auto erased = ordersById_.erase(orderId);
        if (erased == 0)
        {
            return false;
        }
        Persist();
        return true;
    }
}
