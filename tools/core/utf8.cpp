#include "utf8.h"

namespace ppl {

std::u32string toU32(std::string_view s)
{
    std::u32string out;
    out.reserve(s.size());
    size_t i = 0;
    while (i < s.size()) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        char32_t cp = 0xFFFD;
        int extra = 0;
        if (c < 0x80) { cp = c; }
        else if ((c & 0xE0) == 0xC0) { cp = c & 0x1F; extra = 1; }
        else if ((c & 0xF0) == 0xE0) { cp = c & 0x0F; extra = 2; }
        else if ((c & 0xF8) == 0xF0) { cp = c & 0x07; extra = 3; }
        else { out.push_back(0xFFFD); ++i; continue; }
        bool ok = true;
        for (int k = 1; k <= extra; ++k) {
            if (i + k >= s.size()) { ok = false; break; }
            unsigned char cc = static_cast<unsigned char>(s[i + k]);
            if ((cc & 0xC0) != 0x80) { ok = false; break; }
            cp = (cp << 6) | (cc & 0x3F);
        }
        if (!ok) { out.push_back(0xFFFD); ++i; continue; }
        out.push_back(cp);
        i += extra + 1;
    }
    return out;
}

std::string toUtf8(char32_t c)
{
    std::string out;
    if (c < 0x80) {
        out.push_back(static_cast<char>(c));
    } else if (c < 0x800) {
        out.push_back(static_cast<char>(0xC0 | (c >> 6)));
        out.push_back(static_cast<char>(0x80 | (c & 0x3F)));
    } else if (c < 0x10000) {
        out.push_back(static_cast<char>(0xE0 | (c >> 12)));
        out.push_back(static_cast<char>(0x80 | ((c >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (c & 0x3F)));
    } else {
        out.push_back(static_cast<char>(0xF0 | (c >> 18)));
        out.push_back(static_cast<char>(0x80 | ((c >> 12) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | ((c >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (c & 0x3F)));
    }
    return out;
}

std::string toUtf8(std::u32string_view text)
{
    std::string out;
    out.reserve(text.size());
    for (char32_t c : text)
        out += toUtf8(c);
    return out;
}

int utf16Length(std::u32string_view text)
{
    int n = 0;
    for (char32_t c : text)
        n += c > 0xFFFF ? 2 : 1;
    return n;
}

std::u32string asciiUpper(std::u32string_view text)
{
    std::u32string out(text);
    for (auto &c : out)
        if (c >= U'a' && c <= U'z')
            c = c - U'a' + U'A';
    return out;
}

std::string asciiUpper(std::string_view text)
{
    std::string out(text);
    for (auto &c : out)
        if (c >= 'a' && c <= 'z')
            c = static_cast<char>(c - 'a' + 'A');
    return out;
}

bool isDigit(char32_t c) { return c >= U'0' && c <= U'9'; }

bool isSpace(char32_t c)
{
    return c == U' ' || c == U'\t' || c == U'\r' || c == U'\n' || c == 0x00A0 || c == 0xFEFF
        || c == U'\f' || c == U'\v' || c == 0x2009 || c == 0x202F;
}

bool isLetter(char32_t c)
{
    if ((c >= U'a' && c <= U'z') || (c >= U'A' && c <= U'Z'))
        return true;
    if (c < 0x80)
        return false;
    // Latin-1 supplement and Latin extended letters (Polish letters etc.)
    if (c >= 0x00C0 && c <= 0x024F && c != 0x00D7 && c != 0x00F7)
        return true;
    // Greek and Coptic (θ, π, Σ, Δ, Π, σ …)
    if (c >= 0x0370 && c <= 0x03FF && c != 0x037E && c != 0x0387)
        return true;
    // Cyrillic
    if (c >= 0x0400 && c <= 0x04FF)
        return true;
    // Phonetic extensions (ᴇ – small capital E used by the calculator for exponents)
    if (c >= 0x1D00 && c <= 0x1D7F)
        return true;
    return false;
}

} // namespace ppl
