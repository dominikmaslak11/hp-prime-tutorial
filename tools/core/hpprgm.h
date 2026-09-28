#pragma once

// HP Prime .hpprgm program files (Connectivity Kit / firmware 2.x container).
//
// The container is a nested TLV structure starting with the magic 7C 61 8A B2.
// The PPL source is stored verbatim as UTF-16LE with LF line endings and a terminating NUL.
// Files are produced by replacing the source inside a template written by the Connectivity Kit
// and fixing the length of every record that contains it; the calculator rebuilds its compiled
// cache from the source.
//
// Format knowledge and the template come from hp-prime-kit by Jordi Rigau (MIT licence),
// https://github.com/JordiRigau/hp-prime-kit — measured on a G2 (firmware 2.4.15515) and on the
// Virtual Calculator 2.4. See tools/THIRD_PARTY.md.

#include <string>
#include <vector>

namespace ppl::hpprgm {

struct SourceLocation {
    std::vector<size_t> lengthOffsets;   // u32 lengths of every record that ends where the source ends
    size_t start = 0;                    // first byte of the UTF-16LE text
    size_t end = 0;                      // one past the terminating NUL
};

constexpr size_t HeaderEnd = 0x98;       // a bare program: the source starts exactly here

// The code template shipped with the tools (embedded in the executable).
const std::string &defaultTemplate();

bool locateSource(const std::string &file, SourceLocation &loc, std::string *error);
// Source as UTF-8 (NUL and nothing else stripped).
bool readSource(const std::string &file, std::string &utf8, std::string *error);
// LF line endings, no trailing newline — what the Connectivity Kit stores.
std::string normalizeSource(const std::string &utf8);
// Replaces the source of the template. Fails if the template carries a compiled block
// (unless force) or has no recognisable source.
bool writeSource(const std::string &templ, const std::string &utf8Source, std::string &out, std::string *error,
                 bool force = false);
bool hasCompiledBlock(const SourceLocation &loc);

} // namespace ppl::hpprgm
