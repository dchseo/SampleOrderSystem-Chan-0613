#pragma once
#include <random>
#include <string>
#include <vector>
#include "../Model/Order.h"
#include "../Model/Sample.h"

namespace Generator
{
    // 유효한 값 범위 내에서 랜덤 주문(Order)을 만들어 낸다.
    // 반드시 존재하는 시료(Sample)를 참조해야 하므로, availableSamples가 비어 있으면 생성할 수 없다.
    class RandomOrderGenerator
    {
    public:
        // existingOrderIds: 기존에 저장된(또는 이번 실행에서 이미 만든) 주문 ID 목록.
        // "ORD-" 접두사 뒤 숫자의 최댓값 다음부터 이어서 채번해 ID 충돌을 피한다.
        explicit RandomOrderGenerator(const std::vector<std::string>& existingOrderIds);

        // availableSamples 중 하나를 무작위로 골라 그 SampleId를 참조하는 주문을 만든다.
        // availableSamples가 비어 있으면 std::invalid_argument를 던진다.
        Model::Order Generate(const std::vector<Model::Sample>& availableSamples);

    private:
        int nextNumber_;
        std::mt19937 engine_;
        std::uniform_int_distribution<size_t> customerPicker_;
        std::uniform_int_distribution<int> quantityRange_;
        std::uniform_int_distribution<int> statusPicker_;
    };
}
