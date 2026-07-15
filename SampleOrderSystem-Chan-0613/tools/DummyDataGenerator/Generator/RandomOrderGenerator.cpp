#include "RandomOrderGenerator.h"
#include "IdSequence.h"
#include <array>
#include <stdexcept>

namespace Generator
{
    namespace
    {
        // 기능 명세서(반도체 시료 생산주문관리) 예시 화면에 등장하는 고객사/연구기관명을 후보로 사용한다.
        constexpr std::array<const char*, 8> kCustomerNames = {
            "삼성전자 파운드리",
            "SK하이닉스",
            "LG이노텍",
            "DB하이텍",
            "한국전자통신연구원",
            "KAIST 반도체연구실",
            "포항공대 나노공정실",
            "네패스",
        };

        constexpr int kMinQuantity = 1;
        constexpr int kMaxQuantity = 300;

        // 모니터링(4가지 상태) / 출고 처리(CONFIRMED) 화면을 모두 테스트할 수 있도록,
        // REJECTED를 포함한 5가지 상태에 고르게 분포시킨다.
        constexpr std::array<Model::OrderStatus, 5> kStatuses = {
            Model::OrderStatus::Reserved,
            Model::OrderStatus::Producing,
            Model::OrderStatus::Confirmed,
            Model::OrderStatus::Released,
            Model::OrderStatus::Rejected,
        };
    }

    RandomOrderGenerator::RandomOrderGenerator(const std::vector<std::string>& existingOrderIds)
        : nextNumber_(NextSequenceNumber(existingOrderIds, "ORD-"))
        , engine_(std::random_device{}())
        , customerPicker_(0, kCustomerNames.size() - 1)
        , quantityRange_(kMinQuantity, kMaxQuantity)
        , statusPicker_(0, static_cast<int>(kStatuses.size()) - 1)
    {
    }

    Model::Order RandomOrderGenerator::Generate(const std::vector<Model::Sample>& availableSamples)
    {
        if (availableSamples.empty())
        {
            throw std::invalid_argument("주문 더미를 생성하려면 참조할 시료가 최소 1건 이상 필요합니다");
        }

        std::uniform_int_distribution<size_t> samplePicker(0, availableSamples.size() - 1);
        const std::string& sampleId = availableSamples[samplePicker(engine_)].GetSampleId();

        const std::string orderId = FormatId("ORD-", nextNumber_++, 4);
        const std::string customerName = kCustomerNames[customerPicker_(engine_)];
        const int quantity = quantityRange_(engine_);

        Model::Order order(orderId, sampleId, customerName, quantity);
        order.SetStatus(kStatuses[statusPicker_(engine_)]);
        return order;
    }
}
