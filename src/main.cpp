// #include <stdio.h>

#include <pico/bootrom.h> //reset_usb_boot
#include <pico/stdlib.h>
#include <pico/time.h>
#include <hardware/watchdog.h>

#include "keyboard_matrix.h"
#include "display.hpp"
#include "usb_hid.h"
#include "keyboard_logic.h"






// void onKeypress(KeyPosition pos, KeyState state) {
//     static char buffer[256];

//     snprintf(buffer, sizeof(buffer), "Key %d; %d - %d", pos.row, pos.col, state);
//     // printf("ON KP %s\n", buffer);
//     Display& display = Display::getInstance();
//     display.print(buffer);

//     // Display& display = Display::getInstance();
//     // display.print(buffer);
// }



int main() {
    auto& keyboardMatrix = KeyboardMatrix::getInstance();
    auto& display = Display::getInstance();
    auto& hid = UsbHid::getInstance();
    auto& keyboardLogic = KeyboardLogic::getInstance();


    stdio_init_all();

    display.init();
    hid.init();
    keyboardLogic.init();
    keyboardMatrix.init();


    multicore_launch_core1([] { Display::getInstance().loop();});

    for (;;) {
        keyboardMatrix.update();
        hid.update();
        keyboardLogic.update();
    }
}
