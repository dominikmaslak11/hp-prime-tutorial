#include "hpprgm.h"

#include "utf8.h"

#include <algorithm>
#include <cstdint>
#include <cstring>

namespace ppl::hpprgm {

extern const unsigned char kCodeTemplate[];
extern const size_t kCodeTemplateSize;

namespace {

const unsigned char kMagic[4] = {0x7C, 0x61, 0x8A, 0xB2};

uint32_t u32(const std::string &b, size_t off)
{
    return static_cast<uint32_t>(static_cast<unsigned char>(b[off])) | static_cast<uint32_t>(static_cast<unsigned char>(b[off + 1])) << 8
           | static_cast<uint32_t>(static_cast<unsigned char>(b[off + 2])) << 16
           | static_cast<uint32_t>(static_cast<unsigned char>(b[off + 3])) << 24;
}

void putU32(std::string &b, size_t off, uint32_t v)
{
    b[off] = static_cast<char>(v & 0xFF);
    b[off + 1] = static_cast<char>((v >> 8) & 0xFF);
    b[off + 2] = static_cast<char>((v >> 16) & 0xFF);
    b[off + 3] = static_cast<char>((v >> 24) & 0xFF);
}

std::u32string decodeUtf16le(const std::string &b, size_t start, size_t end, bool *ok)
{
    std::u32string out;
    *ok = true;
    for (size_t i = start; i + 1 < end; i += 2) {
        char32_t u = static_cast<unsigned char>(b[i]) | static_cast<unsigned char>(b[i + 1]) << 8;
        if (u >= 0xD800 && u <= 0xDBFF) {
            if (i + 3 >= end) { *ok = false; return out; }
            char32_t lo = static_cast<unsigned char>(b[i + 2]) | static_cast<unsigned char>(b[i + 3]) << 8;
            if (lo < 0xDC00 || lo > 0xDFFF) { *ok = false; return out; }
            out.push_back(0x10000 + ((u - 0xD800) << 10) + (lo - 0xDC00));
            i += 2;
        } else if (u >= 0xDC00 && u <= 0xDFFF) {
            *ok = false;
            return out;
        } else {
            out.push_back(u);
        }
    }
    return out;
}

bool isControl(char32_t c)
{
    return c < 0x20 || (c >= 0x7F && c < 0xA0) || (c >= 0xD800 && c <= 0xDFFF) || c == 0xFFFE || c == 0xFFFF;
}

// A PPL source block: ends in a UTF-16 NUL and is (almost) all printable text.
bool looksLikeSource(const std::string &b, size_t start, size_t end)
{
    size_t len = end - start;
    if (len < 8 || len % 2 || b[end - 2] != 0 || b[end - 1] != 0)
        return false;
    bool ok;
    std::u32string txt = decodeUtf16le(b, start, end - 2, &ok);
    if (!ok || txt.empty())
        return false;
    size_t n = std::min<size_t>(txt.size(), 2000), printable = 0;
    for (size_t i = 0; i < n; ++i)
        if (txt[i] == U'\n' || txt[i] == U'\t' || !isControl(txt[i]))
            ++printable;
    return printable * 10 >= n * 9;
}

std::string encodeUtf16le(const std::u32string &text)
{
    std::string out;
    out.reserve(text.size() * 2 + 2);
    auto put = [&](char32_t u) {
        out.push_back(static_cast<char>(u & 0xFF));
        out.push_back(static_cast<char>((u >> 8) & 0xFF));
    };
    for (char32_t c : text) {
        if (c > 0xFFFF) {
            c -= 0x10000;
            put(0xD800 + (c >> 10));
            put(0xDC00 + (c & 0x3FF));
        } else {
            put(c);
        }
    }
    put(0); // terminating NUL
    return out;
}

} // namespace

const std::string &defaultTemplate()
{
    static const std::string t(reinterpret_cast<const char *>(kCodeTemplate), kCodeTemplateSize);
    return t;
}

bool locateSource(const std::string &b, SourceLocation &loc, std::string *error)
{
    auto fail = [&](const char *m) {
        if (error)
            *error = m;
        return false;
    };
    if (b.size() < 16 || std::memcmp(b.data(), kMagic, 4) != 0)
        return fail("to nie jest plik programu HP Prime (brak sygnatury 7C 61 8A B2).");
    const size_t n = b.size();
    // Candidates: [u32 length][u32 tag][UTF-16LE text][NUL]
    std::vector<std::pair<size_t, size_t>> cands;
    for (size_t o = 0x0C; o + 12 < n; ++o) {
        if (b[o + 9] != 0)
            continue;
        unsigned char c = static_cast<unsigned char>(b[o + 8]);
        if (!(c == 0x0A || (c >= 0x20 && c < 0x7F) || c >= 0xA0))
            continue;
        size_t end = o + 4 + u32(b, o);
        if (end > n || end < o + 8 || end - (o + 8) < 8)
            continue;
        if (!looksLikeSource(b, o + 8, end))
            continue;
        cands.push_back({o, end});
    }
    if (cands.empty())
        return fail("nie znaleziono kodu źródłowego (pusty program?).");
    // the largest candidate decides where the source ends; among those ending there, the innermost wins
    size_t bestEnd = 0, bestSpan = 0;
    for (const auto &[o, e] : cands)
        if (e - o > bestSpan) {
            bestSpan = e - o;
            bestEnd = e;
        }
    size_t off = 0;
    for (const auto &[o, e] : cands)
        if (e == bestEnd)
            off = std::max(off, o);
    loc = SourceLocation();
    for (size_t o = 0x0C; o <= off; ++o)
        if (o + 4 <= n && o + 4 + u32(b, o) == bestEnd)
            loc.lengthOffsets.push_back(o);
    loc.start = off + 8;
    loc.end = bestEnd;
    return true;
}

bool hasCompiledBlock(const SourceLocation &loc) { return loc.start > HeaderEnd; }

bool readSource(const std::string &file, std::string &utf8, std::string *error)
{
    SourceLocation loc;
    if (!locateSource(file, loc, error))
        return false;
    bool ok;
    std::u32string txt = decodeUtf16le(file, loc.start, loc.end, &ok);
    while (!txt.empty() && txt.back() == 0)
        txt.pop_back();
    utf8 = toUtf8(txt);
    return true;
}

std::string normalizeSource(const std::string &utf8)
{
    std::string t;
    t.reserve(utf8.size());
    for (size_t i = 0; i < utf8.size(); ++i) {
        if (utf8[i] == '\r') {
            t.push_back('\n');
            if (i + 1 < utf8.size() && utf8[i + 1] == '\n')
                ++i;
        } else {
            t.push_back(utf8[i]);
        }
    }
    if (!t.empty() && t.back() == '\n')
        t.pop_back();
    return t;
}

bool writeSource(const std::string &templ, const std::string &utf8Source, std::string &out, std::string *error, bool force)
{
    SourceLocation loc;
    if (!locateSource(templ, loc, error))
        return false;
    if (hasCompiledBlock(loc) && !force) {
        if (error)
            *error = "szablon zawiera skompilowany blok (" + std::to_string(loc.start - HeaderEnd)
                     + " B) — użyj programu utworzonego w Connectivity Kit, który nie był jeszcze na kalkulatorze.";
        return false;
    }
    std::string blob = encodeUtf16le(toU32(utf8Source));
    long long delta = static_cast<long long>(blob.size()) - static_cast<long long>(loc.end - loc.start);
    out = templ.substr(0, loc.start) + blob + templ.substr(loc.end);
    for (size_t off : loc.lengthOffsets)
        putU32(out, off, static_cast<uint32_t>(static_cast<long long>(u32(out, off)) + delta));
    return true;
}

} // namespace ppl::hpprgm
