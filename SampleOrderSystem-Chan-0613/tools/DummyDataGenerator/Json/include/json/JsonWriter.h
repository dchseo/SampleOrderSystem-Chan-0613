#pragma once
#include "JsonValue.h"
#include <ostream>
#include <string>

namespace json {

struct WriteOptions {
    bool pretty = false;        // 들여쓰기 여부
    int indentSize = 2;
    bool sortKeys = false;      // 객체 키를 알파벳 순으로 정렬할지 여부
    bool escapeUnicode = false; // 비ASCII 문자를 \uXXXX 로 escape 할지 여부
};

// JsonValue DOM을 JSON 텍스트로 직렬화한다.
class JsonWriter {
public:
    static std::string write(const JsonValue& value, const WriteOptions& opts = {});
    static void writeToStream(std::ostream& os, const JsonValue& value, const WriteOptions& opts = {});

private:
    static void writeValue(std::ostream& os, const JsonValue& value, const WriteOptions& opts, int depth);
    static void writeString(std::ostream& os, const std::string& s, const WriteOptions& opts);
    static void writeIndent(std::ostream& os, const WriteOptions& opts, int depth);
};

} // namespace json
