#include "keyboard_logic.h"

void onKeypress(KeyPosition pos, KeyState state) {
    KeyboardLogic::getInstance().onKeypress(pos, state);
}
void onKbdLedRequest(uint8_t state) {
    KeyboardLogic::getInstance().onKbdLedRequest(state);
}
void KeyboardStateMachine::forceRedraw() {
    Display::getInstance().forceRedraw();
}
// bool  KeyboardStateMachine::usbHidWorks() const {
//     return UsbHid::getInstance().usbHidWorks();
// }
