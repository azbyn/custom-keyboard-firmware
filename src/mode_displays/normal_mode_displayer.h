#pragma once
#include <stdio.h> //snprintf

#include "keyboard_state_machine.h"

#include "display_common.h"

#include "img_cheeki_breeki.h"
#include "img_cheeki_breeki_open.h"
#include "img_compose.h"

// #include "img_fast_type.h"

//not doing this has faster uploads
// #define USE_BACKGROUND 

#ifdef USE_BACKGROUND
#include "img_bg.h"
#endif

class NormalModeDisplayer {
    static constexpr uint16_t lcd_width = St7789::width;
    static constexpr uint16_t lcd_height = St7789::height;

#ifdef USE_BACKGROUND
    static_assert(img_bg.width == lcd_width);
    static_assert(img_bg.height == lcd_height);
#endif

    static constexpr int border_y = 5;

    //use this as a special "alpha" color = show the background beneath
    static constexpr Color alpha = 0x0000;
    static constexpr Color notblack = 1;

public:
    //no buffers here, we're "racing the beam" - yes ik that doesn't apply here

    void draw(St7789& lcd, const KeyboardStateSnapshot& snapshot) {
        topTimeDisplayer.calculateStuff(snapshot.secSinceBoot);
        bottomStateDisplayer.calculateStuff(snapshot);
        middleDisplayer.calculateStuff(snapshot);
        lcd.begin_draw();
        for (auto y = 0; y < lcd_height; ++y) {
            for (auto x = 0; x < lcd_width; ++x) {
                lcd.put(get_col(x, y, snapshot));
            }
        }
        lcd.end_draw();
    }
private:
    struct TopTimeDisplayer {
        static constexpr int border_x = 15;

        static constexpr int scale = 2;
        static constexpr const char backupTxt[] = "999 99:99:99";


        static constexpr int height = scale*char_h;

        static constexpr int leftPad = lcd_width - (2*border_x + char_w*scale*(sizeof(backupTxt)-1));
        static_assert(leftPad > 0);
        
        //yeah, i lied, i meant no pixel buffers
        char timeBuff[sizeof(backupTxt)];

        void calculateStuff(uint32_t timeS) {
            
            // int s = timeS % 60;
            int m_full = timeS / 60;
            // int m = m_full % 60;
            int h_full = m_full / 60;
            int d_full = h_full / 24;
            if (d_full < 1000) {
                // if you keep this running for more than 3 years i don't care that the 
                // display doesn't show the uptime correctly
                snprintf(timeBuff,sizeof(timeBuff),
                    "% 3d %02d:%02d:%02d",

                    d_full,
                    h_full % 24,
                    m_full % 60,
                    timeS % 60
                );
            } else {
                strncpy(timeBuff, backupTxt, sizeof(timeBuff));
            }
        }
        Color get_col(uint16_t x, uint16_t y) {
            
            // Color bg = colors::black;
            Color fg = colors::white;
            if (x < border_x || x >= (lcd_width-border_x)) {
                return alpha;
            }
            x -= border_x;

            if (x < leftPad) return alpha;
            x -= leftPad;
            //which char are we on
            int i = x / scale / char_w;
            // int j = y / scale / char_h;
            
            // if (i >= buffer_w) return bg;
            // if (j >= buffer_h) return bg;
            auto px = x / scale % char_w;
            auto py = y / scale % char_h;

            char c = timeBuff[i];
            return draw_char_get_col(px, py, c, alpha, fg);

            // return colors::red;
        }

    } topTimeDisplayer;

    //todo make some icons
    struct BottomStateDisplayer {
        static constexpr int border_x = 15;
        static constexpr int scale = 2;
        static constexpr int height = scale*char_h;

        static constexpr const char templateTxt[] = "1 C S P * W";

        //center
        static constexpr int startX = (lcd_width - (2*border_x + char_w*scale*(sizeof(templateTxt)-1))) /2;
        static constexpr int endX = startX + char_w*scale*(sizeof(templateTxt)-1);
        static_assert(startX > 0);

        // how much space we have
        char buffer[sizeof(templateTxt)];

        void calculateStuff(const KeyboardStateSnapshot& snapshot) {
            //num, caps, scroll, ruski, compose, unicode mode

            char unicodeModeChr = '?';

            static_assert(UIM__Size == 3);
            // constexpr auto unicodeModeToChr
            switch (snapshot.unicodeInputMode) {
                case UIM_WinCompose: unicodeModeChr = 'W'; break;
                case UIM_WinNumpad: unicodeModeChr = 'N'; break;
                case UIM_Linux: unicodeModeChr = 'L'; break;
            }

            snprintf(buffer, sizeof(buffer),
                "%c %c %c %c %c %c",
                (snapshot.numLock? '1' : '_'),
                (snapshot.capsLock? 'A' : 'a'),
                (snapshot.scrollLock? 'S' : '_'),
                (snapshot.ruskiMode? 'P' : '_'),//should be cyrillic R
                (snapshot.composeMode? '*' : '_'),
                unicodeModeChr
            );
            
        }
        Color get_col(uint16_t x, uint16_t y) {
            constexpr Color bg = colors::black;
            constexpr Color fg = colors::white;

            if (x < startX || x >= endX) { return bg; }
            x -= startX;
            //which char are we on
            int i = x / scale / char_w;
            // int j = y / scale / char_h;
            
            // if (i >= buffer_w) return bg;
            // if (j >= buffer_h) return bg;
            auto px = x / scale % char_w;
            auto py = y / scale % char_h;

            char c = buffer[i];
            return draw_char_get_col(px, py, c, bg, fg);

            // return colors::red;
        }

    } bottomStateDisplayer;

    struct MiddleDisplayer {
        static constexpr int height = lcd_height - (
            border_y *2 + TopTimeDisplayer::height + BottomStateDisplayer::height);
        static constexpr int width = lcd_width;// - border_x*2;

        bool ruskiMode, composeMode;
        const Image<img_cheeki_breeki.width, img_cheeki_breeki.height>* ruskiImgPtr;
        void calculateStuff(const KeyboardStateSnapshot& snapshot) {
            //nothing to calculate, just store state;
            this->ruskiMode = snapshot.ruskiMode;
            this->composeMode = snapshot.composeMode;
            ruskiImgPtr = snapshot.keyPressed? &img_cheeki_breeki_open : &img_cheeki_breeki;
        }
        
//centered in the left half
        static constexpr int cheeki_start_y = (height- img_cheeki_breeki.height) /2;
        static constexpr int cheeki_start_x = (width/2- img_cheeki_breeki.width) /2;

        static constexpr int cheeki_end_y = cheeki_start_y + img_cheeki_breeki.height;
        static constexpr int cheeki_end_x = cheeki_start_x + img_cheeki_breeki.width;
        
        static_assert(cheeki_start_y >= 0);
        static_assert(cheeki_start_x >= 0);
        static_assert(cheeki_end_x < width/2);
        static_assert(cheeki_end_y < height);

        static_assert(img_cheeki_breeki.height == img_cheeki_breeki_open.height);
        static_assert(img_cheeki_breeki.width == img_cheeki_breeki_open.width);


        //right half
        static constexpr int compose_start_y = (height- img_compose.height) /2;
        static constexpr int compose_start_x = (width/2- img_compose.width) /2 + width/2;

        static constexpr int compose_end_y = compose_start_y + img_compose.height;
        static constexpr int compose_end_x = compose_start_x + img_compose.width;
        
        static_assert(compose_start_y >= 0);
        static_assert(compose_start_x >= width/2);
        static_assert(compose_end_x < width);
        static_assert(compose_end_y < height);

        // static_assert(img_fast_type::height == height);
        // static_assert(img_fast_type::width == width);

        
        Color get_col(uint16_t x, uint16_t y) {
            if (x < width / 2) {
                if (!ruskiMode) return alpha;
                if (x < cheeki_start_x || y < cheeki_start_y || x >= cheeki_end_x || y >= cheeki_end_y)
                    return alpha;
                x -= cheeki_start_x;
                y -= cheeki_start_y;
                return ruskiImgPtr->get(x, y);
            } 

            if (!composeMode) return alpha;
            
            if (x < compose_start_x || y < compose_start_y || x >= compose_end_x || y >= compose_end_y)
                return alpha;
            x -= compose_start_x;
            y -= compose_start_y;
            return img_compose.get(x, y);
        }
    } middleDisplayer;
    
private:
    Color get_col(uint16_t x, uint16_t y, const KeyboardStateSnapshot& snapshot) {
        #ifdef USE_BACKGROUND
        auto bg_color = snapshot.showBg? img_bg.get(x, y) : colors::black;
        #else
        auto bg_color = colors::black;
        #endif
        if (y < border_y || y >= (lcd_height-border_y)) {
            return bg_color;
        }
        auto bg_or_col = [&] (auto col) {
            if (!col) return bg_color;
            return col;
        };
        // x -= border_x;
        y -= border_y;
        // we're drawing the top bar
        if (y < topTimeDisplayer.height) {
            return bg_or_col(topTimeDisplayer.get_col(x, y));
        }
        y -= topTimeDisplayer.height;
        if (y < middleDisplayer.height) { 
            return bg_or_col(middleDisplayer.get_col(x, y));
        }
        y -= middleDisplayer.height;
        //drawing bottom bar
        return bg_or_col(bottomStateDisplayer.get_col(x, y));
        
        // return colors::black;
    }
    
};
