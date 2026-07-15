#include "../TestFramework.h"

// 하네스 자체가 정상 동작하는지 확인하는 self-test.
// Phase 1부터는 이 파일 옆에 Model/Repository에 대한 실제 Unit Test 파일을 추가한다.

TEST(Unit, FrameworkSelfTest_AssertTruePasses)
{
    ASSERT_TRUE(1 + 1 == 2);
}

TEST(Unit, FrameworkSelfTest_AssertEqPasses)
{
    ASSERT_EQ(4, 2 * 2);
}

TEST(Unit, FrameworkSelfTest_AssertFalsePasses)
{
    ASSERT_FALSE(1 == 2);
}
