#include "json/JsonWriter.h"
#include <algorithm>
#include <sstream>
#include <cstdio>

namespace json {

namespace {

void appendHex4(std::ostream& os, unsigned int value) {
    char buf[8];
    std::snprintf(buf, sizeof(buf), "\\u%04x", value & 0xFFFFu);
    os << buf;
}

// s[i] 위치의 UTF-8 코드포인트를 디코딩하고 i를 다음 문자 시작 위치로 전진시킨다.
unsigned int decodeUtf8(const std::string& s, size_t& i) {
    unsigned char c = static_cast<unsigned char>(s[i]);
    unsigned int cp;
    size_t extra;
    if ((c & 0x80) == 0)      { cp = c;          extra = 0; }
    else if ((c & 0xE0) == 0xC0) { cp = c & 0x1F; extra = 1; }
    else if ((c & 0xF0) == 0xE0) { cp = c & 0x0F; extra = 2; }
    else if ((c & 0xF8) == 0xF0) { cp = c & 0x07; extra = 3; }
    else { cp = c; extra = 0; }

    for (size_t k = 0; k < extra && i + 1 < s.size(); ++k) {
        ++i;
        unsigned char cc = static_cast<unsigned char>(s[i]);
        cp = (cp << 6) | (cc & 0x3F);
    }
    ++i;
    return cp;
}

} // namespace

void JsonWriter::writeString(std::ostream& os, const std::string& s, const WriteOptions& opts) {
    os << '"';
    size_t i = 0;
    while (i < s.size()) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        switch (c) {
            case '"':  os << "\\\""; ++i; continue;
            case '\\': os << "\\\\"; ++i; continue;
            case '\b': os << "\\b";  ++i; continue;
            case '\f': os << "\\f";  ++i; continue;
            case '\n': os << "\\n";  ++i; continue;
            case '\r': os << "\\r";  ++i; continue;
            case '\t': os << "\\t";  ++i; continue;
            default: break;
        }
        if (c < 0x20) {
            appendHex4(os, c);
            ++i;
            continue;
        }
        if (c < 0x80) {
            os << static_cast<char>(c);
            ++i;
            continue;
        }
        if (opts.escapeUnicode) {
            size_t next = i;
            unsigned int cp = decodeUtf8(s, next);
            if (cp > 0xFFFF) {
                unsigned int v = cp - 0x10000;
                appendHex4(os, 0xD800 + (v >> 10));
                appendHex4(os, 0xDC00 + (v & 0x3FF));
            } else {
                appendHex4(os, cp);
            }
            i = next;
        } else {
            os << static_cast<char>(c);
            ++i;
        }
    }
    os << '"';
}

void JsonWriter::writeIndent(std::ostream& os, const WriteOptions& opts, int depth) {
    if (!opts.pretty) return;
    os << '\n';
    os << std::string(static_cast<size_t>(depth) * static_cast<size_t>(opts.indentSize), ' ');
}

void JsonWriter::writeValue(std::ostream& os, const JsonValue& value, const WriteOptions& opts, int depth) {
    switch (value.type()) {
        case JsonType::Null:
            os << "null";
            break;
        case JsonType::Boolean:
            os << (value.asBool() ? "true" : "false");
            break;
        case JsonType::Number:
            if (value.isIntegerNumber()) {
                os << value.asInt();
            } else {
                std::ostringstream tmp;
                tmp.precision(17);
                tmp << value.asDouble();
                os << tmp.str();
            }
            break;
        case JsonType::String:
            writeString(os, value.asString(), opts);
            break;
        case JsonType::Array: {
            const auto& arr = value.asArray();
            if (arr.empty()) { os << "[]"; break; }
            os << '[';
            bool first = true;
            for (const auto& item : arr) {
                if (!first) os << ',';
                first = false;
                writeIndent(os, opts, depth + 1);
                writeValue(os, item, opts, depth + 1);
            }
            writeIndent(os, opts, depth);
            os << ']';
            break;
        }
        case JsonType::Object: {
            auto entries = value.asObject();
            if (opts.sortKeys) {
                std::sort(entries.begin(), entries.end(),
                          [](const auto& a, const auto& b) { return a.first < b.first; });
            }
            if (entries.empty()) { os << "{}"; break; }
            os << '{';
            bool first = true;
            for (const auto& kv : entries) {
                if (!first) os << ',';
                first = false;
                writeIndent(os, opts, depth + 1);
                writeString(os, kv.first, opts);
                os << ':';
                if (opts.pretty) os << ' ';
                writeValue(os, kv.second, opts, depth + 1);
            }
            writeIndent(os, opts, depth);
            os << '}';
            break;
        }
    }
}

void JsonWriter::writeToStream(std::ostream& os, const JsonValue& value, const WriteOptions& opts) {
    writeValue(os, value, opts, 0);
}

std::string JsonWriter::write(const JsonValue& value, const WriteOptions& opts) {
    std::ostringstream oss;
    writeToStream(oss, value, opts);
    return oss.str();
}

} // namespace json
