#pragma once
#include <string.h>
#include <array>

#include "display_common.h"

#include "settings_navigator.h"



class MenuModeDisplayer {
    static constexpr uint16_t width = St7789::width;
    static constexpr uint16_t height = St7789::height;

    static constexpr int scale = 2;

    static constexpr int min_border_x = 1;
    static constexpr int min_border_y = 10;

    
    static constexpr int buffer_w = (width - 2*min_border_x)/scale/char_w;
    static constexpr int buffer_h = (height - 2*min_border_y)/scale/char_h;

    static constexpr int border_x = (width-buffer_w*scale*char_w)/2;
    static constexpr int border_y = (height-buffer_h*scale*char_h)/2;

    
    
    //only write with the mutex locked
    // char buffer[buffer_w*buffer_h] = {'H', 'i'};
    // size_t cursor = 0;

public:

    void draw(St7789& lcd, const KeyboardStateSnapshot& ss, const SettingsNavigator& settings) {
        calculate(ss, settings);
        // char buffer[buffer_w*buffer_h];
        // char localBuffer[buffer_w*buffer_h];

        // {
            // mutex_enter_blocking(&bufferMutex);
        // memcpy(localBuffer, buffer, sizeof(localBuffer));
            // mutex_exit(&bufferMutex);
        // }

        lcd.begin_draw();
        for (auto y = 0; y < height; ++y) {
            for (auto x = 0; x < width; ++x) {
                lcd.put(get_col(x, y));
            }
        }
        lcd.end_draw();
    }
private:
    std::array<std::array<char, buffer_w+1>,  SettingsNavigator::settingItemsLength> buffer;
    int selectedIdx = 0;
    // char buffer[buffer_w][SettingsNavigator::settingItemsLength] = {};
    void calculate(const KeyboardStateSnapshot& ss, const SettingsNavigator& settings) {
        selectedIdx = settings.getCurrentSettingIdx();
        for (int i = 0; i < SettingsNavigator::settingItemsLength; ++i) {
            auto s = SettingsNavigator::settingItems[i];

            auto val = s->getValName(ss);
            auto valSz = strlen(val);

            buffer[i] = {};
            
            strncpy(buffer[i].data(), s->name, buffer_w+1);
            strncpy(buffer[i].data() + buffer_w-valSz, val, valSz+1);
        }
    }
    Color get_col(uint16_t x, uint16_t y) const {
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

            // if (j == SettingsNavigator::settingItemsLength) {
            //     auto px = x / scale % char_w;
            //     auto py = y / scale % char_h;
            //     return draw_char_get_col(px, py, 'A'+selectedIdx, bg, fg);
            // }

            if (j>= SettingsNavigator::settingItemsLength) return bg;
            
            if (i >= buffer_w) return bg;
            if (j >= buffer_h) return bg;
            auto px = x / scale % char_w;
            auto py = y / scale % char_h;

            char c = buffer[j][i];//+j*buffer_w];
            if (j == selectedIdx)
                return draw_char_get_col(px, py, c, fg, bg);
            else
                return draw_char_get_col(px, py, c, bg, fg);
        }
    }
};