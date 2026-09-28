#pragma once

#include <initializer_list>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace ppl {

// Minimal JSON value used by the command database, LSP and MCP servers.
// Objects keep insertion order. Strings are UTF-8.
class Json {
public:
    enum class Type { Null, Bool, Number, String, Array, Object };

    Json() = default;
    Json(std::nullptr_t) {}
    Json(bool b) : m_type(Type::Bool), m_bool(b) {}
    Json(int n) : m_type(Type::Number), m_number(n) {}
    Json(long n) : m_type(Type::Number), m_number(static_cast<double>(n)) {}
    Json(long long n) : m_type(Type::Number), m_number(static_cast<double>(n)) {}
    Json(size_t n) : m_type(Type::Number), m_number(static_cast<double>(n)) {}
    Json(double n) : m_type(Type::Number), m_number(n) {}
    Json(const char *s) : m_type(Type::String), m_string(s) {}
    Json(std::string s) : m_type(Type::String), m_string(std::move(s)) {}
    Json(std::string_view s) : m_type(Type::String), m_string(s) {}

    static Json array() { Json j; j.m_type = Type::Array; return j; }
    static Json object() { Json j; j.m_type = Type::Object; return j; }
    static Json object(std::initializer_list<std::pair<std::string, Json>> items);

    // Returns Null on error and fills *error when given.
    static Json parse(std::string_view text, std::string *error = nullptr);
    std::string dump() const;

    Type type() const { return m_type; }
    bool isNull() const { return m_type == Type::Null; }
    bool isBool() const { return m_type == Type::Bool; }
    bool isNumber() const { return m_type == Type::Number; }
    bool isString() const { return m_type == Type::String; }
    bool isArray() const { return m_type == Type::Array; }
    bool isObject() const { return m_type == Type::Object; }

    bool asBool(bool def = false) const { return m_type == Type::Bool ? m_bool : def; }
    double asNumber(double def = 0) const { return m_type == Type::Number ? m_number : def; }
    int asInt(int def = 0) const { return m_type == Type::Number ? static_cast<int>(m_number) : def; }
    const std::string &asString() const;

    // Object access. operator[] on a missing key returns a shared Null value.
    const Json &operator[](std::string_view key) const;
    bool contains(std::string_view key) const;
    Json &set(std::string key, Json value);
    const std::vector<std::pair<std::string, Json>> &items() const { return m_object; }

    // Array access.
    const Json &operator[](size_t index) const;
    size_t size() const;
    Json &push(Json value);
    const std::vector<Json> &elements() const { return m_array; }

private:
    void dumpTo(std::string &out) const;

    Type m_type = Type::Null;
    bool m_bool = false;
    double m_number = 0;
    std::string m_string;
    std::vector<Json> m_array;
    std::vector<std::pair<std::string, Json>> m_object;
};

} // namespace ppl
