#include <iostream>
#include <sstream>

#include "../../View/MainMenuView.h"
#include "../TestFramework.h"

// View는 로직을 갖지 않으므로(CLAUDE.md 역할 원칙), 여기서는 입력 파싱/가드 로직만 검증한다.
// PLAN.md Phase 3: "잘못된 메뉴 입력값 — 존재하지 않는 메뉴 번호, 숫자가 아닌 입력, 빈 입력이
// View에서 크래시 없이 처리되고, cin이 이후 입력을 계속 읽을 수 있는 상태로 남는지" 확인한다.

namespace
{
    // std::cin의 streambuf를 임시로 바꿔치기하고, 테스트가 실패(예외)하더라도 반드시
    // 원래 상태로 복구한다.
    struct CinRedirectGuard
    {
        explicit CinRedirectGuard(std::istream& replacement)
            : original(std::cin.rdbuf(replacement.rdbuf()))
        {
        }

        ~CinRedirectGuard()
        {
            std::cin.rdbuf(original);
        }

        std::streambuf* original;
    };
}

TEST(Adversarial, MainMenuView_PromptMenuChoice_HandlesNonNumericInputAndRecovers)
{
    std::istringstream input("abc\n1\n");
    CinRedirectGuard guard(input);

    View::MainMenuView view;
    ASSERT_TRUE(view.PromptMenuChoice() == View::MainMenuOption::Invalid);
    // 잘못된 입력을 버린 뒤에도 스트림이 정상 상태로 남아 다음 입력을 이어서 읽을 수 있어야 한다.
    ASSERT_TRUE(view.PromptMenuChoice() == View::MainMenuOption::SampleManagement);
}

TEST(Adversarial, MainMenuView_PromptMenuChoice_HandlesOutOfRangeMenuNumber)
{
    std::istringstream input("99\n");
    CinRedirectGuard guard(input);

    View::MainMenuView view;
    ASSERT_TRUE(view.PromptMenuChoice() == View::MainMenuOption::Invalid);
}

TEST(Adversarial, MainMenuView_PromptMenuChoice_HandlesEmptyInput)
{
    std::istringstream input(""); // 즉시 EOF
    CinRedirectGuard guard(input);

    View::MainMenuView view;
    ASSERT_TRUE(view.PromptMenuChoice() == View::MainMenuOption::Invalid);
}
