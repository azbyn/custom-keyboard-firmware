#pragma once
#include <stdio.h>
#include "keyboard_state_machine.h"

#include <pico/bootrom.h> //reset_usb_boot
#include <hardware/watchdog.h>

#include "usb_cdc.h"
#include "usb_hid.h"

struct SettingItem {
    const char* name;

    void (*move)(int value, KeyboardStateSnapshot&);
    const char* (*getValName)(const KeyboardStateSnapshot&);
    
    // = delete;


    // const char* const* options;
    // size_t optionsLength;

    // can only be +1 or -1
    // void move(int value, KeyboardSettings&) const {}// = delete;

    // void left(KeyboardSettings&) = delete;
    // void right(KeyboardSettings&) = delete;
    // const char* getValName(const KeyboardSettings&) const {}// = delete;
};

#define BOOL_SETTING(_name, _what) \
    (SettingItem { \
        .name = _name, \
        .move = [] ( [[maybe_unused]] int value, [[maybe_unused]] KeyboardStateSnapshot& s) { \
            /*if (value < 0) return;*/\
            _what = !_what;\
        },\
        .getValName = [] ([[maybe_unused]] const KeyboardStateSnapshot& s) {\
            return _what ? "on" : "off";\
        }\
    })
#define BOOL_SETTING_GETTER_SETTER(_name, _get, _set) \
    (SettingItem { \
        .name = _name, \
        .move = [] ( [[maybe_unused]] int value, [[maybe_unused]] KeyboardStateSnapshot& s) { \
            /*if (value < 0) return;*/\
            _set(!_get());\
        },\
        .getValName = [] ([[maybe_unused]] const KeyboardStateSnapshot& s) {\
            return (_get()) ? "on" : "off";\
        }\
    })
    
#define ACTION_SETTING(_name, _action) \
    (SettingItem {\
        .name = _name,\
        .move = [] (int x, [[maybe_unused]] KeyboardStateSnapshot& s) {\
            if (x > 0) { _action; } \
        },\
        .getValName = [] (const KeyboardStateSnapshot&) -> const char* { return ""; }\
    })
#define ENUM_SETTING(_name, _what, _OptionsSize, _optionNamesArray) \
    (SettingItem { \
        .name = _name,\
        .move = [] (int value, [[maybe_unused]] KeyboardStateSnapshot& s) {\
            auto new_val = _what - value;\
            \
            if (new_val < 0) {\
                new_val = _OptionsSize - 1;\
            }\
            else if (new_val >= _OptionsSize) {\
                new_val = 0;\
            }\
            _what = (decltype(_what)) new_val;\
        },\
        .getValName = [] ([[maybe_unused]]const KeyboardStateSnapshot& s) {\
            constexpr std::array<const char*, _OptionsSize> optionNames = _optionNamesArray;\
            /*the 'if' should never happen*/\
            if (_what >= _OptionsSize || _what < 0)  return "?";\
            return optionNames[_what];\
        }\
    })
/*
namespace UnicodeInputModeSetting {
static constexpr std::array<const char*, UIM__Size> optionNames = [] {
    std::array<const char*, UIM__Size> res;
    res[UIM_WinCompose] = "WinComp";
    res[UIM_WinNumpad] = "Numpad";
    res[UIM_Linux] = "Linux";
    return res;
}();
static_assert(UIM__Size == 3);
constexpr void move(int value, KeyboardStateSnapshot& s)  {
    auto new_val = s.unicodeInputMode - value;

    if (new_val < 0) {
        new_val = UIM__Size - 1;
    } 
    else if (new_val >= UIM__Size) {
        new_val = 0;
    }
    s.unicodeInputMode = (UnicodeInputMode) new_val;
}
constexpr const char* getValName(const KeyboardStateSnapshot& s)  {
    //the 'if' should never happen
    if (s.unicodeInputMode >= UIM__Size || s.unicodeInputMode < 0)  return "?";
    return optionNames[s.unicodeInputMode];
}


constexpr SettingItem setting = {
    .name = "UniInput",
    .move = &move,
    .getValName = &getValName
};

};*/



namespace BrightnessSetting {

constexpr void move(int value, KeyboardStateSnapshot& s)  {
    int new_val = ((int)s.screenBrightness) + value * 64;

    //magical code so you don't get different values when you go backward vs forward,
    //(255, 191, 127, 63, 0) vs (0, 64, 128, 192, 255) 

    new_val++;
    new_val &= 0x1Fe;
    // &= 0xF7;
    if (new_val < 0)
        new_val = 0;
    else if (new_val > 255)
        new_val = 255;

    s.screenBrightness = new_val;
}
static char buffer[4];
inline const char* getValName(const KeyboardStateSnapshot& s)  {
    snprintf(buffer, sizeof(buffer), "%d", s.screenBrightness);
    return buffer;
}

constexpr SettingItem setting = {
    .name = "Display Bg",
    .move = &move,
    .getValName = &getValName
};

};

namespace MusicNumSetting {

constexpr void move(int value, KeyboardStateSnapshot& s)  {
    int new_val = s.musicItemNum + value;
    if (new_val < 1)
        new_val = 9;
    else if (new_val > 9)
        new_val = 1;

    s.musicItemNum = new_val;
}
static char buffer[4];
inline const char* getValName(const KeyboardStateSnapshot& s)  {
    snprintf(buffer, sizeof(buffer), "%d", s.musicItemNum);
    return buffer;
}

constexpr SettingItem setting = {
    .name = "Music Num",
    .move = &move,
    .getValName = &getValName
};

};


class SettingsNavigator {
    int currentSettingIdx = 0;


    // int getTopOffset = 0;

    //defined in keyboard logic
    // static constexpr int getMaxDisplaySz() {
    //     return MenuModeDisplayer::sz_y;
    // }
    static constexpr SettingItem unicodeInputModeSetting =
        ENUM_SETTING("UniInput", s.unicodeInputMode, UIM__Size, 
            ([] {
                std::array<const char*, UIM__Size> res;
                res[UIM_WinCompose] = "WinComp";
                res[UIM_WinNumpad] = "Numpad";
                res[UIM_Linux] = "Linux";
                return res;
            }()));
    static constexpr SettingItem reportRepeatMode =
        ENUM_SETTING("RepeatReport", s.repeatReportType, RRT__Size, 
            ([] {
                std::array<const char*, RRT__Size> res;
                res[RRT_No] = "No";
                res[RRT_One] = "One";
                res[RRT_Five] = "Five";

                res[RRT_Infinite] = "Inf";
                return res;
            }()));
    static constexpr SettingItem version = {
        .name = "Version",
        .move = [] (int, KeyboardStateSnapshot&) {},
        .getValName = [] (const KeyboardStateSnapshot&) { return PICO_PROGRAM_VERSION_STRING; },
    };
public:
    static constexpr SettingItem settingItems[] = {
        version,
        unicodeInputModeSetting,
        reportRepeatMode,

        BOOL_SETTING("Snd key umnted", s.sendCodesWhenKbdUnmounted),

        // UnicodeInputModeSetting::setting,

        BOOL_SETTING("Display Bg", s.showBg),
        // BOOL_SETTING("RepeatReport", s.repeatReport),

        BOOL_SETTING("Auto No Caps", s.autoNoCaps),

        // &BrightnessSetting::setting,

        MusicNumSetting::setting,
        
        ACTION_SETTING("Restart", {watchdog_reboot(0,0,0);}),
        ACTION_SETTING("Keybindings Show", { s.state = DS_ShowBindings;}),

        BOOL_SETTING_GETTER_SETTER("Serial", UsbCdc::isCdcEnabled, UsbCdc::setCdcEnabled),
        BOOL_SETTING_GETTER_SETTER("Nkro", UsbHid::isNkro, UsbHid::setNkro),

        ACTION_SETTING("Debug Log", { s.state = DS_Debug; }),
        ACTION_SETTING("Enter Bootloader", { reset_usb_boot(0,0); }),
        ACTION_SETTING("Back", { s.state = DS_Normal;}),
    };
    constexpr uint8_t getCurrentSettingIdx() const {return currentSettingIdx;}
    
    SettingsNavigator() {}
    static constexpr int settingItemsLength = sizeof(settingItems)/sizeof(settingItems[0]);
    static SettingsNavigator& getInstance() {
        static SettingsNavigator instance;
        return instance;
    }


    void up() {
        moveVert(-1);
    }
    void down() {
        moveVert(+1);
    }

    void left()  { 
        settingItems[currentSettingIdx].move(-1, getSS());
        KeyboardStateMachine::getInstance().forceRedraw();
    }
    void right() { 
        settingItems[currentSettingIdx].move(+1, getSS());  
        KeyboardStateMachine::getInstance().forceRedraw();
    }

    void exit() { KeyboardStateMachine::getInstance().setDisplayState(DS_Normal); }

private:
    KeyboardStateSnapshot& getSS() { return KeyboardStateMachine::getMutableStateSnapshot(); }
    void moveVert(int y) {
        int new_val = currentSettingIdx + y;

        if (new_val >= settingItemsLength) new_val = 0;
        else if (new_val < 0) new_val = settingItemsLength - 1;

        currentSettingIdx = new_val;
        KeyboardStateMachine::getInstance().forceRedraw();
    }
};