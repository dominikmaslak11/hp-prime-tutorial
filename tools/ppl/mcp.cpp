#include "mcp.h"

#include "analyzer.h"
#include "cli.h"
#include "formatter.h"
#include "graphics.h"
#include "jsonrpc.h"
#include "runner.h"
#include "services.h"
#include "utf8.h"

#include <algorithm>

namespace ppl::mcp {

namespace {

using rpc::Framing;

const char *kInstructions =
    "Narzędzia dla języka HP PPL (kalkulator HP Prime). Zanim pokażesz użytkownikowi program PPL, sprawdź go "
    "narzędziem ppl_validate i popraw wszystkie błędy. Nie wymyślaj komend: gdy nie masz pewności, użyj "
    "ppl_search_commands albo ppl_command_help. Narzędziem ppl_run uruchomisz program w symulatorze i sprawdzisz wynik. Zasady języka opisuje ppl_language_guide.";

Json schema(std::initializer_list<std::pair<std::string, Json>> props, std::initializer_list<const char *> required)
{
    Json p = Json::object();
    for (const auto &kv : props)
        p.set(kv.first, kv.second);
    Json req = Json::array();
    for (const char *r : required)
        req.push(r);
    return Json::object({{"type", "object"}, {"properties", p}, {"required", req}});
}

Json prop(const char *type, const char *description)
{
    return Json::object({{"type", type}, {"description", description}});
}

Json toolList()
{
    Json tools = Json::array();
    tools.push(Json::object(
        {{"name", "ppl_validate"},
         {"title", "Sprawdź program PPL"},
         {"description", "Sprawdza kod HP PPL (składnia, bloki BEGIN/END, średniki, liczba argumentów komend, "
                         "deklaracje funkcji, typy zmiennych systemowych). Zwraca listę błędów i ostrzeżeń z numerami linii."},
         {"inputSchema", schema({{"code", prop("string", "Pełny kod programu PPL")}}, {"code"})}}));
    tools.push(Json::object(
        {{"name", "ppl_check_file"},
         {"title", "Sprawdź plik PPL"},
         {"description", "Sprawdza plik z programem HP PPL zapisany na dysku (.hpppl, .ppl, .txt)."},
         {"inputSchema", schema({{"path", prop("string", "Ścieżka do pliku")}}, {"path"})}}));
    tools.push(Json::object(
        {{"name", "ppl_format"},
         {"title", "Formatuj program PPL"},
         {"description", "Poprawia wcięcia i wielkość liter słów kluczowych w kodzie HP PPL."},
         {"inputSchema", schema({{"code", prop("string", "Kod PPL")},
                                 {"indent", prop("integer", "Szerokość wcięcia (domyślnie 2)")},
                                 {"symbols", prop("string", "\"ascii\" zamienia ≠ ≤ ≥ na <> <= >=, \"calculator\" odwrotnie")}},
                                {"code"})}}));
    tools.push(Json::object(
        {{"name", "ppl_command_help"},
         {"title", "Opis komendy PPL"},
         {"description", "Składnia, opis i przykład komendy HP PPL albo zmiennej systemowej (po polsku)."},
         {"inputSchema", schema({{"name", prop("string", "Nazwa komendy, np. INPUT, TEXTOUT_P, CAS.idivis")}}, {"name"})}}));
    tools.push(Json::object(
        {{"name", "ppl_search_commands"},
         {"title", "Szukaj komend PPL"},
         {"description", "Wyszukuje komendy HP PPL po nazwie, kategorii lub opisie (np. 'lista', 'tekst', 'klawisz')."},
         {"inputSchema", schema({{"query", prop("string", "Szukany tekst")}}, {"query"})}}));
    {
        Json strArray = Json::object({{"type", "array"}, {"items", Json::object({{"type", "string"}})}});
        Json intArray = Json::object({{"type", "array"}, {"items", Json::object({{"type", "integer"}})}});
        Json props = Json::object();
        props.set("code", prop("string", "Kod programu PPL (albo podaj path)"));
        props.set("path", prop("string", "Ścieżka do pliku z programem (zamiast code)"));
        props.set("call", prop("string", "Wywołanie, np. SUMDIV(12). Puste = jedyna funkcja EXPORT bez parametrów."));
        Json in = strArray;
        in.set("description", "Kolejne odpowiedzi dla INPUT (każde pole osobno) i CHOOSE (numer opcji); 'cancel' = Anuluj");
        props.set("inputs", in);
        Json keys = intArray;
        keys.set("description", "Kody klawiszy zwracane kolejno przez GETKEY/WAIT(0)/FREEZE, np. 8 (▶), 30 (Enter), 4 (Esc)");
        props.set("keys", keys);
        Json ans = strArray;
        ans.set("description", "Odpowiedzi dla MSGBOX z OK/Cancel: 'ok' lub 'cancel'");
        props.set("answers", ans);
        props.set("screenshot", prop("boolean", "Dołącz zrzut ekranu 320×240, jeśli program rysował (domyślnie true)"));
        props.set("maxSteps", prop("integer", "Limit kroków (domyślnie 2 000 000) — chroni przed nieskończoną pętlą"));
        tools.push(Json::object(
            {{"name", "ppl_run"},
             {"title", "Uruchom program PPL (symulator)"},
             {"description", "Uruchamia program HP PPL w symulatorze i zwraca wynik, wyjście PRINT/MSGBOX, błędy wykonania "
                             "(z numerem linii) i zrzut ekranu. Dane dla INPUT/CHOOSE i klawisze podaj z góry. To symulator, "
                             "nie firmware: nie obsługuje symbolicznego CAS, jednostek, widoków aplikacji ani grafiki 3D — "
                             "wtedy zgłasza 'nieobsługiwane w symulatorze'. Liczy na double zaokrąglanym do 12 cyfr."},
             {"inputSchema", Json::object({{"type", "object"}, {"properties", props}, {"required", Json::array()}})}}));
    }
    tools.push(Json::object(
        {{"name", "ppl_language_guide"},
         {"title", "Przewodnik po PPL"},
         {"description", "Zwięzły przewodnik po języku HP PPL: struktura programu, składnia, typowe pułapki, lista komend."},
         {"inputSchema", schema({}, {})}}));
    return Json::object({{"tools", tools}});
}

std::string validationReport(const std::string &code)
{
    Analyzer analyzer;
    AnalysisResult r = analyzer.analyze(code);
    std::string out;
    int errors = r.errorCount(), warnings = r.warningCount();
    if (errors == 0 && warnings == 0)
        out = "OK: brak błędów i ostrzeżeń.\n";
    else
        out = "Błędy: " + std::to_string(errors) + ", ostrzeżenia: " + std::to_string(warnings) + "\n";
    for (const auto &d : r.diagnostics)
        out += "linia " + std::to_string(d.line) + ", kol. " + std::to_string(d.column) + " [" + severityName(d.severity)
               + "/" + d.code + "] " + d.message + "\n";
    if (!r.functions.empty()) {
        out += "\nFunkcje:";
        for (const auto &f : r.functions) {
            std::string params;
            for (size_t k = 0; k < f.params.size(); ++k)
                params += (k ? ", " : "") + f.params[k];
            out += "\n- " + std::string(f.exported ? "EXPORT " : "") + f.name + "(" + params + ")"
                   + (f.defined ? "" : " [tylko deklaracja]");
        }
        out += "\n";
    }
    return out;
}

Json textResult(const std::string &text, bool isError = false)
{
    Json content = Json::array().push(Json::object({{"type", "text"}, {"text", text}}));
    return Json::object({{"content", content}, {"isError", isError}});
}

Json callTool(const std::string &name, const Json &args)
{
    const auto &db = CommandDatabase::instance();
    if (name == "ppl_validate") {
        if (!args["code"].isString())
            return textResult("Brak argumentu 'code'.", true);
        return textResult(validationReport(args["code"].asString()));
    }
    if (name == "ppl_check_file") {
        std::string src, err;
        if (!cli::readSourceFile(args["path"].asString(), src, &err))
            return textResult("Nie można odczytać pliku: " + args["path"].asString() + " (" + err + ")", true);
        return textResult(validationReport(src));
    }
    if (name == "ppl_format") {
        FormatOptions opt;
        if (args["indent"].isNumber())
            opt.indentSize = std::clamp(args["indent"].asInt(), 1, 8);
        std::u32string text = Formatter::format(toU32(args["code"].asString()), opt);
        const std::string &sym = args["symbols"].asString();
        if (sym == "ascii")
            text = Formatter::toAscii(text);
        else if (sym == "calculator")
            text = Formatter::toCalculatorSymbols(text);
        return textResult(toUtf8(text));
    }
    if (name == "ppl_command_help") {
        std::string n = args["name"].asString();
        if (n.rfind("CAS.", 0) == 0)
            n = n.substr(4);
        if (const CommandInfo *c = db.findCommand(n))
            return textResult(c->name + "\n\n" + Services::commandMarkdown(*c));
        if (const VariableGroup *g = db.findVariable(toU32(n)))
            return textResult(Services::variableMarkdown(n, *g));
        std::string msg = "Nie ma komendy '" + n + "' w bazie HP PPL.";
        auto similar = Services::searchCommands(n);
        if (!similar.empty()) {
            msg += " Podobne: ";
            for (size_t i = 0; i < similar.size() && i < 8; ++i)
                msg += (i ? ", " : "") + similar[i]->name;
        }
        return textResult(msg);
    }
    if (name == "ppl_search_commands") {
        auto res = Services::searchCommands(args["query"].asString());
        if (res.empty())
            return textResult("Brak wyników.");
        std::string out;
        for (size_t i = 0; i < res.size() && i < 40; ++i)
            out += res[i]->name + " — " + res[i]->syntax + " — " + res[i]->description + "\n";
        if (res.size() > 40)
            out += "… oraz " + std::to_string(res.size() - 40) + " innych. Zawęź zapytanie.\n";
        return textResult(out);
    }
    if (name == "ppl_run") {
        runner::Options opt;
        opt.code = args["code"].asString();
        opt.file = args["path"].asString();
        if (opt.code.empty() && opt.file.empty())
            return textResult("Podaj 'code' albo 'path'.", true);
        opt.call = args["call"].asString();
        for (const auto &v : args["inputs"].elements())
            opt.inputs.push_back(v.isString() ? v.asString() : v.dump());
        for (const auto &v : args["answers"].elements())
            opt.answers.push_back(v.asString());
        for (const auto &v : args["keys"].elements())
            opt.keys.push_back(v.asInt());
        opt.maxSteps = args["maxSteps"].isNumber() ? static_cast<uint64_t>(args["maxSteps"].asNumber()) : 2000000;
        opt.screenshot = !args.contains("screenshot") || args["screenshot"].asBool(true);
        runner::Result r = runner::run(opt);
        std::string text;
        for (const auto &l : r.output)
            text += l + "\n";
        if (r.ok && !r.killed)
            text += "Wynik: " + r.result + "\n";
        if (!r.error.empty())
            text += "Błąd wykonania" + (r.errorLine ? " (" + r.errorProgram + ", linia " + std::to_string(r.errorLine) + ")" : std::string())
                    + ": " + r.error + "\n";
        text += "Kroków: " + std::to_string(r.steps) + (r.screenUsed ? ", program rysował na ekranie" : "") + "\n";
        Json content = Json::array().push(Json::object({{"type", "text"}, {"text", text}}));
        if (!r.png.empty())
            content.push(Json::object({{"type", "image"}, {"data", base64(r.png)}, {"mimeType", "image/png"}}));
        return Json::object({{"content", content}, {"isError", !r.ok}});
    }
    if (name == "ppl_language_guide")
        return textResult(Services::languageGuide());
    return Json();
}

} // namespace

int run()
{
    rpc::setupStdio();
    while (true) {
        bool parseError = false;
        auto msg = rpc::readMessage(Framing::NewlineDelimited, &parseError);
        if (!msg)
            return 0;
        if (parseError) {
            rpc::writeMessage(Framing::NewlineDelimited, rpc::makeError(Json(), -32700, "Parse error"));
            continue;
        }
        const std::string &method = (*msg)["method"].asString();
        const Json &id = (*msg)["id"];
        const Json &params = (*msg)["params"];
        bool isRequest = msg->contains("id");
        if (!isRequest || method.empty())
            continue; // notifications (initialized, cancelled …) and responses

        if (method == "initialize") {
            std::string version = params["protocolVersion"].asString();
            static const char *supported[] = {"2025-06-18", "2025-03-26", "2024-11-05"};
            bool ok = std::any_of(std::begin(supported), std::end(supported), [&](const char *v) { return version == v; });
            Json result = Json::object(
                {{"protocolVersion", ok ? version : std::string(supported[0])},
                 {"capabilities", Json::object({{"tools", Json::object({{"listChanged", false}})}})},
                 {"serverInfo", Json::object({{"name", "ppl-mcp"}, {"title", "HP Prime PPL"}, {"version", PPL_VERSION}})},
                 {"instructions", kInstructions}});
            rpc::writeMessage(Framing::NewlineDelimited, rpc::makeResponse(id, result));
        } else if (method == "ping") {
            rpc::writeMessage(Framing::NewlineDelimited, rpc::makeResponse(id, Json::object()));
        } else if (method == "tools/list") {
            rpc::writeMessage(Framing::NewlineDelimited, rpc::makeResponse(id, toolList()));
        } else if (method == "tools/call") {
            Json result = callTool(params["name"].asString(), params["arguments"]);
            if (result.isNull())
                rpc::writeMessage(Framing::NewlineDelimited,
                                  rpc::makeError(id, -32602, "Unknown tool: " + params["name"].asString()));
            else
                rpc::writeMessage(Framing::NewlineDelimited, rpc::makeResponse(id, result));
        } else {
            rpc::writeMessage(Framing::NewlineDelimited, rpc::makeError(id, -32601, "Method not found: " + method));
        }
    }
}

} // namespace ppl::mcp
