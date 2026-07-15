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
        if (stock >= referenceQuantity)
        {
            return InventoryLevel::Sufficient;
        }
        if (stock <= 0)
        {
            return InventoryLevel::Depleted;
        }
        return InventoryLevel::Low;
    }
}
