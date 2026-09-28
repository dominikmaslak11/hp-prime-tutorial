#include "runner.h"

#include "cli.h"
#include "graphics.h"
#include "interpreter.h"
#include "scripthost.h"
#include "utf8.h"

#include <deque>
#include <filesystem>

namespace ppl::runner {

namespace {

std::u32string stem(const std::string &path)
{
    namespace fs = std::filesystem;
    fs::path p = fs::path(reinterpret_cast<const char8_t *>(path.c_str()));
    std::u8string s = p.stem().u8string();
    std::string name(s.begin(), s.end());
    size_t dot = name.find('.');
    if (dot != std::string::npos)
        name = name.substr(0, dot);
    return toU32(name);
}

} // namespace

static void runImpl(const Options &opt, ScriptedHost &host, Result &res);

Result run(const Options &opt)
{
    Result res;
    ScriptedHost host;
    runImpl(opt, host, res);
    // host output first (PRINT, MSGBOX …), then messages added by the runner
    std::vector<std::string> all = host.output;
    all.insert(all.end(), res.output.begin(), res.output.end());
    res.output = std::move(all);
    return res;
}

static void runImpl(const Options &opt, ScriptedHost &host, Result &res)
{
    for (const auto &v : opt.inputs) host.addInput(v);
    for (const auto &v : opt.answers) host.addAnswer(v);
    for (int k : opt.keys) host.addKey(k);
    Interpreter in(host);
    host.interpreter = &in;
    in.setMaxSteps(opt.maxSteps);
    in.setSeed(opt.seed);

    std::string source = opt.code;
    std::u32string mainName = U"PROGRAM";
    if (!opt.file.empty()) {
        std::string err;
        if (!cli::readSourceFile(opt.file, source, &err)) {
            res.error = opt.file + ": " + err;
            return;
        }
        mainName = stem(opt.file);
    }
    try {
        in.loadProgram(toU32(source), mainName);
    } catch (const ParseError &e) {
        res.error = "Błąd składni: " + e.message;
        res.errorLine = e.line;
        res.errorProgram = toUtf8(mainName);
        return;
    }
    // other programs as libraries
    std::vector<std::string> libs = opt.libraries;
    if (opt.siblingLibraries && !opt.file.empty()) {
        namespace fs = std::filesystem;
        std::error_code ec;
        fs::path dir = fs::path(reinterpret_cast<const char8_t *>(opt.file.c_str())).parent_path();
        if (dir.empty())
            dir = ".";
        for (const auto &e : fs::directory_iterator(dir, ec)) {
            auto ext = e.path().extension().u8string();
            std::string x(ext.begin(), ext.end());
            if (x != ".hpppl" && x != ".ppl")
                continue;
            auto p8 = e.path().u8string();
            std::string p(p8.begin(), p8.end());
            if (fs::equivalent(e.path(), fs::path(reinterpret_cast<const char8_t *>(opt.file.c_str())), ec))
                continue;
            libs.push_back(p);
        }
    }
    for (const auto &lib : libs) {
        std::string src, err;
        if (!cli::readSourceFile(lib, src, &err))
            continue;
        try {
            in.loadProgram(toU32(src), stem(lib));
        } catch (const ParseError &) {
            res.output.push_back("[uwaga] pominięto program z błędami składni: " + lib);
        }
    }

    // default call: the only EXPORT function without parameters
    std::u32string call = toU32(opt.call);
    if (call.empty()) {
        std::vector<const FunctionDef *> candidates;
        for (const auto &f : in.units()[0].ast.functions)
            if (f->exported && !f->body.empty())
                candidates.push_back(f.get());
        if (candidates.size() == 1 && candidates[0]->params.empty()) {
            call = candidates[0]->name + U"()";
        } else {
            std::string list;
            for (const auto *f : candidates) {
                std::string ps;
                for (size_t k = 0; k < f->params.size(); ++k)
                    ps += (k ? "," : "") + toUtf8(f->params[k]);
                list += (list.empty() ? "" : ", ") + toUtf8(f->name) + "(" + ps + ")";
            }
            res.error = "Podaj wywołanie, np. ppl run PLIK \"NAZWA(argumenty)\". Funkcje z EXPORT: "
                        + (list.empty() ? std::string("brak") : list);
            return;
        }
    }

    uint64_t screenStart = in.graphics().version();
    try {
        Value v = in.run(call);
        res.ok = true;
        res.result = toUtf8(in.format(v));
    } catch (const KillSignal &) {
        res.ok = true;
        res.killed = true;
        res.output.push_back("[KILL] Program przerwany.");
    } catch (const RuntimeError &e) {
        res.error = e.what();
        res.errorLine = in.lastErrorLine();
        res.errorProgram = toUtf8(in.lastErrorProgram());
    } catch (const ReturnSignal &r) {
        res.ok = true;
        res.result = toUtf8(in.format(r.value));
    }
    res.steps = in.steps();
    res.screenUsed = in.graphics().version() != screenStart;
    if (opt.screenshot && res.screenUsed)
        res.png = in.graphics().png(0, opt.screenshotScale);
    return;
}

Json toJson(const Result &r, bool includeImage)
{
    Json out = Json::array();
    for (const auto &l : r.output)
        out.push(l);
    Json j = Json::object({{"ok", r.ok}, {"result", r.result}, {"output", out}, {"steps", static_cast<double>(r.steps)},
                           {"screenUsed", r.screenUsed}});
    if (r.killed)
        j.set("killed", true);
    if (!r.error.empty())
        j.set("error", Json::object({{"message", r.error}, {"line", r.errorLine}, {"program", r.errorProgram}}));
    if (includeImage && !r.png.empty())
        j.set("screenshotPng", base64(r.png));
    return j;
}

} // namespace ppl::runner
