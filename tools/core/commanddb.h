#pragma once

#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace ppl {

struct CommandInfo {
    std::string name;        // UTF-8, as written on the calculator
    std::u32string name32;
    std::string kind;        // keyword | command | appfunc | cas
    std::string category;
    std::string syntax;
    std::string description;
    std::string example;
    std::string chapter;     // tutorial chapter file, e.g. "07-wejscie.md"
    int minArgs = -1;        // -1 = unknown
    int maxArgs = -1;        // -1 = unlimited / unknown
    bool fromHpList = false; // only in HP's name list (data/hp-names.tsv), not described in the tutorial
    bool hasArgInfo() const { return minArgs >= 0; }
};

struct VariableGroup {
    std::string group;
    std::string description;
    std::string chapter;
    std::vector<std::string> names;
    bool fromHpList = false; // app variables known only from HP's name list
};

class CommandDatabase {
public:
    // Database built from the JSON embedded in the executable.
    static const CommandDatabase &instance();

    bool load(std::string_view json, std::string *error = nullptr);
    // Adds the names from HP's list (name, kind, group, menu, syntax, source) missing from the JSON.
    void addHpNames(std::string_view tsv);

    // Exact match first, then ASCII case-insensitive match.
    const CommandInfo *findCommand(std::u32string_view name) const;
    const CommandInfo *findCommand(std::string_view utf8Name) const;
    // Variable group containing the name (exact, case-sensitive).
    const VariableGroup *findVariable(std::u32string_view name) const;
    bool isSystemVariable(std::u32string_view name) const { return findVariable(name) != nullptr; }

    bool isKeyName(std::u32string_view name) const;       // K_Sin, KS_Enter, …
    bool isAppName(std::string_view name) const;
    std::vector<std::string> allKeyNames() const;

    const std::vector<CommandInfo> &commands() const { return m_commands; }
    const std::vector<VariableGroup> &variableGroups() const { return m_groups; }
    const std::vector<std::string> &appNames() const { return m_apps; }
    const std::vector<std::string> &keyBaseNames() const { return m_keys; }

    // Link to the tutorial chapter on GitHub (empty when unknown).
    static std::string chapterUrl(const std::string &chapter);

private:
    std::vector<CommandInfo> m_commands;
    std::vector<VariableGroup> m_groups;
    std::vector<std::string> m_apps;
    std::vector<std::string> m_keys;
    std::map<std::u32string, size_t> m_exact;
    std::map<std::u32string, size_t> m_upper;
    std::map<std::u32string, size_t> m_variables;
    void index();
};

} // namespace ppl
