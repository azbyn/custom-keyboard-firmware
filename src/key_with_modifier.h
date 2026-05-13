#pragma once
#include <stdint.h>
#include <tusb.h>


#define HID_KEY_WILDCARD HID_KEY_F13


struct KeyWithModifier {
    uint8_t modifiers;
    uint8_t key;
};

static constexpr const char* maybeKeyToStr(uint8_t key) {
    switch (key) {
    case HID_KEY_WILDCARD: return "Wild";
    case HID_KEY_A: return "A";
    case HID_KEY_B: return "B";
    case HID_KEY_C: return "C";
    case HID_KEY_D: return "D";
    case HID_KEY_E: return "E";
    case HID_KEY_F: return "F";
    case HID_KEY_G: return "G";
    case HID_KEY_H: return "H";
    case HID_KEY_I: return "I";
    case HID_KEY_J: return "J";
    case HID_KEY_K: return "K";
    case HID_KEY_L: return "L";
    case HID_KEY_M: return "M";
    case HID_KEY_N: return "N";
    case HID_KEY_O: return "O";
    case HID_KEY_P: return "P";
    case HID_KEY_Q: return "Q";
    case HID_KEY_R: return "R";
    case HID_KEY_S: return "S";
    case HID_KEY_T: return "T";
    case HID_KEY_U: return "U";
    case HID_KEY_V: return "V";
    case HID_KEY_W: return "W";
    case HID_KEY_X: return "X";
    case HID_KEY_Y: return "Y";
    case HID_KEY_Z: return "Z";
    case HID_KEY_1: return "1";
    case HID_KEY_2: return "2";
    case HID_KEY_3: return "3";
    case HID_KEY_4: return "4";
    case HID_KEY_5: return "5";
    case HID_KEY_6: return "6";
    case HID_KEY_7: return "7";
    case HID_KEY_8: return "8";
    case HID_KEY_9: return "9";
    case HID_KEY_0: return "0";
    case HID_KEY_ENTER:  return "Ent";
    case HID_KEY_ESCAPE: return "Esc";
    case HID_KEY_BACKSPACE: return "BS";
    case HID_KEY_TAB: return "Tab";
    case HID_KEY_SPACE: return "Spc";
    case HID_KEY_MINUS: return "-";
    case HID_KEY_EQUAL: return "=";
    case HID_KEY_BRACKET_LEFT: return "[";
    case HID_KEY_BRACKET_RIGHT: return "]";
    case HID_KEY_BACKSLASH: return "\\";
    case HID_KEY_EUROPE_1: return "Eur";
    case HID_KEY_SEMICOLON: return ";";
    case HID_KEY_APOSTROPHE: return "'";
    case HID_KEY_GRAVE: return "`";
    case HID_KEY_COMMA: return ",";
    case HID_KEY_PERIOD: return ".";
    case HID_KEY_SLASH: return "/";
    case HID_KEY_CAPS_LOCK: return "Caps";

    case HID_KEY_F1: return "F1";
    case HID_KEY_F2: return "F2";
    case HID_KEY_F3: return "F3";
    case HID_KEY_F4: return "F4";
    case HID_KEY_F5: return "F5";
    case HID_KEY_F6: return "F6";
    case HID_KEY_F7: return "F7";
    case HID_KEY_F8: return "F8";
    case HID_KEY_F9: return "F9";
    case HID_KEY_F10: return "F10";
    case HID_KEY_F11: return "F11";
    case HID_KEY_F12: return "F12";
    case HID_KEY_PRINT_SCREEN: return "PrtScr";
    case HID_KEY_SCROLL_LOCK: return "ScrlLk";
    case HID_KEY_PAUSE: return "Pause";
    case HID_KEY_INSERT: return "Ins";
    case HID_KEY_HOME: return "Home";
    case HID_KEY_PAGE_UP: return "PgUp";
    case HID_KEY_DELETE: return "Del";
    case HID_KEY_END: return "End";
    case HID_KEY_PAGE_DOWN: return "PgDown";

    case HID_KEY_ARROW_RIGHT: return "ArrowR";
    case HID_KEY_ARROW_LEFT: return "ArrowL";
    case HID_KEY_ARROW_DOWN: return "ArrowD";
    case HID_KEY_ARROW_UP: return "ArrowU";
    case HID_KEY_NUM_LOCK: return "NumLk";
    case HID_KEY_KEYPAD_DIVIDE: return "KP/";
    case HID_KEY_KEYPAD_MULTIPLY: return "KP*";
    case HID_KEY_KEYPAD_SUBTRACT: return "KP-";
    case HID_KEY_KEYPAD_ADD: return "KP+";
    case HID_KEY_KEYPAD_ENTER: return "KP_Enter";
    case HID_KEY_KEYPAD_1: return "KP1";
    case HID_KEY_KEYPAD_2: return "KP2";
    case HID_KEY_KEYPAD_3: return "KP3";
    case HID_KEY_KEYPAD_4: return "KP4";
    case HID_KEY_KEYPAD_5: return "KP5";
    case HID_KEY_KEYPAD_6: return "KP6";
    case HID_KEY_KEYPAD_7: return "KP7";
    case HID_KEY_KEYPAD_8: return "KP8";
    case HID_KEY_KEYPAD_9: return "KP9";
    case HID_KEY_KEYPAD_0: return "KP0";
    case HID_KEY_KEYPAD_DECIMAL: return "KP.";
    default: return nullptr;
    }
}

// static void keyToStr(char* dst, size_t sz, uint8_t key) {
static constexpr const char* maybeMediaKeyToStr(uint16_t key) {
    switch (key) {
    case HID_USAGE_CONSUMER_BRIGHTNESS_INCREMENT: return "Brightness+";
    case HID_USAGE_CONSUMER_BRIGHTNESS_DECREMENT: return "Brightness-";
  // Power Control
    case HID_USAGE_CONSUMER_POWER: return "Power";
    case HID_USAGE_CONSUMER_RESET: return "Reset";
    case HID_USAGE_CONSUMER_SLEEP: return "Sleep";
        // Media Control
    case HID_USAGE_CONSUMER_PLAY_PAUSE: return "Play";
    case HID_USAGE_CONSUMER_SCAN_NEXT: return "Next";
    case HID_USAGE_CONSUMER_SCAN_PREVIOUS: return "Prev";
    case HID_USAGE_CONSUMER_STOP: return "Stop";
    case HID_USAGE_CONSUMER_MUTE: return "Mute";
    case HID_USAGE_CONSUMER_VOLUME_INCREMENT: return "Vol+";
    case HID_USAGE_CONSUMER_VOLUME_DECREMENT: return "Vol-";

    default: return nullptr;
    }
}

template <>
struct std::formatter<KeyWithModifier>
    // : std::formatter<std::string_view>
{
    constexpr auto parse(std::format_parse_context& ctx) {
        return ctx.begin(); // no custom format
    }
    
    template <typename FormatContext>
    auto format(const KeyWithModifier& k, FormatContext& ctx) const {
        auto out = ctx.out();
        if (k.modifiers & KEYBOARD_MODIFIER_LEFTGUI)
            out = std::format_to(out, "W");
        if (k.modifiers & KEYBOARD_MODIFIER_LEFTALT)
            out = std::format_to(out, "A");
        if (k.modifiers & KEYBOARD_MODIFIER_LEFTCTRL)
            out = std::format_to(out, "C");
        if (k.modifiers & KEYBOARD_MODIFIER_LEFTSHIFT)
            out = std::format_to(out, "S");
        if (k.modifiers & KEYBOARD_MODIFIER_RIGHTSHIFT)
            out = std::format_to(out, "rS");
        if (k.modifiers & KEYBOARD_MODIFIER_RIGHTCTRL)
            out = std::format_to(out, "rC");
        if (k.modifiers & KEYBOARD_MODIFIER_RIGHTALT)
            out = std::format_to(out, "rA");
        if (k.modifiers & KEYBOARD_MODIFIER_RIGHTGUI)
            out = std::format_to(out, "rW");
        if (k.modifiers)
            out = std::format_to(out, "-");
        
        auto maybeKeyStr = maybeKeyToStr(k.key);
        if (maybeKeyStr)
            out = std::format_to(out, "{}", maybeKeyStr);
        else
            out = std::format_to(out, "{:02x}", k.key);
        return out;
    }

    // static void keyToStr(char* dst, size_t sz, uint8_t key) {
};