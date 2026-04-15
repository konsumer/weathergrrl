#pragma once
// wifi_config.h — Touch-based WiFi credential entry and NVS storage.
// Requires canvas.h (gfx, SCREEN_W, SCREEN_H) and touch.h.
//
// API:
//   wifi_config_load(ssid, ssid_len, pass, pass_len)  — read from NVS
//   wifi_config_save(ssid, pass)                       — write to NVS
//   wifi_config_screen()                               — show keyboard UI,
//                                                        blocks until OK tapped,
//                                                        saves on confirm

#include <Preferences.h>
#include "canvas.h"
#include "touch.h"

#define _WC_NS      "wifi"   // NVS namespace
#define _WC_MAXLEN  63

// ─── NVS helpers ──────────────────────────────────────────────────────────────

static inline void wifi_config_load(char* ssid, size_t ssid_len,
                                    char* pass, size_t pass_len) {
  Preferences p;
  p.begin(_WC_NS, true);
  p.getString("ssid", ssid, ssid_len);
  p.getString("pass", pass, pass_len);
  p.end();
}

static inline void wifi_config_save(const char* ssid, const char* pass) {
  Preferences p;
  p.begin(_WC_NS, false);
  p.putString("ssid", ssid);
  p.putString("pass", pass);
  p.end();
}

// ─── Layout ───────────────────────────────────────────────────────────────────
// Header: 2 × 24px input fields at top
// Keyboard: 4 rows filling the rest

#define _WC_FIELD_H  24
#define _WC_HDR_H    (_WC_FIELD_H * 2)
#define _WC_ROWS     4
#define _WC_ROW_H    ((SCREEN_H - _WC_HDR_H) / _WC_ROWS)
#define _WC_KEY_W    (SCREEN_W / 10)   // 32px on 320-wide screen

// ─── Key rows ─────────────────────────────────────────────────────────────────
static const char _wc_r0[] = "1234567890";
static const char _wc_r1[] = "qwertyuiop";
static const char _wc_r2[] = "asdfghjkl";   // + [<-] at col 9

// Row 3: [space x6][.][@][SFT][OK]

// ─── Drawing helpers ──────────────────────────────────────────────────────────

static inline void _wc_key(int16_t x, int16_t y, int16_t w, int16_t h,
                            const char* label, uint16_t bg, uint16_t fg) {
  gfx->fillRect(x + 1, y + 1, w - 2, h - 2, bg);
  gfx->setTextColor(fg);
  gfx->setTextSize(1);
  int16_t lw = (int16_t)(strlen(label) * 6);
  gfx->setCursor(x + (w - lw) / 2, y + (_WC_ROW_H - 8) / 2);
  gfx->print(label);
}

static inline void _wc_field(int16_t y, const char* label,
                              const char* value, bool active, bool secret) {
  gfx->fillRect(0, y, SCREEN_W, _WC_FIELD_H, active ? DARKBLUE : DARKGRAY);
  gfx->setTextColor(WHITE);
  gfx->setTextSize(1);
  gfx->setCursor(4, y + (_WC_FIELD_H - 8) / 2);
  gfx->print(label);

  // How many chars fit after the label
  int16_t label_px  = (int16_t)(strlen(label) * 6);
  int16_t avail     = (SCREEN_W - 4 - label_px - 6) / 6;  // 6px cursor
  size_t  len       = strlen(value);
  size_t  start     = (len > (size_t)avail) ? len - avail : 0;

  for (size_t i = start; i < len; i++) {
    gfx->print(secret ? '*' : value[i]);
  }
  if (active) gfx->print('_');
}

static inline void _wc_keyboard(bool shifted) {
  int16_t ky = _WC_HDR_H;
  gfx->fillRect(0, ky, SCREEN_W, SCREEN_H - ky, BLACK);

  // Row 0 — numbers
  for (int i = 0; i < 10; i++) {
    char s[2] = { _wc_r0[i], '\0' };
    _wc_key(i * _WC_KEY_W, ky, _WC_KEY_W, _WC_ROW_H, s, GRAY, WHITE);
  }
  ky += _WC_ROW_H;

  // Row 1 — qwertyuiop
  for (int i = 0; i < 10; i++) {
    char c  = shifted ? (char)toupper(_wc_r1[i]) : _wc_r1[i];
    char s[2] = { c, '\0' };
    _wc_key(i * _WC_KEY_W, ky, _WC_KEY_W, _WC_ROW_H, s, GRAY, WHITE);
  }
  ky += _WC_ROW_H;

  // Row 2 — asdfghjkl + backspace
  for (int i = 0; i < 9; i++) {
    char c  = shifted ? (char)toupper(_wc_r2[i]) : _wc_r2[i];
    char s[2] = { c, '\0' };
    _wc_key(i * _WC_KEY_W, ky, _WC_KEY_W, _WC_ROW_H, s, GRAY, WHITE);
  }
  _wc_key(9 * _WC_KEY_W, ky, _WC_KEY_W, _WC_ROW_H, "<-", DARKGRAY, WHITE);
  ky += _WC_ROW_H;

  // Row 3 — [space x6][.][@][SFT][OK]
  int16_t spc_w = _WC_KEY_W * 6;
  _wc_key(0,                    ky, spc_w,     _WC_ROW_H, "space", GRAY,    WHITE);
  _wc_key(spc_w,                ky, _WC_KEY_W, _WC_ROW_H, ".",     GRAY,    WHITE);
  _wc_key(spc_w + _WC_KEY_W,   ky, _WC_KEY_W, _WC_ROW_H, "@",     GRAY,    WHITE);
  _wc_key(spc_w + _WC_KEY_W*2, ky, _WC_KEY_W, _WC_ROW_H,
          shifted ? "ABC" : "abc", shifted ? BLUE : DARKGRAY, WHITE);
  _wc_key(spc_w + _WC_KEY_W*3, ky, _WC_KEY_W, _WC_ROW_H, "OK",    GREEN,   BLACK);
}

// ─── Main screen ──────────────────────────────────────────────────────────────

// Displays an on-screen keyboard and lets the user enter WiFi credentials.
// Blocks until the user taps OK, then saves to NVS.
static inline void wifi_config_screen() {
  char ssid[_WC_MAXLEN + 1] = {};
  char pass[_WC_MAXLEN + 1] = {};
  wifi_config_load(ssid, sizeof(ssid), pass, sizeof(pass));

  bool    shifted    = false;
  int     active     = 0;   // 0 = SSID field, 1 = PASS field
  int16_t tx, ty;
  uint32_t last_tap  = 0;

  gfx->fillScreen(BLACK);

  auto redraw_fields = [&]() {
    _wc_field(0,            "SSID: ", ssid, active == 0, false);
    _wc_field(_WC_FIELD_H,  "PASS: ", pass, active == 1, true);
  };

  redraw_fields();
  _wc_keyboard(shifted);

  while (true) {
    if (!touch_read(&tx, &ty)) continue;

    uint32_t now = millis();
    if (now - last_tap < 200) continue;   // debounce
    last_tap = now;

    // ── Header: switch active field ──────────────────────────────────────────
    if (ty < _WC_HDR_H) {
      active = (ty < _WC_FIELD_H) ? 0 : 1;
      redraw_fields();
      continue;
    }

    // ── Keyboard hit ─────────────────────────────────────────────────────────
    int row = (ty - _WC_HDR_H) / _WC_ROW_H;
    int col = tx / _WC_KEY_W;
    col = constrain(col, 0, 9);

    char* buf = (active == 0) ? ssid : pass;
    size_t len = strlen(buf);
    char ch = '\0';

    if (row == 0) {
      ch = _wc_r0[col];

    } else if (row == 1) {
      ch = shifted ? (char)toupper(_wc_r1[col]) : _wc_r1[col];

    } else if (row == 2) {
      if (col == 9) {
        if (len > 0) buf[len - 1] = '\0';
        redraw_fields();
        continue;
      }
      ch = shifted ? (char)toupper(_wc_r2[col]) : _wc_r2[col];

    } else if (row == 3) {
      int16_t spc_w = _WC_KEY_W * 6;
      if (tx < spc_w) {
        ch = ' ';
      } else {
        int extra = (tx - spc_w) / _WC_KEY_W;
        if      (extra == 0) { ch = '.'; }
        else if (extra == 1) { ch = '@'; }
        else if (extra == 2) {
          shifted = !shifted;
          _wc_keyboard(shifted);
          continue;
        } else {
          // OK — save and exit
          wifi_config_save(ssid, pass);
          return;
        }
      }
    }

    if (ch && len < _WC_MAXLEN) {
      buf[len]     = ch;
      buf[len + 1] = '\0';
      if (shifted) {
        shifted = false;
        _wc_keyboard(shifted);
      }
      redraw_fields();
    }
  }
}
