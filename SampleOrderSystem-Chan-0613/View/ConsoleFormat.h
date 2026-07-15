#pragma once
#include <string>

namespace View
{
    // 콘솔 표 정렬 유틸리티.
    //
    // std::setw는 std::string의 "바이트 길이"를 기준으로 폭을 계산한다. 그런데 UTF-8로
    // 인코딩된 한글은 1글자가 3바이트인 반면, 고정폭 콘솔에서는 2칸을 차지한다(영문/숫자는
    // 1글자 1바이트 1칸). 그래서 한글 헤더("평균생산시간" 등)와 영문/숫자 데이터(시료ID,
    // 수치)가 함께 있는 표에서는 std::setw만으로 헤더와 데이터 행의 열이 서로 어긋난다 —
    // 헤더 쪽은 바이트 길이가 더 커서(예: "평균생산시간" 18바이트) 실제 표시 폭(12칸)보다
    // 적게 패딩되고, 데이터 쪽은 표시 폭과 바이트 길이가 같아(영문/숫자) 그대로 패딩되기
    // 때문이다. DisplayWidth/PadRight는 실제 "표시 폭"(한글 2, ASCII 1)을 기준으로 계산해
    // 이 어긋남을 없앤다.
    int DisplayWidth(const std::string& text);
    std::string PadRight(const std::string& text, int displayWidth);

    // 표 헤더와 데이터 행 사이에 그어 줄 구분선. width만큼 '-'를 반복한다(개행 문자 없음).
    std::string SeparatorLine(int width);

    // ANSI 컬러 코드. Windows 콘솔에서 이 코드가 실제로 색으로 렌더링되려면 콘솔 모드에
    // ENABLE_VIRTUAL_TERMINAL_PROCESSING이 설정되어 있어야 한다(main.cpp에서 시작 시 1회
    // 설정, tools/DataMonitor가 이미 쓰던 것과 동일한 방식).
    namespace Color
    {
        constexpr const char* Reset = "\x1b[0m";
        constexpr const char* BoldBlue = "\x1b[1;34m";
        constexpr const char* Green = "\x1b[32m";  // 재고 여유
        constexpr const char* Yellow = "\x1b[33m"; // 재고 부족
        constexpr const char* Red = "\x1b[31m";    // 재고 고갈
    }

    // text를 colorCode로 감싸고 끝에 Reset을 붙여 반환한다.
    std::string Colorize(const std::string& text, const char* colorCode);
}
