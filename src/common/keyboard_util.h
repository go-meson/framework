//-*-c++-*-
#pragma once
#include <string>

#include "ui/events/keycodes/keyboard_codes.h"

namespace meson {

// Return key code of the |str|, and also determine whether the SHIFT key is
// pressed.
ui::KeyboardCode KeyboardCodeFromStr(const std::string& str, bool* shifted);

// Ported from ui/events/blink/blink_event_util.h
int WebEventModifiersToEventFlags(int modifiers);
}
