#pragma once
#include <random>
#include <string>
#include <vector>
#include "../Model/Sample.h"

namespace Generator
{
    // 유효한 값 범위 내에서 랜덤 시료(Sample)를 만들어 낸다.
    // 파일 I/O나 콘솔 출력에는 관여하지 않으며, 완성된 Sample 객체만 반환한다.
    class RandomSampleGenerator
    {
    public:
        // existingSampleIds: 기존에 저장된(또는 이번 실행에서 이미 만든) 시료 ID 목록.
        // "S-" 접두사 뒤 숫자의 최댓값 다음부터 이어서 채번해 ID 충돌을 피한다.
        explicit RandomSampleGenerator(const std::vector<std::string>& existingSampleIds);

        Model::Sample Generate();

    private:
        int nextNumber_;
        std::mt19937 engine_;
        std::uniform_int_distribution<size_t> namePicker_;
        std::uniform_real_distribution<double> productionTimeRange_;
        std::uniform_real_distribution<double> yieldRange_;
        std::uniform_int_distribution<int> stockRange_;
    };
}
