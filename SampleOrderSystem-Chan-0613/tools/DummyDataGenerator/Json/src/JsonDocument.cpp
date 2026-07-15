#include "json/JsonDocument.h"
#include "json/JsonParser.h"
#include "json/JsonException.h"
#include "json/JsonPointer.h"
#include <fstream>
#include <sstream>
#include <filesystem>

namespace json {

namespace {

std::string readFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        throw JsonIOException("파일을 열 수 없습니다", path);
    }
    std::ostringstream ss;
    ss << file.rdbuf();
    std::string content = ss.str();
    // UTF-8 BOM 제거
    if (content.size() >= 3 &&
        static_cast<unsigned char>(content[0]) == 0xEF &&
        static_cast<unsigned char>(content[1]) == 0xBB &&
        static_cast<unsigned char>(content[2]) == 0xBF) {
        content.erase(0, 3);
    }
    return content;
}

} // namespace

JsonDocument::JsonDocument() : root_(JsonValue::makeObject()) {}
JsonDocument::JsonDocument(JsonValue root) : root_(std::move(root)) {}

JsonDocument JsonDocument::loadFromFile(const std::string& path) {
    std::string content = readFile(path);
    return JsonDocument(JsonParser::parse(content));
}

bool JsonDocument::tryLoadFromFile(const std::string& path, JsonDocument& out, std::string& error) {
    try {
        out = loadFromFile(path);
        return true;
    } catch (const std::exception& e) {
        error = e.what();
        return false;
    }
}

JsonDocument JsonDocument::loadFromString(std::string_view text) {
    return JsonDocument(JsonParser::parse(text));
}

bool JsonDocument::saveToFile(const std::string& path, const WriteOptions& opts) const {
    namespace fs = std::filesystem;
    fs::path targetPath(path);
    fs::path tempPath = targetPath;
    tempPath += ".tmp";

    {
        std::ofstream file(tempPath, std::ios::binary | std::ios::trunc);
        if (!file) {
            return false;
        }
        JsonWriter::writeToStream(file, root_, opts);
        if (!file) {
            return false;
        }
    }

    std::error_code ec;
    fs::rename(tempPath, targetPath, ec);
    if (ec) {
        fs::remove(tempPath, ec);
        return false;
    }
    return true;
}

std::string JsonDocument::toString(const WriteOptions& opts) const {
    return JsonWriter::write(root_, opts);
}

JsonValue& JsonDocument::root() { return root_; }
const JsonValue& JsonDocument::root() const { return root_; }

JsonValue* JsonDocument::find(const std::string& pointerPath) {
    return JsonPointer(pointerPath).resolve(root_);
}

const JsonValue* JsonDocument::find(const std::string& pointerPath) const {
    return JsonPointer(pointerPath).resolve(root_);
}

void JsonDocument::upsert(const std::string& pointerPath, JsonValue value) {
    JsonPointer(pointerPath).assign(root_, std::move(value));
}

bool JsonDocument::remove(const std::string& pointerPath) {
    return JsonPointer(pointerPath).remove(root_);
}

} // namespace json
