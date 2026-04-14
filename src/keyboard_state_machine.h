#pragma once
#include <pico/stdlib.h>
#include "pins.h"

enum UnicodeInputMode {
    WinCompose,
    WinNumpad,
    Linux,
};

class KeyboardStateMachine {
    enum State {
        Normal,
        Menu,
        Debug
    } state;
    bool ruskiMode = false;

public:
    bool menuMode = false;
    bool windowsMode = true;//send alt+ escapes if in windows mode
    //ie try to automatically disable caps and enable num lock
    bool autoNoCaps = true;
    bool raltMode = false;


    //win key stuff
    bool pressingLWin = false;
    bool pressingRWin = false;

private:
    KeyboardStateMachine() {}
public:
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
    void update() {
        static uint32_t prevSecSinceBoot = 0;

        uint32_t sSinceBoot = to_ms_since_boot(get_absolute_time()) / 1000;

        if (sSinceBoot != prevSecSinceBoot) {
            gpio_put(pins::ledMenu, sSinceBoot&1);
            prevSecSinceBoot = sSinceBoot;
        }
    }

    void setComposeMode(bool val) {
        gpio_put(pins::ledCompose, val);
    }

    void toggleRuskiMode() {
        setRuskiMode(!getRuskiMode());
    }
    
    constexpr bool getRuskiMode() const {
        return ruskiMode;
    }
    void setRuskiMode(bool val) {
        ruskiMode = val;
        gpio_put(pins::ledRuski, ruskiMode);

    }
};