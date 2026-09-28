#pragma once

#include "commanddb.h"
#include "diagnostic.h"

#include <string>
#include <string_view>
#include <vector>

namespace ppl {

struct FunctionInfo {
    std::string name;
    std::u32string name32;
    std::vector<std::string> params;
    bool exported = false;
    bool isView = false;
    bool isKey = false;
    bool defined = false;
    std::string viewTitle;
    int line = 0;          // position of the name in the header
    int column = 0;
    int offset = -1;
    int endLine = 0;       // line of the closing END
    int endOffset = -1;
    int bodyOffset = -1;   // offset of BEGIN
    int declOffset = -1;   // first forward declaration
};

struct VariableInfo {
    std::string name;
    std::u32string name32;
    bool exported = false;
    int line = 0;
    int column = 0;
    int offset = 0;
};

struct LocalInfo {
    std::string name;
    std::u32string name32;
    bool isParam = false;
    int functionIndex = -1;
    int line = 0;
    int column = 0;
    int offset = 0;
};

struct AnalysisResult {
    std::vector<Diagnostic> diagnostics;
    std::vector<FunctionInfo> functions;
    std::vector<VariableInfo> fileVariables;
    std::vector<LocalInfo> locals;

    int count(Severity s) const;
    int errorCount() const { return count(Severity::Error); }
    int warningCount() const { return count(Severity::Warning); }
    // Function whose body contains the offset, or -1.
    int functionAt(int offset) const;
};

class Analyzer {
public:
    explicit Analyzer(const CommandDatabase &db = CommandDatabase::instance()) : m_db(db) {}

    AnalysisResult analyze(const std::u32string &source) const;
    AnalysisResult analyze(std::string_view utf8Source) const;

private:
    const CommandDatabase &m_db;
};

} // namespace ppl
