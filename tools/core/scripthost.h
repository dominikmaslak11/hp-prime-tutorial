#pragma once

// Non-interactive host: scripted answers, virtual clock, output collected in memory.
// Used by `ppl run`, the MCP tool ppl_run and the tests.

#include "host.h"
#include "interpreter.h"
#include "utf8.h"

#include <cstdlib>
#include <deque>
#include <string>
#include <vector>

namespace ppl {

class ScriptedHost : public Host {
public:
    std::vector<std::string> output;
    Interpreter *interpreter = nullptr;

    void addInput(const std::string &s) { m_inputs.push_back(toU32(s)); }
    void addAnswer(const std::string &s) { m_answers.push_back(s); }
    void addKey(int k) { m_keys.push_back(k); }

    void print(const std::u32string &line) override { output.push_back(toUtf8(line)); }
    void notice(const std::u32string &text) override { output.push_back(toUtf8(text)); }
    bool messageBox(const std::u32string &text, bool okCancel) override
    {
        bool ok = true;
        if (okCancel && !m_answers.empty()) {
            ok = m_answers.front() != "cancel" && m_answers.front() != "0";
            m_answers.pop_front();
        }
        output.push_back("[MSGBOX] " + toUtf8(text) + (okCancel ? (ok ? "  → OK" : "  → Cancel") : ""));
        return ok;
    }
    bool input(InputRequest &req) override
    {
        for (auto &f : req.fields) {
            if (m_inputs.empty())
                throw RuntimeError("INPUT \"" + toUtf8(req.title) + "\": brak danych dla pola \"" + toUtf8(f.label)
                                   + "\". Podaj je opcją --input (kolejne wartości oddzielone ';').");
            std::u32string v = m_inputs.front();
            m_inputs.pop_front();
            if (v == U"cancel") {
                output.push_back("[INPUT] " + toUtf8(req.title) + " → Cancel");
                return false;
            }
            if (f.kind == InputField::Kind::Edit)
                f.text = v;
            else
                f.number = std::atoi(toUtf8(v).c_str());
            output.push_back("[INPUT] " + toUtf8(req.title) + ": " + toUtf8(f.label) + " " + toUtf8(v));
        }
        return true;
    }
    int choose(const std::u32string &title, const std::vector<std::u32string> &items) override
    {
        if (m_inputs.empty())
            throw RuntimeError("CHOOSE \"" + toUtf8(title) + "\": brak wyboru. Podaj numer opcji opcją --input.");
        std::u32string v = m_inputs.front();
        m_inputs.pop_front();
        int k = v == U"cancel" ? 0 : std::atoi(toUtf8(v).c_str());
        std::string chosen = k >= 1 && k <= static_cast<int>(items.size()) ? toUtf8(items[k - 1]) : "Cancel";
        output.push_back("[CHOOSE] " + toUtf8(title) + " → " + std::to_string(k) + " (" + chosen + ")");
        return k;
    }
    int getKey() override
    {
        if (m_keys.empty())
            return -1;
        int k = m_keys.front();
        m_keys.pop_front();
        return k;
    }
    bool isKeyDown(int key) override
    {
        if (!m_keys.empty() && m_keys.front() == key) {
            m_keys.pop_front();
            return true;
        }
        return false;
    }
    int waitForEvent(double) override { return getKey(); }
    void sleep(double seconds) override { m_clock += seconds * 1000.0; }
    double ticks() override
    {
        return m_clock + (interpreter ? static_cast<double>(interpreter->steps()) * 0.02 : 0);
    }

private:
    std::deque<std::u32string> m_inputs;
    std::deque<std::string> m_answers;
    std::deque<int> m_keys;
    double m_clock = 0;
};

} // namespace ppl
