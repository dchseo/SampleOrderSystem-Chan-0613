#pragma once
#include <functional>
#include <sstream>
#include <string>
#include <vector>

// 외부 프레임워크(GoogleTest 등) 의존 없이, 이 프로젝트의 "외부 의존성 없음" 원칙(CLAUDE.md
// "기술 스택")을 그대로 따르는 최소 assert 기반 테스트 러너.
//
// Unit Test와 적대적 테스트는 suite 이름으로만 구분한다 (예: TEST(Unit, ...), TEST(Adversarial, ...)).
// 실제 물리적 위치(tests/unit/, tests/adversarial/)는 파일을 넣는 폴더로만 구분하고, 러너 동작에는
// 영향을 주지 않는다.
namespace Testing
{
    // 검증 실패 시 던지는 예외. 러너가 이 예외를 잡아 실패로 기록하고 다음 테스트를 계속 진행한다.
    class AssertionFailure : public std::exception
    {
    public:
        explicit AssertionFailure(std::string message);

        const char* what() const noexcept override;

    private:
        std::string message_;
    };

    class TestRegistry
    {
    public:
        static TestRegistry& Instance();

        void Add(const std::string& suite, const std::string& name, std::function<void()> body);

        // 등록된 모든 테스트를 실행한다. 반환값은 실패한 테스트 개수(0이면 전부 통과).
        int RunAll() const;

    private:
        struct Entry
        {
            std::string suite;
            std::string name;
            std::function<void()> body;
        };

        std::vector<Entry> entries_;
    };

    // TEST(...) 매크로가 프로그램 시작 시(정적 초기화 시점) 자기 자신을 TestRegistry에 등록하기
    // 위해 사용하는 보조 타입. 직접 사용하지 않는다.
    struct AutoRegister
    {
        AutoRegister(const std::string& suite, const std::string& name, std::function<void()> body);
    };
}

// 사용 예:
//   TEST(Unit, CeilDivision_RoundsUp) { ASSERT_EQ(2, (5 + 1) / 2); }
//   TEST(Adversarial, ApproveOrder_RejectsUnknownOrderId) { ASSERT_THROWS(controller.ApproveOrder("X"), std::exception); }
#define TEST(suite, name)                                                                     \
    static void suite##_##name##_TestBody();                                                  \
    namespace                                                                                 \
    {                                                                                         \
        ::Testing::AutoRegister suite##_##name##_TestReg(                                     \
            #suite, #name, &suite##_##name##_TestBody);                                       \
    }                                                                                          \
    static void suite##_##name##_TestBody()

#define ASSERT_TRUE(condition)                                                                \
    do                                                                                        \
    {                                                                                          \
        if (!(condition))                                                                     \
        {                                                                                      \
            std::ostringstream failMsg_;                                                      \
            failMsg_ << __FILE__ << ":" << __LINE__ << " ASSERT_TRUE(" #condition ") failed";  \
            throw ::Testing::AssertionFailure(failMsg_.str());                                \
        }                                                                                      \
    } while (false)

#define ASSERT_FALSE(condition) ASSERT_TRUE(!(condition))

#define ASSERT_EQ(expected, actual)                                                           \
    do                                                                                        \
    {                                                                                          \
        const auto expectedValue_ = (expected);                                               \
        const auto actualValue_ = (actual);                                                   \
        if (!(expectedValue_ == actualValue_))                                                \
        {                                                                                       \
            std::ostringstream failMsg_;                                                      \
            failMsg_ << __FILE__ << ":" << __LINE__ << " ASSERT_EQ(" #expected ", " #actual    \
                      << ") failed: expected=" << expectedValue_ << " actual=" << actualValue_; \
            throw ::Testing::AssertionFailure(failMsg_.str());                                \
        }                                                                                      \
    } while (false)

#define ASSERT_THROWS(expression, exceptionType)                                              \
    do                                                                                        \
    {                                                                                          \
        bool threw_ = false;                                                                  \
        try                                                                                    \
        {                                                                                       \
            (expression);                                                                      \
        }                                                                                       \
        catch (const exceptionType&)                                                           \
        {                                                                                       \
            threw_ = true;                                                                     \
        }                                                                                       \
        if (!threw_)                                                                            \
        {                                                                                        \
            std::ostringstream failMsg_;                                                        \
            failMsg_ << __FILE__ << ":" << __LINE__ << " ASSERT_THROWS(" #expression           \
                      << ") did not throw " #exceptionType;                                     \
            throw ::Testing::AssertionFailure(failMsg_.str());                                  \
        }                                                                                        \
    } while (false)
