#pragma once
#include "JsonLexer.h"
#include "JsonValue.h"
#include <string>
#include <string_view>

namespace json {

// JSON 텍스트를 JsonValue DOM으로 변환한다.
class JsonParser {
public:
    // 실패 시 JsonParseException 을 던진다.
    static JsonValue parse(std::string_view text);

    // 예외 없이 사용하고 싶은 경우를 위한 결과 타입.
    struct Result {
        bool ok = false;
        JsonValue value;
        std::string error;
        size_t errorLine = 0;
        size_t errorColumn = 0;
    };
    static Result tryParse(std::string_view text) noexcept;

private:
    static constexpr size_t kMaxDepth = 512; // 비정상적으로 깊은 중첩으로 인한 스택 오버플로 방지

    explicit JsonParser(std::string_view text);

    JsonValue parseDocument();
    JsonValue parseValue(size_t depth);
    JsonValue parseObject(size_t depth);
    JsonValue parseArray(size_t depth);
    [[noreturn]] void error(const std::string& message);

    JsonLexer lexer_;
    Token current_;
};

} // namespace json
