#include "cli.h"

#include "analyzer.h"
#include "formatter.h"
#include "json.h"
#include "services.h"
#include "utf8.h"

#include <cstdio>
#include <fstream>
#include <iterator>
#include <sstream>

namespace ppl::cli {

namespace {

void out(const std::string &s) { std::fwrite(s.data(), 1, s.size(), stdout); }
void err(const std::string &s) { std::fwrite(s.data(), 1, s.size(), stderr); }

std::string utf16ToUtf8(const std::string &bytes, bool bigEndian, size_t start)
{
    std::u32string cps;
    for (size_t i = start; i + 1 < bytes.size(); i += 2) {
        unsigned char a = static_cast<unsigned char>(bytes[i]);
        unsigned char b = static_cast<unsigned char>(bytes[i + 1]);
        char32_t unit = bigEndian ? (a << 8 | b) : (b << 8 | a);
        if (unit >= 0xD800 && unit <= 0xDBFF && i + 3 < bytes.size()) {
            unsigned char c = static_cast<unsigned char>(bytes[i + 2]);
            unsigned char d = static_cast<unsigned char>(bytes[i + 3]);
            char32_t lo = bigEndian ? (c << 8 | d) : (d << 8 | c);
            if (lo >= 0xDC00 && lo <= 0xDFFF) {
                cps.push_back(0x10000 + ((unit - 0xD800) << 10) + (lo - 0xDC00));
                i += 2;
                continue;
            }
        }
        cps.push_back(unit);
    }
    return toUtf8(cps);
}

} // namespace

bool readSourceFile(const std::string &path, std::string &utf8, std::string *error)
{
    std::ifstream f(path, std::ios::binary);
    if (!f) {
        if (error)
            *error = "nie można otworzyć pliku";
        return false;
    }
    std::string bytes((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    if (bytes.size() >= 3 && static_cast<unsigned char>(bytes[0]) == 0xEF && static_cast<unsigned char>(bytes[1]) == 0xBB
        && static_cast<unsigned char>(bytes[2]) == 0xBF)
        utf8 = bytes.substr(3);
    else if (bytes.size() >= 2 && static_cast<unsigned char>(bytes[0]) == 0xFF && static_cast<unsigned char>(bytes[1]) == 0xFE)
        utf8 = utf16ToUtf8(bytes, false, 2);
    else if (bytes.size() >= 2 && static_cast<unsigned char>(bytes[0]) == 0xFE && static_cast<unsigned char>(bytes[1]) == 0xFF)
        utf8 = utf16ToUtf8(bytes, true, 2);
    else
        utf8 = bytes;
    // normalise line endings
    std::string norm;
    norm.reserve(utf8.size());
    for (size_t i = 0; i < utf8.size(); ++i) {
        if (utf8[i] == '\r') {
            norm.push_back('\n');
            if (i + 1 < utf8.size() && utf8[i + 1] == '\n')
                ++i;
        } else {
            norm.push_back(utf8[i]);
        }
    }
    utf8 = std::move(norm);
    return true;
}

bool writeTextFile(const std::string &path, const std::string &utf8)
{
    std::ofstream f(path, std::ios::binary);
    if (!f)
        return false;
    f.write(utf8.data(), static_cast<std::streamsize>(utf8.size()));
    return static_cast<bool>(f);
}

void printUsage()
{
    out("ppl " PPL_VERSION " — narzędzia dla języka HP PPL (HP Prime)\n\n"
        "Użycie:\n"
        "  ppl check [opcje] PLIK...    sprawdza programy; kod wyjścia 1, gdy są błędy\n"
        "      --json                   wynik w formacie JSON\n"
        "      --no-warnings            pokazuj tylko błędy\n"
        "      --werror                 traktuj ostrzeżenia jak błędy\n"
        "  ppl format [--write] [--indent N] [--ascii | --symbols] PLIK\n"
        "                               formatuje program (bez --write wypisuje wynik)\n"
        "  ppl help KOMENDA             opis komendy PPL\n"
        "  ppl search TEKST             wyszukiwanie komend\n"
        "  ppl run PLIK [WYWOŁANIE]     uruchamia program w symulatorze\n"
        "      --input a;b;c            odpowiedzi dla INPUT/CHOOSE    --keys 8,8,4   klawisze dla GETKEY\n"
        "      --answers ok,cancel      odpowiedzi MSGBOX z OK/Cancel  --screen p.png zrzut ekranu\n"
        "      --max-steps N  --seed N  --json\n"
        "  ppl guide                    przewodnik po PPL (Markdown) dla asystentów AI\n"
        "  ppl lsp                      serwer Language Server Protocol (stdio)\n"
        "  ppl mcp                      serwer Model Context Protocol dla agentów AI (stdio)\n"
        "  ppl gen tmgrammar PLIK.json  gramatyka TextMate dla VS Code\n"
        "  ppl gen notepadpp KATALOG    język UDL i autouzupełnianie dla Notepad++\n"
        "  ppl version                  wersja programu\n");
}

int runCheck(const std::vector<std::string> &args)
{
    bool json = false, noWarnings = false, werror = false;
    std::vector<std::string> files;
    for (const auto &a : args) {
        if (a == "--json") json = true;
        else if (a == "--no-warnings") noWarnings = true;
        else if (a == "--werror") werror = true;
        else files.push_back(a);
    }
    if (files.empty()) {
        err("ppl check: podaj co najmniej jeden plik\n");
        return 2;
    }
    Analyzer analyzer;
    int totalErrors = 0, totalWarnings = 0;
    Json report = Json::array();
    for (const auto &path : files) {
        std::string src, e;
        if (!readSourceFile(path, src, &e)) {
            err(path + ": " + e + "\n");
            ++totalErrors;
            continue;
        }
        AnalysisResult r = analyzer.analyze(src);
        Json fileJson = Json::object({{"file", path}});
        Json diags = Json::array();
        for (const auto &d : r.diagnostics) {
            if (noWarnings && d.severity != Severity::Error)
                continue;
            if (d.severity == Severity::Error) ++totalErrors;
            if (d.severity == Severity::Warning) ++totalWarnings;
            if (json) {
                diags.push(Json::object({{"line", d.line}, {"column", d.column}, {"severity", severityName(d.severity)},
                                         {"code", d.code}, {"message", d.message}}));
            } else {
                out(path + ":" + std::to_string(d.line) + ":" + std::to_string(d.column) + ": " + severityName(d.severity) + ": "
                    + d.message + " [" + d.code + "]\n");
            }
        }
        fileJson.set("diagnostics", diags);
        report.push(fileJson);
    }
    if (json) {
        out(report.dump() + "\n");
    } else {
        out(std::to_string(files.size()) + " plik(ów): " + std::to_string(totalErrors) + " błąd(ów), "
            + std::to_string(totalWarnings) + " ostrzeżeń\n");
    }
    return (totalErrors > 0 || (werror && totalWarnings > 0)) ? 1 : 0;
}

int runFormat(const std::vector<std::string> &args)
{
    bool write = false, ascii = false, symbols = false;
    FormatOptions opt;
    std::string file;
    for (size_t i = 0; i < args.size(); ++i) {
        if (args[i] == "--write") write = true;
        else if (args[i] == "--ascii") ascii = true;
        else if (args[i] == "--symbols") symbols = true;
        else if (args[i] == "--indent" && i + 1 < args.size()) opt.indentSize = std::max(1, std::atoi(args[++i].c_str()));
        else file = args[i];
    }
    if (file.empty()) {
        err("ppl format: podaj plik\n");
        return 2;
    }
    std::string src, e;
    if (!readSourceFile(file, src, &e)) {
        err(file + ": " + e + "\n");
        return 2;
    }
    std::u32string text = Formatter::format(toU32(src), opt);
    if (ascii)
        text = Formatter::toAscii(text);
    if (symbols)
        text = Formatter::toCalculatorSymbols(text);
    std::string result = toUtf8(text);
    if (write) {
        if (!writeTextFile(file, result)) {
            err(file + ": nie można zapisać\n");
            return 2;
        }
    } else {
        out(result);
        if (!result.empty() && result.back() != '\n')
            out("\n");
    }
    return 0;
}

int runHelp(const std::vector<std::string> &args)
{
    if (args.empty()) {
        printUsage();
        return 0;
    }
    const auto &db = CommandDatabase::instance();
    std::string name = args[0];
    if (name.rfind("CAS.", 0) == 0)
        name = name.substr(4);
    if (const CommandInfo *c = db.findCommand(name)) {
        out(c->name + "\n\n" + Services::commandMarkdown(*c) + "\n");
        return 0;
    }
    if (const VariableGroup *g = db.findVariable(toU32(name))) {
        out(Services::variableMarkdown(name, *g) + "\n");
        return 0;
    }
    err("Nie znaleziono '" + name + "'. Spróbuj: ppl search " + name + "\n");
    return 1;
}

int runSearch(const std::vector<std::string> &args)
{
    std::string q;
    for (const auto &a : args)
        q += (q.empty() ? "" : " ") + a;
    auto res = Services::searchCommands(q);
    for (const auto *c : res)
        out(c->name + "  —  " + c->syntax + "\n    " + c->description + "\n");
    if (res.empty()) {
        out("Brak wyników.\n");
        return 1;
    }
    return 0;
}

int runGuide(const std::vector<std::string> &)
{
    out(Services::languageGuide());
    return 0;
}

} // namespace ppl::cli

// ---------------------------------------------------------------- ppl run
#include "runner.h"

namespace ppl::cli {

namespace {
std::vector<std::string> splitList(const std::string &s, char sep)
{
    std::vector<std::string> out;
    std::string cur;
    for (char c : s) {
        if (c == sep) {
            out.push_back(cur);
            cur.clear();
        } else {
            cur.push_back(c);
        }
    }
    out.push_back(cur);
    return out;
}
} // namespace

int runRun(const std::vector<std::string> &args)
{
    runner::Options opt;
    bool json = false;
    std::string screen;
    for (size_t i = 0; i < args.size(); ++i) {
        const std::string &a = args[i];
        auto val = [&]() -> std::string {
            if (i + 1 >= args.size()) {
                err("ppl run: brak wartości dla " + a + "\n");
                std::exit(2);
            }
            return args[++i];
        };
        if (a == "--input") { for (auto &v : splitList(val(), ';')) opt.inputs.push_back(v); }
        else if (a == "--answers") { for (auto &v : splitList(val(), ',')) opt.answers.push_back(v); }
        else if (a == "--keys") { for (auto &v : splitList(val(), ',')) if (!v.empty()) opt.keys.push_back(std::atoi(v.c_str())); }
        else if (a == "--screen") screen = val();
        else if (a == "--scale") opt.screenshotScale = std::atoi(val().c_str());
        else if (a == "--max-steps") opt.maxSteps = std::strtoull(val().c_str(), nullptr, 10);
        else if (a == "--seed") opt.seed = std::strtoull(val().c_str(), nullptr, 10);
        else if (a == "--lib") opt.libraries.push_back(val());
        else if (a == "--no-libs") opt.siblingLibraries = false;
        else if (a == "--json") json = true;
        else if (opt.file.empty()) opt.file = a;
        else if (opt.call.empty()) opt.call = a;
        else { err("ppl run: nieznany argument " + a + "\n"); return 2; }
    }
    if (opt.file.empty()) {
        err("Użycie: ppl run PLIK [\"WYWOŁANIE(argumenty)\"] [--input a;b] [--keys 30,4] [--answers ok,cancel]\n"
            "                    [--screen ekran.png] [--scale 2] [--max-steps N] [--seed N] [--json]\n");
        return 2;
    }
    opt.screenshot = !screen.empty() || json;
    runner::Result r = runner::run(opt);
    if (!screen.empty() && !r.png.empty())
        writeTextFile(screen, r.png);
    if (json) {
        out(runner::toJson(r, false).dump() + "\n");
    } else {
        for (const auto &l : r.output)
            out(l + "\n");
        if (r.ok && !r.killed)
            out("=> " + r.result + "\n");
        if (!r.error.empty())
            err("Błąd wykonania" + (r.errorLine ? " (" + r.errorProgram + ", linia " + std::to_string(r.errorLine) + ")" : std::string())
                + ": " + r.error + "\n");
        if (!screen.empty())
            out(r.png.empty() ? "(ekran nie był używany — nie zapisano " + screen + ")\n" : "Zapisano ekran: " + screen + "\n");
    }
    return r.ok ? 0 : 1;
}

} // namespace ppl::cli
