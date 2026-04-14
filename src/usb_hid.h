#pragma once

#include <stdint.h>
#include <pico/stdlib.h>
#include <bsp/board_api.h>


#include <tusb.h>
#include "tusb_config.h"

#include "fixed_vector.h"
#include "bit_array.h"

#include "display.hpp"
#include "utils.h"

constexpr uint8_t BOARD_TUD_RHPORT = 0;
constexpr uint8_t REPORT_ID_KEYBOARD = 1;
constexpr uint8_t REPORT_ID_CONSUMER_CONTROL = 2;

constexpr size_t  KeyReportLength = 6; //can't be changed without also changing what functions we call & stuff

#define HID_KEY_ERR_ROLLOVER 0x01
//#define BOARD_TUD_RHPORT      0
//#define REPORT_ID_KEYBOARD  1
//#define REPORT_LEN 6

struct KeyWithModifier {
    uint8_t modifiers;
    uint8_t key;
};

void onKbdLedRequest(uint8_t state);
// {

//}

//a singleton class is used here because i don't have to forward declare everything
class UsbHid {
    FixedVector<KeyWithModifier, 512/*256*/> sequence_buffer;

    uint8_t modifiers = 0;
    BitArray<0x63/*0x82*/> keys;
    bool sendingConsumerKey = false;
    //uint8_t keys[KeyReportLength];

    UsbHid() {}

public:
    static UsbHid& getInstance(){
        static UsbHid instance;
        return instance;
    }
    UsbHid(const UsbHid&) = delete;
    UsbHid& operator=(const UsbHid&) = delete;


    void init() {
        board_init();
        tud_init(BOARD_TUD_RHPORT);
        if (board_init_after_tusb) {
            board_init_after_tusb();
        }
    }
    void update() {
        tud_task(); // tinyusb device task
        if (!sequence_buffer.empty()) {
            if (millis() > lastSendSeqMs+100)
                sendReportFromSequence();
        }
        //if 
    }
    void setModifierState(uint8_t modifier, bool state, bool _sendReport = true) {
        if (state)
            this->modifiers |= modifier;
        else
            this->modifiers &= ~modifier;
        if (_sendReport)
            sendReport();
    }
    constexpr uint8_t getModifiers() const { return modifiers; }
    constexpr bool getShiftState() const { 
        return modifiers & (KEYBOARD_MODIFIER_RIGHTSHIFT | KEYBOARD_MODIFIER_LEFTSHIFT ); 
    }
    constexpr bool getAltState() const {
        return modifiers & (KEYBOARD_MODIFIER_RIGHTALT | KEYBOARD_MODIFIER_LEFTALT ); 
    }
    constexpr bool getCtrlState() const {
        return modifiers & (KEYBOARD_MODIFIER_RIGHTCTRL | KEYBOARD_MODIFIER_LEFTCTRL ); 
    }
    
    void press(uint8_t key) {
        addKeyToKeyreport(key);
        sendReport();
    }
    void release(uint8_t key) {
        removeKeyFromKeyreport(key);

        sendReport();
    }
    void pressMedia(uint16_t key) {
        tud_hid_report(REPORT_ID_CONSUMER_CONTROL, &key, 2);
        sendingConsumerKey = true;
    }
    void releaseMedia() {
        uint16_t empty_key = 0;

        tud_hid_report(REPORT_ID_CONSUMER_CONTROL, &empty_key, 2);
        sendingConsumerKey = false;
    }

    /// @brief Send a sequence, one key at a time
    /// @param seq 
    /// @param len 
    /// @return returns true if all good, returns false if buffer is full 
    bool sendSequence(const KeyWithModifier* seq, size_t len) {
        auto res = sequence_buffer.push_n(seq, len);
        if (!res) return res;
        sendReportFromSequence();
        
        return true;
    }
    bool sendUnicodeSequence(char16_t val, bool windowsMode) {
        Display::printf("U+%04x;%d\n", val, sequence_buffer.size());
        //unicode is 21 bits -> 0x10FFFF max -> 7 digits
        //32 bits -> 10 digits
        // if (val > 0x10FFFF) return false;
        char buff[16];

        //4*2 for the keys and 1 extra to begin unicode input
        FixedVector<KeyWithModifier, 16> keys;
        //KeyWithModifier keys[16];


        //in windows mode, send alt + numbers on keypad (in hex)
        //else send shift_ctrl_u <hex>
        if (windowsMode) {
            snprintf(buff, sizeof(buff), "%04X", val);

            //snprintf(buff, sizeof(buff), "%d", val);
            keys.push_back({KEYBOARD_MODIFIER_RIGHTALT, HID_KEY_KEYPAD_ADD});
            for (const char* p = buff; *p; ++p) {
                char c = *p;
                uint8_t key;
                if (c >= '1' && c <= '9') {
                    key = uint8_t(HID_KEY_KEYPAD_1-1+c-'0');
                } else if (c=='0') {
                    key = HID_KEY_KEYPAD_0;
                } else if (c >= 'A' && c <= 'F'){
                    key = HID_KEY_A + (c-'A');
                } else {
                    continue;
                }
                keys.push_back({KEYBOARD_MODIFIER_RIGHTALT, key});
                if (*(p+1) == c)
                    keys.push_back({KEYBOARD_MODIFIER_RIGHTALT, 0});

            }
        } else {
            snprintf(buff, sizeof(buff), "%04X", val);
            keys.push_back({KEYBOARD_MODIFIER_LEFTCTRL|KEYBOARD_MODIFIER_LEFTSHIFT, 
                HID_KEY_U});
            for (const char* p = buff; *p; ++p) {
                char c = *p;
                uint8_t key;
                if (c >= '1' && c <= '9') {
                    key = uint8_t(HID_KEY_1-1+c-'0');
                } else if (c=='0') {
                    key = HID_KEY_0;
                } else if (c >= 'A' && c <= 'F'){
                    key = HID_KEY_A + (c-'A');
                } else {
                    continue;
                }
                keys.push_back({0, key});
                if (*(p+1) == c)
                    keys.push_back({0,0});
            }
            keys.push_back({0, HID_KEY_ENTER});
        }
        keys.push_back({0, 0});
        return sendSequence(keys.begin(), keys.size());
    }
    int seq_buffer_size() const {
        return sequence_buffer.size();
    }
    
private:
    uint32_t lastSendSeqMs = 0;

    void sendReportFromSequence() {
        lastSendSeqMs = millis();
        
        if (sequence_buffer.empty()) return;


        uint8_t keys[KeyReportLength] = {};
        auto x = sequence_buffer.pop_front();

        // Display::printf("S%d-", sequence_buffer.size());
        Display::printf("[", sequence_buffer.size());

        printKey(x);
        if (sequence_buffer.empty()) {
            Display::printf("\n");
        } else {
            Display::printf(";");
        }

        keys[0] = x.key;

        sendReport(x.modifiers, keys);
    }
    void sendReport(uint8_t modifiers, const uint8_t* keys) const {
        #if 0
        Display::printf(
            "%c%c%c%c_%c%c%c%c\n"
            "%02x %02x %02x %02x %02x %02x\n",
            ((modifiers & KEYBOARD_MODIFIER_LEFTCTRL) ? 'c': '_'),
            ((modifiers & KEYBOARD_MODIFIER_LEFTSHIFT) ? 's': '_'),
            ((modifiers & KEYBOARD_MODIFIER_LEFTALT) ? 'a': '_'),
            ((modifiers & KEYBOARD_MODIFIER_LEFTGUI) ? 'g': '_'),

            ((modifiers & KEYBOARD_MODIFIER_RIGHTCTRL) ? 'C': '_'),
            ((modifiers & KEYBOARD_MODIFIER_RIGHTSHIFT) ? 'S': '_'),
            ((modifiers & KEYBOARD_MODIFIER_RIGHTALT) ? 'A': '_'),
            ((modifiers & KEYBOARD_MODIFIER_RIGHTGUI) ? 'G': '_'),

            keys[0], keys[1], keys[2],
            keys[3], keys[4], keys[5]
        );
        #endif
        tud_hid_keyboard_report(REPORT_ID_KEYBOARD, modifiers, keys);
    }
public:
    static void printKey(KeyWithModifier k) {
         if (k.modifiers & (KEYBOARD_MODIFIER_RIGHTGUI|KEYBOARD_MODIFIER_LEFTGUI))
            Display::printf("G");
        if (k.modifiers & (KEYBOARD_MODIFIER_RIGHTALT|KEYBOARD_MODIFIER_LEFTALT))
            Display::printf("A");
        if (k.modifiers & (KEYBOARD_MODIFIER_RIGHTCTRL|KEYBOARD_MODIFIER_LEFTCTRL))
            Display::printf("C");
        if (k.modifiers & (KEYBOARD_MODIFIER_RIGHTSHIFT|KEYBOARD_MODIFIER_LEFTSHIFT))
            Display::printf("S");

        printKey(k.key);
    }

    static void printKey(uint8_t key) {
        auto& d = Display::getInstance();
        switch (key) {
        case HID_KEY_A: d.print("A"); break;
        case HID_KEY_B: d.print("B"); break;
        case HID_KEY_C: d.print("C"); break;
        case HID_KEY_D: d.print("D"); break;
        case HID_KEY_E: d.print("E"); break;
        case HID_KEY_F: d.print("F"); break;
        case HID_KEY_G: d.print("G"); break;
        case HID_KEY_H: d.print("H"); break;
        case HID_KEY_I: d.print("I"); break;
        case HID_KEY_J: d.print("J"); break;
        case HID_KEY_K: d.print("K"); break;
        case HID_KEY_L: d.print("L"); break;
        case HID_KEY_M: d.print("M"); break;
        case HID_KEY_N: d.print("N"); break;
        case HID_KEY_O: d.print("O"); break;
        case HID_KEY_P: d.print("P"); break;
        case HID_KEY_Q: d.print("Q"); break;
        case HID_KEY_R: d.print("R"); break;
        case HID_KEY_S: d.print("S"); break;
        case HID_KEY_T: d.print("T"); break;
        case HID_KEY_U: d.print("U"); break;
        case HID_KEY_V: d.print("V"); break;
        case HID_KEY_W: d.print("W"); break;
        case HID_KEY_X: d.print("X"); break;
        case HID_KEY_Y: d.print("Y"); break;
        case HID_KEY_Z: d.print("Z"); break;
        case HID_KEY_1: d.print("1"); break;
        case HID_KEY_2: d.print("2"); break;
        case HID_KEY_3: d.print("3"); break;
        case HID_KEY_4: d.print("4"); break;
        case HID_KEY_5: d.print("5"); break;
        case HID_KEY_6: d.print("6"); break;
        case HID_KEY_7: d.print("7"); break;
        case HID_KEY_8: d.print("8"); break;
        case HID_KEY_9: d.print("9"); break;
        case HID_KEY_0: d.print("0"); break;
        case HID_KEY_ENTER: d.print("RT"); break;
        default: Display::printf("%02x", key); break;

// #define HID_KEY_ESCAPE                      0x29
// #define HID_KEY_BACKSPACE                   0x2A
// #define HID_KEY_TAB                         0x2B
// #define HID_KEY_SPACE                       0x2C
// #define HID_KEY_MINUS                       0x2D
// #define HID_KEY_EQUAL                       0x2E
// #define HID_KEY_BRACKET_LEFT                0x2F
// #define HID_KEY_BRACKET_RIGHT               0x30
// #define HID_KEY_BACKSLASH                   0x31
// #define HID_KEY_EUROPE_1                    0x32
// #define HID_KEY_SEMICOLON                   0x33
// #define HID_KEY_APOSTROPHE                  0x34
// #define HID_KEY_GRAVE                       0x35
// #define HID_KEY_COMMA                       0x36
// #define HID_KEY_PERIOD                      0x37
// #define HID_KEY_SLASH                       0x38
// #define HID_KEY_CAPS_LOCK                   0x39
// #define HID_KEY_F1                          0x3A
// #define HID_KEY_F2                          0x3B
// #define HID_KEY_F3                          0x3C
// #define HID_KEY_F4                          0x3D
// #define HID_KEY_F5                          0x3E
// #define HID_KEY_F6                          0x3F
// #define HID_KEY_F7                          0x40
// #define HID_KEY_F8                          0x41
// #define HID_KEY_F9                          0x42
// #define HID_KEY_F10                         0x43
// #define HID_KEY_F11                         0x44
// #define HID_KEY_F12                         0x45
// #define HID_KEY_PRINT_SCREEN                0x46
// #define HID_KEY_SCROLL_LOCK                 0x47
// #define HID_KEY_PAUSE                       0x48
// #define HID_KEY_INSERT                      0x49
// #define HID_KEY_HOME                        0x4A
// #define HID_KEY_PAGE_UP                     0x4B
// #define HID_KEY_DELETE                      0x4C
// #define HID_KEY_END                         0x4D
// #define HID_KEY_PAGE_DOWN                   0x4E
// #define HID_KEY_ARROW_RIGHT                 0x4F
// #define HID_KEY_ARROW_LEFT                  0x50
// #define HID_KEY_ARROW_DOWN                  0x51
// #define HID_KEY_ARROW_UP                    0x52
// #define HID_KEY_NUM_LOCK                    0x53
// #define HID_KEY_KEYPAD_DIVIDE               0x54
// #define HID_KEY_KEYPAD_MULTIPLY             0x55
// #define HID_KEY_KEYPAD_SUBTRACT             0x56
// #define HID_KEY_KEYPAD_ADD                  0x57
// #define HID_KEY_KEYPAD_ENTER                0x58
// #define HID_KEY_KEYPAD_1                    0x59
// #define HID_KEY_KEYPAD_2                    0x5A
// #define HID_KEY_KEYPAD_3                    0x5B
// #define HID_KEY_KEYPAD_4                    0x5C
// #define HID_KEY_KEYPAD_5                    0x5D
// #define HID_KEY_KEYPAD_6                    0x5E
// #define HID_KEY_KEYPAD_7                    0x5F
// #define HID_KEY_KEYPAD_8                    0x60
// #define HID_KEY_KEYPAD_9                    0x61
// #define HID_KEY_KEYPAD_0                    0x62
// #define HID_KEY_KEYPAD_DECIMAL              0x63
// #define HID_KEY_EUROPE_2                    0x64
// #define HID_KEY_APPLICATION                 0x65
// #define HID_KEY_POWER                       0x66
// #define HID_KEY_KEYPAD_EQUAL                0x67
        }
    }
public:
    void sendReport() const {
        FixedVector<uint8_t, KeyReportLength> keysReport;
        
        for (auto i = 0; i < this->keys.actualCapacity(); ++i)  {
            if (!this->keys.get(i)) continue;
            bool hasSpace = keysReport.push_back(i);
            if (!hasSpace) {
                for (auto j = 0; j < KeyReportLength; ++j) {
                    keysReport[j] = HID_KEY_ERR_ROLLOVER;
                }
                break;
            }
        }

       
        this->sendReport(
        // tud_hid_keyboard_report(REPORT_ID_KEYBOARD,
             this->modifiers, keysReport.begin());
    }
private:
    void addKeyToKeyreport(uint8_t keycode) {
        if (keycode == 0) return;
        this->keys.set(keycode, true);
        #if 0
        if (keycode == 0) {
            /* not a key */
            return;
        }
        for (uint8_t i=0; i<6; i++) {
            if ( keys[i] == keycode ) {
                /* key is already present, no need to add it */
                return;
            }
        }
        for (uint8_t i=0; i<6; i++) {
            if ( keys[i] == 0 ) {
                /* found an empty slot, add keycode */
                keys[i] = keycode;
                return;
            }
        }
        /*
            If code reaches this point, the keys array is full with other keys.
            By spec this would prompt an error report.
            We just ignore the additional key, effectively imposing a hard limit to 6 simultaneous keys + modifiers.
        */
       #endif
    }

    void removeKeyFromKeyreport(uint8_t keycode) {
        if (keycode == 0) return;
        this->keys.set(keycode, false);
        #if 0
        if (keycode == 0) {
            /* not a key */
            return;
        }
        for (uint8_t i=0; i<6; i++) {
            if (keys[i] == keycode) {
                /* found keycode in slot, remove */
                keys[i] = 0;
                return;
            }
        }
        #endif
    }
public:
    // Invoked when sent REPORT successfully to host
    // Application can use this to send the next report
    // Note: For composite reports, report[0] is report ID
    void tud_hid_report_complete_cb(uint8_t instance, uint8_t const* report, 
            uint16_t len) {
        (void) instance;
        (void) len;
        //(void) report;

        if (report[0] == REPORT_ID_CONSUMER_CONTROL) {
            if (sendingConsumerKey) releaseMedia();
        } else {
            sendReportFromSequence();
        }



        /*
        uint8_t next_report_id = report[0] + 1u;

        if (next_report_id < REPORT_ID_COUNT)
        {
            send_hid_report(next_report_id, board_button_read());
        }*/
    }

    // Invoked when received GET_REPORT control request
    // Application must fill buffer report's content and return its length.
    // Return zero will cause the stack to STALL request
    uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, 
            hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen) {
        // TODO not Implemented
        (void) instance;
        (void) report_id;
        (void) report_type;
        (void) buffer;
        (void) reqlen;

        return 0;
    }

    // Invoked when received SET_REPORT control request or
    // received data on OUT endpoint ( Report ID = 0, Type = 0 )
    void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, 
            hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize) {
        (void) instance;

        if (report_type == HID_REPORT_TYPE_OUTPUT) {
            // Set keyboard LED e.g Capslock, Numlock etc...
            if (report_id == REPORT_ID_KEYBOARD) {
                // bufsize should be (at least) 1
                if ( bufsize < 1 ) return;

                onKbdLedRequest(buffer[0]);
            }
        }
    }


};

//todo handle tud_hid_set_protocol_cb to handle nkro
//and
