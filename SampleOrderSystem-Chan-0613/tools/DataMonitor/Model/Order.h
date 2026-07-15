#pragma once
#include <string>
#include "json/JsonValue.h"

namespace Model
{
    // JSON 직렬화 시 문자열로 매핑 (DataPersistence PoC 연계, CLAUDE.md 참고)
    enum class OrderStatus
    {
        Reserved,
        Producing,
        Confirmed,
        Released,
        Rejected
    };

    std::string ToString(OrderStatus status);
    OrderStatus OrderStatusFromString(const std::string& text);

    class Order
    {
    public:
        Order() = default;
        Order(std::string orderId, std::string sampleId, std::string customerName, int quantity);

        const std::string& GetOrderId() const noexcept;
        const std::string& GetSampleId() const noexcept;
        const std::string& GetCustomerName() const noexcept;
        int GetQuantity() const noexcept;
        OrderStatus GetStatus() const noexcept;

        void SetStatus(OrderStatus status) noexcept;

        // JSON 직렬화 편의 메서드 (DataPersistence PoC) — 필드/JSON 키 매핑은 CLAUDE.md 참고
        json::JsonValue ToJson() const;
        static Order FromJson(const json::JsonValue& json);

    private:
        std::string orderId_;
        std::string sampleId_;
        std::string customerName_;
        int quantity_ = 0;
        OrderStatus status_ = OrderStatus::Reserved;
    };
}
