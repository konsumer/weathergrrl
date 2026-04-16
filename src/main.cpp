#ifndef WOKWI_SIMULATION
#  include <WiFi.h>
#  include <HTTPClient.h>
#  include <WiFiClientSecure.h>
#  include <ArduinoJson.h>
#endif
#include <time.h>
#include "canvas.h"
#include "touch.h"
#include "wifi_config.h"
#include "outfits.h"
#include "weather.h"

// ── State ─────────────────────────────────────────────────────────────────────
static float            g_temp    = 72.0f;
static WeatherCondition g_cond    = SUNNY;
static int32_t          g_utc_off = 0;      // seconds east of UTC
static uint32_t         g_last_wx = 0;
#define WX_INTERVAL (10UL * 60 * 1000)      // refresh weather every 10 min

int16_t bgColor = BLUE;
int16_t fgColor = WHITE;

// ── WMO weather code → WeatherCondition ──────────────────────────────────────
static WeatherCondition wmo_cond(int code) {
    if (code == 0 || code == 1)                return SUNNY;
    if (code <= 3 || code == 45 || code == 48) return CLOUDY;
    if (code >= 95)                            return THUNDER;
    if (code >= 70)                            return SNOW;
    return RAIN;
}

// ── HTTPS JSON fetch + weather (real hardware only) ───────────────────────────
#ifndef WOKWI_SIMULATION
static bool fetch_json(const char* url, JsonDocument& doc) {
    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;
    http.setTimeout(10000);
    if (!http.begin(client, url)) {
        Serial.printf("[wx] begin failed: %s\n", url);
        return false;
    }
    int code = http.GET();
    if (code != HTTP_CODE_OK) {
        Serial.printf("[wx] HTTP %d: %s\n", code, url);
        http.end();
        return false;
    }
    // getString() buffers the full body before parsing — more reliable than
    // getStream() + WiFiClientSecure which can close before ArduinoJson finishes
    String body = http.getString();
    http.end();
    Serial.printf("[wx] %d bytes from %s\n", body.length(), url);
    DeserializationError err = deserializeJson(doc, body);
    if (err) {
        Serial.printf("[wx] JSON error: %s\n", err.c_str());
        return false;
    }
    return true;
}

static bool fetch_weather() {
    // 1. Geolocation
    JsonDocument geo;
    if (!fetch_json("https://vpncheck-david-konsumers-projects-edc64e5e.vercel.app/geo", geo))
        return false;
    float lat = geo["ll"][0] | 0.0f;
    float lon = geo["ll"][1] | 0.0f;
    Serial.printf("[wx] lat=%.4f lon=%.4f\n", lat, lon);
    if (lat == 0.0f && lon == 0.0f) return false;

    // 2. Current weather (Fahrenheit, auto timezone)
    char url[256];
    snprintf(url, sizeof(url),
        "https://api.open-meteo.com/v1/forecast"
        "?latitude=%.4f&longitude=%.4f"
        "&current=temperature_2m,weather_code"
        "&temperature_unit=fahrenheit"
        "&timezone=auto&forecast_days=1",
        lat, lon);

    JsonDocument wx;
    if (!fetch_json(url, wx)) return false;

    g_temp    = wx["current"]["temperature_2m"] | g_temp;
    g_cond    = wmo_cond(wx["current"]["weather_code"] | 0);
    g_utc_off = wx["utc_offset_seconds"]        | g_utc_off;
    Serial.printf("[wx] temp=%.1f code=%d utc_off=%d\n",
                  g_temp, (int)wx["current"]["weather_code"], g_utc_off);
    return true;
}
#endif // WOKWI_SIMULATION

// ── Drawing ───────────────────────────────────────────────────────────────────

// Top bar: HH:MM:SS (large) + weather icon — redrawn every second
static void draw_clock() {
    time_t now = time(nullptr) + g_utc_off;
    struct tm t;
    gmtime_r(&now, &t);
    char buf[12];
    snprintf(buf, sizeof(buf), "%02d:%02d:%02d", t.tm_hour, t.tm_min, t.tm_sec);

    gfx->fillRect(8, 100, 190, 30, bgColor);
    gfx->setFont();
    gfx->setTextColor(fgColor);
    gfx->setTextSize(4);
    gfx->setCursor(8, 100);
    gfx->print(buf);

    // Weather glyph, right side of the bar
    draw_weather_glyph(gfx, g_cond, 280, 8);
}

// Bottom-left corner: current temp (small) — redrawn with clock
static void draw_temp() {
    char buf[10];
    snprintf(buf, sizeof(buf), "%.0fF", g_temp);
    gfx->fillRect(SCREEN_W - 105, SCREEN_H - 30, 45, 30, bgColor);
    gfx->setFont();
    gfx->setTextColor(fgColor, bgColor);
    gfx->setTextSize(2);
    gfx->setCursor(SCREEN_W - 100, SCREEN_H - 23);
    gfx->print(buf);
}

// Full redraw: background + outfit + overlays — called on weather update
static void draw_full() {
    gfx->fillScreen(bgColor);
    draw_clock();
    draw_outfit_for_temp(gfx, g_temp, 80);
    draw_temp();
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
    draw_outfit_for_temp(gfx, g_temp, 80);
    gfx->setFont();
    gfx->setTextColor(fgColor);
    gfx->setTextSize(3);
    gfx->setCursor(8,100);
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
    fetch_weather();
    g_last_wx = millis();
#endif // WOKWI_SIMULATION

    draw_full();
}

// ── Loop ──────────────────────────────────────────────────────────────────────
void loop() {
#ifndef WOKWI_SIMULATION
    if (millis() - g_last_wx >= WX_INTERVAL) {
        fetch_weather();
        g_last_wx = millis();
        draw_full();   // redraws outfit for updated temp
    }
#endif

    draw_clock();
    draw_temp();
    delay(1000);
}
