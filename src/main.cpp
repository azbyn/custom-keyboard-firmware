// #include <stdio.h>

#include <pico/bootrom.h> //reset_usb_boot
#include <pico/stdlib.h>
#include <pico/time.h>
#include <hardware/watchdog.h>

#include "keyboard_matrix.h"
#include "display.hpp"
#include "usb_hid.h"
#include "keyboard_logic.h"

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
    int32_t kmTime = 0, hidTime = 0, klTime = 0;

    uint32_t secSinceBoot = 0;
    for (;;) {
        // uint32_t t0 = to_us_since_boot(get_absolute_time());
        keyboardMatrix.update();
        // uint32_t t1 = to_us_since_boot(get_absolute_time());
        hid.update();
        // uint32_t t2 = to_us_since_boot(get_absolute_time());
        keyboardLogic.update();
        // uint32_t t3 = to_us_since_boot(get_absolute_time());

        // kmTime  += t1 - t0;
        // hidTime += t2 - t1;
        // klTime  += t3 - t2;

        // uint32_t newSecSinceBoot = to_ms_since_boot(get_absolute_time()) / 1000;
        // if (newSecSinceBoot != secSinceBoot) {
        //     Display::printf("%d: k%d; h%d, l%d\n", 
        //         secSinceBoot, 
        //         us_to_ms(kmTime),
        //         us_to_ms(hidTime),
        //         us_to_ms(klTime)
        //     );
        //     kmTime = 0; hidTime = 0; klTime = 0;
        //     secSinceBoot = newSecSinceBoot;
        // }
    }
}
