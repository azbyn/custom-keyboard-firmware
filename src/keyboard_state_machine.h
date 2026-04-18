#pragma once
#include <pico/stdlib.h>
#include "pins.h"

// #include "usb_hid.h" //UnicodeInputMode

// #include "display.hpp"

enum UnicodeInputMode {
    UIM_WinCompose,
    UIM_WinNumpad,
    UIM_Linux,

    UIM__Size,
};
enum DisplayState {
    DS_OFF,
    DS_Normal,
    DS_Menu,
    DS_Debug,
    DS_Search,
};

//anything in here should trigger a redraw if changed
struct KeyboardStateSnapshot {
    bool numLock, capsLock, scrollLock;
    bool ruskiMode;
    bool composeMode;

    //not part of "state" so we can remember the previous state
    bool usbSuspended = false;
    bool usbMounted = false;

    bool keyPressed = false;

    
    DisplayState state = DS_Normal; 
    
    //stats
    uint32_t secSinceBoot = 0;
    uint32_t updatesLastS = 0;
    
    // KeyboardSettings settings;//settings
    UnicodeInputMode unicodeInputMode = UIM_WinCompose;
    bool showBg = true;
    uint8_t screenBrightness = 255;
    bool autoNoCaps = true;



};

class KeyboardStateMachine {
private:
    KeyboardStateSnapshot stateSnapshot;
    uint32_t updatesThisS = 0;
public:
    //ie try to automatically disable caps and enable num lock
    bool raltMode = false;

private:
    KeyboardStateMachine() {}
public:
    constexpr static KeyboardStateSnapshot getStateSnapshot()  {
        return getInstance().stateSnapshot;
    }

    static KeyboardStateSnapshot& getMutableStateSnapshot()  {
        return getInstance().stateSnapshot;
    }
    static KeyboardStateMachine& getInstance() {
        static KeyboardStateMachine instance;
        return instance;
    }
    KeyboardStateMachine(const KeyboardStateMachine&) = delete;
    KeyboardStateMachine& operator=(const KeyboardStateMachine&) = default;

    void init() {
        constexpr Pin leds[3] = {pins::ledCompose, pins::ledMenu, pins::ledRuski};
        for (auto pin : leds) {
            gpio_init(pin);
            gpio_set_dir(pin, true);
        }
    }
private:
public:
    struct KeyboardLeds {
        //rather than remembering which arg is which, pass a struct
        bool numlock, capslock,scrollock, compose;
    };
    void updateKeyboardLeds(KeyboardLeds leds) {
        auto& ss = stateSnapshot;
        ss.capsLock = leds.capslock;
        ss.numLock = leds.numlock;
        ss.scrollLock = leds.scrollock;
        ss.composeMode = leds.compose;

        gpio_put(pins::ledCompose, ss.composeMode);
        forceRedraw();
    }
    void update() {
        uint32_t newSecSinceBoot = to_ms_since_boot(get_absolute_time()) / 1000;
        updatesThisS++;
        if (newSecSinceBoot != stateSnapshot.secSinceBoot) {
            // gpio_put(pins::ledMenu, secSinceBoot&1);
            stateSnapshot.secSinceBoot = newSecSinceBoot;
            stateSnapshot.updatesLastS = updatesThisS;
            updatesThisS = 0;
            forceRedraw();
        }
    }
   


    void toggleRuskiMode() {
        setRuskiMode(!getRuskiMode());
    }
    
    constexpr bool getRuskiMode() const {
        return stateSnapshot.ruskiMode;
    }
    void setRuskiMode(bool val) {
        stateSnapshot.ruskiMode = val;
        gpio_put(pins::ledRuski, val);
        forceRedraw();
    }
    constexpr UnicodeInputMode getUnicodeInputMode() const { 
        return stateSnapshot.unicodeInputMode; 
    }
    // constexpr KeyboardSettings& getSettings() {
    //     return stateSnapshot.settings;
    // }

    constexpr DisplayState getDisplayState() const {
        return stateSnapshot.state;
    }
    constexpr bool getAutoCapsMode() const {
        return stateSnapshot.autoNoCaps;
    }
    void setDisplayState(DisplayState state)  {
        gpio_put(pins::ledMenu, state == DS_Menu);
        if (stateSnapshot.state == state) return;

        stateSnapshot.state = state;
        forceRedraw();
    }
    void onUsbSuspendMode(bool value) {
        if (stateSnapshot.usbSuspended == value) return;
        stateSnapshot.usbSuspended = value;
        //todo add a delay on suspend
        forceRedraw();
    }
    void onUsbMount(bool value) {
        if (value && stateSnapshot.usbSuspended) {
            //is this needed - sometimes when uploading new sw it's stuck in suspend mode?
            stateSnapshot.usbSuspended = false;
        }
        // if (stateSnapshot.usbMounted == value) return;

        stateSnapshot.usbMounted = value;
        forceRedraw();
    }
    void onKeypress(bool value) {
        stateSnapshot.keyPressed = value;
        forceRedraw();
    }
private:
//defined in keyboard logic.cpp to make c++ happy about cyclic dependencies
    void forceRedraw();
};