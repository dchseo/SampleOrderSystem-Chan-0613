#include "json/JsonLexer.h"
#include "json/JsonException.h"
#include <cctype>

namespace json {

namespace {

void appendUtf8(std::string& out, unsigned int codepoint) {
    if (codepoint <= 0x7F) {
        out += static_cast<char>(codepoint);
    } else if (codepoint <= 0x7FF) {
        out += static_cast<char>(0xC0 | (codepoint >> 6));
        out += static_cast<char>(0x80 | (codepoint & 0x3F));
    } else if (codepoint <= 0xFFFF) {
        out += static_cast<char>(0xE0 | (codepoint >> 12));
        out += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (codepoint & 0x3F));
    } else {
        out += static_cast<char>(0xF0 | (codepoint >> 18));
        out += static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F));
        out += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (codepoint & 0x3F));
    }
}

} // namespace

JsonLexer::JsonLexer(std::string_view source) : source_(source) {}

char JsonLexer::current() const {
    return pos_ < source_.size() ? source_[pos_] : '\0';
}

char JsonLexer::advance() {
    char c = current();
    ++pos_;
    if (c == '\n') { ++line_; column_ = 1; } else { ++column_; }
    return c;
}

void JsonLexer::skipWhitespace() {
    while (pos_ < source_.size()) {
        char c = current();
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            advance();
        } else {
            break;
        }
    }
}

void JsonLexer::error(const std::string& message) const {
    throw JsonParseException(message, line_, column_);
}

Token JsonLexer::lexLiteral(std::string_view literal, TokenType type) {
    size_t startLine = line_, startColumn = column_;
    for (char expected : literal) {
        if (current() != expected) {
            error("잘못된 리터럴입니다 (예상값: '" + std::string(literal) + "')");
        }
        advance();
    }
    Token t;
    t.type = type;
    t.text = std::string(literal);
    t.line = startLine;
    t.column = startColumn;
    return t;
}

Token JsonLexer::lexString() {
    size_t startLine = line_, startColumn = column_;
    advance(); // 여는 따옴표 소비
    std::string value;
    while (true) {
        if (pos_ >= source_.size()) {
            error("문자열이 닫히지 않았습니다");
        }
        char c = advance();
        if (c == '"') break;
        if (c == '\\') {
            char esc = advance();
            switch (esc) {
                case '"':  value += '"';  break;
                case '\\': value += '\\'; break;
                case '/':  value += '/';  break;
                case 'b':  value += '\b'; break;
                case 'f':  value += '\f'; break;
                case 'n':  value += '\n'; break;
                case 'r':  value += '\r'; break;
                case 't':  value += '\t'; break;
                case 'u': {
                    unsigned int cp = 0;
                    for (int i = 0; i < 4; ++i) {
                        char h = advance();
                        cp <<= 4;
                        if (h >= '0' && h <= '9') cp |= static_cast<unsigned int>(h - '0');
                        else if (h >= 'a' && h <= 'f') cp |= static_cast<unsigned int>(h - 'a' + 10);
                        else if (h >= 'A' && h <= 'F') cp |= static_cast<unsigned int>(h - 'A' + 10);
                        else error("잘못된 \\u 이스케이프입니다");
                    }
                    if (cp >= 0xD800 && cp <= 0xDBFF && current() == '\\') {
                        size_t savePos = pos_, saveLine = line_, saveCol = column_;
                        advance(); // '\'
                        if (current() == 'u') {
                            advance(); // 'u'
                            unsigned int low = 0;
                            bool ok = true;
                            for (int i = 0; i < 4 && ok; ++i) {
                                char h = advance();
                                low <<= 4;
                                if (h >= '0' && h <= '9') low |= static_cast<unsigned int>(h - '0');
                                else if (h >= 'a' && h <= 'f') low |= static_cast<unsigned int>(h - 'a' + 10);
                                else if (h >= 'A' && h <= 'F') low |= static_cast<unsigned int>(h - 'A' + 10);
                                else ok = false;
                            }
                            if (ok && low >= 0xDC00 && low <= 0xDFFF) {
                                unsigned int combined = 0x10000 + ((cp - 0xD800) << 10) + (low - 0xDC00);
                                appendUtf8(value, combined);
                                break;
                            }
                        }
                        pos_ = savePos; line_ = saveLine; column_ = saveCol;
                    }
                    appendUtf8(value, cp);
                    break;
                }
                default:
                    error("알 수 없는 이스케이프 문자입니다: \\" + std::string(1, esc));
            }
        } else {
            value += c;
        }
    }
    Token t;
    t.type = TokenType::String;
    t.text = std::move(value);
    t.line = startLine;
    t.column = startColumn;
    return t;
}

Token JsonLexer::lexNumber() {
    size_t startLine = line_, startColumn = column_;
    size_t start = pos_;
    if (current() == '-') advance();
    if (current() == '0') {
        advance();
    } else if (std::isdigit(static_cast<unsigned char>(current()))) {
        while (std::isdigit(static_cast<unsigned char>(current()))) advance();
    } else {
        error("숫자 형식이 올바르지 않습니다");
    }
    if (current() == '.') {
        advance();
        if (!std::isdigit(static_cast<unsigned char>(current()))) error("소수점 뒤에 숫자가 필요합니다");
        while (std::isdigit(static_cast<unsigned char>(current()))) advance();
    }
    if (current() == 'e' || current() == 'E') {
        advance();
        if (current() == '+' || current() == '-') advance();
        if (!std::isdigit(static_cast<unsigned char>(current()))) error("지수 표기법 뒤에 숫자가 필요합니다");
        while (std::isdigit(static_cast<unsigned char>(current()))) advance();
    }
    Token t;
    t.type = TokenType::Number;
    t.text = std::string(source_.substr(start, pos_ - start));
    t.line = startLine;
    t.column = startColumn;
    return t;
}

Token JsonLexer::next() {
    if (hasPeeked_) {
        hasPeeked_ = false;
        return peeked_;
    }
    skipWhitespace();
    if (pos_ >= source_.size()) {
        Token t;
        t.type = TokenType::EndOfFile;
        t.line = line_;
        t.column = column_;
        return t;
    }
    char c = current();
    switch (c) {
        case '{': { Token t; t.type = TokenType::LBrace;   t.text = "{"; t.line = line_; t.column = column_; advance(); return t; }
        case '}': { Token t; t.type = TokenType::RBrace;   t.text = "}"; t.line = line_; t.column = column_; advance(); return t; }
        case '[': { Token t; t.type = TokenType::LBracket; t.text = "["; t.line = line_; t.column = column_; advance(); return t; }
        case ']': { Token t; t.type = TokenType::RBracket; t.text = "]"; t.line = line_; t.column = column_; advance(); return t; }
        case ':': { Token t; t.type = TokenType::Colon;    t.text = ":"; t.line = line_; t.column = column_; advance(); return t; }
        case ',': { Token t; t.type = TokenType::Comma;    t.text = ","; t.line = line_; t.column = column_; advance(); return t; }
        case '"': return lexString();
        case 't': return lexLiteral("true", TokenType::True);
        case 'f': return lexLiteral("false", TokenType::False);
        case 'n': return lexLiteral("null", TokenType::Null);
        default:
            if (c == '-' || std::isdigit(static_cast<unsigned char>(c))) {
                return lexNumber();
            }
            error(std::string("예상치 못한 문자입니다: '") + c + "'");
    }
    return Token{};
}

const Token& JsonLexer::peek() {
    if (!hasPeeked_) {
        peeked_ = next();
        hasPeeked_ = true;
    }
    return peeked_;
}

} // namespace json
