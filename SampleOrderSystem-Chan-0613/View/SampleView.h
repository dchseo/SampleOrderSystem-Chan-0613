#pragma once
#include <string>
#include <vector>
#include "../Model/Sample.h"

namespace View
{
    struct SampleRegistrationInput
    {
        std::string sampleId;
        std::string name;
        double avgProductionTime = 0.0;
        double yieldRate = 0.0;
    };

    class SampleView
    {
    public:
        void ShowMenu() const;
        SampleRegistrationInput PromptSampleRegistration() const;
        std::string PromptSearchKeyword() const;
        void ShowSampleList(const std::vector<Model::Sample>& samples) const;
        void ShowRegistrationResult(bool success, const std::string& sampleId) const;
    };
}
