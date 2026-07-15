#include "json/JsonPointer.h"
#include "json/JsonException.h"
#include <cctype>
#include <utility>

namespace json {

std::vector<JsonPointer::Segment> JsonPointer::parsePath(const std::string& path) {
    std::vector<Segment> segments;
    size_t i = 0;
    const size_t n = path.size();
    while (i < n) {
        if (path[i] == '.') {
            ++i;
            continue;
        }
        if (path[i] == '[') {
            size_t j = i + 1;
            while (j < n && std::isdigit(static_cast<unsigned char>(path[j]))) ++j;
            if (j == i + 1 || j >= n || path[j] != ']') {
                throw JsonException("잘못된 경로 형식입니다 (배열 인덱스): " + path);
            }
            Segment seg;
            seg.isIndex = true;
            seg.index = static_cast<size_t>(std::stoull(path.substr(i + 1, j - i - 1)));
            segments.push_back(seg);
            i = j + 1;
            continue;
        }
        size_t j = i;
        while (j < n && path[j] != '.' && path[j] != '[') ++j;
        if (j == i) {
            throw JsonException("잘못된 경로 형식입니다: " + path);
        }
        Segment seg;
        seg.isIndex = false;
        seg.key = path.substr(i, j - i);
        segments.push_back(seg);
        i = j;
    }
    if (segments.empty()) {
        throw JsonException("빈 경로는 사용할 수 없습니다");
    }
    return segments;
}

JsonPointer::JsonPointer(std::string path) : path_(std::move(path)), segments_(parsePath(path_)) {}

JsonValue* JsonPointer::resolve(JsonValue& root) const {
    JsonValue* current = &root;
    for (const auto& seg : segments_) {
        if (seg.isIndex) {
            if (!current->isArray() || seg.index >= current->size()) return nullptr;
            current = &(*current)[seg.index];
        } else {
            if (!current->isObject()) return nullptr;
            JsonValue* next = current->find(seg.key);
            if (!next) return nullptr;
            current = next;
        }
    }
    return current;
}

const JsonValue* JsonPointer::resolve(const JsonValue& root) const {
    const JsonValue* current = &root;
    for (const auto& seg : segments_) {
        if (seg.isIndex) {
            if (!current->isArray() || seg.index >= current->size()) return nullptr;
            current = &(*current)[seg.index];
        } else {
            if (!current->isObject()) return nullptr;
            const JsonValue* next = current->find(seg.key);
            if (!next) return nullptr;
            current = next;
        }
    }
    return current;
}

void JsonPointer::assign(JsonValue& root, JsonValue value) const {
    JsonValue* current = &root;
    for (size_t i = 0; i < segments_.size(); ++i) {
        const auto& seg = segments_[i];
        if (seg.isIndex) {
            if (current->isNull()) *current = JsonValue::makeArray();
            if (!current->isArray()) {
                throw JsonTypeException("경로 중간에 배열이 아닌 값이 있어 인덱스로 접근할 수 없습니다: " + path_);
            }
            while (current->size() <= seg.index) current->push_back(JsonValue());
            current = &(*current)[seg.index];
        } else {
            if (current->isNull()) *current = JsonValue::makeObject();
            if (!current->isObject()) {
                throw JsonTypeException("경로 중간에 객체가 아닌 값이 있어 키로 접근할 수 없습니다: " + path_);
            }
            current = &(*current)[seg.key];
        }
        if (i + 1 == segments_.size()) {
            *current = std::move(value);
        }
    }
}

bool JsonPointer::remove(JsonValue& root) const {
    JsonValue* current = &root;
    for (size_t i = 0; i + 1 < segments_.size(); ++i) {
        const auto& seg = segments_[i];
        if (seg.isIndex) {
            if (!current->isArray() || seg.index >= current->size()) return false;
            current = &(*current)[seg.index];
        } else {
            if (!current->isObject()) return false;
            JsonValue* next = current->find(seg.key);
            if (!next) return false;
            current = next;
        }
    }
    const auto& last = segments_.back();
    if (last.isIndex) {
        if (!current->isArray()) return false;
        return current->erase(last.index);
    }
    if (!current->isObject()) return false;
    return current->erase(last.key);
}

} // namespace json
