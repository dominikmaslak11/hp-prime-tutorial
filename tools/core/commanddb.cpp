#include "commanddb.h"

#include "json.h"
#include "utf8.h"

#include <algorithm>
#include <cstdio>

namespace ppl {

extern const char kCommandsJson[];

const CommandDatabase &CommandDatabase::instance()
{
    static const CommandDatabase db = [] {
        CommandDatabase d;
        std::string err;
        if (!d.load(kCommandsJson, &err))
            std::fprintf(stderr, "ppl: embedded commands.json is invalid: %s\n", err.c_str());
        return d;
    }();
    return db;
}

bool CommandDatabase::load(std::string_view json, std::string *error)
{
    Json root = Json::parse(json, error);
    if (!root.isObject())
        return false;

    m_commands.clear();
    m_groups.clear();
    m_exact.clear();
    m_upper.clear();
    m_variables.clear();

    for (const Json &c : root["commands"].elements()) {
        CommandInfo info;
        info.name = c["name"].asString();
        info.name32 = toU32(info.name);
        info.kind = c["kind"].asString();
        info.category = c["cat"].asString();
        info.syntax = c["syntax"].asString();
        info.description = c["desc"].asString();
        info.example = c["example"].asString();
        info.chapter = c["ch"].asString();
        if (c.contains("min"))
            info.minArgs = c["min"].asInt(-1);
        if (c.contains("max"))
            info.maxArgs = c["max"].asInt(-1);
        m_commands.push_back(std::move(info));
    }
    for (size_t k = 0; k < m_commands.size(); ++k) {
        m_exact.emplace(m_commands[k].name32, k);
        m_upper.emplace(asciiUpper(m_commands[k].name32), k);
    }

    for (const Json &g : root["variables"].elements()) {
        VariableGroup group;
        group.group = g["group"].asString();
        group.description = g["desc"].asString();
        group.chapter = g["ch"].asString();
        for (const Json &n : g["names"].elements())
            group.names.push_back(n.asString());
        m_groups.push_back(std::move(group));
    }
    for (size_t k = 0; k < m_groups.size(); ++k)
        for (const auto &n : m_groups[k].names)
            m_variables.emplace(toU32(n), k);

    m_apps.clear();
    for (const Json &a : root["apps"].elements())
        m_apps.push_back(a.asString());
    m_keys.clear();
    for (const Json &k : root["keyNames"].elements())
        m_keys.push_back(k.asString());
    return true;
}

const CommandInfo *CommandDatabase::findCommand(std::u32string_view name) const
{
    std::u32string key(name);
    auto it = m_exact.find(key);
    if (it != m_exact.end())
        return &m_commands[it->second];
    auto up = m_upper.find(asciiUpper(key));
    if (up != m_upper.end())
        return &m_commands[up->second];
    return nullptr;
}

const CommandInfo *CommandDatabase::findCommand(std::string_view utf8Name) const
{
    return findCommand(toU32(utf8Name));
}

const VariableGroup *CommandDatabase::findVariable(std::u32string_view name) const
{
    auto it = m_variables.find(std::u32string(name));
    if (it != m_variables.end())
        return &m_groups[it->second];
    return nullptr;
}

bool CommandDatabase::isKeyName(std::u32string_view name) const
{
    std::string n = toUtf8(name);
    for (const char *prefix : {"KSA_", "KS_", "KA_", "K_"}) {
        std::string p(prefix);
        if (n.rfind(p, 0) == 0) {
            std::string base = n.substr(p.size());
            if (std::find(m_keys.begin(), m_keys.end(), base) == m_keys.end())
                return false;
            if ((p == "KS_" && (base == "Help" || base == "On")))
                return false;
            return true;
        }
    }
    return false;
}

bool CommandDatabase::isAppName(std::string_view name) const
{
    return std::find(m_apps.begin(), m_apps.end(), name) != m_apps.end();
}

std::vector<std::string> CommandDatabase::allKeyNames() const
{
    std::vector<std::string> out;
    for (const char *prefix : {"K_", "KS_", "KA_", "KSA_"})
        for (const auto &k : m_keys) {
            std::string p(prefix);
            if (p == "KS_" && (k == "Help" || k == "On"))
                continue;
            out.push_back(p + k);
        }
    return out;
}

std::string CommandDatabase::chapterUrl(const std::string &chapter)
{
    if (chapter.empty())
        return {};
    return "https://github.com/dominikmaslak11/hp-prime-tutorial/blob/main/rozdzialy/" + chapter;
}

} // namespace ppl
