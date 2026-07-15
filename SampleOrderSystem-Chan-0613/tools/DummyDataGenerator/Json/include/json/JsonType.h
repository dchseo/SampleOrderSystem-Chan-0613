#pragma once

namespace json {

enum class JsonType {
    Null,
    Boolean,
    Number,
    String,
    Array,
    Object
};

inline const char* toString(JsonType type) {
    switch (type) {
        case JsonType::Null:    return "Null";
        case JsonType::Boolean: return "Boolean";
        case JsonType::Number:  return "Number";
        case JsonType::String:  return "String";
        case JsonType::Array:   return "Array";
        case JsonType::Object:  return "Object";
    }
    return "Unknown";
}

} // namespace json
