#pragma once
#include <pico/stdlib.h>
#include <hardware/gpio.h>

#include "pins.h"

constexpr bool HIGH = true;
constexpr bool LOW = false;



inline void pinMode_input_pulldown(Pin ulPin) {
    gpio_init(ulPin);
    gpio_set_dir(ulPin, false);
    gpio_pull_down(ulPin);
    gpio_put(ulPin, true);
}
inline void pinMode_input_pullup(Pin ulPin) {
    gpio_init(ulPin);
    gpio_set_dir(ulPin, false);
    gpio_pull_up(ulPin);
    gpio_put(ulPin, false);
}
inline void pinMode_input(Pin ulPin) {
    gpio_init(ulPin);
    gpio_set_dir(ulPin, false);
    gpio_disable_pulls(ulPin);
    //gpio_pull_up(ulPin);
    //gpio_put(ulPin, false);
}
inline void pinMode_output(Pin ulPin) {
    gpio_init(ulPin);
    gpio_set_drive_strength(ulPin, GPIO_DRIVE_STRENGTH_4MA);
    gpio_set_dir(ulPin, true);
}
inline void digitalWrite(Pin ulPin, bool value) {
    gpio_put(ulPin, value);
}
inline uint32_t millis() {
    return to_ms_since_boot(get_absolute_time());
}
// inline bool digitalRead(Pin ulPin) {
//     return gpio_get();
// }