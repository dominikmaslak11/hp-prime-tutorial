#include "lsp.h"

#include "analyzer.h"
#include "formatter.h"
#include "jsonrpc.h"
#include "services.h"
#include "utf8.h"

#include <map>

namespace ppl::lsp {

namespace {

using rpc::Framing;

struct Document {
    TextDocument text;
    AnalysisResult analysis;
    int version = 0;
};

class Server {
public:
    int run()
    {
        rpc::setupStdio();
        while (true) {
            bool parseError = false;
            auto msg = rpc::readMessage(Framing::ContentLength, &parseError);
            if (!msg)
                return shutdownRequested ? 0 : 1;
            if (parseError) {
                send(rpc::makeError(Json(), -32700, "Parse error"));
                continue;
            }
            handle(*msg);
            if (exitRequested)
                return shutdownRequested ? 0 : 1;
        }
    }

private:
    std::map<std::string, Document> docs;
    Analyzer analyzer;
    bool shutdownRequested = false;
    bool exitRequested = false;
    FormatOptions formatOptions;

    void send(const Json &j) { rpc::writeMessage(Framing::ContentLength, j); }

    void handle(const Json &msg)
    {
        const std::string &method = msg["method"].asString();
        const Json &id = msg["id"];
        const bool isRequest = msg.contains("id") && !method.empty();
        const Json &params = msg["params"];

        if (method.empty())
            return; // response to a server request — not used

        if (method == "initialize") {
            send(rpc::makeResponse(id, initializeResult()));
        } else if (method == "initialized" || method == "$/setTrace" || method == "$/cancelRequest"
                   || method == "workspace/didChangeConfiguration" || method == "textDocument/didSave") {
            // nothing to do
        } else if (method == "shutdown") {
            shutdownRequested = true;
            send(rpc::makeResponse(id, Json()));
        } else if (method == "exit") {
            exitRequested = true;
        } else if (method == "textDocument/didOpen") {
            const Json &td = params["textDocument"];
            update(td["uri"].asString(), td["text"].asString(), td["version"].asInt());
        } else if (method == "textDocument/didChange") {
            const Json &td = params["textDocument"];
            const Json &changes = params["contentChanges"];
            if (changes.size() > 0)
                update(td["uri"].asString(), changes[changes.size() - 1]["text"].asString(), td["version"].asInt());
        } else if (method == "textDocument/didClose") {
            std::string uri = params["textDocument"]["uri"].asString();
            docs.erase(uri);
            send(rpc::makeNotification("textDocument/publishDiagnostics",
                                       Json::object({{"uri", uri}, {"diagnostics", Json::array()}})));
        } else if (isRequest) {
            Json result;
            bool ok = true;
            if (method == "textDocument/completion") result = completion(params);
            else if (method == "textDocument/hover") result = hover(params);
            else if (method == "textDocument/signatureHelp") result = signatureHelp(params);
            else if (method == "textDocument/definition") result = definition(params);
            else if (method == "textDocument/documentSymbol") result = documentSymbols(params);
            else if (method == "textDocument/formatting") result = formatting(params);
            else if (method == "ppl/convert") result = convert(params);
            else ok = false;
            if (ok)
                send(rpc::makeResponse(id, std::move(result)));
            else
                send(rpc::makeError(id, -32601, "Method not found: " + method));
        }
    }

    Json initializeResult()
    {
        Json caps = Json::object();
        caps.set("textDocumentSync", Json::object({{"openClose", true}, {"change", 1}}));
        caps.set("completionProvider", Json::object({{"triggerCharacters", Json::array().push(".").push("_")},
                                                     {"resolveProvider", false}}));
        caps.set("hoverProvider", true);
        caps.set("signatureHelpProvider", Json::object({{"triggerCharacters", Json::array().push("(").push(",")},
                                                        {"retriggerCharacters", Json::array().push(",")}}));
        caps.set("definitionProvider", true);
        caps.set("documentSymbolProvider", true);
        caps.set("documentFormattingProvider", true);
        return Json::object({{"capabilities", caps},
                             {"serverInfo", Json::object({{"name", "ppl-lsp"}, {"version", PPL_VERSION}})}});
    }

    Json range(const TextDocument &doc, int offset, int length) const
    {
        auto [l1, c1] = doc.positionAt(offset);
        auto [l2, c2] = doc.positionAt(offset + length);
        return Json::object(
            {{"start", Json::object({{"line", l1}, {"character", doc.utf16Column(l1, c1)}})},
             {"end", Json::object({{"line", l2}, {"character", doc.utf16Column(l2, c2)}})}});
    }

    void update(const std::string &uri, const std::string &text, int version)
    {
        Document &d = docs[uri];
        d.text = TextDocument(toU32(text));
        d.analysis = analyzer.analyze(d.text.text());
        d.version = version;
        Json diags = Json::array();
        for (const auto &dg : d.analysis.diagnostics) {
            Json item = Json::object({{"range", range(d.text, dg.offset, dg.length)},
                                      {"severity", static_cast<int>(dg.severity)},
                                      {"code", dg.code},
                                      {"source", "ppl"},
                                      {"message", dg.message}});
            if (dg.code == "unused-local")
                item.set("tags", Json::array().push(1)); // Unnecessary
            diags.push(item);
        }
        send(rpc::makeNotification("textDocument/publishDiagnostics",
                                   Json::object({{"uri", uri}, {"version", version}, {"diagnostics", diags}})));
    }

    Document *docFor(const Json &params)
    {
        auto it = docs.find(params["textDocument"]["uri"].asString());
        return it == docs.end() ? nullptr : &it->second;
    }

    int offsetFor(const Document &d, const Json &params) const
    {
        const Json &pos = params["position"];
        int line = pos["line"].asInt();
        int col = d.text.columnFromUtf16(line, pos["character"].asInt());
        return d.text.offsetAt(line, col);
    }

    Json completion(const Json &params)
    {
        Document *d = docFor(params);
        if (!d)
            return Json::array();
        auto items = Services::completions(d->text, d->analysis, offsetFor(*d, params));
        Json list = Json::array();
        for (const auto &it : items) {
            Json j = Json::object({{"label", it.label}, {"kind", static_cast<int>(it.kind)}, {"insertText", it.insertText}});
            if (!it.detail.empty())
                j.set("detail", it.detail);
            if (!it.documentation.empty())
                j.set("documentation", Json::object({{"kind", "markdown"}, {"value", it.documentation}}));
            if (it.snippet)
                j.set("insertTextFormat", 2);
            if (!it.sortText.empty())
                j.set("sortText", it.sortText);
            if (!it.filterText.empty())
                j.set("filterText", it.filterText);
            if (it.triggerSignatureHelp)
                j.set("command", Json::object({{"title", "Parametry"}, {"command", "editor.action.triggerParameterHints"}}));
            list.push(j);
        }
        return Json::object({{"isIncomplete", false}, {"items", list}});
    }

    Json hover(const Json &params)
    {
        Document *d = docFor(params);
        if (!d)
            return Json();
        int start = 0, length = 0;
        auto md = Services::hover(d->text, d->analysis, offsetFor(*d, params), &start, &length);
        if (!md)
            return Json();
        return Json::object({{"contents", Json::object({{"kind", "markdown"}, {"value", *md}})},
                             {"range", range(d->text, start, length)}});
    }

    Json signatureHelp(const Json &params)
    {
        Document *d = docFor(params);
        if (!d)
            return Json();
        auto sig = Services::signatureHelp(d->text, d->analysis, offsetFor(*d, params));
        if (!sig)
            return Json();
        Json prms = Json::array();
        for (const auto &[s, e] : sig->parameters)
            prms.push(Json::object({{"label", Json::array().push(s).push(e)}}));
        Json info = Json::object({{"label", sig->label},
                                  {"documentation", Json::object({{"kind", "markdown"}, {"value", sig->documentation}})},
                                  {"parameters", prms}});
        return Json::object({{"signatures", Json::array().push(info)},
                             {"activeSignature", 0},
                             {"activeParameter", sig->activeParameter}});
    }

    Json definition(const Json &params)
    {
        Document *d = docFor(params);
        if (!d)
            return Json();
        auto off = Services::definition(d->text, d->analysis, offsetFor(*d, params));
        if (!off)
            return Json();
        return Json::object({{"uri", params["textDocument"]["uri"].asString()}, {"range", range(d->text, *off, 0)}});
    }

    Json documentSymbols(const Json &params)
    {
        Document *d = docFor(params);
        Json list = Json::array();
        if (!d)
            return list;
        const auto &a = d->analysis;
        for (size_t i = 0; i < a.functions.size(); ++i) {
            const auto &f = a.functions[i];
            if (!f.defined || f.offset < 0)
                continue;
            std::string params;
            for (size_t k = 0; k < f.params.size(); ++k)
                params += (k ? ", " : "") + f.params[k];
            std::string detail = f.isKey ? "KEY" : f.isView ? "VIEW \"" + f.viewTitle + "\"" : f.exported ? "EXPORT" : "";
            int endOff = f.endOffset >= 0 ? f.endOffset + 3 : f.offset + static_cast<int>(f.name32.size());
            Json children = Json::array();
            for (const auto &l : a.locals) {
                if (l.functionIndex != static_cast<int>(i))
                    continue;
                children.push(Json::object({{"name", l.name},
                                            {"detail", l.isParam ? "parametr" : "LOCAL"},
                                            {"kind", 13},
                                            {"range", range(d->text, l.offset, static_cast<int>(l.name32.size()))},
                                            {"selectionRange", range(d->text, l.offset, static_cast<int>(l.name32.size()))}}));
            }
            list.push(Json::object({{"name", f.name + "(" + params + ")"},
                                    {"detail", detail},
                                    {"kind", 12},
                                    {"range", range(d->text, f.offset, std::max(1, endOff - f.offset))},
                                    {"selectionRange", range(d->text, f.offset, static_cast<int>(f.name32.size()))},
                                    {"children", children}}));
        }
        for (const auto &v : a.fileVariables) {
            list.push(Json::object({{"name", v.name},
                                    {"detail", v.exported ? "EXPORT" : "zmienna pliku"},
                                    {"kind", 13},
                                    {"range", range(d->text, v.offset, static_cast<int>(v.name32.size()))},
                                    {"selectionRange", range(d->text, v.offset, static_cast<int>(v.name32.size()))}}));
        }
        return list;
    }

    Json fullEdit(const Document &d, const std::u32string &newText)
    {
        Json edits = Json::array();
        if (newText == d.text.text())
            return edits;
        edits.push(Json::object({{"range", range(d.text, 0, static_cast<int>(d.text.text().size()))},
                                 {"newText", toUtf8(newText)}}));
        return edits;
    }

    Json formatting(const Json &params)
    {
        Document *d = docFor(params);
        if (!d)
            return Json::array();
        FormatOptions opt = formatOptions;
        int tab = params["options"]["tabSize"].asInt(0);
        if (tab > 0)
            opt.indentSize = tab;
        return fullEdit(*d, Formatter::format(d->text.text(), opt));
    }

    // Custom request: {textDocument, mode: "ascii" | "symbols"} → TextEdit[]
    Json convert(const Json &params)
    {
        Document *d = docFor(params);
        if (!d)
            return Json::array();
        const std::string &mode = params["mode"].asString();
        std::u32string out = mode == "symbols" ? Formatter::toCalculatorSymbols(d->text.text())
                                               : Formatter::toAscii(d->text.text());
        return fullEdit(*d, out);
    }
};

} // namespace

int run()
{
    Server server;
    return server.run();
}

} // namespace ppl::lsp
