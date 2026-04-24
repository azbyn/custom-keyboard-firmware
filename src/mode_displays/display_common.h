#pragma once

#include <stdint.h>

#include <hardware/spi.h>
#include <hardware/gpio.h>
#include <pico/time.h>

#include "pins.h"
#include "font8x8_basic.h"

class St7789 {
public:
    static constexpr uint16_t width = 320;
    static constexpr uint16_t height = 172;

    static constexpr uint8_t MADCTL_MY = 1 << 7;
    static constexpr uint8_t MADCTL_MX = 1 << 6;
    static constexpr uint8_t MADCTL_MV = 1 << 5;
    static constexpr uint8_t MADCTL_ML = 1 << 4;
    static constexpr uint8_t MADCTL_MH = 1 << 2;


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

        gpio_init(pins::disp::bl);
        gpio_set_dir(pins::disp::bl, GPIO_OUT);


        // Init display
        gpio_put(pins::disp::bl, true);

        lcd_reset();

        lcd_write_cmd(0x01); // Software reset
        sleep_ms(150);

        lcd_write_cmd(0x11); // Sleep out
        sleep_ms(150);

        lcd_write_cmd(0x3A); // Color mode
        uint8_t color_mode = 0x55; // 16-bit color
        lcd_write_data(&color_mode, 1);

        lcd_write_cmd(0x36); // Memory access control
      
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
        set_window(0, y_plus, this->width - 1, this->height - 1+y_plus);

        gpio_put(pins::disp::dc, 1);
        gpio_put(pins::disp::cs, 0);
    }
    void put(uint16_t color) {
        uint8_t hi = color >> 8;
        uint8_t lo = color & 0xFF;

        uint8_t pixel[2] = {hi, lo};
        spi_write_blocking(pins::disp::spiPtr, pixel, 2);  
    }
    void end_draw() {
        gpio_put(pins::disp::cs, 1);
    }

    void fill_color(uint16_t color) {

        uint8_t hi = color >> 8;
        uint8_t lo = color & 0xFF;

        begin_draw();

        for (int j = 0; j < this->height; j++) {
            for (int i = 0; i < this->width; i++) {
                uint8_t pixel[2] = {hi, lo};
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

static constexpr int char_w = 8;
static constexpr int char_h = 8;

static Color draw_char_get_col(uint16_t px, uint16_t py, 
                          char c,
                          Color bg, Color fg) {
    //shouldn happen, but to be safe
    if (px > char_w) return bg;
    if (py > char_h) return bg;

    //unknown char - draw nothing
    if (c < 0x20 || c >= 0x7F)
        return bg;
    const uint8_t* p = font8x8_basic[c-0x20];
    if (p[py] & (1 << px))
        return fg;

    return bg;
}
    