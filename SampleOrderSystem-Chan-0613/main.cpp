#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <iostream>
#include <string>

#include "tests/TestFramework.h"

// Phase 0 스캐폴딩 단계의 임시 진입점.
// Phase 4에서 Json Repository 조립(DI) + 메인 메뉴 루프로 교체된다 (PLAN.md Phase 4 참고).
// "--test" 인자를 받으면 등록된 모든 테스트(Unit + 적대적)를 실행하고 결과를 종료 코드로 반환한다.
int main(int argc, char** argv)
{
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    if (argc > 1 && std::string(argv[1]) == "--test")
    {
        const int failed = Testing::TestRegistry::Instance().RunAll();
        return failed == 0 ? 0 : 1;
    }

    std::cout << "반도체 시료 생산주문관리 시스템 - 아직 조립되지 않음 (Phase 0 스캐폴딩 단계)\n";
    std::cout << "메뉴 기반 애플리케이션은 Phase 4에서 조립됩니다.\n";
    std::cout << "테스트만 실행하려면: SampleOrderSystem-Chan-0613.exe --test\n";
    return 0;
}
