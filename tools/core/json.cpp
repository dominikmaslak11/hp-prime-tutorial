#include "json.h"

#include "utf8.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace ppl {

namespace {

const Json &nullJson()
{
    static const Json n;
    return n;
}

class Parser {
public:
    explicit Parser(std::string_view t) : s(t) {}

    Json parseDocument(std::string *error)
    {
        skipWs();
        Json v = parseValue();
        skipWs();
        if (ok && i != s.size())
            fail("unexpected trailing characters");
        if (!ok) {
            if (error)
                *error = err + " at offset " + std::to_string(i);
            return Json();
        }
        return v;
    }

private:
    std::string_view s;
    size_t i = 0;
    bool ok = true;
    std::string err;
    int depth = 0;

    void fail(const std::string &m)
    {
        if (ok) {
            ok = false;
            err = m;
        }
    }

    void skipWs()
    {
        while (i < s.size() && (s[i] == ' ' || s[i] == '\t' || s[i] == '\n' || s[i] == '\r'))
            ++i;
    }

    bool consume(char c)
    {
        skipWs();
        if (i < s.size() && s[i] == c) {
            ++i;
            return true;
        }
        return false;
    }

    Json parseValue()
    {
        if (!ok)
            return {};
        if (++depth > 512) {
            fail("nesting too deep");
            return {};
        }
        skipWs();
        Json result;
        if (i >= s.size()) {
            fail("unexpected end of input");
        } else if (s[i] == '{') {
            result = parseObject();
        } else if (s[i] == '[') {
            result = parseArray();
        } else if (s[i] == '"') {
            result = Json(parseString());
        } else if (s.compare(i, 4, "true") == 0) {
            i += 4;
            result = Json(true);
        } else if (s.compare(i, 5, "false") == 0) {
            i += 5;
            result = Json(false);
        } else if (s.compare(i, 4, "null") == 0) {
            i += 4;
        } else if (s[i] == '-' || (s[i] >= '0' && s[i] <= '9')) {
            result = parseNumber();
        } else {
            fail("unexpected character");
        }
        --depth;
        return result;
    }

    Json parseObject()
    {
        Json obj = Json::object();
        ++i; // {
        if (consume('}'))
            return obj;
        while (ok) {
            skipWs();
            if (i >= s.size() || s[i] != '"') {
                fail("expected string key");
                break;
            }
            std::string key = parseString();
            if (!consume(':')) {
                fail("expected ':'");
                break;
            }
            Json v = parseValue();
            obj.set(std::move(key), std::move(v));
            if (consume(','))
                continue;
            if (consume('}'))
                break;
            fail("expected ',' or '}'");
        }
        return obj;
    }

    Json parseArray()
    {
        Json arr = Json::array();
        ++i; // [
        if (consume(']'))
            return arr;
        while (ok) {
            arr.push(parseValue());
            if (consume(','))
                continue;
            if (consume(']'))
                break;
            fail("expected ',' or ']'");
        }
        return arr;
    }

    unsigned parseHex4()
    {
        if (i + 4 > s.size()) {
            fail("bad unicode escape");
            return 0;
        }
        unsigned v = 0;
        for (int k = 0; k < 4; ++k) {
            char c = s[i++];
            v <<= 4;
            if (c >= '0' && c <= '9') v |= c - '0';
            else if (c >= 'a' && c <= 'f') v |= c - 'a' + 10;
            else if (c >= 'A' && c <= 'F') v |= c - 'A' + 10;
            else { fail("bad unicode escape"); return 0; }
        }
        return v;
    }

    std::string parseString()
    {
        std::string out;
        ++i; // opening quote
        while (ok) {
            if (i >= s.size()) {
                fail("unterminated string");
                break;
            }
            char c = s[i++];
            if (c == '"')
                break;
            if (c != '\\') {
                out.push_back(c);
                continue;
            }
            if (i >= s.size()) {
                fail("unterminated escape");
                break;
            }
            char e = s[i++];
            switch (e) {
            case '"': out.push_back('"'); break;
            case '\\': out.push_back('\\'); break;
            case '/': out.push_back('/'); break;
            case 'b': out.push_back('\b'); break;
            case 'f': out.push_back('\f'); break;
            case 'n': out.push_back('\n'); break;
            case 'r': out.push_back('\r'); break;
            case 't': out.push_back('\t'); break;
            case 'u': {
                unsigned cp = parseHex4();
                if (cp >= 0xD800 && cp <= 0xDBFF && i + 6 <= s.size() && s[i] == '\\' && s[i + 1] == 'u') {
                    i += 2;
                    unsigned lo = parseHex4();
                    cp = 0x10000 + ((cp - 0xD800) << 10) + (lo - 0xDC00);
                }
                out += toUtf8(static_cast<char32_t>(cp));
                break;
            }
            default:
                fail("bad escape");
            }
        }
        return out;
    }

    Json parseNumber()
    {
        size_t start = i;
        if (s[i] == '-')
            ++i;
        while (i < s.size() && ((s[i] >= '0' && s[i] <= '9') || s[i] == '.' || s[i] == 'e' || s[i] == 'E'
                                || s[i] == '+' || s[i] == '-'))
            ++i;
        std::string num(s.substr(start, i - start));
        char *end = nullptr;
        double v = std::strtod(num.c_str(), &end);
        if (end == num.c_str())
            fail("bad number");
        return Json(v);
    }
};

void escapeString(const std::string &s, std::string &out)
{
    out.push_back('"');
    for (unsigned char c : s) {
        switch (c) {
        case '"': out += "\\\""; break;
        case '\\': out += "\\\\"; break;
        case '\n': out += "\\n"; break;
        case '\r': out += "\\r"; break;
        case '\t': out += "\\t"; break;
        case '\b': out += "\\b"; break;
        case '\f': out += "\\f"; break;
        default:
            if (c < 0x20) {
                char buf[8];
                std::snprintf(buf, sizeof buf, "\\u%04x", c);
                out += buf;
            } else {
                out.push_back(static_cast<char>(c));
            }
        }
    }
    out.push_back('"');
}

} // namespace

Json Json::object(std::initializer_list<std::pair<std::string, Json>> items)
{
    Json j = object();
    for (const auto &it : items)
        j.set(it.first, it.second);
    return j;
}

Json Json::parse(std::string_view text, std::string *error)
{
    Parser p(text);
    return p.parseDocument(error);
}

const std::string &Json::asString() const
{
    static const std::string empty;
    return m_type == Type::String ? m_string : empty;
}

const Json &Json::operator[](std::string_view key) const
{
    if (m_type == Type::Object)
        for (const auto &kv : m_object)
            if (kv.first == key)
                return kv.second;
    return nullJson();
}

bool Json::contains(std::string_view key) const
{
    if (m_type == Type::Object)
        for (const auto &kv : m_object)
            if (kv.first == key)
                return true;
    return false;
}

Json &Json::set(std::string key, Json value)
{
    if (m_type != Type::Object) {
        *this = object();
    }
    for (auto &kv : m_object) {
        if (kv.first == key) {
            kv.second = std::move(value);
            return *this;
        }
    }
    m_object.emplace_back(std::move(key), std::move(value));
    return *this;
}

const Json &Json::operator[](size_t index) const
{
    if (m_type == Type::Array && index < m_array.size())
        return m_array[index];
    return nullJson();
}

size_t Json::size() const
{
    if (m_type == Type::Array)
        return m_array.size();
    if (m_type == Type::Object)
        return m_object.size();
    return 0;
}

Json &Json::push(Json value)
{
    if (m_type != Type::Array)
        *this = array();
    m_array.push_back(std::move(value));
    return *this;
}

std::string Json::dump() const
{
    std::string out;
    dumpTo(out);
    return out;
}

void Json::dumpTo(std::string &out) const
{
    switch (m_type) {
    case Type::Null: out += "null"; break;
    case Type::Bool: out += m_bool ? "true" : "false"; break;
    case Type::Number: {
        if (std::isfinite(m_number) && std::floor(m_number) == m_number && std::fabs(m_number) < 1e15) {
            out += std::to_string(static_cast<long long>(m_number));
        } else if (!std::isfinite(m_number)) {
            out += "null";
        } else {
            char buf[32];
            std::snprintf(buf, sizeof buf, "%.17g", m_number);
            out += buf;
        }
        break;
    }
    case Type::String: escapeString(m_string, out); break;
    case Type::Array: {
        out.push_back('[');
        bool first = true;
        for (const auto &v : m_array) {
            if (!first)
                out.push_back(',');
            first = false;
            v.dumpTo(out);
        }
        out.push_back(']');
        break;
    }
    case Type::Object: {
        out.push_back('{');
        bool first = true;
        for (const auto &kv : m_object) {
            if (!first)
                out.push_back(',');
            first = false;
            escapeString(kv.first, out);
            out.push_back(':');
            kv.second.dumpTo(out);
        }
        out.push_back('}');
        break;
    }
    }
}

} // namespace ppl
