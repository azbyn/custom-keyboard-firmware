#pragma once

#include <stdint.h>
#include <pico/stdlib.h>
#include <bsp/board_api.h>


#include <tusb.h>
#include "tusb_config.h"

#include "fixed_vector.h"
#include "bit_array.h"

#include "utils.h"
#include "keyboard_state_machine.h"

constexpr uint8_t BOARD_TUD_RHPORT = 0;
constexpr uint8_t REPORT_ID_KEYBOARD = 1;
constexpr uint8_t REPORT_ID_CONSUMER_CONTROL = 2;

#define HID_KEY_WILDCARD HID_KEY_F13

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

#define MAX_KEY_BITMAP 0x63
// constexpr size_t MAX_KEY_BITMAP = 0x63;// 0x6F;
// 0x63;
//0xDF;
/*0x82*/

struct NkroReport {
    uint8_t modifiers;
    uint8_t reserved;
    BitArray<MAX_KEY_BITMAP> keys;

    constexpr void addKeyToKeyreport(uint8_t keycode) {
        if (keycode == 0) return;
        this->keys.set(keycode, true);
    }

    constexpr void removeKeyFromKeyreport(uint8_t keycode) {
        if (keycode == 0) return;
        this->keys.set(keycode, false);
    }
};
static_assert(sizeof(NkroReport) == decltype(NkroReport::keys)::sizeBytes()+2);

//a singleton class is used here because i don't have to forward declare everything
class UsbHid {
    FixedVector<KeyWithModifier, 512/*256*/> sequence_buffer;
    NkroReport keyReport;

    bool sendingConsumerKey = false;
    //uint8_t keys[KeyReportLength];
    
    // bool suspended = false;


    friend void tud_suspend_cb(bool remote_wakeup_en);
    friend void tud_resume_cb(void);
    friend void tud_mount_cb();
    friend void tud_umount_cb();


    UsbHid() {}

public:
    static constexpr size_t keysNumBytes = decltype(NkroReport::keys)::sizeBytes();
    static constexpr size_t keysNumKeys = decltype(NkroReport::keys)::sizeBits();

    static UsbHid& getInstance(){
        static UsbHid instance;
        return instance;
    }
    UsbHid(const UsbHid&) = delete;
    UsbHid& operator=(const UsbHid&) = delete;

    RepeatReportType getRepeatReportType() const;


private:
    enum MountState {
        S_Off,
        S_On,
        S_Unmounted,
        S_SuspendedWakeable,
        S_SuspendedUnwakeable,
    } mountState = S_Off;
    bool usbHidWorks() const {
        return mountState == S_On || mountState== S_SuspendedWakeable;
        // return tud_mounted() && tud_hid_ready(); 
    }
public:
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
            this->keyReport.modifiers |= modifier;
        else
            this->keyReport.modifiers &= ~modifier;
        if (_sendReport)
            sendReport(false);
    }
    constexpr uint8_t getModifiers() const { return keyReport.modifiers; }
    constexpr bool getShiftState() const { 
        return getModifiers() & (KEYBOARD_MODIFIER_RIGHTSHIFT | KEYBOARD_MODIFIER_LEFTSHIFT); 
    }
    constexpr bool getAltState() const {
        return getModifiers() & (KEYBOARD_MODIFIER_RIGHTALT | KEYBOARD_MODIFIER_LEFTALT);
    }
    constexpr bool getCtrlState() const {
        return getModifiers() & (KEYBOARD_MODIFIER_RIGHTCTRL | KEYBOARD_MODIFIER_LEFTCTRL);
    }
    constexpr bool getGuiState() const {
        return getModifiers() & (KEYBOARD_MODIFIER_RIGHTGUI | KEYBOARD_MODIFIER_LEFTGUI);
    }
    
    void press(uint8_t key) {
        keyReport.addKeyToKeyreport(key);
        sendReport(false);
    }
    void release(uint8_t key) {
        keyReport.removeKeyFromKeyreport(key);

        sendReport(false);
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
        if (sequence_buffer.empty())
            sendReportFromSequence();
            
        return true;
    }
private:
    // template <bool useKeypad>
    uint8_t hexDigitToKey(char c, bool useKeypad) {
        //no real error handling, we just call it from below where we know we only get 0-9, A-F
        if (c >= '1' && c <= '9') {
            uint8_t key1 = useKeypad? HID_KEY_KEYPAD_1 : HID_KEY_1;
            return uint8_t(key1-1+c-'0');
        }

        if (c== '0') {
            return useKeypad? HID_KEY_KEYPAD_0: HID_KEY_0;
        }
        if (c >= 'A' && c <= 'F') {
            return HID_KEY_A + (c-'A');
        }
        return 0;
    }
public:
    constexpr static bool isNkro() { return KeyboardStateMachine::getStateSnapshot().nkro; }
    static void setNkro(bool val) {
        auto& mss = KeyboardStateMachine::getMutableStateSnapshot();
        if (mss.nkro == val) return;
        mss.nkro = val;

        tud_disconnect();
        sleep_ms(100);
        tud_connect();

        display_printf("NKRO_%d;", val);
    } 
public:
    bool sendUnicodeSequence(char16_t val, UnicodeInputMode mode) {
        // display_printf1("U+%04x;%d\n", val, sequence_buffer.size());
        //unicode is 21 bits -> 0x10FFFF max -> 7 digits
        //32 bits -> 10 digits
        // if (val > 0x10FFFF) return false;
        char buff[16];

        //4*2 for the keys and 1 extra to begin unicode input
        FixedVector<KeyWithModifier, 16> keys;
        //KeyWithModifier keys[16];

        snprintf(buff, sizeof(buff), "%04X", val);

        switch (mode) {
            case UIM_WinCompose:
                keys.push_back({KEYBOARD_MODIFIER_RIGHTALT, 0});
                keys.push_back({0, HID_KEY_U});

                //yes i could refactor this bit, but i think it's less readable
                 for (const char* p = buff; *p; ++p) {
                    char c = *p;
                    uint8_t key = hexDigitToKey(c, /*useKeypad*/ false);
                    keys.push_back({0, key});
                    if (*(p+1) == c)
                        keys.push_back({0, 0});
                }
            
                keys.push_back({0, HID_KEY_ENTER});
                break;
            case UIM_WinNumpad: {
                keys.push_back({KEYBOARD_MODIFIER_RIGHTALT, HID_KEY_KEYPAD_ADD});

                for (const char* p = buff; *p; ++p) {
                    char c = *p;
                    uint8_t key = hexDigitToKey(c, /*useKeypad*/ true);
                    keys.push_back({KEYBOARD_MODIFIER_RIGHTALT, key});
                    if (*(p+1) == c)
                        keys.push_back({KEYBOARD_MODIFIER_RIGHTALT, 0});
                }
            } break;
            case UIM_Linux: {
                keys.push_back({KEYBOARD_MODIFIER_LEFTCTRL|KEYBOARD_MODIFIER_LEFTSHIFT, HID_KEY_U});
                
                for (const char* p = buff; *p; ++p) {
                    char c = *p;
                    uint8_t key = hexDigitToKey(c, /*useKeypad*/ false);
                    keys.push_back({0, key});
                    if (*(p+1) == c)
                        keys.push_back({0, 0});
                }

                keys.push_back({0, HID_KEY_ENTER});
            } break;

            case UIM__Size: break; //never should reach here
        }
        
        keys.push_back({0, 0});
        return sendSequence(keys.begin(), keys.size());
    }

    //for debuggin reasons
    size_t seq_buffer_size() const {
        return sequence_buffer.size();
    }
    
private:
    uint32_t lastSendSeqMs = 0;

    void sendReportFromSequence() {
        lastSendSeqMs = millis();
        
        if (sequence_buffer.empty()) return;
    
        auto x = sequence_buffer.pop_front();

        if (isNkro()) {
            NkroReport rep;
            rep.modifiers = x.modifiers;
            rep.keys.set(x.key, true);
            sendReport_nkro(rep, false);
            return;
        } 


        // Display::printf("S%d-", sequence_buffer.size());
        #if 0
        display_printf("[%d-", sequence_buffer.size());

        printKey(x);
        if (sequence_buffer.empty()) {
            display_print("\n");
        } else {
            display_print(";");
        }
        #endif

        uint8_t keys[6] = {};

        keys[0] = x.key;

        sendReport_6k(x.modifiers, keys, false);
    }
    int numSentRepeatedReport = 0;
    void sendReport_nkro(const NkroReport& report, bool isRepeatedReport) {    
        if (isRepeatedReport) {
            ++this->numSentRepeatedReport;
        } else {
            numSentRepeatedReport = 0;
        }
        if (!isRepeatedReport) {
            display_printf(
                "(%s%s%s%s;",// ;%d]"
                
                // "%02x %02x %02x %02x %02x %02x\n",
                (getCtrlState()  ? "c": ""),
                (getShiftState() ? "S": ""),
                (getAltState()   ? "a": ""),
                (getGuiState()   ? "g": "")

                // keys[0], keys[1], keys[2],
                // keys[3], keys[4], keys[5]
            );
        }
        // for (size_t i = 0; i < report.keys.sizeBits(); ++i) {
        //     if (report.keys.get(i))
        //         printKey(i);
        // }
        const uint8_t* x = reinterpret_cast<const uint8_t*>(&report);

        cdc_print("[");
        for (size_t i = 0; i < sizeof(report); ++i) {
            cdc_printf("%02x", x[i]);
            if (i % 4 == 3) 
                cdc_print(" ");
            // if (report.keys.get(i))
        }
        cdc_print("]\r\n");


        bool ok = usbHidWorks() || KeyboardStateMachine::getStateSnapshot().sendCodesWhenKbdUnmounted;
        if (isRepeatedReport && !ok) {
            display_printf("SEND_REP_W_OFF");
        }
        if (!isRepeatedReport)
            display_printf(";%d", ok);
        if (!ok) return;

        tud_hid_report(/*report_id*/REPORT_ID_KEYBOARD, &report, sizeof(report));
    }
    void sendReport_6k(uint8_t modifiers, const uint8_t* keys, bool isRepeatedReport) {    
        if (isRepeatedReport) {
            ++this->numSentRepeatedReport;
        } else {
            numSentRepeatedReport = 0;
        }
        if (!isRepeatedReport) {
            display_printf(
                "(%s%s%s%s;",// ;%d]"
                // "%02x %02x %02x %02x %02x %02x\n",
                ((modifiers & (KEYBOARD_MODIFIER_LEFTCTRL | KEYBOARD_MODIFIER_RIGHTCTRL)) ? "c": ""),
                ((modifiers & (KEYBOARD_MODIFIER_LEFTSHIFT | KEYBOARD_MODIFIER_RIGHTSHIFT)) ? "S": ""),
                ((modifiers & (KEYBOARD_MODIFIER_LEFTALT | KEYBOARD_MODIFIER_RIGHTALT)) ? "a": ""),
                ((modifiers & (KEYBOARD_MODIFIER_LEFTGUI | KEYBOARD_MODIFIER_RIGHTGUI)) ? "g": "")

                // keys[0], keys[1], keys[2],
                // keys[3], keys[4], keys[5]
            );
        }
        for (int i = 0; i<6;++i)
            if (keys[i])
                printKey(keys[i]);

        bool ok = usbHidWorks() || KeyboardStateMachine::getStateSnapshot().sendCodesWhenKbdUnmounted;
        if (isRepeatedReport && !ok) {
            display_printf("SEND_REP_W_OFF");
        }
        if (!isRepeatedReport)
            display_printf(";%d", ok);
        if (!ok) return;
        
        tud_hid_keyboard_report(REPORT_ID_KEYBOARD, modifiers, keys);
    }
public:
    static void printKey(KeyWithModifier k) {
        if (k.modifiers & (KEYBOARD_MODIFIER_RIGHTGUI|KEYBOARD_MODIFIER_LEFTGUI))
            display_print("W");
        if (k.modifiers & (KEYBOARD_MODIFIER_RIGHTALT|KEYBOARD_MODIFIER_LEFTALT))
            display_print("A");
        if (k.modifiers & (KEYBOARD_MODIFIER_RIGHTCTRL|KEYBOARD_MODIFIER_LEFTCTRL))
            display_print("C");
        if (k.modifiers & (KEYBOARD_MODIFIER_RIGHTSHIFT|KEYBOARD_MODIFIER_LEFTSHIFT))
            display_print("S");

        printKey(k.key);
    }

    // static void keyToStr(char* dst, size_t sz, uint8_t key) {
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

        default: return nullptr;

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

    static void printKey(uint8_t key) {
        const char* maybeKey = maybeKeyToStr(key);
        if (!maybeKey)
            display_printf("%02x", key);
        else   
            display_print(maybeKey);
        // if (buffer) 
    }
public:
    void sendReport(bool isRepeatedReport) {
        
        if (isNkro()) {
            return this->sendReport_nkro(this->keyReport, isRepeatedReport);
        }
        
        FixedVector<uint8_t, 6> keysReport = {};
        for (size_t i = 0; i < this->keyReport.keys.actualCapacity(); ++i)  {
            if (!this->keyReport.keys.get(i)) continue;
            bool hasSpace = keysReport.push_back(i);
            if (!hasSpace) {
                for (size_t j = 0; j < keysReport.size(); ++j) {
                    keysReport[j] = HID_KEY_ERR_ROLLOVER;
                }
                break;
            }
        }

       
        this->sendReport_6k(this->getModifiers(), keysReport.begin(), isRepeatedReport);
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
            if (sendingConsumerKey) {
                releaseMedia();
                return;
            }
        } else if (report[0] == REPORT_ID_KEYBOARD) {
            if (sequence_buffer.empty()) {
                auto rrt = getRepeatReportType();
                switch (rrt) {
                    case RRT__Size: break;//shouldn't happen
                    case RRT_No: break;//don't send the report again
                    case RRT_One:
                        if (!numSentRepeatedReport)
                            sendReport(true);  
                        break;
                    case RRT_Five:
                        if (numSentRepeatedReport < 5)
                            sendReport(true);
                        break;
                    case RRT_Infinite: 
                        sendReport(true);  
                        break;
                }
                return;
            }

            sendReportFromSequence();
        }
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

