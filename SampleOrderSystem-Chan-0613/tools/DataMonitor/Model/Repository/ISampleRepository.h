#pragma once
#include <optional>
#include <string>
#include <vector>
#include "../Sample.h"

namespace Model
{
    // DataPersistence PoC가 JsonSampleRepository로 교체할 자리 (CLAUDE.md 참고)
    class ISampleRepository
    {
    public:
        virtual ~ISampleRepository() = default;

        virtual void Add(const Sample& sample) = 0;
        virtual bool Update(const Sample& sample) = 0;
        virtual std::optional<Sample> FindById(const std::string& sampleId) const = 0;
        virtual std::vector<Sample> FindByNameContains(const std::string& keyword) const = 0;
        virtual std::vector<Sample> GetAll() const = 0;
    };
}
