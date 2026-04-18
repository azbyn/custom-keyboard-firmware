#pragma once 
#include <stdint.h>
#include <array>
#include <pico/bootrom.h> //reset_usb_boot
#include <hardware/watchdog.h>

#include <tusb.h>

#include "pins.h"
#include "keyboard_state_machine.h"
#include "usb_hid.h"



namespace key_layout_definitions {
constexpr uint8_t ALT   = 0x1;
constexpr uint8_t CTRL  = 0x2;
constexpr uint8_t SHIFT = 0x4;
constexpr uint8_t LINUX = 0x8;

struct UnicodeKeyActions {
    char16_t arr[6] = {};
   
    constexpr char16_t get(bool shift, bool alt, bool russian) const {
        auto i = uint(shift) | uint(alt << 1) | uint(russian << 2);
        if (i < 2) return 0; // should never happen.
        return arr[i - 2];
    }
};


struct KeyAction {
    union {
        // uint8_t keyboardKey;
        KeyWithModifier keyboardKeyWithModifier;
        uint16_t mediaKey;
        void (*lambda)(KeyboardStateMachine&);
    };
    enum {
        Nothing = 0,
        KeyboardKeyWithModifier,
        //KeyWithModifier,
        MediaKey,
        Lambda
    } tag;

    const char* help = nullptr;

    constexpr KeyAction(): tag{Nothing}, lambda{nullptr} {}
    constexpr bool isNothing() const { return tag == Nothing; }
};
constexpr KeyAction KA_Nothing() {
    return KeyAction();
}
constexpr KeyAction KA_KeyWithModifier(KeyWithModifier val, const char* help = nullptr) {
    KeyAction x;
    x.keyboardKeyWithModifier = val; x.tag = KeyAction::KeyboardKeyWithModifier;
    x.help = help;
    return x;

    //return KeyAction { .keyboardKey = val, .tag = KeyAction::KeyboardKey };
}
constexpr KeyAction KA_MediaKey(uint16_t val, const char* help = nullptr) {
    KeyAction x;
    x.mediaKey = val; x.tag = KeyAction::MediaKey; x.help = help;
    return x;

//    return KeyAction { .mediaKey = val, .tag = KeyAction::MediaKey };
}
constexpr KeyAction KA_Lambda(void (*val)(KeyboardStateMachine&), const char* help = nullptr) {
    KeyAction x;
    x.lambda = val; x.tag = KeyAction::Lambda; x.help = help;
    return x;
    //return KeyAction { .lambda = val, .tag = KeyAction::Lambda };
}


#define HID_KEY__     HID_KEY_NONE
#define HID_KEY_FN    HID_KEY_GUI_RIGHT
#define HID_KEY_RCTRL HID_KEY_CONTROL_RIGHT
#define HID_KEY_LCTRL HID_KEY_CONTROL_LEFT
#define HID_KEY_LALT  HID_KEY_ALT_LEFT
#define HID_KEY_RALT  HID_KEY_ALT_RIGHT
#define HID_KEY_SPC   HID_KEY_SPACE
#define HID_KEY_WILDCARD  HID_KEY_F13

#define HID_USAGE_CONSUMER_FAST_FORWARD 0x00B3
#define HID_USAGE_CONSUMER_REWIND 0x00B4


// #define R(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18) \
//     {HID_KEY_##a0, HID_KEY_##a1,  HID_KEY_##a2,  HID_KEY_##a3,  HID_KEY_##a4, \
//     HID_KEY_##a5,  HID_KEY_##a6,  HID_KEY_##a7,  HID_KEY_##a8,  HID_KEY_##a9, \
//     HID_KEY_##a10, HID_KEY_##a11, HID_KEY_##a12, HID_KEY_##a13, HID_KEY_##a14,\
//     HID_KEY_##a15, HID_KEY_##a16, HID_KEY_##a17, HID_KEY_##a18}
#define R(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17) \
    {HID_KEY_##a0, HID_KEY_##a1,  HID_KEY_##a2,  HID_KEY_##a3,  HID_KEY_##a4, \
    HID_KEY_##a5,  HID_KEY_##a6,  HID_KEY_##a7,  HID_KEY_##a8,  HID_KEY_##a9, \
    HID_KEY_##a10, HID_KEY_##a11, HID_KEY_##a12, HID_KEY_##a13, HID_KEY_##a14,\
    HID_KEY_##a15, HID_KEY_##a16, HID_KEY_##a17}
// constexpr uint8_t keys[pins::rowLen][pins::colLen] = {
//     //0,          1,    2,     3,  4,  5,   6,  7,  8,     9,      10,           11,            12,            13,                    14,           15,       16,            17,              18
//     R(ESCAPE,     _,    F1,    F2, F3, F4,  F5, F6, F7,    F8,     F9,           F10,           F11,           F12,                   PRINT_SCREEN, _,        _,             _,               _),
//     R(GRAVE,      1,    2,     3,  4,  5,   6,  7,  8,     9,      0,            MINUS,         EQUAL,         BACKSPACE,             HOME,         END,      KEYPAD_DIVIDE, KEYPAD_MULTIPLY, KEYPAD_SUBTRACT),
//     R(TAB,        Q,    W,     E,  R,  T,   Y,  U,  I,     O,      P,            BRACKET_LEFT,  BRACKET_RIGHT, BACKSLASH,             DELETE,       KEYPAD_7, KEYPAD_8,      KEYPAD_9,        KEYPAD_ADD),
//     R(GUI_LEFT,   A,    S,     D,  F,  G,   H,  J,  K,     L,      SEMICOLON,    APOSTROPHE,    _,             ENTER,                 WILDCARD,     KEYPAD_4, KEYPAD_5,      KEYPAD_6,        _),
//     R(SHIFT_LEFT, Z,    X,     C,  V,  B,   N,  M,  COMMA, PERIOD, SLASH,        _,             SHIFT_RIGHT,   ARROW_UP,              _,            KEYPAD_1, KEYPAD_2,      KEYPAD_3,        KEYPAD_ENTER),
//     R(RCTRL,      LALT, LCTRL, _,  _,  SPC, _,  _,  _,     RALT,   FN,           RCTRL,         ARROW_LEFT,    ARROW_DOWN,            ARROW_RIGHT,  KEYPAD_0, _,             KEYPAD_DECIMAL,  _),
// };
constexpr uint8_t keys[pins::rowLen][18] = {
    //0,          1,    2,     3,  4,  5,   6,  7,  8,     9,      10,           11,            12,            13,                    14,           15,              16,            17
    R(ESCAPE,     _,    F1,    F2, F3, F4,  F5, F6, F7,    F8,     F9,           F10,           F11,           F12,                   PRINT_SCREEN, KEYPAD_SUBTRACT, KEYPAD_ADD,    KEYPAD_ENTER),
    R(GRAVE,      1,    2,     3,  4,  5,   6,  7,  8,     9,      0,            MINUS,         EQUAL,         BACKSPACE,             HOME,         END,             KEYPAD_DIVIDE, KEYPAD_MULTIPLY),
    R(TAB,        Q,    W,     E,  R,  T,   Y,  U,  I,     O,      P,            BRACKET_LEFT,  BRACKET_RIGHT, BACKSLASH,             DELETE,       KEYPAD_7,        KEYPAD_8,      KEYPAD_9),
    R(GUI_LEFT,   A,    S,     D,  F,  G,   H,  J,  K,     L,      SEMICOLON,    APOSTROPHE,    _,             ENTER,                 WILDCARD,     KEYPAD_4,        KEYPAD_5,      KEYPAD_6),
    R(SHIFT_LEFT, Z,    X,     C,  V,  B,   N,  M,  COMMA, PERIOD, SLASH,        _,             SHIFT_RIGHT,   ARROW_UP,              _,            KEYPAD_1,        KEYPAD_2,      KEYPAD_3),
    R(RCTRL,      LALT, LCTRL, _,  _,  SPC, _,  _,  _,     RALT,   FN,           RCTRL,         ARROW_LEFT,    ARROW_DOWN,            ARROW_RIGHT,  KEYPAD_0,        _,             KEYPAD_DECIMAL),
};
#undef R

constexpr int x = KEYBOARD_MODIFIER_LEFTGUI;



//constexpr char32_t mvs = 0x180E; //mongolian vowel separator
constexpr std::array<UnicodeKeyActions, 0x39> unicodeKeyActions = ([]() {
    std::array<UnicodeKeyActions, 0x39> x {};
    //x[0] = {},
    //x[1] = {},
    //x[2] = {},
    //x[3] = {},
    x[HID_KEY_Q] = {u'â', u'Â', u'я', u'Я', 0,    0};
    x[HID_KEY_W] = {u'ß', u'ß', u'ѣ', u'Ѣ', u'ѫ', u'Ѫ'}; // yus/yat is here because i had free space
    x[HID_KEY_E] = {u'ę', u'Ę', u'е', u'Е', u'э', u'Э'};
    x[HID_KEY_R] = {u'š', u'Š', u'р', u'Р', u'щ', u'Щ'};
    x[HID_KEY_T] = {u'ț', u'Ț', u'т', u'Т', u'ц', u'Ц'};
    x[HID_KEY_Y] = {u'ű', u'Ű', u'ы', u'Ы', u'і', u'І'}; //ukrainian i
    x[HID_KEY_U] = {u'ü', u'Ü', u'у', u'У', u'ю', u'Ю'};
    x[HID_KEY_I] = {u'î', u'Î', u'и', u'И', u'ы', u'Ы'};
    x[HID_KEY_O] = {u'ö', u'Ö', u'о', u'О', u'ё', u'Ё'};
    x[HID_KEY_P] = {u'ó', u'Ó', u'п', u'П', 0,    0};
    
    x[HID_KEY_A] = {u'ă', u'Ă', u'а', u'А', u'ъ', u'Ъ'};
    x[HID_KEY_S] = {u'ș', u'Ș', u'с', u'С', u'ш', u'Ш'};
    x[HID_KEY_D] = {u'ą', u'Ą', u'д', u'Д', u'џ', u'Џ'};
    x[HID_KEY_F] = {u'ä', u'Ä', u'ф', u'ф', 0,    0};
    x[HID_KEY_G] = {u'đ', u'Đ', u'г', u'Г', u'ґ', u'Ґ'};
    x[HID_KEY_H] = {u'ő', u'Ő', u'х', u'Х', 0,    0};
    x[HID_KEY_J] = {u'ú', u'Ú', u'й', u'й', u'ї', u'Ї'};
    x[HID_KEY_K] = {u'ś', u'Ś', u'к', u'К', 0,    0};
    x[HID_KEY_L] = {u'ł', u'Ł', u'л', u'Л', u'љ', u'Љ'};
    
    x[HID_KEY_Z] = {u'ż', u'Ż', u'з', u'З', u'ж', u'Ж'};
    x[HID_KEY_X] = {u'ź', u'Ź', u'щ', u'Щ', 0,    0};
    x[HID_KEY_C] = {u'ć', u'Ć', u'ц', u'Ц', u'ч', u'Ч'};
    x[HID_KEY_V] = {u'č', u'Č', u'в', u'В', 0,    0};
    x[HID_KEY_B] = {u'ž', u'Ž', u'б', u'Б', 0,    0};
    x[HID_KEY_N] = {u'ń', u'Ń', u'н', u'Н', u'ь', u'Ь'};
    x[HID_KEY_M] = {u'ñ', u'Ñ', u'м', u'М', 0,    0};

    x[HID_KEY_BRACKET_LEFT]  = {u'„', u'„', 0,    0,    u'„', u'„'};
    x[HID_KEY_BRACKET_RIGHT] = {u'”', u'”', 0,    0,    u'”', u'”'};
    x[HID_KEY_BACKSLASH]     = {0,    0,    0,    0,    0,    0};
    
    x[HID_KEY_SEMICOLON]     = {u'“', u'”', 0,    0,    u'“', u'”'};
    x[HID_KEY_APOSTROPHE]    = {u'”', u'”', 0,    0,    u'”', u'”'};
    
    x[HID_KEY_COMMA]         = {u'«', u'«', 0,    0,    u'«', u'«'};
    x[HID_KEY_PERIOD]        = {u'»', u'»', 0,    0,    u'»', u'»'};
    x[HID_KEY_SLASH]         = {0,    0,    0,    0,    0,    0};

    // [HID_KEY_GRAVE]     = {0,    0,    0,    0,    0,    0};
    // [HID_KEY_1]         = {0,    0,    0,    0,    0,    0};
    // [HID_KEY_2]         = {0,    0,    0,    0,    0,    0};
    // [HID_KEY_3]         = {0,    0,    0,    0,    0,    0};
    // [HID_KEY_4]         = {0,    0,    0,    0,    0,    0};
    // [HID_KEY_5]         = {0,    0,    0,    0,    0,    0};
    // [HID_KEY_6]         = {0,    0,    0,    0,    0,    0};
    // [HID_KEY_7]         = {0,    0,    0,    0,    0,    0};
    // [HID_KEY_8]         = {0,    0,    0,    0,    0,    0};
    // [HID_KEY_9]         = {0,    0,    0,    0,    0,    0};
    // [HID_KEY_0]         = {0,    0,    0,    0,    0,    0};
    // [HID_KEY_MINUS]     = {0,    0,    0,    0,    0,    0};
    // [HID_KEY_EQUAL]     = {0,    0,    0,    0,    0,    0};
    // [HID_KEY_BACKSPACE] = {0,    0,    0,    0,    0,    0};

    // [HID_KEY_ENTER]     = {0,    0,    0,    0,    0,    0};
    // [HID_KEY_ESCAPE]    = {0,    0,    0,    0,    0,    0};
    // [HID_KEY_TAB]       = {0,    0,    0,    0,    0,    0};
    // [HID_KEY_SPACE]     = {0,    0,    0,    0,    0,    0};//mvs?
    // [HID_KEY_EUROPE_1]  = {0,    0,    0,    0,    0,    0};

    return x;
})();


constexpr std::array<std::array<KeyAction, 0x16>, 0x39> winKeyActions = ([]() {
    std::array<std::array<KeyAction, 0x16>, 0x39> res;

    constexpr uint8_t KM_WIN   = KEYBOARD_MODIFIER_LEFTGUI;
    constexpr uint8_t KM_CTRL  = KEYBOARD_MODIFIER_LEFTCTRL;
    constexpr uint8_t KM_ALT   = KEYBOARD_MODIFIER_LEFTALT;
    constexpr uint8_t KM_SHIFT = KEYBOARD_MODIFIER_LEFTSHIFT;

    // res[HID_KEY_R][SHIFT|CTRL] = KA_Lambda([](KeyboardStateMachine&) {
    //     reset_usb_boot(0,0);
    // });
    // res[HID_KEY_R][SHIFT|ALT] = KA_Lambda([](KeyboardStateMachine&) {
    //     watchdog_reboot(0,0,0);
    // });
    res[HID_KEY_R][0] = KA_Lambda([] (KeyboardStateMachine& sm) {
        sm.toggleRuskiMode();
    }, "RuskiMode");
    res[HID_KEY_R][LINUX] = KA_Lambda([] (KeyboardStateMachine& sm) {
        sm.toggleRuskiMode();
    });

    //alt tab
    res[HID_KEY_O][0] = KA_KeyWithModifier({KM_ALT, HID_KEY_TAB}, "AltTab");

    res[HID_KEY_P][0]    = KA_MediaKey(HID_USAGE_CONSUMER_PLAY_PAUSE, "Play");
    res[HID_KEY_P][CTRL] = KA_KeyWithModifier({0, HID_KEY_PRINT_SCREEN}, "PrScr");
    res[HID_KEY_P][ALT]  = KA_KeyWithModifier({KM_WIN|KM_SHIFT, HID_KEY_S}, "Win+S");

    
    res[HID_KEY_Z][0]    = KA_MediaKey(HID_USAGE_CONSUMER_SCAN_PREVIOUS, "Prev");
    res[HID_KEY_X][0]    = KA_MediaKey(HID_USAGE_CONSUMER_SCAN_NEXT, "Next");

    //press windows key by its own
    res[HID_KEY_X][CTRL] = KA_KeyWithModifier({KM_WIN, 0}, "Win");
    res[HID_KEY_X][ALT] = KA_KeyWithModifier({KM_WIN, 0}, "Win");

    res[HID_KEY_C][ALT] = KA_KeyWithModifier({0, HID_KEY_CAPS_LOCK}, "Caps");
    res[HID_KEY_N][ALT] = KA_KeyWithModifier({0, HID_KEY_NUM_LOCK}, "NumL");

    res[HID_KEY_M][0] = KA_Lambda([] (KeyboardStateMachine& sm) {
        auto num = sm.getStateSnapshot().musicItemNum;
        if (num <= 0 || num > 9) return;
        uint8_t key = HID_KEY_1 - 1 + num;
        KeyWithModifier seq[2] = {
            {KEYBOARD_MODIFIER_LEFTGUI, key},
            {0, 0}
        };
        UsbHid::getInstance().sendSequence(seq, 2);
    }, "Music");

    res[HID_KEY_M][CTRL] = KA_MediaKey(HID_USAGE_CONSUMER_MUTE, "Mute");

    res[HID_KEY_BRACKET_LEFT][0]  = KA_MediaKey(HID_USAGE_CONSUMER_REWIND, "Rewind");
    res[HID_KEY_BRACKET_RIGHT][0] = KA_MediaKey(HID_USAGE_CONSUMER_FAST_FORWARD, "FastFwd");

    res[HID_KEY_SEMICOLON][0]   = KA_MediaKey(HID_USAGE_CONSUMER_VOLUME_DECREMENT);
    res[HID_KEY_APOSTROPHE][0]  = KA_MediaKey(HID_USAGE_CONSUMER_VOLUME_INCREMENT);

    res[HID_KEY_COMMA][0]   = KA_MediaKey(HID_USAGE_CONSUMER_BRIGHTNESS_DECREMENT);
    res[HID_KEY_PERIOD][0]  = KA_MediaKey(HID_USAGE_CONSUMER_BRIGHTNESS_INCREMENT);

    //COMMA, PERIOD, SLASH  
    
    //super-alt-k is already mute microphone

    //alt tab
    res[HID_KEY_SPACE][0] = KA_KeyWithModifier({KEYBOARD_MODIFIER_LEFTALT, HID_KEY_TAB}, "AltTab");

    return res;
})();

};