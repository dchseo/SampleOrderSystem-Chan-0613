#include "ConsoleFormat.h"

namespace View
{
    namespace
    {
        // UTF-8 선두 바이트로 해당 문자가 차지하는 바이트 수와 표시 폭(칸 수)을 판단한다.
        // 이 프로젝트는 한글과 ASCII만 다루므로, 멀티바이트 시퀀스는 전부 2칸(한글 기준)으로
        // 취급한다.
        struct CharInfo
        {
            int byteLength;
            int displayWidth;
        };

        CharInfo ClassifyLeadByte(unsigned char leadByte)
        {
            if ((leadByte & 0x80) == 0x00) return { 1, 1 }; // ASCII
            if ((leadByte & 0xE0) == 0xC0) return { 2, 2 }; // 2바이트 시퀀스
            if ((leadByte & 0xF0) == 0xE0) return { 3, 2 }; // 3바이트 시퀀스 (한글 등)
            if ((leadByte & 0xF8) == 0xF0) return { 4, 2 }; // 4바이트 시퀀스
            return { 1, 1 }; // 잘못된 선두 바이트 — 방어적으로 1바이트/1칸 취급
        }
    }

    int DisplayWidth(const std::string& text)
    {
        int width = 0;
        size_t i = 0;
        while (i < text.size())
        {
            const CharInfo info = ClassifyLeadByte(static_cast<unsigned char>(text[i]));
            width += info.displayWidth;
            i += static_cast<size_t>(info.byteLength);
        }
        return width;
    }

    std::string PadRight(const std::string& text, int displayWidth)
    {
        const int currentWidth = DisplayWidth(text);
        if (currentWidth >= displayWidth)
        {
            return text;
        }
        return text + std::string(static_cast<size_t>(displayWidth - currentWidth), ' ');
    }
}
