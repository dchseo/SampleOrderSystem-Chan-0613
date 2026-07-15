#pragma once
#include "JsonValue.h"
#include <string>
#include <vector>

namespace json {

// "user.address.city", "items[2].name" 형식의 경로 문자열로 JsonValue 트리에
// CRUD 연산을 수행하기 위한 모듈.
class JsonPointer {
public:
    explicit JsonPointer(std::string path);

    // --- Read --- 경로가 없으면 nullptr 반환 (예외를 던지지 않음)
    JsonValue* resolve(JsonValue& root) const;
    const JsonValue* resolve(const JsonValue& root) const;

    // --- Create / Update --- 중간 경로가 없으면 자동으로 객체/배열을 생성한다.
    void assign(JsonValue& root, JsonValue value) const;

    // --- Delete --- 마지막 세그먼트를 삭제하고 성공 여부를 반환한다.
    bool remove(JsonValue& root) const;

private:
    struct Segment {
        bool isIndex = false;
        std::string key;
        size_t index = 0;
    };

    static std::vector<Segment> parsePath(const std::string& path);

    std::string path_;
    std::vector<Segment> segments_;
};

} // namespace json
