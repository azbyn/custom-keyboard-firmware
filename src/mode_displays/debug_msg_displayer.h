#pragma once
#include <pico/multicore.h> // mutex
#include <string.h>

#include "display_common.h"



class DebugMsgDisplayer {
    static constexpr uint16_t width = St7789::width;
    static constexpr uint16_t height = St7789::height;

    static constexpr int scale = 2;

    static constexpr int min_border_x = 10;
    static constexpr int min_border_y = 10;

    
    static constexpr int buffer_w = (width - 2*min_border_x)/scale/char_w;
    static constexpr int buffer_h = (height - 2*min_border_y)/scale/char_h;

    static constexpr int border_x = (width-buffer_w*scale*char_w)/2;
    static constexpr int border_y = (height-buffer_h*scale*char_h)/2;
    
    //only write with the mutex locked
    char buffer[buffer_w*buffer_h] = {'H', 'i'};
    size_t cursor = 0;
public:
    //only callable with mutex locked
    void cls() {
        for (int i = 0; i < buffer_w*buffer_h; ++i)
            buffer[i] = 0;
        cursor = 0;
    }
    //DON'T draw with mutex closed, it closes it when needed itself
    void draw(St7789& lcd, mutex_t& bufferMutex) {
        char localBuffer[buffer_w*buffer_h];

        {
            mutex_enter_blocking(&bufferMutex);
            memcpy(localBuffer, buffer, sizeof(localBuffer));
            mutex_exit(&bufferMutex);
        }

        lcd.begin_draw();
        for (auto y = 0; y < height; ++y) {
            for (auto x = 0; x < width; ++x) {
                lcd.put(get_col(x, y, localBuffer));
            }
        }
        lcd.end_draw();
    }
    //call with mutex locked
    void print(const char* x) {
        while (*x)
            putc(*x++);
    }
private:

    void putc(char c) {
        if (cursor >= buffer_w * buffer_h - 1) {
            advance_line();
            cursor -= buffer_w;
        }
        if (c == '\t') {
            c = ' ';
        }
        else if (c == '\n') {
            cursor = ((cursor / buffer_w) + 1) * buffer_w;
            return;
        }

        buffer[cursor++] = c;
    }
    void advance_line() {
        int i = 0;
        //move output one line higher;
        for (; i < buffer_w * (buffer_h-1); ++i)
            buffer[i] = buffer[i+buffer_w];
        for (; i < buffer_w * buffer_h; ++i)
            buffer[i] = 0;
    }
    Color get_col(uint16_t x, uint16_t y, char* localBuffer) const {
        Color bg = colors::black;
        Color fg = colors::white;
        if (x < border_x || y < border_y || x >= (width-border_x) || y >= (height-border_y)) {
            return colors::red;
        } else {
            x -= border_x;
            y -= border_y;
            //which char are we on
            int i = x / scale / char_w;
            int j = y / scale / char_h;
            
            if (i >= buffer_w) return bg;
            if (j >= buffer_h) return bg;
            auto px = x / scale % char_w;
            auto py = y / scale % char_h;

            char c = localBuffer[i+j*buffer_w];
            return draw_char_get_col(px, py, c, bg, fg);
        }
    }
};