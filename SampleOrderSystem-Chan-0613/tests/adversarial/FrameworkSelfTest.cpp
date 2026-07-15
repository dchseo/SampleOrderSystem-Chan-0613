#include <stdexcept>

#include "../TestFramework.h"

// 적대적 테스트는 "정상적으로 실패/거부되는지"를 확인하는 것이 목표다.
// 이 self-test는 ASSERT_THROWS 매크로 자체가 예외를 올바르게 검증하는지 확인한다.
// Phase 2부터는 이 파일 옆에 실제 Controller에 대한 적대적 테스트 파일을 추가한다.

namespace
{
    void ThrowOutOfRange()
    {
        throw std::out_of_range("intentional for self-test");
    }

    void DoNotThrow()
    {
    }
}

TEST(Adversarial, FrameworkSelfTest_AssertThrowsDetectsExpectedException)
{
    ASSERT_THROWS(ThrowOutOfRange(), std::out_of_range);
}

TEST(Adversarial, FrameworkSelfTest_AssertThrowsFailsWhenNoExceptionThrown)
{
    // ASSERT_THROWS가 "예외가 없으면 실패"를 실제로 감지하는지, 직접 try/catch로 검증한다
    // (여기서 AssertionFailure가 던져지는 것이 정상이므로 그 자체를 잡아 통과시킨다).
    bool detectedMissingThrow = false;
    try
    {
        ASSERT_THROWS(DoNotThrow(), std::out_of_range);
    }
    catch (const Testing::AssertionFailure&)
    {
        detectedMissingThrow = true;
    }
    ASSERT_TRUE(detectedMissingThrow);
}
