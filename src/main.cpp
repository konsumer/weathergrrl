#include <WiFi.h>
#include <time.h>
#include "canvas.h"
#include "outfits.h"

void setup() {
    canvas_begin();

    _Display_ptr->fillScreen(ORANGE);
    draw_outfit_for_temp(_Display_ptr, 30.0f, 100, 0);
    _Display_ptr->setTextSize(3);
    _Display_ptr->setCursor(30, 60);
    _Display_ptr->setTextSize(3);
    _Display_ptr->setCursor(30, 60);
    _Display_ptr->print("Setting");
    _Display_ptr->setCursor(30, 90);
    _Display_ptr->print("up");
    _Display_ptr->setCursor(30, 120);
    _Display_ptr->print("wifi...");
    
    WiFi.begin("Wokwi-guest", "");
    while (WiFi.status() != WL_CONNECTED) {
        Serial.println(WiFi.status());
        delay(500);
    }
    configTime(-5 * 3600, 3600, "pool.ntp.org");
    
    _Display_ptr->fillScreen(RED);
    _Display_ptr->setTextSize(3);
    _Display_ptr->setCursor(30, 60);
    _Display_ptr->print("Wifi");
    _Display_ptr->setCursor(30, 90);
    _Display_ptr->print("not");
    _Display_ptr->setCursor(30, 120);
    _Display_ptr->print("connected");
    _Display_ptr->setTextSize(1);
    _Display_ptr->setCursor(30, 150);
    _Display_ptr->print("Press the screen to set it up.");
}

void loop() {
}
