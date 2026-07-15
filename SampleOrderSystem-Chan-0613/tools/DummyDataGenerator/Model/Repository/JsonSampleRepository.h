#pragma once
#include <string>
#include <unordered_map>
#include "ISampleRepository.h"

namespace Model
{
    // ISampleRepository의 JSON 파일 기반 구현체.
    // 저장 정책: write-through — Add/Update/Remove 호출 시마다 즉시 파일 전체를 다시 저장한다.
    // 필드 <-> JSON 키 매핑 및 파일 단위 규약은 CLAUDE.md 참고.
    class JsonSampleRepository : public ISampleRepository
    {
    public:
        explicit JsonSampleRepository(std::string filePath);

        void Add(const Sample& sample) override;
        bool Update(const Sample& sample) override;
        std::optional<Sample> FindById(const std::string& sampleId) const override;
        std::vector<Sample> FindByNameContains(const std::string& keyword) const override;
        std::vector<Sample> GetAll() const override;

        // 인터페이스에는 없지만 CRUD 데모/향후 관리 도구를 위해 제공하는 삭제 기능.
        bool Remove(const std::string& sampleId);

    private:
        void Load();
        void Persist() const;

        std::string filePath_;
        std::unordered_map<std::string, Sample> samplesById_;
    };
}
