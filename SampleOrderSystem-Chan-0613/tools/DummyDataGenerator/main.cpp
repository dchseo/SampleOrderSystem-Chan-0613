// DummyDataGenerator — Dummy 데이터 생성 Tool (DummyDataGenerator-Chan-0613 PoC 이식).
//
// SampleOrderSystem 본 애플리케이션과 동일한 data/ 폴더(../../data/samples.json,
// ../../data/orders.json)에 JsonSampleRepository/JsonOrderRepository의 Add(...)를 통해
// 더미 시료/주문 데이터를 실제로 추가한다. 본 애플리케이션이 쓰는 파일을 그대로 공유하므로,
// 별도 실행 파일로 빌드해 필요할 때만 실행한다 (평소 실행되는 도구가 아님).
//
// 사용법: DummyDataGenerator.exe [생성할 시료 수] [생성할 주문 수]
//   인자를 생략하면 기본값(시료 5건, 주문 10건)을 사용한다.
//
// 제한: 생성되는 주문의 상태(RESERVED/PRODUCING/CONFIRMED/RELEASED/REJECTED)는 무작위로
// 배정되며, 실제 OrderController.ApproveOrder()를 거치지 않는다. 따라서 상태가 PRODUCING인
// 더미 주문이라도 production_queue.json에는 대응하는 생산 작업이 생기지 않는다 — 이 도구는
// 모니터링/출고 처리 화면을 다양한 상태로 테스트하기 위한 용도이며, 생산 라인 정합성까지
// 보장하지는 않는다.

#include "Generator/RandomOrderGenerator.h"
#include "Generator/RandomSampleGenerator.h"
#include "Model/Order.h"
#include "Model/Repository/JsonOrderRepository.h"
#include "Model/Repository/JsonSampleRepository.h"
#include "Model/Repository/ISampleRepository.h"
#include "Model/Sample.h"

#include <iostream>
#include <string>
#include <vector>

#ifdef _WIN32
#include <Windows.h>
#endif

using namespace Model;

namespace
{
    constexpr const char* kSamplesFilePath = "../../data/samples.json";
    constexpr const char* kOrdersFilePath = "../../data/orders.json";
    constexpr int kDefaultSampleCount = 5;
    constexpr int kDefaultOrderCount = 10;

    int ParseCountArg(const char* text, int defaultValue)
    {
        try
        {
            size_t consumed = 0;
            const int value = std::stoi(text, &consumed);
            if (consumed == std::string(text).size() && value >= 0)
            {
                return value;
            }
        }
        catch (...)
        {
            // 파싱 실패 시 기본값 사용
        }
        std::cout << "  (\"" << text << "\"을(를) 개수로 해석할 수 없어 기본값 " << defaultValue << "을 사용합니다)\n";
        return defaultValue;
    }

    std::vector<std::string> CollectSampleIds(const std::vector<Sample>& samples)
    {
        std::vector<std::string> ids;
        ids.reserve(samples.size());
        for (const auto& sample : samples)
        {
            ids.push_back(sample.GetSampleId());
        }
        return ids;
    }

    std::vector<std::string> CollectOrderIds(const std::vector<Order>& orders)
    {
        std::vector<std::string> ids;
        ids.reserve(orders.size());
        for (const auto& order : orders)
        {
            ids.push_back(order.GetOrderId());
        }
        return ids;
    }
}

int main(int argc, char** argv)
{
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    const int sampleCount = argc > 1 ? ParseCountArg(argv[1], kDefaultSampleCount) : kDefaultSampleCount;
    const int orderCount = argc > 2 ? ParseCountArg(argv[2], kDefaultOrderCount) : kDefaultOrderCount;

    std::cout << "DummyDataGenerator — Dummy 데이터 생성 Tool\n";
    std::cout << "시료 " << sampleCount << "건, 주문 " << orderCount << "건을 생성하여 "
               << kSamplesFilePath << ", " << kOrdersFilePath << "에 추가합니다.\n\n";

    JsonSampleRepository sampleRepository(kSamplesFilePath);
    JsonOrderRepository orderRepository(kOrdersFilePath);

    // 1. 시료(Sample) 더미 생성 — 기존 ID와 겹치지 않도록 이어서 채번한다.
    std::vector<Sample> existingSamples = sampleRepository.GetAll();
    Generator::RandomSampleGenerator sampleGenerator(CollectSampleIds(existingSamples));

    int createdSampleCount = 0;
    for (int i = 0; i < sampleCount; ++i)
    {
        const Sample sample = sampleGenerator.Generate();
        sampleRepository.Add(sample);
        existingSamples.push_back(sample);
        std::cout << "  [시료 추가] " << sample.GetSampleId() << " " << sample.GetName()
                   << " (재고 " << sample.GetStock() << ")\n";
        ++createdSampleCount;
    }

    // 2. 주문(Order) 더미 생성 — 반드시 존재하는 시료를 참조해야 하므로,
    //    이번에 새로 만든 시료를 포함한 전체 시료 목록에서 무작위로 선택한다.
    int createdOrderCount = 0;
    int skippedOrderCount = 0;
    if (existingSamples.empty())
    {
        std::cout << "\n  참조할 시료가 하나도 없어 주문 더미는 생성하지 않았습니다"
                   << " (시료를 먼저 생성해야 합니다).\n";
        skippedOrderCount = orderCount;
    }
    else
    {
        Generator::RandomOrderGenerator orderGenerator(CollectOrderIds(orderRepository.GetAll()));
        std::cout << "\n";
        for (int i = 0; i < orderCount; ++i)
        {
            const Order order = orderGenerator.Generate(existingSamples);
            orderRepository.Add(order);
            std::cout << "  [주문 추가] " << order.GetOrderId() << " " << order.GetSampleId()
                       << " " << order.GetCustomerName() << " 수량=" << order.GetQuantity()
                       << " 상태=" << ToString(order.GetStatus()) << "\n";
            ++createdOrderCount;
        }
    }

    std::cout << "\n생성 완료: 시료 " << createdSampleCount << "건, 주문 " << createdOrderCount << "건"
               << (skippedOrderCount > 0 ? (" (주문 " + std::to_string(skippedOrderCount) + "건 스킵)") : "")
               << "\n";
    std::cout << "(SampleOrderSystem을 재실행해도 " << kSamplesFilePath << ", " << kOrdersFilePath
               << "에 추가된 더미 데이터가 그대로 보이는지 확인하세요)\n";

    return 0;
}
