#pragma once
#include <math.h>
#ifndef WOKWI_SIMULATION
#  include <HTTPClient.h>
#  include <WiFiClientSecure.h>
#  include <ArduinoJson.h>
#endif

// ─── Types ────────────────────────────────────────────────────────────────────
enum WeatherCondition { SUNNY, CLOUDY, RAIN, SNOW, THUNDER };

#define WX_FORECAST_DAYS 5
struct WxDay { float hi, lo; WeatherCondition cond; char day[4]; };

// ─── State ────────────────────────────────────────────────────────────────────
static float            wx_temp    = 72.0f;
static WeatherCondition wx_cond    = SUNNY;
static int32_t          wx_utc_off = 0;
static WxDay            wx_days[WX_FORECAST_DAYS] = {};

// ─── Layout ───────────────────────────────────────────────────────────────────
#define WX_BAR_H    38   // top bar: clock + current conditions
#define WX_FCAST_H  52   // bottom bar: 5-day forecast

// ─── Helpers ──────────────────────────────────────────────────────────────────
static WeatherCondition _wx_wmo(int c) {
    if (c == 0 || c == 1)                return SUNNY;
    if (c <= 3 || c == 45 || c == 48)   return CLOUDY;
    if (c >= 95)                         return THUNDER;
    if (c >= 70)                         return SNOW;
    return RAIN;
}

// "2026-04-16" → "Mon"
static void _wx_day_label(const char* iso, char out[4]) {
    int Y = 0, M = 0, D = 0;
    if (sscanf(iso, "%d-%d-%d", &Y, &M, &D) != 3) { out[0] = '\0'; return; }
    if (M < 3) { M += 12; Y--; }
    // Zeller's congruence
    int h = (D + (13*(M+1))/5 + Y%100 + Y%100/4 + Y/100/4 + 5*(Y/100)) % 7;
    const char* n[] = { "Sat","Sun","Mon","Tue","Wed","Thu","Fri" };
    memcpy(out, n[h], 3); out[3] = '\0';
}

// Text centred in a cell of width cw, textSize must already be set
static void _wx_center(int16_t cx, int16_t y, int16_t cw,
                       const char* text, uint16_t color) {
    gfx->setTextColor(color);
    int16_t tw = (int16_t)(strlen(text) * 6);   // 6px per char at textSize=1
    gfx->setCursor(cx + (cw - tw) / 2, y);
    gfx->print(text);
}

// ─── Weather icon (GFX primitives, ~20×20 px, top-left at x,y) ───────────────
static void draw_weather_glyph(Arduino_GFX* g, WeatherCondition cond,
                               int16_t x, int16_t y) {
    switch (cond) {
    case SUNNY: {
        uint16_t sun = RGB565(255, 220, 0);
        g->fillCircle(x+10, y+10, 5, sun);
        for (int a = 0; a < 8; a++) {
            float r = a * 3.14159f / 4.0f;
            g->drawLine(x+10+(int16_t)(7*cosf(r)),  y+10+(int16_t)(7*sinf(r)),
                        x+10+(int16_t)(10*cosf(r)), y+10+(int16_t)(10*sinf(r)), sun);
        }
        break;
    }
    case CLOUDY: {
        uint16_t c = RGB565(180, 180, 180);
        g->fillCircle(x+ 7, y+12, 5, c);
        g->fillCircle(x+13, y+12, 5, c);
        g->fillCircle(x+10, y+ 8, 5, c);
        break;
    }
    case RAIN: {
        uint16_t cl = RGB565(130,130,130), rain = RGB565(80,160,255);
        g->fillCircle(x+6,  y+9, 4, cl);
        g->fillCircle(x+12, y+9, 4, cl);
        g->fillCircle(x+9,  y+6, 4, cl);
        g->fillRect(x+4, y+11, 12, 3, cl);
        for (int i = 0; i < 3; i++)
            g->drawLine(x+5+i*4, y+15, x+4+i*4, y+19, rain);
        break;
    }
    case SNOW: {
        uint16_t cl = RGB565(130,130,130), snow = RGB565(220,240,255);
        g->fillCircle(x+6,  y+9, 4, cl);
        g->fillCircle(x+12, y+9, 4, cl);
        g->fillCircle(x+9,  y+6, 4, cl);
        g->fillRect(x+4, y+11, 12, 3, cl);
        for (int i = 0; i < 3; i++)
            g->fillCircle(x+5+i*4, y+17, 1, snow);
        break;
    }
    case THUNDER: {
        uint16_t cl = RGB565(100,100,120), bolt = RGB565(255,220,0);
        g->fillCircle(x+6,  y+8, 4, cl);
        g->fillCircle(x+12, y+8, 4, cl);
        g->fillCircle(x+9,  y+5, 4, cl);
        g->fillRect(x+4, y+10, 12, 3, cl);
        g->drawLine(x+11,y+13, x+8, y+17, bolt);
        g->drawLine(x+8, y+17, x+11,y+17, bolt);
        g->drawLine(x+11,y+17, x+8, y+21, bolt);
        break;
    }
    }
}

// ─── Drawing ──────────────────────────────────────────────────────────────────

// Redraws only the top bar — call every second from loop()
static void wx_draw_clock() {
    time_t now = time(nullptr) + wx_utc_off;
    struct tm t;
    gmtime_r(&now, &t);
    char buf[12];
    snprintf(buf, sizeof(buf), "%02d:%02d:%02d", t.tm_hour, t.tm_min, t.tm_sec);

    gfx->fillRect(0, 0, SCREEN_W, WX_BAR_H, DARKBLUE);
    gfx->setFont();
    gfx->setTextColor(WHITE, DARKBLUE);
    gfx->setTextSize(3);
    gfx->setCursor(8, 7);
    gfx->print(buf);

    // Current temp (right of clock)
    snprintf(buf, sizeof(buf), "%.0fF", wx_temp);
    int16_t tw = (int16_t)(strlen(buf) * 12);  // textSize=2 → 12px/char
    gfx->setTextSize(2);
    gfx->setTextColor(YELLOW, DARKBLUE);
    gfx->setCursor(SCREEN_W - tw - 26, 11);
    gfx->print(buf);

    // Current condition icon (far right)
    draw_weather_glyph(gfx, wx_cond, SCREEN_W - 22, 9);
}

// Draws the 5-day forecast strip at the bottom — called from wx_draw_full()
static void wx_draw_forecast() {
    const int16_t y0 = SCREEN_H - WX_FCAST_H;
    const int16_t cw = SCREEN_W / WX_FORECAST_DAYS;   // 64px each

    gfx->fillRect(0, y0, SCREEN_W, WX_FCAST_H, DARKBLUE);
    gfx->drawFastHLine(0, y0, SCREEN_W, BLUE);

    gfx->setFont();
    gfx->setTextSize(1);

    for (int i = 0; i < WX_FORECAST_DAYS; i++) {
        int16_t cx = i * cw;
        const WxDay& d = wx_days[i];

        if (i > 0) gfx->drawFastVLine(cx, y0, WX_FCAST_H, BLUE);

        // Day name
        _wx_center(cx, y0 + 2, cw, d.day, WHITE);

        // Icon centred in cell
        draw_weather_glyph(gfx, d.cond, cx + (cw - 20) / 2, y0 + 12);

        // Hi (warm) / Lo (cool)
        char buf[8];
        snprintf(buf, sizeof(buf), "%.0fF", d.hi);
        _wx_center(cx, y0 + 34, cw, buf, RGB565(255, 140,  80));
        snprintf(buf, sizeof(buf), "%.0fF", d.lo);
        _wx_center(cx, y0 + 44, cw, buf, RGB565(100, 180, 255));
    }
}

// Full repaint — call on startup and after each weather refresh
static void wx_draw_full() {
    gfx->fillScreen(DARKBLUE);
    draw_outfit_for_temp(gfx, wx_temp, 80);   // outfit as background
    wx_draw_forecast();                         // bottom bar (over outfit)
    wx_draw_clock();                            // top bar (over outfit)
}

// ─── Fetch (WiFi / real hardware only) ────────────────────────────────────────
#ifndef WOKWI_SIMULATION
static bool _wx_fetch_json(const char* url, JsonDocument& doc) {
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
    String body = http.getString();
    http.end();
    Serial.printf("[wx] %d bytes\n", body.length());
    DeserializationError err = deserializeJson(doc, body);
    if (err) {
        Serial.printf("[wx] JSON error: %s\n", err.c_str());
        return false;
    }
    return true;
}

static bool wx_fetch() {
    // 1. Geolocation
    JsonDocument geo;
    if (!_wx_fetch_json(
            "https://vpncheck-david-konsumers-projects-edc64e5e.vercel.app/geo", geo))
        return false;
    float lat = geo["ll"][0] | 0.0f;
    float lon = geo["ll"][1] | 0.0f;
    Serial.printf("[wx] lat=%.4f lon=%.4f\n", lat, lon);
    if (lat == 0.0f && lon == 0.0f) return false;

    // 2. Current + 5-day forecast
    char url[300];
    snprintf(url, sizeof(url),
        "https://api.open-meteo.com/v1/forecast"
        "?latitude=%.4f&longitude=%.4f"
        "&current=temperature_2m,weather_code"
        "&daily=weather_code,temperature_2m_max,temperature_2m_min"
        "&temperature_unit=fahrenheit"
        "&timezone=auto&forecast_days=%d",
        lat, lon, WX_FORECAST_DAYS);

    JsonDocument wx;
    if (!_wx_fetch_json(url, wx)) return false;

    wx_temp    = wx["current"]["temperature_2m"]  | wx_temp;
    wx_cond    = _wx_wmo(wx["current"]["weather_code"] | 0);
    wx_utc_off = wx["utc_offset_seconds"]          | wx_utc_off;

    for (int i = 0; i < WX_FORECAST_DAYS; i++) {
        wx_days[i].hi   = wx["daily"]["temperature_2m_max"][i] | 0.0f;
        wx_days[i].lo   = wx["daily"]["temperature_2m_min"][i] | 0.0f;
        wx_days[i].cond = _wx_wmo(wx["daily"]["weather_code"][i] | 0);
        _wx_day_label(wx["daily"]["time"][i] | "", wx_days[i].day);
    }

    Serial.printf("[wx] %.1fF code=%d utc=%d\n",
                  wx_temp, (int)wx["current"]["weather_code"], wx_utc_off);
    return true;
}
#endif // WOKWI_SIMULATION
