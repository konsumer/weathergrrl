#pragma once
#include <math.h>

enum WeatherCondition {
    SUNNY,
    CLOUDY,
    RAIN,
    SNOW,
    THUNDER
};

// Draws a ~20×20 weather icon with top-left at (x, y) using GFX primitives.
static inline void draw_weather_glyph(Arduino_GFX* gfx, WeatherCondition condition,
                                      int16_t x, int16_t y) {
    switch (condition) {

    case SUNNY: {
        // Yellow circle with 8 rays
        uint16_t sun = RGB565(255, 220, 0);
        gfx->fillCircle(x + 10, y + 10, 5, sun);
        for (int a = 0; a < 8; a++) {
            float rad = a * 3.14159f / 4.0f;
            int16_t x0 = x + 10 + (int16_t)(7  * cosf(rad));
            int16_t y0 = y + 10 + (int16_t)(7  * sinf(rad));
            int16_t x1 = x + 10 + (int16_t)(10 * cosf(rad));
            int16_t y1 = y + 10 + (int16_t)(10 * sinf(rad));
            gfx->drawLine(x0, y0, x1, y1, sun);
        }
        break;
    }

    case CLOUDY: {
        // Three overlapping gray circles
        uint16_t cloud = RGB565(180, 180, 180);
        gfx->fillCircle(x +  7, y + 12, 5, cloud);
        gfx->fillCircle(x + 13, y + 12, 5, cloud);
        gfx->fillCircle(x + 10, y +  8, 5, cloud);
        break;
    }

    case RAIN: {
        // Gray cloud + blue drops
        uint16_t cloud = RGB565(130, 130, 130);
        uint16_t rain  = RGB565( 80, 160, 255);
        gfx->fillCircle(x +  6, y +  9, 4, cloud);
        gfx->fillCircle(x + 12, y +  9, 4, cloud);
        gfx->fillCircle(x +  9, y +  6, 4, cloud);
        gfx->fillRect(x + 4, y + 11, 12, 3, cloud);
        // drops
        for (int i = 0; i < 3; i++) {
            gfx->drawLine(x + 5 + i*4, y + 15, x + 4 + i*4, y + 19, rain);
        }
        break;
    }

    case SNOW: {
        // Gray cloud + white dots
        uint16_t cloud = RGB565(130, 130, 130);
        uint16_t snow  = RGB565(220, 240, 255);
        gfx->fillCircle(x +  6, y +  9, 4, cloud);
        gfx->fillCircle(x + 12, y +  9, 4, cloud);
        gfx->fillCircle(x +  9, y +  6, 4, cloud);
        gfx->fillRect(x + 4, y + 11, 12, 3, cloud);
        // flakes
        for (int i = 0; i < 3; i++) {
            gfx->fillCircle(x + 5 + i*4, y + 17, 1, snow);
        }
        break;
    }

    case THUNDER: {
        // Gray cloud + yellow bolt
        uint16_t cloud = RGB565(100, 100, 120);
        uint16_t bolt  = RGB565(255, 220,   0);
        gfx->fillCircle(x +  6, y +  8, 4, cloud);
        gfx->fillCircle(x + 12, y +  8, 4, cloud);
        gfx->fillCircle(x +  9, y +  5, 4, cloud);
        gfx->fillRect(x + 4, y + 10, 12, 3, cloud);
        // zigzag bolt
        gfx->drawLine(x + 11, y + 13, x +  8, y + 17, bolt);
        gfx->drawLine(x +  8, y + 17, x + 11, y + 17, bolt);
        gfx->drawLine(x + 11, y + 17, x +  8, y + 21, bolt);
        break;
    }
    }
}
