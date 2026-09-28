#pragma once

#include <string>
#include <string_view>

namespace ppl {

// Decodes UTF-8 into code points. Invalid bytes become U+FFFD.
std::u32string toU32(std::string_view utf8);

// Encodes code points as UTF-8.
std::string toUtf8(std::u32string_view text);
std::string toUtf8(char32_t c);

// Number of UTF-16 code units needed for the code point range [begin, end).
int utf16Length(std::u32string_view text);

// ASCII-only upper case (keywords and command names are ASCII or Greek; Greek is left as is).
std::u32string asciiUpper(std::u32string_view text);
std::string asciiUpper(std::string_view text);

bool isLetter(char32_t c);
bool isDigit(char32_t c);
bool isSpace(char32_t c);

} // namespace ppl
