#pragma once
#include <string.h>
#include <array>

// #include <format>//only to be used in static_assert
// #include <algorithm>//only to be used in static_assert
#include <string_view>//only to be used in constexpr stuff

#include "display_common.h"
#include "fixed_vector.h"

#include "key_layout_definitions.h"



namespace constexpr_helpers {
constexpr size_t constexpr_strlen(const char* x, size_t max) {
    if (!x) return 0;
    // return std::string_view(x).size();

    size_t i = 0;
    while (i < max && x[i]) ++i;
        
    return i;
};
// return false if nok
template <size_t sz>
constexpr std::array<char, sz> keyToStr(uint8_t modifiers, uint8_t key) {
    // std::array<char, sz> res;
    FixedVector<char, sz> res;

    bool ok = true;
    auto pushStr = [&ok, &res](std::string_view ptr) {
        if (!ok) return;
        if (!res.push_n(ptr.data(), ptr.size())) {
            ok = false;
        }
    };
    pushStr("Win");
    // sd::array<char, sz>
    if (modifiers & key_layout_definitions::ALT) {
        pushStr("Alt");
    }
    if (modifiers & key_layout_definitions::CTRL) {
        pushStr("Ctrl");
    }
     if (modifiers & key_layout_definitions::SHIFT) {
        pushStr("S");
    }
    
    pushStr("+");
    const char* keyStr = UsbHid::maybeKeyToStr(key);
    if (!keyStr) return std::array<char, sz> {'?', 0};
    pushStr(keyStr);
    
    //assume zero terminated
    if (!ok || res.size() >= sz-1) return std::array<char, sz> {'?', 0};

    return res.array();
}
};

class ShowBindingsModeDisplayer {
    static constexpr uint16_t width = St7789::width;
    static constexpr uint16_t height = St7789::height;

    static constexpr int scale = 2;

    static constexpr int min_border_x = 1;
    static constexpr int min_border_y = 10;

    
    static constexpr int sz_x = (width - 2*min_border_x)/scale/char_w;
    static constexpr int sz_y = (height - 2*min_border_y)/scale/char_h;

    static constexpr int border_x = (width-sz_x*scale*char_w)/2;
    static constexpr int border_y = (height-sz_y*scale*char_h)/2;

    static constexpr int maxActionCharLength = 6;
    static constexpr int maxKeyCharLength = sz_x-maxActionCharLength-1;

    struct KeyHelp {
        std::array<char, maxKeyCharLength+1> key;
        // char key[maxKeyCharLength+1];// = {};
        // const char* action;// = nullptr;
        std::array<char, maxActionCharLength+1> action;
    };
    // std::string_view action;
    // const char* key;
    
    static constexpr KeyHelp extraKeyHelps[] = {
        //hard(er) to remember unicode stuff
        {.key="Gr+y",    .action = "hun u"},
        {.key="Gr+h",    .action = "hun o"},
        
        {.key= "Ru+W",   .action="sh"},
        {.key= "RuGr+W", .action="soft"},

        {.key= "RuGr+R", .action="sht"},
        {.key= "RuGr+Y", .action="ukr yi"},
        {.key= "RuGr+A", .action="hard"},
        {.key= "RuGr+G", .action="ukr g"},
        {.key= "RuGr+J", .action="ukr i"},
        {.key= "RuGr+N", .action="soft"},
        //
        {.key="W+Wild",  .action = "Menu"},
    };
    static constexpr size_t extraKeyHelpsCount = sizeof(extraKeyHelps)/ sizeof(extraKeyHelps[0]);

    static constexpr size_t usedKeysCount = []() {
        size_t count = 0;
        for (const auto& keyArr : key_layout_definitions::winKeyActions)
            for (const auto& keyAct : keyArr)
                if (keyAct.help != nullptr) ++count;
        return count;
    }();

    static constexpr std::array<KeyHelp, usedKeysCount + extraKeyHelpsCount> keyHelps = []() {
        std::array<KeyHelp, usedKeysCount + extraKeyHelpsCount> res = {};
        size_t idx = 0;

        // for (int i = 0; i < )

        for (const auto& keyHelp : extraKeyHelps) {
            res[idx++] = keyHelp;
        //     ++idx;
        }
        
        for (uint8_t key = 0; key < key_layout_definitions::winKeyActions.size(); ++key) {
            auto arr = key_layout_definitions::winKeyActions[key];
        // for (const auto& keyArr : key_layout_definitions::winKeyActions) {
            for (uint8_t mod = 0; mod < arr.size(); ++mod) {
                const char* help = arr[mod].help;
                if (!help) continue;

                auto keyHelp = constexpr_helpers::keyToStr<maxKeyCharLength+1>(mod, key);

                int i = 0;
                while (help[i]) {
                    // if (i >= res[idx].action.size)
                    res[idx].action[i] = help[i];
                    ++i;
                }
                
                // res[idx].action = help;
                res[idx].key = keyHelp;
                // memcpy(res[idx].key.data(), keyHelp.data(), maxKeyCharLength+1);

                ++idx;
                //  = {.key=keyHelp, .action=help};

            }
        }
        //     for (const auto& keyAct : keyArr) {
        //         if (keyAct.help != nullptr) {
        //             ++count;
        //         }
        //     }
        // }
        return res;
    }();

    static constexpr size_t _maxActionHelpSize = []() {
        size_t max = 0;
        for (const auto& keyHelp : keyHelps) {
            // if (!keyHelp.action) continue;
            // auto sz = std::string_view(keyHelp.action).size();
            auto sz = constexpr_helpers::constexpr_strlen(keyHelp.action.data(), keyHelp.action.size());

            if (sz > max) max = sz;
        }
        return max;
    }();

  

    static constexpr size_t _maxKeyHelpSize = []() -> size_t {
        size_t max = 0;
        int i = 0;
        for (const auto& keyHelp : keyHelps) {
            ++i;
            if (keyHelp.key[0] == '?') return 420 + i*1000;
            auto sz = constexpr_helpers::constexpr_strlen(keyHelp.key.data(), keyHelp.key.size());// std::string_view(keyHelp.key).size();
            if (sz > max) max = sz;
        }
        return max;
    }();

    
    static_assert(_maxActionHelpSize <= maxActionCharLength);
    static_assert(_maxKeyHelpSize <= maxKeyCharLength);

private:
////
    int selectedIdx = 0;
    int idxOffset = 0;
    void move(int x) {
        int newIdx = selectedIdx + x;

        if (newIdx < 0) {
            newIdx = keyHelps.size() - 1;
            idxOffset = keyHelps.size() - sz_y;
        } else if (newIdx >= keyHelps.size()) {
            newIdx = 0;
            idxOffset = 0;
        }
        if (newIdx - idxOffset >= sz_y) {
            idxOffset = newIdx - sz_y + 1;
        } else if (newIdx < idxOffset) {
            idxOffset = newIdx;
        }
        
        selectedIdx = newIdx;
        // prevSelectedIdx();
    }
public:
    void down() { move(1); }
    void up() { move(-1); }

    void draw(St7789& lcd) {
        lcd.begin_draw();
        for (auto y = 0; y < height; ++y) {
            for (auto x = 0; x < width; ++x) {
                lcd.put(get_col(x, y));
            }
        }
        lcd.end_draw();
    }
private:
   
    // std::array<std::array<char, sz_x+1>,  SettingsNavigator::settingItemsLength> buffer;
    // int selectedIdx = 0;
    // // char buffer[buffer_w][SettingsNavigator::settingItemsLength] = {};
    // void calculate(const KeyboardStateSnapshot& ss, const SettingsNavigator& settings) {
    //     selectedIdx = settings.getCurrentSettingIdx();
    //     for (int i = 0; i < SettingsNavigator::settingItemsLength; ++i) {
    //         auto s = SettingsNavigator::settingItems[i];

    //         auto val = s->getValName(ss);
    //         auto valSz = strlen(val);

    //         buffer[i] = {};
            
    //         strncpy(buffer[i].data(), s->name, sz_x+1);
    //         strncpy(buffer[i].data() + sz_x-valSz, val, valSz+1);
    //     }
    // }
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
            int j = y / scale / char_h;

            if (j+idxOffset == selectedIdx) {
                std::swap(bg, fg);
            }
            // if (j == SettingsNavigator::settingItemsLength) {
            //     auto px = x / scale % char_w;
            //     auto py = y / scale % char_h;
            //     return draw_char_get_col(px, py, 'A'+selectedIdx, bg, fg);
            // }

            // if (j>= SettingsNavigator::settingItemsLength) return bg;

            if (x < 0 || x >= width) return bg;

            
            if (i >= sz_x) return bg;
            if (j >= sz_y) return bg;
            

            auto px = x / scale % char_w;
            auto py = y / scale % char_h;

            j += idxOffset;

            char c = ' ';
            if (i < maxKeyCharLength)
                c = keyHelps[j].key[i];
            else if (i == maxKeyCharLength) c = ' ';
            else c = keyHelps[j].action[i-maxKeyCharLength-1];

            return draw_char_get_col(px, py, c, bg, fg);
        }
    }
};