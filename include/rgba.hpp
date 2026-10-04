#pragma once

#include <cstdint>

struct RGBA {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;

    RGBA() : r(0), g(0), b(0), a(255) {}
    RGBA(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha = 255)
        : r(red), g(green), b(blue), a(alpha) {}
};