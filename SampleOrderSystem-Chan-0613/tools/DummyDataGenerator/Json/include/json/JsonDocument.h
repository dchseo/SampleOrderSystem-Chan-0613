#pragma once
#include "JsonValue.h"
#include "JsonWriter.h"
#include <string>
#include <string_view>

namespace json {

// 파일 입출력과 경로 기반 CRUD 진입점을 한 곳에 모은 최상위 파사드.
// 애플리케이션은 대부분의 경우 JsonDocument 만 include 해서 사용하면 된다.
class JsonDocument {
public:
    JsonDocument();
    explicit JsonDocument(JsonValue root);

    static JsonDocument loadFromFile(const std::string& path);      // 실패 시 JsonIOException/JsonParseException
    static bool tryLoadFromFile(const std::string& path, JsonDocument& out, std::string& error);
    static JsonDocument loadFromString(std::string_view text);

    bool saveToFile(const std::string& path, const WriteOptions& opts = {}) const;
    std::string toString(const WriteOptions& opts = {}) const;

    JsonValue& root();
    const JsonValue& root() const;

    // --- 경로 기반 CRUD ("user.address.city", "items[0].name" 형식) ---
    JsonValue* find(const std::string& pointerPath);                 // Read, 없으면 nullptr
    const JsonValue* find(const std::string& pointerPath) const;
    void upsert(const std::string& pointerPath, JsonValue value);    // Create/Update
    bool remove(const std::string& pointerPath);                     // Delete

private:
    JsonValue root_;
};

} // namespace json
