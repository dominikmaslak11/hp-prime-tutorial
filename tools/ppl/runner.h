#pragma once

#include "json.h"

#include <cstdint>
#include <string>
#include <vector>

namespace ppl::runner {

struct Options {
    std::string file;              // program file (or empty when code is given)
    std::string code;              // program source (UTF-8) when not reading a file
    std::string call;              // e.g. "SQIN(5)"; empty = the only EXPORT function without parameters
    std::vector<std::string> inputs;   // answers for INPUT fields / CHOOSE (in order)
    std::vector<std::string> answers;  // MSGBOX with OK/Cancel: "ok" / "cancel"
    std::vector<int> keys;             // key codes returned by GETKEY / WAIT(0) / FREEZE
    std::vector<std::string> libraries; // extra program files
    bool siblingLibraries = true;       // load other programs from the same folder
    uint64_t maxSteps = 5000000;
    uint64_t seed = 12345;
    bool screenshot = false;
    int screenshotScale = 1;
};

struct Result {
    bool ok = false;
    bool killed = false;
    std::string result;                 // formatted return value
    std::vector<std::string> output;    // PRINT / MSGBOX / notices, in order
    std::string error;
    int errorLine = 0;
    std::string errorProgram;
    uint64_t steps = 0;
    bool screenUsed = false;
    std::string png;                    // screenshot (binary PNG) when requested and the screen was used
};

Result run(const Options &opt);
Json toJson(const Result &r, bool includeImage);

} // namespace ppl::runner
