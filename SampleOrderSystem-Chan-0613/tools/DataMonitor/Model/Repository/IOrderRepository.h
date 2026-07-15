#pragma once
#include <optional>
#include <string>
#include <vector>
#include "../Order.h"

namespace Model
{
    // DataPersistence PoC가 JsonOrderRepository로 교체할 자리 (CLAUDE.md 참고)
    class IOrderRepository
    {
    public:
        virtual ~IOrderRepository() = default;

        virtual void Add(const Order& order) = 0;
        virtual bool Update(const Order& order) = 0;
        virtual std::optional<Order> FindById(const std::string& orderId) const = 0;
        virtual std::vector<Order> FindByStatus(OrderStatus status) const = 0;
        virtual std::vector<Order> GetAll() const = 0;
    };
}
