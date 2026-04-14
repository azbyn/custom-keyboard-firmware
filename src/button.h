#pragma once
#include <stdint.h>

#include "utils.h"


class Button {
    Pin pin;
    unsigned long next_valid_time = 0;

    bool lastState = 1;
public:
    Button(Pin pin): pin(pin) {}

    void init() {
        pinMode_input_pullup(pin);
    }

    Button(const Button&) = delete;
    Button(Button&&) = default;
    Button& operator=(const Button&) = delete;
    Button& operator=(Button&&) = default;


    bool isDown() {
        if (gpio_get(pin) == HIGH) {
            lastState = 1;
        } else {
            if (lastState == 1) {
                lastState = 0;
                auto t = millis();
                if (t > next_valid_time) {
                    next_valid_time = t + 200;
                    return true;
                }
            }
        }
        return false;
    }
};