#pragma once
#include <string>

// Dtos.h가 아니라 별도 헤더로 분리한 이유: Sample이 InventoryLevel을 캐시 필드로
// 보유해야 하는데(재고 상태 재평가 시점에만 갱신, CLAUDE.md §2), Dtos.h는 Sample.h를
// include하고 있어 Dtos.h에 정의하면 순환 include가 된다.
namespace Model
{
    enum class InventoryLevel
    {
        Sufficient, // 여유
        Low,        // 부족
        Depleted    // 고갈
    };

    std::string ToString(InventoryLevel level);
    InventoryLevel InventoryLevelFromString(const std::string& text);
}
