#include <U8g2lib.h>  // for the font data

enum WeatherCondition {
    SUNNY,
    CLOUDY,
    RAIN,
    SNOW,
    THUNDERSTORM
};

void draw_weather_glyph(Arduino_GFX* gfx, WeatherCondition condition, int16_t x, int16_t y) {
    // Set the font, then draw a weather glyph at x, y
    gfx->setFont(u8g2_font_unifont_t_weather);
    gfx->setCursor(x, y);
    switch (condition) {
        case SUNNY:
            gfx->print("\xe2\x98\x80");  // ☀ sunny (U+2600)
            break;
        case CLOUDY:
            gfx->print("\xe2\x98\x81");  // ☁ cloudy (U+2601)
            break;
        case RAIN:
            gfx->print("\xe2\x98\x82");  // ☂ rain (U+2602)
            break;
        case SNOW:
            gfx->print("\xe2\x98\x83");  // ☃ snow (U+2603)
            break;
        case THUNDERSTORM:
            gfx->print("\xe2\x9a\xa1");  // ⚡ thunderstorm (U+26A1)
            break;
    }
    gfx->setFont();
}
