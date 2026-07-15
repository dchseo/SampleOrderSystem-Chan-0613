#include "Order.h"
#include <stdexcept>

namespace Model
{
    Order::Order(std::string orderId, std::string sampleId, std::string customerName, int quantity)
        : orderId_(std::move(orderId))
        , sampleId_(std::move(sampleId))
        , customerName_(std::move(customerName))
        , quantity_(quantity)
        , status_(OrderStatus::Reserved)
    {
    }

    const std::string& Order::GetOrderId() const noexcept { return orderId_; }
    const std::string& Order::GetSampleId() const noexcept { return sampleId_; }
    const std::string& Order::GetCustomerName() const noexcept { return customerName_; }
    int Order::GetQuantity() const noexcept { return quantity_; }
    OrderStatus Order::GetStatus() const noexcept { return status_; }

    void Order::SetStatus(OrderStatus status) noexcept { status_ = status; }

    std::string ToString(OrderStatus status)
    {
        switch (status)
        {
        case OrderStatus::Reserved: return "RESERVED";
        case OrderStatus::Producing: return "PRODUCING";
        case OrderStatus::Confirmed: return "CONFIRMED";
        case OrderStatus::Released: return "RELEASED";
        case OrderStatus::Rejected: return "REJECTED";
        }
        return "UNKNOWN";
    }

    OrderStatus OrderStatusFromString(const std::string& text)
    {
        if (text == "RESERVED") return OrderStatus::Reserved;
        if (text == "PRODUCING") return OrderStatus::Producing;
        if (text == "CONFIRMED") return OrderStatus::Confirmed;
        if (text == "RELEASED") return OrderStatus::Released;
        if (text == "REJECTED") return OrderStatus::Rejected;
        throw std::invalid_argument("unknown OrderStatus: " + text);
    }

    json::JsonValue Order::ToJson() const
    {
        auto json = json::JsonValue::makeObject();
        json.set("orderId", orderId_);
        json.set("sampleId", sampleId_);
        json.set("customerName", customerName_);
        json.set("quantity", quantity_);
        json.set("status", ToString(status_));
        return json;
    }

    Order Order::FromJson(const json::JsonValue& json)
    {
        Order order;
        order.orderId_ = json["orderId"].asString();
        order.sampleId_ = json["sampleId"].asString();
        order.customerName_ = json["customerName"].asString();
        order.quantity_ = static_cast<int>(json["quantity"].asInt());
        order.status_ = OrderStatusFromString(json["status"].asString());
        return order;
    }
}
