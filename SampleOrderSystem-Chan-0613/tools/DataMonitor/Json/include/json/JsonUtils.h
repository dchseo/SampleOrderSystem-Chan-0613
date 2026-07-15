#pragma once
#include "JsonValue.h"
#include <optional>

namespace json {
namespace JsonUtils {

// base 에 patch 를 재귀적으로 덮어쓴다 (RFC 7396 JSON Merge Patch와 유사).
// patch에서 값이 null 인 키는 삭제로 취급한다.
JsonValue merge(const JsonValue& base, const JsonValue& patch);

// 두 JsonValue가 구조적으로 동일한지 비교한다.
bool deepEqual(const JsonValue& a, const JsonValue& b);

// 값을 target 타입으로 변환 시도. 실패하면 std::nullopt.
std::optional<JsonValue> tryConvert(const JsonValue& value, JsonType target);

} // namespace JsonUtils
} // namespace json
