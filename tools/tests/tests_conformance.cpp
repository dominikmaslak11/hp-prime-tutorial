// Conformance of the simulator against answers measured on the HP Prime Virtual Calculator 2.4
// (tests/data/hp-prime-kit-results.tsv, from hp-prime-kit by Jordi Rigau, MIT — see THIRD_PARTY.md).
//
// Each row is a call and the answer the emulator gave. Rows fall into:
//   pass         the simulator answers the same (numbers compared to 1e-9 relative)
//   unsupported  the simulator says "nieobsługiwane w symulatorze" (symbolic CAS, geometry, units…)
//   mismatch     a different answer, or an error where the emulator answered, or vice versa
// Rows answered "*compiles*" / "*does not compile*" check the validator instead: a program
// compiles when `ppl check` finds no errors. Programs that compile are loaded before every
// call, so rows can call them (ZQSTAT() …).
// The test fails if the number of passes drops below the recorded baseline, and writes
// every mismatch to build/conformance-report.txt.
#include "testing.h"

#include "analyzer.h"
#include "commanddb.h"
#include "interpreter.h"
#include "scripthost.h"
#include "utf8.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>

using namespace ppl;

namespace {

const int kBaselinePasses = 690;  // raised as the simulator improves (see the report)
const int kBaselineCompile = 30;  // "*compiles*" / "*does not compile*" rows the validator gets right

bool parseNumber(const std::string &s, double &v)
{
    if (s.empty())
        return false;
    std::string t = s;
    for (auto &c : t)
        if (c == 'E' || c == 'e')
            c = 'e';
    char *end = nullptr;
    v = std::strtod(t.c_str(), &end);
    return end && *end == '\0';
}

// The emulator shows "−" (U+2212) for minus, and its log loses the i glyph: "3-4*" is 3-4*i.
std::string normalizeEmulator(std::string s)
{
    for (size_t p; (p = s.find("\xE2\x88\x92")) != std::string::npos;)
        s.replace(p, 3, "-");
    for (size_t p = 0; (p = s.find('*', p)) != std::string::npos; ++p)
        if (p + 1 == s.size() || s[p + 1] == ',' || s[p + 1] == '}' || s[p + 1] == ']')
            s.insert(p + 1, "i");
    return s;
}

std::string unescapeNewlines(const std::string &s)
{
    std::string out;
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '\\' && i + 1 < s.size() && s[i + 1] == 'n') {
            out.push_back('\n');
            ++i;
        } else {
            out.push_back(s[i]);
        }
    }
    return out;
}

std::u32string firstFunctionName(const std::string &src)
{
    std::string s = src;
    if (s.rfind("EXPORT ", 0) == 0)
        s = s.substr(7);
    size_t p = s.find('(');
    return toU32(p == std::string::npos ? s : s.substr(0, p));
}

bool sameAnswer(const std::string &ours, const std::string &theirsRaw)
{
    std::string theirs = normalizeEmulator(theirsRaw);
    if (ours == theirs)
        return true;
    double a, b;
    if (parseNumber(ours, a) && parseNumber(theirs, b))
        return a == b || std::fabs(a - b) <= 1e-9 * std::max(1.0, std::fabs(b));
    // lists/vectors of numbers: compare element-wise
    auto split = [](std::string s) {
        std::vector<std::string> parts;
        std::string cur;
        for (char c : s) {
            if (c == '{' || c == '}' || c == '[' || c == ']' || c == ',') {
                if (!cur.empty())
                    parts.push_back(cur);
                parts.push_back(std::string(1, c));
                cur.clear();
            } else if (c != ' ') {
                cur.push_back(c);
            }
        }
        if (!cur.empty())
            parts.push_back(cur);
        return parts;
    };
    auto x = split(ours), y = split(theirs);
    if (x.size() != y.size())
        return false;
    for (size_t i = 0; i < x.size(); ++i) {
        if (x[i] == y[i])
            continue;
        if (!parseNumber(x[i], a) || !parseNumber(y[i], b))
            return false;
        if (std::fabs(a - b) > 1e-9 * std::max(1.0, std::fabs(b)))
            return false;
    }
    return true;
}

} // namespace

TEST(conformance_with_virtual_calculator)
{
    std::ifstream f(std::string(PPL_REPO_ROOT) + "/tools/tests/data/hp-prime-kit-results.tsv", std::ios::binary);
    CHECK_MSG(static_cast<bool>(f), "brak pliku tests/data/hp-prime-kit-results.tsv");
    std::string line;
    int pass = 0, unsupported = 0, mismatch = 0, total = 0, compilePass = 0, compileTotal = 0;
    std::ostringstream report, compileReport;
    std::vector<std::vector<std::string>> rows;
    std::vector<std::string> libraries; // programs the emulator compiled
    while (std::getline(f, line)) {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        if (line.empty() || line[0] == '#')
            continue;
        std::vector<std::string> col;
        std::string cur;
        for (char c : line) {
            if (c == '\t') {
                col.push_back(cur);
                cur.clear();
            } else {
                cur.push_back(c);
            }
        }
        col.push_back(cur);
        if (col.size() < 3)
            continue;
        if (col[2] == "*compiles*" || col[2] == "*does not compile*") {
            ++compileTotal;
            std::string src = unescapeNewlines(col[1]);
            bool expectOk = col[2] == "*compiles*";
            AnalysisResult r = Analyzer().analyze(src);
            bool ok = std::none_of(r.diagnostics.begin(), r.diagnostics.end(),
                                   [](const Diagnostic &d) { return d.severity == Severity::Error; });
            if (ok == expectOk) {
                ++compilePass;
            } else {
                compileReport << col[0] << "\t" << col[1] << "\n    emulator: " << col[2] << "\n    walidator: ";
                for (const auto &d : r.diagnostics)
                    if (d.severity == Severity::Error)
                        compileReport << d.line << ": " << d.message << " ";
                compileReport << (ok ? "brak błędów" : "") << "\n";
            }
            // AREA(zr) from "global-namespace" would hide the built-in AREA in the other rows
            if (expectOk && !CommandDatabase::instance().findCommand(firstFunctionName(src)))
                libraries.push_back(src);
            continue;
        }
        rows.push_back(col);
    }
    for (const auto &col : rows) {
        ++total;
        const std::string &entry = col[0], &call = col[1], &expected = col[2];
        std::string body = call;
        bool statements = call.find("RETURN") != std::string::npos || call.find(';') != std::string::npos;
        if (!statements)
            body = "RETURN " + call + ";";
        else if (body.back() != ';')
            body += ";";
        std::string program = "EXPORT CONFORM()\nBEGIN\n" + body + "\nEND;\n";
        ScriptedHost host;
        Interpreter in(host);
        host.interpreter = &in;
        in.setMaxSteps(200000);
        in.setSeed(1);
        std::string ours, error;
        bool unsup = false;
        try {
            for (const auto &lib : libraries)
                in.loadProgram(toU32(lib), firstFunctionName(lib));
            in.loadProgram(toU32(program), U"CONFORM");
            ours = toUtf8(in.format(in.run(U"CONFORM()")));
        } catch (const ParseError &e) {
            error = "składnia: " + e.message;
        } catch (const RuntimeError &e) {
            error = e.what();
            unsup = e.code == 99;
        } catch (const KillSignal &) {
            error = "KILL";
        } catch (...) {
            error = "wyjątek";
        }
        bool expectError = expected == "*error*";
        if (unsup && !expectError) {
            ++unsupported;
            continue;
        }
        bool ok = expectError ? !error.empty() : (error.empty() && sameAnswer(ours, expected));
        if (ok) {
            ++pass;
        } else {
            ++mismatch;
            report << entry << "\t" << call << "\n    emulator: " << expected << "\n    symulator: "
                   << (error.empty() ? ours : "BŁĄD " + error) << "\n";
        }
    }
    std::ofstream out("conformance-report.txt", std::ios::binary);
    out << "Zgodność z HP Prime Virtual Calculator 2.4: " << pass << " zgodnych, " << unsupported
        << " nieobsługiwanych, " << mismatch << " rozbieżności (z " << total << ")\n"
        << "Kompilacja (walidator): " << compilePass << "/" << compileTotal << " zgodnych\n\n"
        << compileReport.str() << "\n" << report.str();
    std::printf("  zgodność: %d/%d zgodnych, %d nieobsługiwanych, %d rozbieżności; kompilacja %d/%d"
                " (szczegóły: conformance-report.txt)\n",
                pass, total, unsupported, mismatch, compilePass, compileTotal);
    CHECK_MSG(pass >= kBaselinePasses, "spadek zgodności poniżej " + std::to_string(kBaselinePasses));
    CHECK_MSG(compilePass >= kBaselineCompile, "spadek zgodności walidatora poniżej " + std::to_string(kBaselineCompile));
}
