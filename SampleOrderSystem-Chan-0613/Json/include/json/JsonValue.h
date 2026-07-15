#pragma once
#include "JsonType.h"
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace json {

// JSON의 6가지 타입(null/bool/number/string/array/object)을 표현하는 DOM 노드.
// 모든 CRUD 연산(JsonPointer, JsonDocument 포함)의 최종 대상이 되는 값 타입이다.
class JsonValue {
public:
    using Array  = std::vector<JsonValue>;
    using Object = std::vector<std::pair<std::string, JsonValue>>; // 원본 키 순서 보존

    JsonValue();
    JsonValue(std::nullptr_t);
    JsonValue(bool value);
    JsonValue(int value);
    JsonValue(int64_t value);
    JsonValue(double value);
    JsonValue(std::string value);
    JsonValue(const char* value);

    JsonValue(const JsonValue& other);
    JsonValue(JsonValue&& other) noexcept;
    JsonValue& operator=(const JsonValue& other);
    JsonValue& operator=(JsonValue&& other) noexcept;
    ~JsonValue();

    static JsonValue makeArray();
    static JsonValue makeObject();

    JsonType type() const noexcept { return type_; }
    bool isNull()   const noexcept { return type_ == JsonType::Null; }
    bool isBool()   const noexcept { return type_ == JsonType::Boolean; }
    bool isNumber() const noexcept { return type_ == JsonType::Number; }
    bool isString() const noexcept { return type_ == JsonType::String; }
    bool isArray()  const noexcept { return type_ == JsonType::Array; }
    bool isObject() const noexcept { return type_ == JsonType::Object; }
    bool isIntegerNumber() const noexcept { return type_ == JsonType::Number && isInteger_; }

    // --- 조회 (Read) ---
    bool        asBool()   const;
    int64_t     asInt()    const;
    double      asDouble() const;
    const std::string& asString() const;
    const Array&  asArray()  const;
    Array&        asArray();
    const Object& asObject() const;
    Object&       asObject();

    JsonValue&       operator[](size_t index);        // Array, 범위를 벗어나면 확장
    const JsonValue& operator[](size_t index) const;   // Array, 범위를 벗어나면 예외
    JsonValue&       operator[](const std::string& key); // Object, 없으면 생성
    const JsonValue& operator[](const std::string& key) const; // Object, 없으면 예외

    bool contains(const std::string& key) const;
    JsonValue* find(const std::string& key);
    const JsonValue* find(const std::string& key) const;

    size_t size() const;   // Array/Object 원소 개수, String 길이
    bool empty() const;

    // --- 수정 (Update) / 생성(Create) ---
    void set(const std::string& key, JsonValue value); // 있으면 갱신, 없으면 추가
    void set(size_t index, JsonValue value);            // 인덱스 갱신 (범위 밖이면 예외)
    void push_back(JsonValue value);                    // Array 끝에 추가

    // --- 삭제 (Delete) ---
    bool erase(const std::string& key);
    bool erase(size_t index);
    void clear();

    Array::iterator begin();
    Array::iterator end();
    Array::const_iterator begin() const;
    Array::const_iterator end() const;

    Object::iterator obegin();
    Object::iterator oend();
    Object::const_iterator obegin() const;
    Object::const_iterator oend() const;

    bool operator==(const JsonValue& other) const;
    bool operator!=(const JsonValue& other) const { return !(*this == other); }

    std::string toString(bool pretty = false) const;

private:
    [[noreturn]] void throwTypeError(const char* expected) const;
    void ensureArray();
    void ensureObject();

    JsonType type_ = JsonType::Null;
    bool boolValue_ = false;
    bool isInteger_ = true;
    int64_t intValue_ = 0;
    double doubleValue_ = 0.0;
    std::string stringValue_;
    std::unique_ptr<Array> arrayValue_;
    std::unique_ptr<Object> objectValue_;
};

} // namespace json
