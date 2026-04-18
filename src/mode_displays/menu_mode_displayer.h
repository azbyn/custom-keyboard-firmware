#pragma once
#include <string.h>
#include <array>

#include "display_common.h"

#include "settings_navigator.h"



class MenuModeDisplayer {
    static constexpr uint16_t width = St7789::width;
    static constexpr uint16_t height = St7789::height;

    static constexpr int row_top_pad = 2;
    static constexpr int row_bot_pad = 2;

    static constexpr int row_h = char_h + row_top_pad + row_bot_pad;

    static constexpr int scale = 2;

    static constexpr int min_border_x = 1;
    static constexpr int min_border_y = 10;

    static constexpr int sz_x = (width - 2*min_border_x)/scale/char_w;
    static constexpr int sz_y = (height - 2*min_border_y)/scale/row_h;

    static constexpr int border_x = (width-sz_x*scale*char_w)/2;
    static constexpr int border_y = (height-sz_y*scale*row_h)/2;

    
    
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
    std::array<std::array<char, sz_x+1>,  SettingsNavigator::settingItemsLength> buffer;
    int selectedIdx = 0;
    int idxOffset = 0;
    // char buffer[buffer_w][SettingsNavigator::settingItemsLength] = {};
    int calculateIdxOffset(int prevSelectedIdx) {
        // auto prevSelectedIdx = selectedIdx;
        // selectedIdx = newSelectedIdx;

        if (SettingsNavigator::settingItemsLength < sz_y) {
            return 0;
        }


        //would be easier to define it in settingsNavigator, but i'd rather not have it know about 
        // the display & stuff

        //we wrapped to the bottom
        if (prevSelectedIdx == 0 && selectedIdx == SettingsNavigator::settingItemsLength - 1) {
            return SettingsNavigator::settingItemsLength - sz_y;
        }
        //wrapped to the top
        if (selectedIdx == 0 && prevSelectedIdx == SettingsNavigator::settingItemsLength - 1) {
            return 0;
        }

        //go lower
        if (selectedIdx - idxOffset >= sz_y) {
            return selectedIdx - sz_y + 1;
        }
        //go higher
        if (selectedIdx < idxOffset) {
            return selectedIdx;
        }

        return idxOffset;
    }
    void calculate(const KeyboardStateSnapshot& ss, const SettingsNavigator& settings) {
        auto prevSelectedIdx = selectedIdx;
        selectedIdx = settings.getCurrentSettingIdx();

        idxOffset = calculateIdxOffset(prevSelectedIdx);


        for (int i = 0; i < SettingsNavigator::settingItemsLength; ++i) {
            auto s = SettingsNavigator::settingItems[i];

            auto val = s->getValName(ss);
            auto valSz = strlen(val);

            buffer[i] = {};
            
            strncpy(buffer[i].data(), s->name, sz_x+1);
            strncpy(buffer[i].data() + sz_x-valSz, val, valSz+1);
        }
    }
    Color get_col(uint16_t x, uint16_t y) const {
        Color bg = colors::black;
        Color fg = colors::white;
        if (y < border_y || y >= (height-border_y)) {
            return bg;
        } else {
            x -= border_x;
            y -= border_y;
            //which char are we on
            int i = x / scale / char_w;
            int j = y / scale / row_h;// + idxOffset;

            if (j+idxOffset == selectedIdx) {
                std::swap(bg, fg);
            }

            // if (j == SettingsNavigator::settingItemsLength) {
            //     auto px = x / scale % char_w;
            //     auto py = y / scale % char_h;
            //     return draw_char_get_col(px, py, 'A'+selectedIdx, bg, fg);
            // }

            if (SettingsNavigator::settingItemsLength < sz_y)
                if (j >= SettingsNavigator::settingItemsLength) return bg;

            if (x < 0 || x >= width) return bg;

            if (i >= sz_x || j >= sz_y) return bg;
            
            int px = x / scale % char_w;
            int py = y / scale % row_h - row_top_pad;

            if (py < 0 || py >= char_h) return bg;

            char c = buffer[j+idxOffset][i];
            
            return draw_char_get_col(px, py, c, bg, fg);
        }
    }
};