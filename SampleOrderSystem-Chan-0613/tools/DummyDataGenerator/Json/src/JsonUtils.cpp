#include "json/JsonUtils.h"
#include <charconv>

namespace json {
namespace JsonUtils {

JsonValue merge(const JsonValue& base, const JsonValue& patch) {
    if (!patch.isObject()) {
        return patch;
    }
    if (!base.isObject()) {
        JsonValue result = JsonValue::makeObject();
        for (auto it = patch.obegin(); it != patch.oend(); ++it) {
            if (it->second.isNull()) continue;
            result.set(it->first, it->second);
        }
        return result;
    }

    JsonValue result = base;
    for (auto it = patch.obegin(); it != patch.oend(); ++it) {
        const std::string& key = it->first;
        const JsonValue& patchValue = it->second;
        if (patchValue.isNull()) {
            result.erase(key);
            continue;
        }
        const JsonValue* existing = result.find(key);
        if (existing && existing->isObject() && patchValue.isObject()) {
            result.set(key, merge(*existing, patchValue));
        } else {
            result.set(key, patchValue);
        }
    }
    return result;
}

bool deepEqual(const JsonValue& a, const JsonValue& b) {
    return a == b;
}

std::optional<JsonValue> tryConvert(const JsonValue& value, JsonType target) {
    if (value.type() == target) return value;
    switch (target) {
        case JsonType::String: {
            if (value.isNumber()) {
                return JsonValue(value.isIntegerNumber() ? std::to_string(value.asInt())
                                                           : std::to_string(value.asDouble()));
            }
            if (value.isBool()) {
                return JsonValue(std::string(value.asBool() ? "true" : "false"));
            }
            return std::nullopt;
        }
        case JsonType::Number: {
            if (value.isString()) {
                const std::string& s = value.asString();
                double d = 0.0;
                auto res = std::from_chars(s.data(), s.data() + s.size(), d);
                if (res.ec == std::errc() && res.ptr == s.data() + s.size()) {
                    return JsonValue(d);
                }
                return std::nullopt;
            }
            if (value.isBool()) {
                return JsonValue(static_cast<int64_t>(value.asBool() ? 1 : 0));
            }
            return std::nullopt;
        }
        case JsonType::Boolean: {
            if (value.isNumber()) {
                return JsonValue(value.asDouble() != 0.0);
            }
            if (value.isString()) {
                return JsonValue(!value.asString().empty());
            }
            return std::nullopt;
        }
        default:
            return std::nullopt;
    }
}

} // namespace JsonUtils
} // namespace json
