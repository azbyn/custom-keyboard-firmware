#pragma once

// #include <stdlib.h>
#include <utility> //std::forward
#include <stdio.h> //snprintf

#include <stdint.h>
#include <hardware/spi.h>
#include <hardware/gpio.h>
#include <pico/time.h>

#include <pico/multicore.h>

#include "font8x8_basic.h"
#include "pins.h"

class St7789 {
    bool dataMode = false;
public:
    static constexpr uint16_t width = 320;
    static constexpr uint16_t height = 172;

    void init() {
        // Init SPI
        spi_init(pins::disp::spiPtr, 40'000'000); // 40 MHz
        gpio_set_function(pins::disp::sda, GPIO_FUNC_SPI);
        gpio_set_function(pins::disp::scl, GPIO_FUNC_SPI);

        // Init control pins
        gpio_init(pins::disp::cs);
        gpio_set_dir(pins::disp::cs, GPIO_OUT);
        gpio_put(pins::disp::cs, 1);

        gpio_init(pins::disp::dc);
        gpio_set_dir(pins::disp::dc, GPIO_OUT);

        gpio_init(pins::disp::rst);
        gpio_set_dir(pins::disp::rst, GPIO_OUT);

    // Init display
    
    lcd_reset();

    lcd_write_cmd(0x01); // Software reset
    sleep_ms(150);

    lcd_write_cmd(0x11); // Sleep out
    sleep_ms(150);

    lcd_write_cmd(0x3A); // Color mode
    uint8_t color_mode = 0x55; // 16-bit color
    lcd_write_data(&color_mode, 1);

    lcd_write_cmd(0x36); // Memory access control
    constexpr uint8_t MADCTL_MY = 1 << 7;
    constexpr uint8_t MADCTL_MX = 1 << 6;
    constexpr uint8_t MADCTL_MV = 1 << 5;
    constexpr uint8_t MADCTL_ML = 1 << 4;
    constexpr uint8_t MADCTL_MH = 1 << 2;

    uint8_t madctl = MADCTL_MX| MADCTL_MV;// | (1<<3);
    lcd_write_data(&madctl, 1);

    // INVON (21h): Display Inversion On
    lcd_write_cmd(0x21);
    sleep_ms(10);

    // NORON (13h): Normal Display Mode On
    lcd_write_cmd(0x13);
    sleep_ms(10);


    lcd_write_cmd(0x29); // Display on
    sleep_ms(50);
}
/////////////////////

// Pin definitions
// #define PIN_MOSI pins::disp::sda
// #define PIN_SCK  pins::disp::scl
// #define PIN_CS   pins::disp::cs
// #define PIN_DC   pins::disp::dc
// #define PIN_RST  pins::disp::rst


// // Display resolution (adjust if needed)
// #define ST7789_WIDTH  320
// #define ST7789_HEIGHT 172

// --- Helper functions ---
private:
    void lcd_write_cmd(uint8_t cmd) {
        gpio_put(pins::disp::dc, 0);
        gpio_put(pins::disp::cs, 0);
        spi_write_blocking(pins::disp::spiPtr, &cmd, 1);
        gpio_put(pins::disp::cs, 1);
    }

    void lcd_write_data(const uint8_t *data, size_t len) {
        gpio_put(pins::disp::dc, 1);
        gpio_put(pins::disp::cs, 0);
        spi_write_blocking(pins::disp::spiPtr, data, len);
        gpio_put(pins::disp::cs, 1);
    }

    void lcd_reset() {
        gpio_put(pins::disp::rst, 0);
        sleep_ms(50);
        gpio_put(pins::disp::rst, 1);
        sleep_ms(50);
    }
public:


// Set drawing window
void set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    uint8_t data[4];

    lcd_write_cmd(0x2A); // Column addr set
    data[0] = x0 >> 8; data[1] = x0 & 0xFF;
    data[2] = x1 >> 8; data[3] = x1 & 0xFF;
    lcd_write_data(data, 4);

    lcd_write_cmd(0x2B); // Row addr set
    data[0] = y0 >> 8; data[1] = y0 & 0xFF;
    data[2] = y1 >> 8; data[3] = y1 & 0xFF;
    lcd_write_data(data, 4);

    lcd_write_cmd(0x2C); // Memory write
}

void begin_draw() {
    auto y_plus = (240- this->height)/2;
    //todo add (240-width)?
    set_window(0, y_plus, this->width - 1, this->height - 1+y_plus);

    gpio_put(pins::disp::dc, 1);
    gpio_put(pins::disp::cs, 0);
}
void put(uint16_t color) {
    uint8_t hi = color >> 8;
    uint8_t lo = color & 0xFF;

    uint8_t pixel[2] = {hi, lo};
            // if (i < 30) {
                // pixel[0] = hi;
                // pixel[1] = lo;
            // } else {
                // pixel[0] = 0xFF;
                // pixel[1] = 0xFF;
            // }
            // = {lo, hi};//, lo};
            // spi_write16_blocking(SPI_PORT, &color, 1);
    spi_write_blocking(pins::disp::spiPtr, pixel, 2);  
}
void end_draw() {
    gpio_put(pins::disp::cs, 1);
}

// Fill screen with a single color
void fill_color(uint16_t color) {

    uint8_t hi = color >> 8;
    uint8_t lo = color & 0xFF;

begin_draw();

    // Send lots of pixels
    for (int j = 0; j < this->height; j++) {
        for (int i = 0; i < this->width; i++) {

            // spi_write16_blocking(SPI_PORT, &color, 1);
            uint8_t pixel[2] = {hi, lo};
            // if (i < 30) {
                // pixel[0] = hi;
                // pixel[1] = lo;
            // } else {
                // pixel[0] = 0xFF;
                // pixel[1] = 0xFF;
            // }
            // = {lo, hi};//, lo};
            // spi_write16_blocking(SPI_PORT, &color, 1);
            spi_write_blocking(pins::disp::spiPtr, pixel, 2);
       }
    }

    end_draw();
}
};
//5 6 5 color
using Color = uint16_t;
namespace colors {
    constexpr Color rgb_to_565(uint8_t r, uint8_t g, uint8_t b) {
        return 
        (((r >> 3) << (5+6)) | // ignore least significant 3 bits to get 5
         ((g >> 2) << 5) |
         (b >> 3));
    }
    constexpr Color red = rgb_to_565(0xff, 0, 0);
    constexpr Color green = rgb_to_565(0, 0xff, 0);
    constexpr Color blue = rgb_to_565(0, 0, 0xff);
    constexpr Color white = 0xff'ff;
    constexpr Color black = 0x00'00;
};

class Display {
    St7789 lcd;

    static constexpr uint16_t width = St7789::width;
    static constexpr uint16_t height = St7789::height;

    static constexpr int scale = 2;

    static constexpr int char_w = 8;
    static constexpr int char_h = 8;

    static constexpr int min_border_x = 10;
    static constexpr int min_border_y = 10;

    
    static constexpr int buffer_w = (width - 2*min_border_x)/scale/char_w;
    static constexpr int buffer_h = (height - 2*min_border_y)/scale/char_h;

    static constexpr int border_x = (width-buffer_w*scale*char_w)/2;
    static constexpr int border_y = (height-buffer_h*scale*char_h)/2;
    
    //only write with the mutex locked
    char buffer[buffer_w*buffer_h] = {'H', 'i'};
    size_t cursor = 0;
    //init as 1 so we get an update after init
    int dirtiness = 1;

    mutex_t bufferMutex;

    Display() {}
public:
    static Display& getInstance() {
        static Display instance;
        return instance;
    }
    Display(Display&&) = default;
    Display(const Display&) = delete;
    Display& operator=(const Display&) = default;
    Display& operator=(Display&&) = delete;

    void init() {
        lcd.init();

        mutex_init(&bufferMutex);
    }
    void loop() {
        for (;;) {
            if (dirtiness > 0)
                draw();
            sleep_ms(1);
        }
    }
    
    void cls() {
        mutex_enter_blocking(&bufferMutex);
        for (int i = 0; i < buffer_w*buffer_h; ++i)
            buffer[i] = 0;
        cursor = 0;
        ++dirtiness;
        mutex_exit(&bufferMutex);
    }
    void print(const char* x) {
        mutex_enter_blocking(&bufferMutex);
        while (*x)
            putc(*x++);
        ++dirtiness;
        mutex_exit(&bufferMutex);
    }
    template <typename... Args>
    static void printf(const char* fmt, Args&&... args) {
        char buffer[256];
        snprintf(buffer, sizeof(buffer), fmt, std::forward<Args>(args)...);
        getInstance().print(buffer);
    }
    static void printf(const char* fmt) {
        getInstance().print(fmt);
    }
private:
    void draw() {
        char localBuffer[buffer_w*buffer_h];
        int prevDirtiness = dirtiness;

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
        {
            mutex_enter_blocking(&bufferMutex);
            //if no new updates happened while we were drawing, we can say we're done
            if (dirtiness == prevDirtiness)
                dirtiness = 0;
            mutex_exit(&bufferMutex);
        }
    }
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
private:
    void advance_line() {
        int i = 0;
        //move output one line higher;
        for (; i < buffer_w * (buffer_h-1); ++i)
            buffer[i] = buffer[i+buffer_w];
        for (; i < buffer_w * buffer_h; ++i)
            buffer[i] = 0;
    }
private:
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
            if (j >= buffer_h) return fg;
            auto px = x / scale % char_w;
            auto py = y / scale % char_h;

            char c = localBuffer[i+j*buffer_w];

            //unknown char - draw nothing
            if (c < 0x20 || c >= 0x7F)
                return bg;
            const uint8_t* p = font8x8_basic[c-0x20];
            if (p[py] & (1 << px))
                return fg;

            return bg;

        }
    }    
};
