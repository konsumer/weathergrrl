#include <WiFi.h>
#include <time.h>
#include "canvas.h"
#include "outfits.h"
#include "weather.h"

float temp = 90.0f;

void setup() {
    canvas_begin();

    gfx->fillScreen(ORANGE);
    draw_outfit_for_temp(gfx, temp, 80);
    gfx->setTextSize(3);
    gfx->setCursor(40, 60);
    gfx->setTextSize(3);
    gfx->setCursor(40, 60);
    gfx->print("Setting");
    gfx->setCursor(40, 90);
    gfx->print("up");
    gfx->setCursor(40, 120);
    gfx->print("wifi...");

    draw_weather_glyph(gfx, SUNNY, 200, 60);
    
    WiFi.begin("Wokwi-guest", "");
    while (WiFi.status() != WL_CONNECTED) {
        Serial.println(WiFi.status());
        delay(500);
    }
    configTime(-5 * 3600, 3600, "pool.ntp.org");
}

void loop() {
}
