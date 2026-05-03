#pragma once

// #include <stdlib.h>
#include <utility> //std::forward
#include <format>

#include <stdio.h> //snprintf
#include <stddef.h> //ptrdiff_t

#include <stdint.h>
// #include <hardware/spi.h>
// #include <hardware/gpio.h>
// #include <pico/time.h>

#include <pico/multicore.h>

// #include "font8x8_basic.h"
// #include "pins.h"

#include "keyboard_state_machine.h"

#include "mode_displays/display_common.h"
#include "mode_displays/debug_msg_displayer.h"
#include "mode_displays/normal_mode_displayer.h"
#include "mode_displays/menu_mode_displayer.h"
#include "mode_displays/show_bindings_mode_displayer.h"

#include "utils.h"

// #include "usb_cdc.h"

class Display {
    St7789 lcd;

    static constexpr uint16_t width = St7789::width;
    static constexpr uint16_t height = St7789::height;

    static constexpr int char_w = 8;
    static constexpr int char_h = 8;

    DebugMsgDisplayer debugMsgDisplayer;
    NormalModeDisplayer normalModeDisplayer;
    MenuModeDisplayer menuModeDisplayer;
    ShowBindingsModeDisplayer showBindingsModeDisplayer;
    //init as 1 so we get an update after init
    int dirtiness = 1;

    mutex_t bufferMutex;

    Display() {}
public:
    auto getDirtiness() const { return dirtiness; }
    
    static Display& getInstance() {
        static Display instance;
        return instance;
    }
    Display(Display&&) = default;
    Display(const Display&) = delete;
    Display& operator=(const Display&) = default;
    Display& operator=(Display&&) = delete;

    void forceRedraw() {
        mutex_enter_blocking(&bufferMutex);
        dirtiness++;
        mutex_exit(&bufferMutex);
    }
    void init() {
        lcd.init();

        mutex_init(&bufferMutex);
    }
    void loop() {
        for (;;) {
            if (dirtiness > 0)
                draw();
            sleep_ms(1);
        }
    }
    
    void cls() {
        mutex_enter_blocking(&bufferMutex);
        this->debugMsgDisplayer.cls();
        ++dirtiness;
        mutex_exit(&bufferMutex);
    }
    void _print(std::string_view x) {
        mutex_enter_blocking(&bufferMutex);
        this->debugMsgDisplayer.print(x);
        ++dirtiness;
        mutex_exit(&bufferMutex);
        // UsbCdc::print(x);
    }
    void putc(char x) {
        mutex_enter_blocking(&bufferMutex);
        this->debugMsgDisplayer.putc(x);
        ++dirtiness;
        mutex_exit(&bufferMutex);
        // UsbCdc::print(x);
    }
   

    template <typename... Args>
    static void print(std::format_string<Args...> fmt, Args&&... args) {
        std::format_to(DisplayCharSink{}, fmt, std::forward<Args>(args)...);
    }
    // static void print(const char* x) {
    //     getInstance().print(x);
    // }
    static void print(std::string_view x) {
        getInstance()._print(x);
    }
    void showKeybindingsUp() {
        showBindingsModeDisplayer.up();
        forceRedraw();
    }
     void showKeybindingsDown() {
        showBindingsModeDisplayer.down();
        forceRedraw();
    }
private:
    uint32_t startDimBlTime = 0;
    bool prevDimmed = false;
    void draw() {
        int prevDirtiness = dirtiness;

        auto stateSnapshot = KeyboardStateMachine::getStateSnapshot();

        //only dim in normal mode
        bool shouldDim = stateSnapshot.usbSuspended && stateSnapshot.state == DS_Normal;

        // gpio_put(pins::ledMenu, stateSnapshot.usbSuspended);
        if (shouldDim) {
            if (!prevDimmed) {
                startDimBlTime = millis() + 1000;
                prevDimmed = true;
            }
            
            if (startDimBlTime) {
                if (millis() >= startDimBlTime) {
                    startDimBlTime = 0;
                    gpio_put(pins::disp::bl, false);
                }
            }
            
            // prevSuspend = stateSnapshot.usbSuspended;
            // this->lcd.fill_color(colors::blue);
            //TODO again, add delay?
            return;
        }
        else if (prevDimmed) {
            prevDimmed = false;
            lcd.init();
        }
        if (!stateSnapshot.usbMounted) {
            this->lcd.fill_color(colors::black);
            //TODO also disable backlight
            return;
        }

        switch (stateSnapshot.state) {
            case DS_Normal:
                // printf("updates %ld\n", stateSnapshot.updatesLastS);
                normalModeDisplayer.draw(this->lcd, stateSnapshot);
                break;
            case DS_Menu:
                menuModeDisplayer.draw(this->lcd, stateSnapshot, SettingsNavigator::getInstance());
                break;
            case DS_Debug: 
                debugMsgDisplayer.draw(this->lcd, this->bufferMutex);
                break;
            case DS_ShowBindings:
                showBindingsModeDisplayer.draw(this->lcd);
                break;
        }

        mutex_enter_blocking(&bufferMutex);
        //if no new updates happened while we were drawing, we can say we're done
        if (dirtiness == prevDirtiness)
            dirtiness = 0;
        mutex_exit(&bufferMutex);
    }
};
