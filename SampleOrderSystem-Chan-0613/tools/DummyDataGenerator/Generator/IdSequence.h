#pragma once
#include <string>
#include <vector>

namespace Generator
{
    // 기존 컬렉션의 ID(예: "S-005", "ORD-0012")에서 prefix 뒤의 숫자를 뽑아 다음 채번 값을 계산한다.
    // 접두사가 일치하지 않거나 숫자가 아니면 무시하므로, 사용자가 자유 형식으로 넣은 ID가 섞여 있어도 안전하다.
    inline int NextSequenceNumber(const std::vector<std::string>& existingIds, const std::string& prefix)
    {
        int maxNumber = 0;
        for (const auto& id : existingIds)
        {
            if (id.size() <= prefix.size() || id.compare(0, prefix.size(), prefix) != 0)
            {
                continue;
            }
            try
            {
                size_t consumed = 0;
                const int number = std::stoi(id.substr(prefix.size()), &consumed);
                if (consumed == id.size() - prefix.size())
                {
                    maxNumber = std::max(maxNumber, number);
                }
            }
            catch (...)
            {
                // 숫자로 해석할 수 없는 ID는 채번 대상에서 제외
            }
        }
        return maxNumber + 1;
    }

    inline std::string FormatId(const std::string& prefix, int number, int width)
    {
        std::string digits = std::to_string(number);
        if (static_cast<int>(digits.size()) < width)
        {
            digits.insert(0, width - digits.size(), '0');
        }
        return prefix + digits;
    }
}
