#pragma once
#include <stdio.h>
#include "keyboard_state_machine.h"

#include <pico/bootrom.h> //reset_usb_boot
#include <hardware/watchdog.h>

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

namespace UnicodeInputModeSetting {
static constexpr const char* optionNames[] = {
    [UIM_WinCompose] = "WinComp",
    [UIM_WinNumpad] = "Numpad",
    [UIM_Linux] = "Linux",
};
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

};

namespace ShowBgSetting {

constexpr void move(int value, KeyboardStateSnapshot& s)  {
    (void) value;
    s.showBg = !s.showBg;
}
constexpr const char* getValName(const KeyboardStateSnapshot& s)  {
    return s.showBg ? "on" : "off";
}


constexpr SettingItem setting = {
    .name = "Display Bg",
    .move = &move,
    .getValName = &getValName
};

};
namespace AutoNoCapsSetting {
constexpr void move(int value, KeyboardStateSnapshot& s)  {
    (void) value;
    s.autoNoCaps = !s.autoNoCaps;
}
constexpr const char* getValName(const KeyboardStateSnapshot& s)  {
    return s.autoNoCaps ? "on" : "off";
}


constexpr SettingItem setting = {
    .name = "Auto No Caps",
    .move = &move,
    .getValName = &getValName
};

};

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


namespace MenuButtons {

inline const char* getValName(const KeyboardStateSnapshot& s)  {
    return "";
}

constexpr SettingItem restart = {
    .name = "Restart",
    .move = [] (int x, KeyboardStateSnapshot& ) {
        if (x > 0) watchdog_reboot(0,0,0);
    },
    .getValName = &getValName
};
constexpr SettingItem bootloader = {
    .name = "Enter Bootloader",
    .move = [] (int x, KeyboardStateSnapshot& ) {
        if (x > 0) reset_usb_boot(0,0);
    },
    .getValName = &getValName
};
constexpr SettingItem debug = {
    .name = "Debug",
    .move = [] (int x, KeyboardStateSnapshot& s) {
        if (x > 0) s.state = DS_Debug;
    },
    .getValName = &getValName
};

constexpr SettingItem back = {
    .name = "Back",
    .move = [] (int x, KeyboardStateSnapshot& s) {
        if (x > 0) s.state = DS_Normal;
    },
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

public:
    static constexpr const SettingItem* settingItems[] = {
        &UnicodeInputModeSetting::setting,
        &ShowBgSetting::setting,
        &BrightnessSetting::setting,
        &AutoNoCapsSetting::setting,
        &MusicNumSetting::setting,
        
        &MenuButtons::restart,
        &MenuButtons::bootloader,
        &MenuButtons::debug,
        &MenuButtons::back,
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

    void left()  { settingItems[currentSettingIdx]->move(-1, getSS()); }
    void right() { settingItems[currentSettingIdx]->move(+1, getSS());  }

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