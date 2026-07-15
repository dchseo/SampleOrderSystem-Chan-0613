#include "../../Model/InventoryCalculator.h"
#include "../TestFramework.h"

using Model::CalculateActualProductionQuantity;
using Model::CalculateShortage;
using Model::CalculateTotalProductionTime;
using Model::ClassifyInventoryLevel;
using Model::InventoryLevel;

TEST(Unit, CalculateShortage_ReturnsDifferenceWhenStockInsufficient)
{
    ASSERT_EQ(50, CalculateShortage(100, 50));
}

TEST(Unit, CalculateShortage_ReturnsZeroWhenStockSufficient)
{
    ASSERT_EQ(0, CalculateShortage(50, 100));
}

TEST(Unit, CalculateShortage_ReturnsZeroWhenStockExactlyMatches)
{
    ASSERT_EQ(0, CalculateShortage(50, 50));
}

TEST(Unit, CalculateActualProductionQuantity_RoundsUpWithCeil)
{
    // CLAUDE.md 예시: 부족분 50, 수율 0.5 -> 실 생산량 100
    ASSERT_EQ(100, CalculateActualProductionQuantity(50, 0.5));
}

TEST(Unit, CalculateActualProductionQuantity_RoundsUpWhenNotEvenlyDivisible)
{
    // 170 / 0.92 = 184.78... -> ceil = 185
    ASSERT_EQ(185, CalculateActualProductionQuantity(170, 0.92));
}

TEST(Unit, CalculateActualProductionQuantity_ReturnsZeroWhenNoShortage)
{
    ASSERT_EQ(0, CalculateActualProductionQuantity(0, 0.9));
}

TEST(Unit, CalculateActualProductionQuantity_ThrowsWhenYieldIsZero)
{
    ASSERT_THROWS(CalculateActualProductionQuantity(50, 0.0), std::invalid_argument);
}

TEST(Unit, CalculateTotalProductionTime_MultipliesAvgTimeByQuantity)
{
    ASSERT_EQ(206.0, CalculateTotalProductionTime(0.5, 412));
}

TEST(Unit, CalculateTotalProductionTime_ExactExample)
{
    // 평균 생산시간 0.8, 실생산량 165 -> 132.0분 (PDF 예시 UI 페이지 17과 동일한 스케일).
    // 0.8은 이진 부동소수점으로 정확히 표현되지 않으므로 오차 허용 비교를 사용한다.
    const double actual = CalculateTotalProductionTime(0.8, 165);
    ASSERT_TRUE(actual > 132.0 - 1e-9 && actual < 132.0 + 1e-9);
}

TEST(Unit, ClassifyInventoryLevel_SufficientWhenStockGreaterOrEqual)
{
    ASSERT_TRUE(ClassifyInventoryLevel(100, 50) == InventoryLevel::Sufficient);
    ASSERT_TRUE(ClassifyInventoryLevel(50, 50) == InventoryLevel::Sufficient);
}

TEST(Unit, ClassifyInventoryLevel_LowWhenStockPositiveButBelowReference)
{
    ASSERT_TRUE(ClassifyInventoryLevel(30, 200) == InventoryLevel::Low);
}

TEST(Unit, ClassifyInventoryLevel_DepletedWhenStockIsZero)
{
    ASSERT_TRUE(ClassifyInventoryLevel(0, 200) == InventoryLevel::Depleted);
}
