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
}
