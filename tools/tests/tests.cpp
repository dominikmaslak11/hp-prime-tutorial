#include "testing.h"

#include "analyzer.h"
#include "commanddb.h"
#include "formatter.h"
#include "json.h"
#include "lexer.h"
#include "services.h"
#include "utf8.h"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <sstream>

using namespace ppl;

namespace {

AnalysisResult analyze(const std::string &src) { return Analyzer().analyze(src); }

bool hasCode(const AnalysisResult &r, const std::string &code, Severity sev)
{
    for (const auto &d : r.diagnostics)
        if (d.code == code && d.severity == sev)
            return true;
    return false;
}

std::string dump(const AnalysisResult &r)
{
    std::string s;
    for (const auto &d : r.diagnostics)
        s += "\n    " + std::to_string(d.line) + ":" + std::to_string(d.column) + " " + severityName(d.severity) + " ["
             + d.code + "] " + d.message;
    return s;
}

} // namespace

// ------------------------------------------------------------------ JSON & database

TEST(json_roundtrip)
{
    std::string err;
    Json j = Json::parse(R"({"a":[1,2.5,"x\ną"],"b":true,"c":null})", &err);
    CHECK(err.empty());
    CHECK(j["a"].size() == 3);
    CHECK(j["a"][1].asNumber() == 2.5);
    CHECK(j["a"][2].asString() == "x\nЕ" || j["a"][2].asString() == "x\n\xC4\x85");
    CHECK(j["b"].asBool());
    CHECK(j["c"].isNull());
    Json back = Json::parse(j.dump(), &err);
    CHECK(err.empty());
    CHECK(back["a"][2].asString() == j["a"][2].asString());
}

TEST(command_database_loaded)
{
    const auto &db = CommandDatabase::instance();
    CHECK(db.commands().size() > 200);
    const CommandInfo *c = db.findCommand(std::string("INPUT"));
    CHECK(c && c->minArgs == 1 && c->maxArgs == 6);
    CHECK(db.findCommand(std::string("textout_p")) != nullptr);  // case-insensitive
    CHECK(db.findCommand(std::string("ΣLIST")) != nullptr);
    CHECK(db.isSystemVariable(U"HAngle"));
    CHECK(db.isSystemVariable(U"L7"));
    CHECK(!db.isSystemVariable(U"xyz"));
    CHECK(db.isKeyName(U"K_Sin"));
    CHECK(db.isKeyName(U"KSA_Plus"));
    CHECK(!db.isKeyName(U"KS_Help"));
    CHECK(!db.isKeyName(U"K_sin"));
}

// ------------------------------------------------------------------ lexer

TEST(lexer_basic_tokens)
{
    std::vector<Diagnostic> d;
    auto toks = Lexer::tokenize(U"A := #FFh + 1.5E-3 + 5_m; // komentarz\nPRINT(\"a\"\"b\\n\");", &d);
    CHECK(d.empty());
    CHECK(toks[0].kind == TokenKind::Identifier);
    CHECK(toks[1].isOp(U":="));
    CHECK(toks[2].kind == TokenKind::Integer);
    CHECK(toks[4].kind == TokenKind::Number && toks[4].text == U"1.5E-3");
    CHECK(toks[6].kind == TokenKind::Number && toks[6].text == U"5_m");
    CHECK(toks[8].kind == TokenKind::Identifier && toks[8].line == 2);
    CHECK(toks[10].kind == TokenKind::String);
}

TEST(lexer_unicode_identifiers)
{
    auto toks = Lexer::tokenize(U"x := →HMS(ΣLIST(L1)) ▶ B→R;");
    CHECK(toks[2].text == U"→HMS");
    CHECK(toks[4].text == U"ΣLIST");
    CHECK(toks[9].isOp(U"▶"));
    CHECK(toks[10].text == U"B→R");
}

TEST(lexer_errors)
{
    std::vector<Diagnostic> d;
    Lexer::tokenize(U"PRINT(„tekst”);", &d);
    CHECK(!d.empty() && d[0].code == "typographic-quote");
    d.clear();
    Lexer::tokenize(U"PRINT(\"abc);", &d);
    CHECK(!d.empty() && d[0].code == "unterminated-string");
    d.clear();
    Lexer::tokenize(U"x := #XYZ;", &d);
    CHECK(!d.empty() && d[0].code == "bad-integer");
}

// ------------------------------------------------------------------ analyzer

TEST(valid_program_has_no_errors)
{
    auto r = analyze(R"(
EXPORT MOPMT(L,R,M)
BEGIN
  LOCAL K:=R/1200;
  K:=L*K/(1-(1+K)^-M);
  RETURN "Payment ="+K;
END;
)");
    CHECK_MSG(r.errorCount() == 0 && r.warningCount() == 0, dump(r));
    CHECK(r.functions.size() == 1 && r.functions[0].exported && r.functions[0].params.size() == 3);
}

TEST(missing_end_is_reported_with_opener_line)
{
    auto r = analyze("EXPORT F(x)\nBEGIN\n  IF x>0 THEN\n    RETURN 1;\n  RETURN 0;\nEND;\n");
    CHECK_MSG(hasCode(r, "missing-end", Severity::Error), dump(r));
}

TEST(missing_semicolon_between_statements)
{
    auto r = analyze("EXPORT F()\nBEGIN\n  LOCAL a;\n  a := 1\n  a := 2;\n  RETURN a;\nEND;\n");
    CHECK_MSG(hasCode(r, "missing-semicolon", Severity::Error), dump(r));
}

TEST(equals_in_local_declaration)
{
    auto r = analyze("EXPORT F()\nBEGIN\n  LOCAL i, s = 0;\n  RETURN s;\nEND;\n");
    CHECK_MSG(hasCode(r, "equals-in-declaration", Severity::Error), dump(r));
}

TEST(equals_in_condition_warns)
{
    auto r = analyze("EXPORT F(x)\nBEGIN\n  IF x=5 THEN RETURN 1; END;\n  RETURN 0;\nEND;\n");
    CHECK_MSG(hasCode(r, "equals-in-condition", Severity::Warning), dump(r));
}

TEST(use_before_declaration)
{
    auto bad = analyze("EXPORT MAIN()\nBEGIN\n  RETURN SUB1(2);\nEND;\n\nSUB1(x)\nBEGIN\n  RETURN x^2;\nEND;\n");
    CHECK_MSG(hasCode(bad, "use-before-declaration", Severity::Error), dump(bad));
    auto good = analyze("SUB1();\nEXPORT MAIN()\nBEGIN\n  RETURN SUB1(2);\nEND;\n\nSUB1(x)\nBEGIN\n  RETURN x^2;\nEND;\n");
    CHECK_MSG(good.errorCount() == 0, dump(good));
}

TEST(argument_counts)
{
    auto r = analyze("EXPORT F()\nBEGIN\n  MSGBOX();\n  RETURN LEFT(\"abc\");\nEND;\n");
    int n = 0;
    for (const auto &d : r.diagnostics)
        if (d.code == "wrong-arg-count")
            ++n;
    CHECK_MSG(n == 2, dump(r));
    auto r2 = analyze("G();\nEXPORT F()\nBEGIN\n  RETURN G(1,2,3);\nEND;\nG(a,b)\nBEGIN\n  RETURN a+b;\nEND;\n");
    CHECK_MSG(hasCode(r2, "wrong-arg-count", Severity::Error), dump(r2));
}

TEST(typed_system_variables)
{
    auto r = analyze("EXPORT F()\nBEGIN\n  L1 := 5;\n  A := {1,2};\n  M1 := [[1,2],[3,4]];\n  L2 := {1};\nEND;\n");
    int n = 0;
    for (const auto &d : r.diagnostics)
        if (d.code == "wrong-type")
            ++n;
    CHECK_MSG(n == 2, dump(r));
}

TEST(loop_over_constant_i_without_local)
{
    auto bad = analyze("EXPORT SUMA()\nBEGIN\n  LOCAL s;\n  FOR i FROM 1 TO 3 DO s := s+i; END;\n  RETURN s;\nEND;\n");
    CHECK_MSG(hasCode(bad, "assign-to-constant", Severity::Error), dump(bad));
    auto good = analyze("EXPORT SUMA()\nBEGIN\n  LOCAL s, i;\n  FOR i FROM 1 TO 3 DO s := s+i; END;\n  RETURN s;\nEND;\n");
    CHECK_MSG(good.errorCount() == 0, dump(good));
}

TEST(break_outside_loop)
{
    auto r = analyze("EXPORT F()\nBEGIN\n  BREAK;\nEND;\n");
    CHECK_MSG(hasCode(r, "break-outside-loop", Severity::Error), dump(r));
}

TEST(unknown_name_and_unused_local)
{
    auto r = analyze("EXPORT F()\nBEGIN\n  LOCAL a, b;\n  a := zmienna + 1;\n  RETURN a;\nEND;\n");
    CHECK_MSG(hasCode(r, "unknown-name", Severity::Warning), dump(r));
    CHECK_MSG(hasCode(r, "unused-local", Severity::Warning), dump(r));
}

TEST(all_block_kinds)
{
    auto r = analyze(R"(
EXPORT ALL(n)
BEGIN
  LOCAL i, s := 0, k;
  FOR i FROM 1 TO n DO
    IF i MOD 2 == 0 THEN CONTINUE; END;
    s := s + i;
  END;
  FOR i FROM n DOWNTO 1 STEP 2 DO s := s - 1; END;
  WHILE s > 100 DO s := s/2; END;
  REPEAT k := GETKEY; UNTIL k <> -1;
  CASE
    IF s<0 THEN RETURN "ujemna"; END;
    IF 0<=s AND s<=1 THEN RETURN "mała"; END;
    DEFAULT RETURN "duża";
  END;
  IFERR
    s := 1/0;
  THEN
    MSGBOX("błąd");
  ELSE
    s ▶ k;
  END;
  RETURN when(s>0, s, -s);
END;
)");
    CHECK_MSG(r.errorCount() == 0 && r.warningCount() == 0, dump(r));
}

TEST(view_key_and_file_variables)
{
    auto r = analyze(R"(
EXPORT SIDES, ROLLS;
licznik;
VIEW "Start", START()
BEGIN
  licznik := 0;
  STARTVIEW(6,1);
END;
KEY K_Sin()
BEGIN
  RETURN "ALOG(";
END;
KEY K_Foo()
BEGIN
  RETURN "";
END;
)");
    CHECK(r.errorCount() == 0);
    CHECK_MSG(hasCode(r, "unknown-key", Severity::Warning), dump(r));
    CHECK(r.fileVariables.size() == 3);
}

TEST(graphics_program)
{
    auto r = analyze(R"(
EXPORT DRAWARCS()
BEGIN
  RECT();
  ARC_P(60,110,30,#008000h);
  ARC_P(140,110,{30,50},#00A0C0h);
  HAngle:=1;
  ARC_P(220,110,30,30,150,#400080h);
  FILLPOLY_P({(80,100),(160,20),(240,100)},#FFC0C0h);
  TEXTOUT_P("x="+2, 10, 10, 2, RGB(0,0,255));
  WAIT(0);
END;
)");
    CHECK_MSG(r.errorCount() == 0 && r.warningCount() == 0, dump(r));
}

TEST(cas_and_template_calculus)
{
    auto r = analyze(R"(
EXPORT CALC()
BEGIN
  PRINT(∂(SIN(X),X=π/4));
  PRINT(∫(SIN(X),X,1,3));
  PRINT(Σ(N^3,N,1,12));
  PRINT(3*A+5|A=-2);
  RETURN CAS.idivis(12);
END;
)");
    CHECK_MSG(r.errorCount() == 0 && r.warningCount() == 0, dump(r));
}

// ------------------------------------------------------------------ formatter

TEST(formatter_indents_blocks)
{
    std::u32string in = U"export f(x)\nbegin\nif x>0 then\nreturn 1;\nelse\nreturn 0;\nend;\nend;";
    std::u32string out = Formatter::format(in);
    std::u32string expected = U"EXPORT f(x)\nBEGIN\n  IF x>0 THEN\n    RETURN 1;\n  ELSE\n    RETURN 0;\n  END;\nEND;";
    CHECK_MSG(out == expected, toUtf8(out));
}

TEST(formatter_iferr_and_repeat)
{
    std::u32string in = U"F()\nBEGIN\nIFERR\nx:=1;\nTHEN\ny:=2;\nEND;\nREPEAT\nk:=GETKEY;\nUNTIL k<>-1;\nEND;";
    std::u32string out = Formatter::format(in);
    std::u32string expected =
        U"F()\nBEGIN\n  IFERR\n    x:=1;\n  THEN\n    y:=2;\n  END;\n  REPEAT\n    k:=GETKEY;\n  UNTIL k<>-1;\nEND;";
    CHECK_MSG(out == expected, toUtf8(out));
}

TEST(symbol_conversion_skips_strings)
{
    std::u32string in = U"IF a<>b AND c<=d THEN PRINT(\"<>\"); END;";
    std::u32string sym = Formatter::toCalculatorSymbols(in);
    CHECK(sym == U"IF a≠b AND c≤d THEN PRINT(\"<>\"); END;");
    CHECK(Formatter::toAscii(sym) == in);
}

// ------------------------------------------------------------------ services

TEST(services_hover_completion_signature)
{
    std::u32string src = U"EXPORT F(abc)\nBEGIN\n  LOCAL wynik;\n  wynik := TEXTOUT_P(\"x\", 1, \nEND;";
    TextDocument doc(src);
    auto a = Analyzer().analyze(src);
    int off = static_cast<int>(src.find(U"TEXTOUT_P")) + 2;
    auto h = Services::hover(doc, a, off);
    CHECK(h && h->find("TEXTOUT_P") != std::string::npos);
    int sigOff = static_cast<int>(src.find(U"1, ")) + 3;
    auto sig = Services::signatureHelp(doc, a, sigOff);
    CHECK(sig && sig->activeParameter == 2);
    CHECK(sig && sig->parameters.size() == 8);
    auto items = Services::completions(doc, a, sigOff);
    bool hasLocal = false, hasCmd = false;
    for (const auto &it : items) {
        hasLocal |= it.label == "wynik";
        hasCmd |= it.label == "MSGBOX";
    }
    CHECK(hasLocal && hasCmd);
    auto def = Services::definition(doc, a, static_cast<int>(src.find(U"wynik :=")));
    CHECK(def && *def == static_cast<int>(src.find(U"wynik;")));
}

// ------------------------------------------------------------------ corpus: all programs of the tutorial

namespace {

std::vector<std::pair<std::string, std::string>> extractBlocks(const std::filesystem::path &file)
{
    std::ifstream f(file, std::ios::binary);
    std::string text((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    std::vector<std::pair<std::string, std::string>> blocks;
    if (file.extension() == ".txt") {
        blocks.push_back({file.filename().string(), text});
        return blocks;
    }
    std::istringstream in(text);
    std::string line, cur;
    bool inside = false;
    int lineNo = 0, start = 0;
    while (std::getline(in, line)) {
        ++lineNo;
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        std::string trimmed = line.substr(line.find_first_not_of(' ') == std::string::npos ? line.size() : line.find_first_not_of(' '));
        if (trimmed.rfind("```", 0) == 0) {
            if (!inside) {
                inside = true;
                cur.clear();
                start = lineNo;
            } else {
                inside = false;
                blocks.push_back({file.filename().string() + ":" + std::to_string(start), cur});
            }
            continue;
        }
        if (inside)
            cur += line + "\n";
    }
    return blocks;
}

bool isCompleteProgram(const std::string &code)
{
    if (code.find("BEGIN") == std::string::npos)
        return false;
    if (code.find("...") != std::string::npos || code.find("…") != std::string::npos)
        return false;
    if (code.find("#cas") != std::string::npos || code.find("zmienna FROM") != std::string::npos
        || code.find("celowo zawiera błędy") != std::string::npos)
        return false;
    return true;
}

} // namespace

TEST(tutorial_corpus_has_no_errors)
{
    namespace fs = std::filesystem;
    fs::path root = fs::path(PPL_REPO_ROOT);
    std::vector<fs::path> files;
    for (const char *dir : {"rozdzialy", "dodatki", "programy"}) {
        fs::path d = root / dir;
        if (!fs::exists(d))
            continue;
        for (const auto &e : fs::directory_iterator(d))
            if (e.path().extension() == ".md" || e.path().extension() == ".txt")
                files.push_back(e.path());
    }
    CHECK_MSG(!files.empty(), "nie znaleziono plików kursu w " + root.string());
    std::sort(files.begin(), files.end());
    int programs = 0, warnings = 0;
    for (const auto &f : files) {
        for (const auto &[where, code] : extractBlocks(f)) {
            if (!isCompleteProgram(code))
                continue;
            ++programs;
            auto r = analyze(code);
            warnings += r.warningCount();
            CHECK_MSG(r.errorCount() == 0, where + dump(r));
            if (std::getenv("PPL_VERBOSE") && r.warningCount() > 0)
                std::printf("  %s%s\n", where.c_str(), dump(r).c_str());
        }
    }
    std::printf("  korpus: %d programów, %d ostrzeżeń\n", programs, warnings);
    CHECK(programs > 80);
}



// ------------------------------------------------------------------ .hpprgm container

#include "hpprgm.h"

TEST(hpprgm_template_roundtrip_is_byte_exact)
{
    const std::string &t = hpprgm::defaultTemplate();
    CHECK(t.size() == 1818);
    std::string src, err, rebuilt;
    CHECK_MSG(hpprgm::readSource(t, src, &err), err);
    CHECK(src.rfind("// Code template", 0) == 0 || !src.empty());
    CHECK(hpprgm::writeSource(t, src, rebuilt, &err));
    CHECK(rebuilt == t);
}

TEST(hpprgm_build_and_read_back)
{
    std::string src = "EXPORT ŻÓŁW(x)\nBEGIN\n  RETURN \"π≠3 ▶ θ\"+x;\nEND;";
    std::string out, back, err;
    CHECK_MSG(hpprgm::writeSource(hpprgm::defaultTemplate(), src, out, &err), err);
    CHECK_MSG(hpprgm::readSource(out, back, &err), err);
    CHECK(back == src);
    hpprgm::SourceLocation loc;
    CHECK(hpprgm::locateSource(out, loc, &err) && loc.start == hpprgm::HeaderEnd && !hpprgm::hasCompiledBlock(loc));
    // file size grows by exactly the UTF-16 size difference
    std::string t0;
    hpprgm::readSource(hpprgm::defaultTemplate(), t0, &err);
    CHECK(out.size() == hpprgm::defaultTemplate().size() + 2 * (toU32(src).size() - toU32(t0).size()));
    CHECK(hpprgm::normalizeSource("a\r\nb\r\n") == "a\nb");
    std::string junk = "not a program";
    CHECK(!hpprgm::readSource(junk, back, &err));
}
