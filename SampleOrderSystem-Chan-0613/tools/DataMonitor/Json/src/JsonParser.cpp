#include "json/JsonParser.h"
#include "json/JsonException.h"

namespace json {

JsonParser::JsonParser(std::string_view text) : lexer_(text) {
    current_ = lexer_.next();
}

void JsonParser::error(const std::string& message) {
    throw JsonParseException(message, current_.line, current_.column);
}

JsonValue JsonParser::parse(std::string_view text) {
    JsonParser parser(text);
    return parser.parseDocument();
}

JsonParser::Result JsonParser::tryParse(std::string_view text) noexcept {
    Result result;
    try {
        result.value = parse(text);
        result.ok = true;
    } catch (const JsonParseException& e) {
        result.ok = false;
        result.error = e.what();
        result.errorLine = e.line();
        result.errorColumn = e.column();
    } catch (const std::exception& e) {
        result.ok = false;
        result.error = e.what();
    }
    return result;
}

JsonValue JsonParser::parseDocument() {
    JsonValue value = parseValue(0);
    if (current_.type != TokenType::EndOfFile) {
        error("최상위 값 뒤에 예상치 못한 데이터가 있습니다");
    }
    return value;
}

JsonValue JsonParser::parseValue(size_t depth) {
    if (depth > kMaxDepth) {
        error("JSON 중첩 깊이가 너무 깊습니다");
    }
    switch (current_.type) {
        case TokenType::LBrace:
            return parseObject(depth + 1);
        case TokenType::LBracket:
            return parseArray(depth + 1);
        case TokenType::String: {
            JsonValue v(current_.text);
            current_ = lexer_.next();
            return v;
        }
        case TokenType::Number: {
            const std::string text = current_.text;
            bool isFloat = text.find('.') != std::string::npos ||
                           text.find('e') != std::string::npos ||
                           text.find('E') != std::string::npos;
            JsonValue v = isFloat ? JsonValue(std::stod(text))
                                  : JsonValue(static_cast<int64_t>(std::stoll(text)));
            current_ = lexer_.next();
            return v;
        }
        case TokenType::True:
            current_ = lexer_.next();
            return JsonValue(true);
        case TokenType::False:
            current_ = lexer_.next();
            return JsonValue(false);
        case TokenType::Null:
            current_ = lexer_.next();
            return JsonValue(nullptr);
        default:
            error("값이 와야 하는 위치에 예상치 못한 토큰이 있습니다");
    }
    return JsonValue();
}

JsonValue JsonParser::parseObject(size_t depth) {
    JsonValue obj = JsonValue::makeObject();
    current_ = lexer_.next(); // '{' 소비
    if (current_.type == TokenType::RBrace) {
        current_ = lexer_.next();
        return obj;
    }
    while (true) {
        if (current_.type != TokenType::String) {
            error("객체의 키는 문자열이어야 합니다");
        }
        std::string key = current_.text;
        current_ = lexer_.next();
        if (current_.type != TokenType::Colon) {
            error("키 다음에 ':' 이 필요합니다");
        }
        current_ = lexer_.next();
        JsonValue value = parseValue(depth);
        obj.set(key, std::move(value));

        if (current_.type == TokenType::Comma) {
            current_ = lexer_.next();
            continue;
        }
        if (current_.type == TokenType::RBrace) {
            current_ = lexer_.next();
            break;
        }
        error("객체에서 ',' 또는 '}' 가 필요합니다");
    }
    return obj;
}

JsonValue JsonParser::parseArray(size_t depth) {
    JsonValue arr = JsonValue::makeArray();
    current_ = lexer_.next(); // '[' 소비
    if (current_.type == TokenType::RBracket) {
        current_ = lexer_.next();
        return arr;
    }
    while (true) {
        JsonValue value = parseValue(depth);
        arr.push_back(std::move(value));

        if (current_.type == TokenType::Comma) {
            current_ = lexer_.next();
            continue;
        }
        if (current_.type == TokenType::RBracket) {
            current_ = lexer_.next();
            break;
        }
        error("배열에서 ',' 또는 ']' 가 필요합니다");
    }
    return arr;
}

} // namespace json
