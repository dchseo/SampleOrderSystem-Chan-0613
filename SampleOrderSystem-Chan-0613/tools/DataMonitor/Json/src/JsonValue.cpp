#include "json/JsonValue.h"
#include "json/JsonException.h"
#include "json/JsonWriter.h"
#include <algorithm>

namespace json {

JsonValue::JsonValue() : type_(JsonType::Null) {}
JsonValue::JsonValue(std::nullptr_t) : type_(JsonType::Null) {}
JsonValue::JsonValue(bool value) : type_(JsonType::Boolean), boolValue_(value) {}
JsonValue::JsonValue(int value) : type_(JsonType::Number), isInteger_(true), intValue_(value) {}
JsonValue::JsonValue(int64_t value) : type_(JsonType::Number), isInteger_(true), intValue_(value) {}
JsonValue::JsonValue(double value) : type_(JsonType::Number), isInteger_(false), doubleValue_(value) {}
JsonValue::JsonValue(std::string value) : type_(JsonType::String), stringValue_(std::move(value)) {}
JsonValue::JsonValue(const char* value) : type_(JsonType::String), stringValue_(value ? value : "") {}

JsonValue::JsonValue(const JsonValue& other)
    : type_(other.type_),
      boolValue_(other.boolValue_),
      isInteger_(other.isInteger_),
      intValue_(other.intValue_),
      doubleValue_(other.doubleValue_),
      stringValue_(other.stringValue_) {
    if (other.arrayValue_)  arrayValue_  = std::make_unique<Array>(*other.arrayValue_);
    if (other.objectValue_) objectValue_ = std::make_unique<Object>(*other.objectValue_);
}

JsonValue::JsonValue(JsonValue&& other) noexcept = default;

JsonValue& JsonValue::operator=(const JsonValue& other) {
    if (this == &other) return *this;
    type_ = other.type_;
    boolValue_ = other.boolValue_;
    isInteger_ = other.isInteger_;
    intValue_ = other.intValue_;
    doubleValue_ = other.doubleValue_;
    stringValue_ = other.stringValue_;
    arrayValue_.reset(other.arrayValue_ ? new Array(*other.arrayValue_) : nullptr);
    objectValue_.reset(other.objectValue_ ? new Object(*other.objectValue_) : nullptr);
    return *this;
}

JsonValue& JsonValue::operator=(JsonValue&& other) noexcept = default;

JsonValue::~JsonValue() = default;

JsonValue JsonValue::makeArray() {
    JsonValue v;
    v.type_ = JsonType::Array;
    v.arrayValue_ = std::make_unique<Array>();
    return v;
}

JsonValue JsonValue::makeObject() {
    JsonValue v;
    v.type_ = JsonType::Object;
    v.objectValue_ = std::make_unique<Object>();
    return v;
}

void JsonValue::throwTypeError(const char* expected) const {
    throw JsonTypeException(std::string("타입 불일치: ") + expected +
                             " 타입이 필요하지만 실제 타입은 " + json::toString(type_) + " 입니다");
}

bool JsonValue::asBool() const {
    if (type_ != JsonType::Boolean) throwTypeError("Boolean");
    return boolValue_;
}

int64_t JsonValue::asInt() const {
    if (type_ != JsonType::Number) throwTypeError("Number");
    return isInteger_ ? intValue_ : static_cast<int64_t>(doubleValue_);
}

double JsonValue::asDouble() const {
    if (type_ != JsonType::Number) throwTypeError("Number");
    return isInteger_ ? static_cast<double>(intValue_) : doubleValue_;
}

const std::string& JsonValue::asString() const {
    if (type_ != JsonType::String) throwTypeError("String");
    return stringValue_;
}

const JsonValue::Array& JsonValue::asArray() const {
    if (type_ != JsonType::Array) throwTypeError("Array");
    return *arrayValue_;
}

JsonValue::Array& JsonValue::asArray() {
    if (type_ != JsonType::Array) throwTypeError("Array");
    return *arrayValue_;
}

const JsonValue::Object& JsonValue::asObject() const {
    if (type_ != JsonType::Object) throwTypeError("Object");
    return *objectValue_;
}

JsonValue::Object& JsonValue::asObject() {
    if (type_ != JsonType::Object) throwTypeError("Object");
    return *objectValue_;
}

void JsonValue::ensureArray() {
    if (type_ == JsonType::Null) {
        type_ = JsonType::Array;
        arrayValue_ = std::make_unique<Array>();
        return;
    }
    if (type_ != JsonType::Array) throwTypeError("Array");
}

void JsonValue::ensureObject() {
    if (type_ == JsonType::Null) {
        type_ = JsonType::Object;
        objectValue_ = std::make_unique<Object>();
        return;
    }
    if (type_ != JsonType::Object) throwTypeError("Object");
}

JsonValue& JsonValue::operator[](size_t index) {
    ensureArray();
    if (index >= arrayValue_->size()) {
        arrayValue_->resize(index + 1);
    }
    return (*arrayValue_)[index];
}

const JsonValue& JsonValue::operator[](size_t index) const {
    if (type_ != JsonType::Array) throwTypeError("Array");
    if (index >= arrayValue_->size()) {
        throw JsonOutOfRangeException("배열 인덱스가 범위를 벗어났습니다: " + std::to_string(index));
    }
    return (*arrayValue_)[index];
}

JsonValue& JsonValue::operator[](const std::string& key) {
    ensureObject();
    for (auto& kv : *objectValue_) {
        if (kv.first == key) return kv.second;
    }
    objectValue_->emplace_back(key, JsonValue());
    return objectValue_->back().second;
}

const JsonValue& JsonValue::operator[](const std::string& key) const {
    if (type_ != JsonType::Object) throwTypeError("Object");
    for (const auto& kv : *objectValue_) {
        if (kv.first == key) return kv.second;
    }
    throw JsonOutOfRangeException("존재하지 않는 키입니다: " + key);
}

bool JsonValue::contains(const std::string& key) const {
    if (type_ != JsonType::Object) return false;
    for (const auto& kv : *objectValue_) {
        if (kv.first == key) return true;
    }
    return false;
}

JsonValue* JsonValue::find(const std::string& key) {
    if (type_ != JsonType::Object) return nullptr;
    for (auto& kv : *objectValue_) {
        if (kv.first == key) return &kv.second;
    }
    return nullptr;
}

const JsonValue* JsonValue::find(const std::string& key) const {
    if (type_ != JsonType::Object) return nullptr;
    for (const auto& kv : *objectValue_) {
        if (kv.first == key) return &kv.second;
    }
    return nullptr;
}

size_t JsonValue::size() const {
    switch (type_) {
        case JsonType::Array:  return arrayValue_->size();
        case JsonType::Object: return objectValue_->size();
        case JsonType::String: return stringValue_.size();
        default: return 0;
    }
}

bool JsonValue::empty() const {
    return size() == 0;
}

void JsonValue::set(const std::string& key, JsonValue value) {
    ensureObject();
    for (auto& kv : *objectValue_) {
        if (kv.first == key) {
            kv.second = std::move(value);
            return;
        }
    }
    objectValue_->emplace_back(key, std::move(value));
}

void JsonValue::set(size_t index, JsonValue value) {
    if (type_ != JsonType::Array) throwTypeError("Array");
    if (index >= arrayValue_->size()) {
        throw JsonOutOfRangeException("배열 인덱스가 범위를 벗어났습니다: " + std::to_string(index));
    }
    (*arrayValue_)[index] = std::move(value);
}

void JsonValue::push_back(JsonValue value) {
    ensureArray();
    arrayValue_->push_back(std::move(value));
}

bool JsonValue::erase(const std::string& key) {
    if (type_ != JsonType::Object) return false;
    auto it = std::find_if(objectValue_->begin(), objectValue_->end(),
                            [&](const auto& kv) { return kv.first == key; });
    if (it == objectValue_->end()) return false;
    objectValue_->erase(it);
    return true;
}

bool JsonValue::erase(size_t index) {
    if (type_ != JsonType::Array) return false;
    if (index >= arrayValue_->size()) return false;
    arrayValue_->erase(arrayValue_->begin() + static_cast<std::ptrdiff_t>(index));
    return true;
}

void JsonValue::clear() {
    if (type_ == JsonType::Array && arrayValue_) arrayValue_->clear();
    else if (type_ == JsonType::Object && objectValue_) objectValue_->clear();
    else if (type_ == JsonType::String) stringValue_.clear();
}

JsonValue::Array::iterator JsonValue::begin() { ensureArray(); return arrayValue_->begin(); }
JsonValue::Array::iterator JsonValue::end()   { ensureArray(); return arrayValue_->end(); }
JsonValue::Array::const_iterator JsonValue::begin() const {
    if (type_ != JsonType::Array) throwTypeError("Array");
    return arrayValue_->begin();
}
JsonValue::Array::const_iterator JsonValue::end() const {
    if (type_ != JsonType::Array) throwTypeError("Array");
    return arrayValue_->end();
}

JsonValue::Object::iterator JsonValue::obegin() { ensureObject(); return objectValue_->begin(); }
JsonValue::Object::iterator JsonValue::oend()   { ensureObject(); return objectValue_->end(); }
JsonValue::Object::const_iterator JsonValue::obegin() const {
    if (type_ != JsonType::Object) throwTypeError("Object");
    return objectValue_->begin();
}
JsonValue::Object::const_iterator JsonValue::oend() const {
    if (type_ != JsonType::Object) throwTypeError("Object");
    return objectValue_->end();
}

bool JsonValue::operator==(const JsonValue& other) const {
    if (type_ != other.type_) return false;
    switch (type_) {
        case JsonType::Null:    return true;
        case JsonType::Boolean: return boolValue_ == other.boolValue_;
        case JsonType::Number:  return asDouble() == other.asDouble();
        case JsonType::String:  return stringValue_ == other.stringValue_;
        case JsonType::Array:   return *arrayValue_ == *other.arrayValue_;
        case JsonType::Object: {
            if (objectValue_->size() != other.objectValue_->size()) return false;
            for (const auto& kv : *objectValue_) {
                const JsonValue* otherVal = other.find(kv.first);
                if (!otherVal || !(*otherVal == kv.second)) return false;
            }
            return true;
        }
    }
    return false;
}

std::string JsonValue::toString(bool pretty) const {
    WriteOptions opts;
    opts.pretty = pretty;
    return JsonWriter::write(*this, opts);
}

} // namespace json
