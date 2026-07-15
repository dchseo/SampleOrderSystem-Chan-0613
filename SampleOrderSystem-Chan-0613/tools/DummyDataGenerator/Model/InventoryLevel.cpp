#include "InventoryLevel.h"
#include <stdexcept>

namespace Model
{
    std::string ToString(InventoryLevel level)
    {
        switch (level)
        {
        case InventoryLevel::Sufficient: return "SUFFICIENT";
        case InventoryLevel::Low: return "LOW";
        case InventoryLevel::Depleted: return "DEPLETED";
        }
        return "UNKNOWN";
    }

    InventoryLevel InventoryLevelFromString(const std::string& text)
    {
        if (text == "SUFFICIENT") return InventoryLevel::Sufficient;
        if (text == "LOW") return InventoryLevel::Low;
        if (text == "DEPLETED") return InventoryLevel::Depleted;
        throw std::invalid_argument("unknown InventoryLevel: " + text);
    }
}
