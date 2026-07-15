#include "SampleView.h"
#include <iostream>
#include <limits>
#include <sstream>
#include "ConsoleFormat.h"

namespace View
{
    void SampleView::ShowMenu() const
    {
        std::cout << "\n--- [1] 시료 관리 ---\n";
        std::cout << "[1] 시료 등록   [2] 시료 목록   [3] 시료 검색   [0] 뒤로\n";
        std::cout << "선택 > ";
    }

    SampleRegistrationInput SampleView::PromptSampleRegistration() const
    {
        SampleRegistrationInput input;
        std::cout << "시료 ID > ";
        std::getline(std::cin, input.sampleId);
        std::cout << "이름 > ";
        std::getline(std::cin, input.name);
        std::cout << "평균 생산시간(분/ea) > ";
        std::cin >> input.avgProductionTime;
        std::cout << "수율(0~1) > ";
        std::cin >> input.yieldRate;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        return input;
    }

    std::string SampleView::PromptSearchKeyword() const
    {
        std::cout << "검색어(이름) > ";
        std::string keyword;
        std::getline(std::cin, keyword);
        return keyword;
    }

    void SampleView::ShowSampleList(const std::vector<Model::Sample>& samples) const
    {
        std::cout << "\n등록 시료 목록 (총 " << samples.size() << "종)\n";
        std::cout << PadRight("ID", 10)
            << PadRight("시료명", 18)
            << PadRight("평균생산시간", 16)
            << PadRight("수율", 8)
            << "재고" << '\n';

        for (const auto& sample : samples)
        {
            std::ostringstream avgProductionTime;
            avgProductionTime << sample.GetAvgProductionTime();
            std::ostringstream yield;
            yield << sample.GetYield();

            std::cout << PadRight(sample.GetSampleId(), 10)
                << PadRight(sample.GetName(), 18)
                << PadRight(avgProductionTime.str(), 16)
                << PadRight(yield.str(), 8)
                << sample.GetStock() << '\n';
        }
    }

    void SampleView::ShowRegistrationResult(bool success, const std::string& sampleId) const
    {
        if (success)
        {
            std::cout << "시료 등록 완료. (ID: " << sampleId << ")\n";
        }
        else
        {
            std::cout << "시료 등록 실패: 이미 존재하는 시료 ID입니다. (ID: " << sampleId << ")\n";
        }
    }
}
