#pragma once
#include <math.h>
#include <HTTPClient.h>

// ─── Unit selection (set via build_flags) ─────────────────────────────────────
// -DWX_CELSIUS  → display °C  (default: °F)
// -DWX_WIND_KMH → display km/h (default: mph)
// State is always stored in metric (°C, km/h); conversion happens at display.
#ifdef WX_CELSIUS
#  define WX_TEMP_UNIT            "C"
#  define WX_FROM_METRIC_TEMP(c)  (c)
#else
#  define WX_TEMP_UNIT            "F"
#  define WX_FROM_METRIC_TEMP(c)  ((c) * 9.0f / 5.0f + 32.0f)
#endif

#ifdef WX_WIND_KMH
#  define WX_WIND_UNIT            "km/h"
#  define WX_FROM_METRIC_WIND(k)  (k)
#else
#  define WX_WIND_UNIT            "mph"
#  define WX_FROM_METRIC_WIND(k)  ((k) * 0.621371f)
#endif
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

#ifdef AHTX0
#include <Wire.h>
#define AHTX0_ADDR 0x38
#define AHTX0_SDA  27
#define AHTX0_SCL  22
#endif

// ─── Types ────────────────────────────────────────────────────────────────────
enum WeatherCondition { SUNNY, CLOUDY, RAIN, SNOW, THUNDER };

#define WX_FORECAST_DAYS 5
struct WxDay { float hi, lo; WeatherCondition cond; char day[4]; };

// ─── State ────────────────────────────────────────────────────────────────────
static float            wx_temp    = 22.0f;   // °C
static WeatherCondition wx_cond    = SUNNY;
static int32_t          wx_utc_off = 0;
static WxDay            wx_days[WX_FORECAST_DAYS] = {};
static float            wx_feels   = 22.0f;   // °C
static float            wx_humidity = 0.0f;
static float            wx_pressure = 0.0f;
static float            wx_wind_spd = 0.0f;
static float            wx_wind_gst = 0.0f;
static int16_t          wx_wind_dir = 0;
static char             wx_sunrise[6] = "--:--";
static char             wx_sunset[6]  = "--:--";

#ifdef AHTX0
static bool _aht_ready = false;

static bool wx_sensor_begin() {
    Wire.begin(AHTX0_SDA, AHTX0_SCL);
    Wire.beginTransmission(AHTX0_ADDR);
    if (Wire.endTransmission() != 0) {
        Serial.println("[wx] Could not find AHTX0 sensor — will use internet temp");
        return false;
    }
    // calibrate
    Wire.beginTransmission(AHTX0_ADDR);
    Wire.write(0xE1); Wire.write(0x08); Wire.write(0x00);
    Wire.endTransmission();
    _aht_ready = true;
    Serial.println("[wx] AHTX0 ready");
    return true;
}

static void wx_sensor_read() {
    if (!_aht_ready) return;
    Wire.beginTransmission(AHTX0_ADDR);
    Wire.write(0xAC); Wire.write(0x33); Wire.write(0x00);
    if (Wire.endTransmission() != 0) return;
    delay(80);  // wait for measurement
    if (Wire.requestFrom(AHTX0_ADDR, 6) != 6) return;
    uint8_t buf[6];
    for (auto& b : buf) b = Wire.read();
    uint32_t raw_hum  = ((uint32_t)buf[1] << 12) | ((uint32_t)buf[2] << 4) | (buf[3] >> 4);
    uint32_t raw_temp = (((uint32_t)buf[3] & 0x0F) << 16) | ((uint32_t)buf[4] << 8) | buf[5];
    wx_humidity = raw_hum  / 1048576.0f * 100.0f;
    wx_temp     = raw_temp / 1048576.0f * 200.0f - 50.0f;
    Serial.printf("[aht] %.1f°C  %.0f%%\n", wx_temp, wx_humidity);
}
#endif

// ─── Layout ───────────────────────────────────────────────────────────────────
#define WX_FCAST_H  52            // forecast strip height
#define WX_FCAST_W  200           // forecast strip covers left portion

// Clock text position (left side, vertically centred above forecast strip)
#define WX_CLOCK_X  20
#define WX_CLOCK_Y  ((SCREEN_H - WX_FCAST_H) / 2 - 20)

// Current-temp position (between her legs, near bottom)
#define WX_TEMP_X   200
#define WX_TEMP_Y   (SCREEN_H - 20)

// Condition icon position (top-right, peeks over her shoulder)
#define WX_ICON_X   (SCREEN_W - 30)
#define WX_ICON_Y   9

// ─── Helpers ──────────────────────────────────────────────────────────────────
static const char* _wx_compass(int deg) {
    const char* d[] = {"N","NE","E","SE","S","SW","W","NW"};
    return d[((deg + 22) % 360) / 45];
}

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

// Redraws clock text only — call every second from loop()
static void wx_draw_clock() {
    time_t now = time(nullptr) + wx_utc_off;
    struct tm t;
    gmtime_r(&now, &t);

    gfx->setFont();

    gfx->setTextColor(YELLOW, DARKBLUE);
    gfx->setTextSize(2);
    gfx->setCursor(20, WX_CLOCK_Y - 60);
    gfx->print("hey grrl, it's");

    // Date line above clock
    const char* dow[] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
    char date[24];
    snprintf(date, sizeof(date), "%s, %d/%d/%d",
             dow[t.tm_wday], t.tm_mon + 1, t.tm_mday, t.tm_year + 1900);
    gfx->fillRect(WX_CLOCK_X, WX_CLOCK_Y - 20, 30 * 6, 8, DARKBLUE);
    
    gfx->setTextSize(1);
    gfx->setTextColor(RGB565(180, 200, 255), DARKBLUE);
    gfx->setCursor(WX_CLOCK_X, WX_CLOCK_Y - 20);
    gfx->print(date);

    // Clock
    char buf[12];
    snprintf(buf, sizeof(buf), "%02d:%02d:%02d", t.tm_hour, t.tm_min, t.tm_sec);
    gfx->fillRect(WX_CLOCK_X, WX_CLOCK_Y, 8 * 18, 24, DARKBLUE);  // 8 chars × 18px wide
    gfx->setFont();
    gfx->setTextColor(WHITE, DARKBLUE);
    gfx->setTextSize(3);
    gfx->setCursor(WX_CLOCK_X, WX_CLOCK_Y);
    gfx->print(buf);
}

// Draws the 5-day forecast strip at the bottom-left — called from wx_draw_full()
static void wx_draw_forecast() {
    const int16_t y0 = SCREEN_H - WX_FCAST_H - 70;
    const int16_t cw = WX_FCAST_W / WX_FORECAST_DAYS;  // 40px each (200/5)

    gfx->setFont();
    gfx->setTextSize(1);

    char buf[8];

    for (int i = 0; i < WX_FORECAST_DAYS; i++) {
        int16_t cx = i * cw;
        const WxDay& d = wx_days[i];

        // Day name
        _wx_center(cx, y0 + 2, cw, d.day, WHITE);

        // Icon centred in cell
        draw_weather_glyph(gfx, d.cond, cx + (cw - 20) / 2, y0 + 12);

        // Hi (warm) / Lo (cool)
        snprintf(buf, sizeof(buf), "%.0f" WX_TEMP_UNIT, WX_FROM_METRIC_TEMP(d.hi));
        _wx_center(cx, y0 + 34, cw, buf, RGB565(255, 140,  80));
        snprintf(buf, sizeof(buf), "%.0f" WX_TEMP_UNIT, WX_FROM_METRIC_TEMP(d.lo));
        _wx_center(cx, y0 + 44, cw, buf, RGB565(100, 180, 255));
    }

    // Extra conditions — two small lines below the forecast cells
    char line[48];
    const int16_t iy = y0 + WX_FCAST_H + 16;   // just below icon+hi+lo rows
    gfx->setTextSize(1);
    gfx->setFont();

    snprintf(line, sizeof(line), "FL:%.0f" WX_TEMP_UNIT "  H:%.0f%%  P:%.0fmb",
             WX_FROM_METRIC_TEMP(wx_feels), wx_humidity, wx_pressure);
    gfx->fillRect(10, iy, WX_FCAST_W-30, 8, DARKBLUE);
    gfx->setTextColor(RGB565(180, 180, 255));
    gfx->setCursor(12, iy);
    gfx->print(line);

    snprintf(line, sizeof(line), "W:%.0f" WX_WIND_UNIT " %s  G:%.0f" WX_WIND_UNIT,
             WX_FROM_METRIC_WIND(wx_wind_spd), _wx_compass(wx_wind_dir), WX_FROM_METRIC_WIND(wx_wind_gst));
    gfx->fillRect(10, iy + 10, WX_FCAST_W-30, 8, DARKBLUE);
    gfx->setTextColor(RGB565(180, 220, 255));
    gfx->setCursor(12, iy + 10);
    gfx->print(line);

    snprintf(line, sizeof(line), "Rise %s  Set %s", wx_sunrise, wx_sunset);
    gfx->fillRect(10, iy + 20, WX_FCAST_W-30, 8, DARKBLUE);
    gfx->setTextColor(RGB565(255, 220, 130));
    gfx->setCursor(12, iy + 20);
    gfx->print(line);

    // Current temp — between her legs, near bottom
    snprintf(buf, sizeof(buf), "%.0f" WX_TEMP_UNIT, WX_FROM_METRIC_TEMP(wx_temp));
    gfx->fillRect(WX_TEMP_X + 12, WX_TEMP_Y, 46, 16, DARKBLUE);
    gfx->setTextSize(2);
    _wx_center(WX_TEMP_X, WX_TEMP_Y, 46, buf, YELLOW);
}

// Full repaint — call on startup and after each weather refresh
static void wx_draw_full() {
    gfx->fillScreen(DARKBLUE);
    draw_weather_glyph(gfx, wx_cond, WX_ICON_X, WX_ICON_Y);  // icon before outfit
    draw_outfit_for_temp(gfx, wx_temp, 80);
    wx_draw_forecast();
    wx_draw_clock();
}

// ─── Fetch ────────────────────────────────────────────────────────────────────
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
    char url[512];
    snprintf(url, sizeof(url),
        "https://api.open-meteo.com/v1/forecast"
        "?latitude=%.4f&longitude=%.4f"
        "&current=temperature_2m,weather_code,apparent_temperature"
        ",relative_humidity_2m,pressure_msl"
        ",wind_speed_10m,wind_direction_10m,wind_gusts_10m"
        "&daily=weather_code,temperature_2m_max,temperature_2m_min,sunrise,sunset"
        "&timezone=auto&forecast_days=%d",   // API always returns metric (°C, km/h)
        lat, lon, WX_FORECAST_DAYS);

    JsonDocument wx;
    if (!_wx_fetch_json(url, wx)) return false;

#ifdef AHTX0
    wx_sensor_read();
#else
    wx_temp     = wx["current"]["temperature_2m"]       | wx_temp;
    wx_humidity = wx["current"]["relative_humidity_2m"] | wx_humidity;
#endif
    wx_cond     = _wx_wmo(wx["current"]["weather_code"] | 0);
    wx_utc_off  = wx["utc_offset_seconds"]              | wx_utc_off;
    wx_feels    = wx["current"]["apparent_temperature"] | wx_feels;
    wx_pressure = wx["current"]["pressure_msl"]         | wx_pressure;
    wx_wind_spd = wx["current"]["wind_speed_10m"]       | wx_wind_spd;
    wx_wind_dir = wx["current"]["wind_direction_10m"]   | wx_wind_dir;
    wx_wind_gst = wx["current"]["wind_gusts_10m"]       | wx_wind_gst;

    for (int i = 0; i < WX_FORECAST_DAYS; i++) {
        wx_days[i].hi   = wx["daily"]["temperature_2m_max"][i] | 0.0f;
        wx_days[i].lo   = wx["daily"]["temperature_2m_min"][i] | 0.0f;
        wx_days[i].cond = _wx_wmo(wx["daily"]["weather_code"][i] | 0);
        _wx_day_label(wx["daily"]["time"][i] | "", wx_days[i].day);
    }

    // Sunrise/sunset today — format "2026-04-16T06:23", keep "HH:MM" after 'T'
    auto _parse_hhmm = [](const char* s, char out[6]) {
        const char* t = strchr(s, 'T');
        if (t && strlen(t) >= 6) { memcpy(out, t + 1, 5); out[5] = '\0'; }
    };
    _parse_hhmm(wx["daily"]["sunrise"][0] | "--:--", wx_sunrise);
    _parse_hhmm(wx["daily"]["sunset"][0]  | "--:--", wx_sunset);

    Serial.printf("[wx] %.1f°C code=%d utc=%d\n",
                  wx_temp, (int)wx["current"]["weather_code"], wx_utc_off);
    return true;
}
