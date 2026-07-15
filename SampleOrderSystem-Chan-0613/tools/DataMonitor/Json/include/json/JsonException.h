#pragma once
#include <stdexcept>
#include <string>

namespace json {

class JsonException : public std::runtime_error {
public:
    explicit JsonException(const std::string& message) : std::runtime_error(message) {}
};

class JsonParseException : public JsonException {
public:
    JsonParseException(const std::string& message, size_t line, size_t column)
        : JsonException(message + " (line " + std::to_string(line) + ", column " + std::to_string(column) + ")"),
          line_(line), column_(column) {}

    size_t line() const { return line_; }
    size_t column() const { return column_; }

private:
    size_t line_;
    size_t column_;
};

class JsonTypeException : public JsonException {
public:
    explicit JsonTypeException(const std::string& message) : JsonException(message) {}
};

class JsonOutOfRangeException : public JsonException {
public:
    explicit JsonOutOfRangeException(const std::string& message) : JsonException(message) {}
};

class JsonIOException : public JsonException {
public:
    JsonIOException(const std::string& message, std::string path)
        : JsonException(message + " (path: " + path + ")"), path_(std::move(path)) {}

    const std::string& path() const { return path_; }

private:
    std::string path_;
};

} // namespace json
