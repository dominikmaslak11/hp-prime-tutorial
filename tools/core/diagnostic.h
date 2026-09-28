#pragma once

#include <string>

namespace ppl {

enum class Severity { Error = 1, Warning = 2, Info = 3, Hint = 4 };

struct Diagnostic {
    Severity severity = Severity::Error;
    int line = 1;     // 1-based
    int column = 1;   // 1-based, in code points
    int offset = 0;   // 0-based code point offset
    int length = 1;   // in code points
    std::string message; // UTF-8, Polish
    std::string code;    // machine-readable id, e.g. "missing-end"
};

inline const char *severityName(Severity s)
{
    switch (s) {
    case Severity::Error: return "error";
    case Severity::Warning: return "warning";
    case Severity::Info: return "info";
    case Severity::Hint: return "hint";
    }
    return "error";
}

} // namespace ppl
