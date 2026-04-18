#pragma once
#include <stdint.h>

template <uint16_t _width, uint16_t _height>
struct Image {
    static constexpr uint16_t width = _width;
    static constexpr uint16_t height = _height;
    const uint16_t data[_width * _height];

    constexpr uint16_t get(int x,  int y) const {
        return data[y*_width + x];
    }
};