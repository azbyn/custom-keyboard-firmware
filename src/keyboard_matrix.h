#pragma once

#include "pins.h"
#include "utils.h"



enum KeyState {
    PRESSED = 1,
    RELEASED = 0,
};
struct KeyPosition {
    int col, row;
    constexpr bool operator==(const KeyPosition& rhs) {
        return this->row == rhs.row && this->col == rhs.col;
    }
    constexpr bool operator!=(const KeyPosition& rhs) {
        return this->row != rhs.row || this->col != rhs.col;
    }
};

void onKeypress(KeyPosition pos, KeyState state);

class KeyboardMatrix {
private:
    bool prevState[pins::rowLen * pins::colLen] = {};
    KeyboardMatrix() {}

public:
    static KeyboardMatrix& getInstance() {
        static KeyboardMatrix instance;
        return instance;
    }
    KeyboardMatrix(const KeyboardMatrix&) = delete;
    KeyboardMatrix& operator=(const KeyboardMatrix&) = delete;

// #define HIGH_LOGIC
    void init() {
        #ifndef HIGH_LOGIC
        for (size_t r = 0; r < pins::rowLen; ++r) {
            pinMode_input_pulldown(pins::rowPins[r]);

            // pinMode_output(r);
            // gpio_put(r, HIGH);
        }
        for (size_t c = 0; c < pins::colLen; ++c) {
            auto cp = pins::colPins[c];
        // for (auto c : pins::colPins) {
            pinMode_output(cp);
            gpio_put(cp, LOW);
        }
        #else
        for (auto r : pins::rowPins) {
            pinMode_output(r);
            gpio_put(r, HIGH);
        }
        for (auto c : pins::colPins) {
            pinMode_input_pullup(c);
        }
        #endif
    }
    void print_state(bool arr[]) {
        return;
        // // Serial.printf("%6d; %6d:", board_micros(), delta_n);
        // // printf("%9d;", board_micros());
        // for (size_t r = 0; r < pins::rowLen; ++r) {
        //     for (size_t c = 0; c < pins::colLen; ++c) {
        //         printf("%c", arr[c * pins::rowLen + r] ? 'x': '_');
        //     }
        //     printf("|");
        // }
        // printf("\n");
    }
    void update() {
        using pins::rowLen, pins::colLen;
        bool newState[pins::rowLen * pins::colLen] = {};

        //stolen from the Keypad.h library
        // https://github.com/Chris--A/Keypad/blob/master/src/Keypad.h

        // https://www.baldengineer.com/arduino-keyboard-matrix-tutorial.html
        #ifndef HIGH_LOGIC
        for (size_t c = 0; c < pins::colLen; ++c) {
            auto colPin = pins::colPins[c];

            pinMode_output(colPin);
            gpio_put(colPin, HIGH); // Begin column pulse output.
            sleep_us(10);

            for (size_t r = 0; r < pins::rowLen; ++r) {
                auto rowPin = pins::rowPins[r];
                // #pragma GCC diagnostic push
                // #pragma GCC diagnostic ignored "-Waggressive-loop-optimizations"
                newState[c * pins::rowLen + r] = gpio_get(rowPin);
                // #pragma GCC diagnostic pop

            }
            gpio_put(colPin, LOW);
            pinMode_input_pulldown(colPin);//?

            // sleep_us(10);

        }

        #else
        for (int r = 0; r < pins::rowLen; ++r) {
            auto rowPin = pins::rowPins[r];
            
            pinMode_output(rowPin);
            gpio_put(rowPin, LOW); // Begin column pulse output.
            sleep_us(30);


            // gpio_put(colPin, HIGH);
            // pinMode_input(colPin);


            for (int c = 0; c < pins::colLen; ++c) {
            // for (size_t r = 0; r < pins::rowLen; ++r) {
                // #pragma GCC diagnostic push
                // #pragma GCC diagnostic ignored "-Waggressive-loop-optimizations"

                auto colPin = pins::colPins[r];
                // pinMode_input_pullup(rowPin);
                newState[c * pins::rowLen + r] = !gpio_get(colPin);
                // pinMode_input(rowPin);
                // #pragma GCC diagnostic pop
            }
            gpio_put(rowPin, HIGH);
            pinMode_input_pullup(rowPin);

            // Set pin to high impedance input. Effectively ends column pulse.
            // gpio_put(colPin, HIGH);
            // pinMode_input(colPin);
            // gpio_put(colPin, LOW); // Begin column pulse output.
        }
        #endif


        //check what changed
        bool has_update = false;
        for (int c = 0; c < colLen; ++c) {
            for (int r = 0; r < rowLen; ++r) {
                auto i = c * rowLen + r;
                auto prev = prevState[i];
                auto now  = newState[i];


                if (prev != now) {
                    has_update = true;
                    onKeypress({.col=c, .row=r}, now ? PRESSED : RELEASED);
                }
                prevState[i] = newState[i];
            }
        }
        if (has_update) {
           print_state(newState);
        }
    }


  
};
