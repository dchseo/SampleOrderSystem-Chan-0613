#include "InventoryCalculator.h"
#include <cmath>
#include <stdexcept>

namespace Model
{
    int CalculateShortage(int orderQuantity, int currentStock)
    {
        const int shortage = orderQuantity - currentStock;
        return shortage > 0 ? shortage : 0;
    }

    int CalculateActualProductionQuantity(int shortageQuantity, double yieldRate)
    {
        if (yieldRate <= 0.0)
        {
            throw std::invalid_argument("yieldRate must be greater than 0");
        }
        if (shortageQuantity <= 0)
        {
            return 0;
        }
        return static_cast<int>(std::ceil(static_cast<double>(shortageQuantity) / yieldRate));
    }

    double CalculateTotalProductionTime(double avgProductionTime, int actualProductionQuantity)
    {
        return avgProductionTime * static_cast<double>(actualProductionQuantity);
    }

    InventoryLevel ClassifyInventoryLevel(int stock, int referenceQuantity)
    {
        // PDF 명세상 "고갈"은 기준 수량과 무관하게 재고 자체가 0인 상태를 뜻하므로,
        // 다른 판정보다 항상 우선한다.
        if (stock <= 0)
        {
            return InventoryLevel::Depleted;
        }
        if (stock >= referenceQuantity)
        {
            return InventoryLevel::Sufficient;
        }
        return InventoryLevel::Low;
    }
}
