#ifndef WOKWI_SIMULATION
#  include <WiFi.h>
#endif
#include <time.h>
#include "canvas.h"
#include "touch.h"
#include "wifi_config.h"
#include "outfits.h"
#include "weather.h"

static uint32_t g_last_wx = 0;
#define WX_INTERVAL (10UL * 60 * 1000)      // refresh weather every 10 min

// Redraws clock + temp text only — call every second from loop().
// Does NOT redraw the condition icon (static until next draw_full).
static void draw_clock() {
    time_t now = time(nullptr) + wx_utc_off;
    struct tm t;
    gmtime_r(&now, &t);
    char buf[12];
    snprintf(buf, sizeof(buf), "%02d:%02d:%02d", t.tm_hour, t.tm_min, t.tm_sec);

    // Clock — left side, vertically centred above forecast strip
    gfx->fillRect(WX_CLOCK_X, WX_CLOCK_Y, 8 * 18, 24, DARKBLUE);  // 8 chars × 18px, 24px tall
    gfx->setFont();
    gfx->setTextColor(WHITE, DARKBLUE);
    gfx->setTextSize(3);
    gfx->setCursor(WX_CLOCK_X, WX_CLOCK_Y);
    gfx->print(buf);
}

// Full repaint — call on startup and after each weather refresh
static void draw_full() {
    gfx->fillScreen(DARKBLUE);
    draw_weather_glyph(gfx, wx_cond, WX_ICON_X, WX_ICON_Y);  // icon first (behind outfit)
    draw_outfit_for_temp(gfx, wx_temp, 80);   // outfit over icon
    wx_draw_forecast();                         // bottom bar
    draw_clock();                            // clock + temp text (no bar)
}

// ── Setup ─────────────────────────────────────────────────────────────────────
void setup() {
    canvas_begin();
    touch_begin();

#ifndef WOKWI_SIMULATION
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

    // NTP sync — UTC; local offset comes from weather API
    configTime(0, 0, "pool.ntp.org");
    uint32_t sync_t = millis();
    while (time(nullptr) < 1000000000UL && millis() - sync_t < 10000)
        delay(200);

    // Initial weather fetch
    wx_fetch();
    g_last_wx = millis();
#endif // WOKWI_SIMULATION

    draw_full();
}

// ── Loop ──────────────────────────────────────────────────────────────────────
void loop() {
#ifndef WOKWI_SIMULATION
    if (millis() - g_last_wx >= WX_INTERVAL) {
        wx_fetch();
        g_last_wx = millis();
        draw_full();
    }
#endif

    draw_clock();
    delay(1000);
}
