#pragma once

#include "value.h"

#include <string>
#include <vector>

namespace ppl {

// One field of an INPUT dialog.
struct InputField {
    enum class Kind { Edit, Check, Choose };
    Kind kind = Kind::Edit;
    std::u32string label;
    std::u32string help;
    std::vector<std::u32string> choices;  // Choose
    int radioGroup = 0;                    // Check: >1 = radio group size (first of group)
    std::u32string text;                   // Edit: current value as text (in/out)
    int number = 0;                        // Check: 0/1, Choose: 1-based index (in/out)
};

struct InputRequest {
    std::u32string title;
    std::vector<InputField> fields;
};

struct TouchPoint {
    bool active = false;
    int x = 0, y = 0, x0 = 0, y0 = 0, type = 0;
};

// Everything the interpreter needs from the outside world.
class Host {
public:
    virtual ~Host() = default;
    virtual void print(const std::u32string &line) = 0;
    virtual void clearTerminal() {}
    virtual bool messageBox(const std::u32string &text, bool okCancel) = 0;
    virtual bool input(InputRequest &request) = 0;              // false = Cancel
    virtual int choose(const std::u32string &title, const std::vector<std::u32string> &items) = 0; // 0 = Cancel
    virtual int getKey() = 0;                                    // -1 = none
    virtual bool isKeyDown(int key) = 0;
    virtual TouchPoint touch() { return {}; }
    virtual int waitForEvent(double seconds) = 0;                // WAIT(0)/FREEZE: key code or -1
    virtual void sleep(double seconds) = 0;
    virtual double ticks() = 0;                                  // milliseconds
    virtual void screenChanged() {}
    virtual void notice(const std::u32string &text) { print(text); } // messages from the simulator itself
    virtual bool stopRequested() { return false; }
};

} // namespace ppl
