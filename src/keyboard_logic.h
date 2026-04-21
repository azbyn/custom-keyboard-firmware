#pragma once

#include <stdint.h>

#include "fixed_vector.h"
#include "usb_hid.h"
#include "keyboard_matrix.h"
#include "display.hpp"
#include "key_layout_definitions.h"
#include "keyboard_state_machine.h"

#include "button.h"
#include "rotary_encoder.h"

#include "settings_navigator.h"

class KeyboardLogic {
    Button encoderBtn = Button(pins::encoder_push);
    RotaryEncoder encoder = RotaryEncoder(pins::encoder_A, pins::encoder_B);
    Button bumperBtn = Button(pins::bumper);

    KeyboardStateMachine& stateMachine;
    UsbHid& hid;
    SettingsNavigator& settingsNavigator;
    bool pressedWinCombo = false;

    //win key stuff
    bool pressingLWin = false;
    bool pressingRWin = false;

    KeyboardLogic()
        : stateMachine(KeyboardStateMachine::getInstance()),
          hid(UsbHid::getInstance()),
          settingsNavigator(SettingsNavigator::getInstance()) {}
//public:
//private:
    
public:
    static KeyboardLogic& getInstance() {
        static KeyboardLogic instance;
        return instance;
    }
    KeyboardLogic(const KeyboardLogic&) = delete;
    KeyboardLogic& operator=(const KeyboardLogic&) = default;
    void init() {
        encoder.init();
        encoderBtn.init();
        bumperBtn.init();
        stateMachine.init();

        if (!stateMachine.getAutoCapsMode()) return;

        // might not be needed, i think we get the request to set the leds on startup regardless
        // KeyWithModifier seq[] = {
        //     {0, HID_KEY_NUM_LOCK},//HID_KEY_SCROLL_LOCK},
        //     {0, 0},
        // };
        // hid.sendSequence(seq, sizeof(seq)/sizeof(seq[0]));
    }

    void update() {
        stateMachine.update();

        int encVal = encoder.update();
        bool encDown = encoderBtn.isDown();
        
        switch (stateMachine.getDisplayState()) {
        case DS_Debug:
            if (encVal < 0) {
                Display::getInstance().cls();
            } 
            if (encDown) 
                stateMachine.setDisplayState(DS_Menu);
            break;
        case DS_Menu:
            if (encVal < 0) {
                settingsNavigator.up();
            } else if (encVal > 0) {
                settingsNavigator.down();
            }
            if (encDown) 
                settingsNavigator.right();

            break;
        case DS_ShowBindings:
            if (encVal < 0) {
                Display::getInstance().showKeybindingsUp();
            } else if (encVal > 0) {
                Display::getInstance().showKeybindingsDown();
            }
            if (encDown) 
                stateMachine.setDisplayState(DS_Normal);
            break;
        default:
            if (encVal < 0) {
                hid.pressMedia(HID_USAGE_CONSUMER_VOLUME_DECREMENT);
            } else if (encVal > 0) {
                hid.pressMedia(HID_USAGE_CONSUMER_VOLUME_INCREMENT);
            } 
            if (encDown)
                stateMachine.setDisplayState(DS_Menu);

                // reset_usb_boot(0,0);

            break;
        };
           
        if (bumperBtn.isDown()) {
            watchdog_reboot(0,0,0);
            // printf("bumper\n");
        }
    }
    void onKbdLedRequest(uint8_t state) {
        stateMachine.updateKeyboardLeds(
            { .numlock = (state & KEYBOARD_LED_NUMLOCK) != 0,
              .capslock = (state & KEYBOARD_LED_CAPSLOCK) != 0,
              .scrollock = (state & KEYBOARD_LED_SCROLLLOCK) != 0,
              .compose = (state & KEYBOARD_LED_COMPOSE) != 0});

        // Display::printf("LED %02x\n", state);
        if (!stateMachine.getAutoCapsMode()) return;

        FixedVector<KeyWithModifier, 4> keys;
        //enable num lock if disabled
        if (!(state & KEYBOARD_LED_NUMLOCK))
            keys.push_back({0, HID_KEY_NUM_LOCK});
        if (state & KEYBOARD_LED_CAPSLOCK)
            keys.push_back({0, HID_KEY_CAPS_LOCK});
        if (state & KEYBOARD_LED_SCROLLLOCK)
            keys.push_back({0, HID_KEY_SCROLL_LOCK});
        if (keys.size() == 0) return;
        keys.push_back({0, 0});

        UsbHid& hid = UsbHid::getInstance();
        hid.sendSequence(keys.begin(), keys.size());
    }

    void onKeypress(KeyPosition pos, KeyState state) {
        stateMachine.onKeypress(state);
        // gpio_put(pins::leds[1], state);
        // return;

        static char buffer[256];
        UsbHid& hid = UsbHid::getInstance();
        // gpio_put(pins::leds[0], i%2);


        // snprintf(buffer, sizeof(buffer), "Key %d; %d - %d", pos.row, pos.col, state);
        // printf("ON KP %s\n", buffer);
        // Display::getInstance().print(buffer);
        uint8_t key = key_layout_definitions::keys[pos.row][pos.col];

        // gpio_put(pins::leds[1], state);

        if (handleSpecialKeypress(pos, state, key)) {
            return;
        }
        
        if (state == KeyState::PRESSED)
            hid.press(key);
        else 
            hid.release(key);
    }

private:
    // return true if handled here
    bool handleSpecialKeypress(KeyPosition pos, KeyState state, uint8_t key) { 
        if (stateMachine.getDisplayState() == DS_Menu) {
            switch (key) {
            case HID_KEY_ESCAPE: 
            case HID_KEY_WILDCARD:
                if (state) settingsNavigator.exit();
                break;

            case HID_KEY_ARROW_LEFT:
                if (state) settingsNavigator.left();
                break;

            case HID_KEY_ENTER:
            case HID_KEY_KEYPAD_ENTER:
            case HID_KEY_ARROW_RIGHT:
                if (state) settingsNavigator.right();
                break;

            case HID_KEY_ARROW_UP:
                if (state) settingsNavigator.up();
                break;

            case HID_KEY_ARROW_DOWN:
                if (state) settingsNavigator.down();
                break;
                
            //so the keys don't get stuck:
            case HID_KEY_LCTRL: hid.setModifierState(KEYBOARD_MODIFIER_LEFTCTRL, state); return true;
            case HID_KEY_LALT: hid.setModifierState(KEYBOARD_MODIFIER_LEFTALT, state); return true;
            case HID_KEY_SHIFT_LEFT: hid.setModifierState(KEYBOARD_MODIFIER_LEFTSHIFT, state); return true;
            case HID_KEY_RCTRL: hid.setModifierState(KEYBOARD_MODIFIER_RIGHTCTRL, state); return true;
            case HID_KEY_SHIFT_RIGHT: hid.setModifierState(KEYBOARD_MODIFIER_RIGHTSHIFT, state); return true;

            case HID_KEY_GUI_LEFT: 
                this->pressingLWin = state;
                pressedWinCombo = false;
                break;

            case HID_KEY_GUI_RIGHT:
                this->pressingRWin = state; 
                pressedWinCombo = false;
                break;
            case HID_KEY_RALT:
                stateMachine.raltMode = state;
                break;
            }
            return true;
        }
        auto& hid = UsbHid::getInstance();
        auto& disp = Display::getInstance();

        switch(key) {
        case HID_KEY_LCTRL: hid.setModifierState(KEYBOARD_MODIFIER_LEFTCTRL, state); return true;
        case HID_KEY_LALT: hid.setModifierState(KEYBOARD_MODIFIER_LEFTALT, state); return true;
        case HID_KEY_SHIFT_LEFT: hid.setModifierState(KEYBOARD_MODIFIER_LEFTSHIFT, state); return true;
        case HID_KEY_RCTRL: hid.setModifierState(KEYBOARD_MODIFIER_RIGHTCTRL, state); return true;
        case HID_KEY_SHIFT_RIGHT: hid.setModifierState(KEYBOARD_MODIFIER_RIGHTSHIFT, state); return true;

        case HID_KEY_GUI_LEFT: 
            // disp.printf("LWIN %d;%d-%d; %d\n", state, 
            //     !!(hid.getModifiers() & KEYBOARD_MODIFIER_LEFTGUI),
            //     !!(hid.getModifiers() & KEYBOARD_MODIFIER_RIGHTGUI),
            //     pressedWinCombo);

            this->pressingLWin = state;

            // we pressed some key combo with action none
            if (!state && (hid.getModifiers() & KEYBOARD_MODIFIER_LEFTGUI)) {
                hid.setModifierState(KEYBOARD_MODIFIER_LEFTGUI, false);
            }
            // i don't like this; send no key
            #if 0
            if (!this->pressingLWin && !this->pressingRWin) {
                
                if (!pressedWinCombo) {
                    //if just pressed the win key, send a quick burst of win key
                    KeyWithModifier seq[2] = {
                        {KEYBOARD_MODIFIER_LEFTGUI, 0,},
                        {0, 0}
                    };
                    hid.sendSequence(seq, 2);
                }

                pressedWinCombo = false;
            }
            #endif
            return true;
            //hid.setModifierState(KEYBOARD_MODIFIER_LEFTGUI, state); return true;
        case HID_KEY_GUI_RIGHT:
            this->pressingRWin = state; 
            if (!state && (hid.getModifiers() & KEYBOARD_MODIFIER_RIGHTGUI))
                hid.setModifierState(KEYBOARD_MODIFIER_RIGHTGUI, false);

            if (!this->pressingLWin && !this->pressingRWin) {
                #if 0
                if (!pressedWinCombo) {
                    //if just pressed the win key, send a quick burst of win key
                    KeyWithModifier seq[2] = {
                        {KEYBOARD_MODIFIER_RIGHTGUI, 0,},
                        {0, 0}
                    };
                    hid.sendSequence(seq, 2);
                }
                #endif
                pressedWinCombo = false;
            }

            return true;
            //hid.setModifierState(KEYBOARD_MODIFIER_RIGHTGUI, state); return true;

        case HID_KEY_RALT:
            stateMachine.raltMode = (state == KeyState::PRESSED);
            return true;
        case HID_KEY_WILDCARD:
            if (this->pressingLWin || this->pressingRWin) {
                stateMachine.setDisplayState(DS_Menu);
                return true;
            }
            hid.setModifierState(KEYBOARD_MODIFIER_RIGHTALT, state); 
            return true;
            
        default: break;
        }

        if (this->pressingLWin || this->pressingRWin) {
            pressedWinCombo = true;
            uint8_t modifiers = 0;
            if (hid.getCtrlState()) 
                modifiers |= key_layout_definitions::CTRL;
            if (hid.getAltState()) 
                modifiers |= key_layout_definitions::ALT;
            if (hid.getShiftState()) 
                modifiers |= key_layout_definitions::SHIFT;
            if (stateMachine.getUnicodeInputMode() == UIM_Linux)
                modifiers |= key_layout_definitions::LINUX;

            using KeyAction = key_layout_definitions::KeyAction;
            KeyAction a = KeyAction();
            if (key < key_layout_definitions::winKeyActions.size())
                a = key_layout_definitions::winKeyActions[key][modifiers];
            if (a.isNothing()) {
                if (state) {
                    auto mod =  this->pressingLWin?KEYBOARD_MODIFIER_LEFTGUI:KEYBOARD_MODIFIER_RIGHTGUI;
                    KeyWithModifier seq[2] = {
                        {mod, key},
                        {0, 0}
                    };
                    hid.sendSequence(seq, 2);
                }
                // disp.print("nWIN\n");

                //let the normal handler handle this
                
                // hid.setModifierState(
                //    , 
                //     state);
            
                return true;
            }
            if (state) {
                //we could theoretically keep pressing the keyWithModifier/media key until 
                //the key combo is released, but it's simpler (fewer edge cases with multiple keys pressed)
                //to just send the key and quickly release it

                switch (a.tag) {
                    //handled above
                    //case KeyAction::Nothing: break;
                    case KeyAction::KeyboardKeyWithModifier: {
                        KeyWithModifier seq[2] {
                            a.keyboardKeyWithModifier,
                            {0,0},
                        };
                        hid.sendSequence(seq, 2);
                    } break;
                    case KeyAction::MediaKey:
                        hid.pressMedia(a.mediaKey);
                        ///hid.releaseMedia();

                        break;
                    case KeyAction::Lambda:
                        a.lambda(this->stateMachine);
                        break;
                }
            } else {
                //do nothing on key release, since we already did on key press
            } 

            return true;
        }

        
        if (stateMachine.raltMode || stateMachine.getRuskiMode()) {
            if (key >= key_layout_definitions::unicodeKeyActions.size()) 
                return false;
            auto unicodeKey = key_layout_definitions::unicodeKeyActions[key];
            bool shiftMode = hid.getShiftState();

            //if in ruski mode if you press ctrl+c it sends it
            if (hid.getCtrlState() || hid.getAltState())
                return false;

            auto val = unicodeKey.get(shiftMode, stateMachine.raltMode, stateMachine.getRuskiMode());
            if (state) {
                Display::printf("rk_");
                hid.printKey(key);
                Display::printf("->%04x;%d\n", val, hid.seq_buffer_size());
            }

            // Display::printf("\n");
            if (!val) return false;

            // only send on push
            if (state) {

                hid.sendUnicodeSequence(val, stateMachine.getUnicodeInputMode());
                // hid.sendUnicodeSequence(u'0', stateMachine.windowsMode);

            }
            return true;
        }

        return false;
    }
};