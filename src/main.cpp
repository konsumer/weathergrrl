#include <WiFi.h>
#include <time.h>
#include "canvas.h"
#include "touch.h"
#include "wifi_config.h"
#include "outfits.h"
#include "weather.h"

static uint32_t g_last_wx = 0;
#define WX_INTERVAL (10UL * 60 * 1000)      // refresh weather every 10 min

// ── Setup ─────────────────────────────────────────────────────────────────────
void setup() {
    canvas_begin();
    touch_begin();

#ifdef WOKWI_SIMULATION
    WiFi.begin("Wokwi-GUEST", "", 6);
#else
    char ssid[64] = {};
    char pass[64] = {};
    wifi_config_load(ssid, sizeof(ssid), pass, sizeof(pass));

    // Hold touch at boot → wipe credentials and re-enter config
    {
        int16_t tx, ty;
        if (touch_read(&tx, &ty)) {
            wifi_config_clear();
            ssid[0] = '\0';
            pass[0] = '\0';
        }
    }

    if (ssid[0] == '\0') {
        wifi_config_screen();
        wifi_config_load(ssid, sizeof(ssid), pass, sizeof(pass));
    }

    // Connecting splash
    gfx->fillScreen(ORANGE);
    draw_outfit_for_temp(gfx, wx_temp, 80);
    gfx->setFont();
    gfx->setTextColor(WHITE);
    gfx->setTextSize(3);
    gfx->setCursor(8, 100);
    gfx->print("Connecting...");

    WiFi.begin(ssid, pass);
    uint32_t t0 = millis();
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        if (millis() - t0 > 15000) {
            wifi_config_clear();
            wifi_config_screen();
            wifi_config_load(ssid, sizeof(ssid), pass, sizeof(pass));
            WiFi.begin(ssid, pass);
            t0 = millis();
        }
    }
#endif

    gfx->fillScreen(ORANGE);
    draw_outfit_for_temp(gfx, wx_temp, 80);
    gfx->setFont();
    gfx->setTextColor(WHITE);
    gfx->setTextSize(3);
    gfx->setCursor(8, 100);
    gfx->print("Getting info...");

#ifdef AHTX0
    wx_sensor_begin();
#endif

    // NTP sync — UTC; local offset comes from weather API
    configTime(0, 0, "pool.ntp.org");
    uint32_t sync_t = millis();
    while (time(nullptr) < 1000000000UL && millis() - sync_t < 10000)
        delay(200);

    wx_fetch();
    g_last_wx = millis();

    wx_draw_full();
}

// ── Loop ──────────────────────────────────────────────────────────────────────
void loop() {
    if (millis() - g_last_wx >= WX_INTERVAL) {
        wx_fetch();
        g_last_wx = millis();
        wx_draw_full();
    }

    wx_draw_clock();
    delay(1000);
}
