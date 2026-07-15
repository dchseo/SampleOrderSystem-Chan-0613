#pragma once
#include <cstddef>
#include <string>
#include <string_view>

namespace json {

enum class TokenType {
    LBrace, RBrace, LBracket, RBracket, Colon, Comma,
    String, Number, True, False, Null, EndOfFile
};

struct Token {
    TokenType type = TokenType::EndOfFile;
    std::string text;
    size_t line = 1;
    size_t column = 1;
};

// JSON 텍스트를 토큰 스트림으로 변환한다. 문자열 토큰은 이스케이프가 이미 해석된 값을 담는다.
class JsonLexer {
public:
    explicit JsonLexer(std::string_view source);

    Token next();
    const Token& peek();

private:
    char current() const;
    char advance();
    void skipWhitespace();
    Token lexString();
    Token lexNumber();
    Token lexLiteral(std::string_view literal, TokenType type);
    [[noreturn]] void error(const std::string& message) const;

    std::string_view source_;
    size_t pos_ = 0;
    size_t line_ = 1;
    size_t column_ = 1;
    bool hasPeeked_ = false;
    Token peeked_;
};

} // namespace json
