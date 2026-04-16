#pragma once
// wifi_config.h — WiFi AP selection + credential entry with NVS storage.
// Requires canvas.h (gfx, SCREEN_W, SCREEN_H) and touch.h.
// WiFi.h must be included before this header (already in main.cpp).
//
// API:
//   wifi_config_load(ssid, ssid_len, pass, pass_len)  — read from NVS
//   wifi_config_save(ssid, pass)                       — write to NVS
//   wifi_config_screen()                               — AP list → password
//                                                        keyboard, saves on OK

#include <Preferences.h>
#include <WiFi.h>
#include "canvas.h"
#include "touch.h"

#define _WC_NS      "wifi"
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

static inline void wifi_config_clear() {
  Preferences p;
  p.begin(_WC_NS, false);
  p.clear();
  p.end();
}

// ─── Layout constants ─────────────────────────────────────────────────────────
// All values chosen so rows tile exactly on 320×240.

// AP list screen
#define _WC_BAR_H   24              // title bar
#define _WC_ITEM_H  31              // list row  — (240-24)/7 = 30.8, ~7 items

// Password keyboard screen
#define _WC_FIELD_H 24              // each input field row
#define _WC_HDR_H   (_WC_FIELD_H * 2)  // 48px — two fields above keyboard
#define _WC_ROWS    4
// Row height kept short so the last row ends at y=216, safely within the
// CYD touch panel's reliable area (~top 90% of the physical screen).
#define _WC_ROW_H   42              // 4 × 42 = 168px; keyboard bottom = y 216
#define _WC_KEY_W   (SCREEN_W / 10) // 32px on 320-wide screen

// ─── Shared drawing helpers ───────────────────────────────────────────────────

static inline void _wc_bar(const char* title) {
  gfx->fillRect(0, 0, SCREEN_W, _WC_BAR_H, DARKBLUE);
  gfx->setTextColor(WHITE, DARKBLUE);
  gfx->setTextSize(1);
  gfx->setCursor(4, (_WC_BAR_H - 8) / 2);
  gfx->print(title);
}

static inline void _wc_key(int16_t x, int16_t y, int16_t w, int16_t h,
                            const char* label, uint16_t bg, uint16_t fg) {
  gfx->fillRect(x + 1, y + 1, w - 2, h - 2, bg);
  gfx->setTextColor(fg, bg);
  gfx->setTextSize(1);
  int16_t lw = (int16_t)(strlen(label) * 6);
  gfx->setCursor(x + (w - lw) / 2, y + (h - 8) / 2);
  gfx->print(label);
}

// ─── AP list screen ───────────────────────────────────────────────────────────
// Returns true and fills out_ssid when an AP is picked from scan results.
// Returns false when "Enter custom..." is chosen (out_ssid untouched).

static inline bool _wc_ap_select(char* out_ssid) {
  gfx->fillScreen(BLACK);
  _wc_bar("Scanning WiFi...");

  int n = WiFi.scanNetworks();
  if (n < 0) n = 0;
  if (n > 24) n = 24;

  const int total   = n + 1;               // APs + "Enter custom..."
  const int list_y  = _WC_BAR_H;
  const int list_h  = SCREEN_H - list_y;
  const int visible = list_h / _WC_ITEM_H; // ~6–7

  // Pre-select current SSID in the list if present
  int presel = -1;
  for (int i = 0; i < n; i++) {
    if (strcmp(WiFi.SSID(i).c_str(), out_ssid) == 0) { presel = i; break; }
  }

  int scroll = 0;
  if (presel > 0 && presel >= visible) scroll = presel - 1;

  auto draw = [&]() {
    _wc_bar("Select WiFi");
    gfx->fillRect(0, list_y, SCREEN_W, list_h, BLACK);

    for (int v = 0; v < visible; v++) {
      int idx = scroll + v;
      if (idx >= total) break;
      int16_t iy  = list_y + v * _WC_ITEM_H;
      bool custom = (idx == n);
      bool sel    = (idx == presel);

      uint16_t bg = sel ? DARKBLUE : (v % 2 == 0 ? DARKGRAY : BLACK);
      gfx->fillRect(0, iy, SCREEN_W, _WC_ITEM_H, bg);
      gfx->setTextColor(WHITE, bg);
      gfx->setTextSize(1);
      gfx->setCursor(6, iy + (_WC_ITEM_H - 8) / 2);

      if (custom) {
        gfx->print("[ Enter custom... ]");
      } else {
        // Truncated SSID
        String s = WiFi.SSID(idx);
        if (s.length() > 24) { s = s.substring(0, 23); s += '~'; }
        gfx->print(s);

        // Signal bars (rightmost ~30px)
        int32_t rssi = WiFi.RSSI(idx);
        int bars = (rssi >= -55) ? 4 : (rssi >= -67) ? 3 : (rssi >= -78) ? 2 : 1;
        for (int b = 0; b < 4; b++) {
          int16_t bh = 4 + b * 4;
          int16_t bx = SCREEN_W - 6 - (3 - b) * 8;
          int16_t by = iy + _WC_ITEM_H - 3 - bh;
          gfx->fillRect(bx, by, 6, bh, b < bars ? GREEN : DARKGRAY);
        }
      }
    }

    // Scroll arrows
    if (scroll > 0) {
      gfx->setTextColor(YELLOW, BLACK);
      gfx->setCursor(SCREEN_W / 2 - 3, list_y + 2);
      gfx->print("^");
    }
    if (scroll + visible < total) {
      gfx->setTextColor(YELLOW, BLACK);
      gfx->setCursor(SCREEN_W / 2 - 3, SCREEN_H - 10);
      gfx->print("v");
    }
  };

  draw();

  int16_t tx, ty;
  uint32_t last_tap   = 0;
  int16_t  drag_start = -1;

  while (true) {
    if (!touch_read(&tx, &ty)) { drag_start = -1; continue; }

    uint32_t now = millis();

    // Capture drag start on first touch
    if (drag_start < 0) { drag_start = ty; continue; }

    int16_t dy = ty - drag_start;

    // Swipe detection (settle before treating as tap)
    if (abs(dy) > 24) {
      if (now - last_tap < 150) continue;
      last_tap = now;
      drag_start = -1;
      if (dy > 0 && scroll > 0)                { scroll--; draw(); }
      else if (dy < 0 && scroll + visible < total) { scroll++; draw(); }
      continue;
    }

    if (now - last_tap < 200) continue;
    last_tap  = now;
    drag_start = -1;

    if (ty < list_y) continue;
    int tapped = scroll + (ty - list_y) / _WC_ITEM_H;
    if (tapped >= total) continue;

    if (tapped == n) {
      // Custom — caller will enter SSID manually
      out_ssid[0] = '\0';
      return false;
    }

    strncpy(out_ssid, WiFi.SSID(tapped).c_str(), _WC_MAXLEN);
    out_ssid[_WC_MAXLEN] = '\0';
    return true;
  }
}

// ─── Password keyboard screen ─────────────────────────────────────────────────
// Two pages share the same 4-row layout so all keys stay within the safe touch
// area (row 3 centre ≈ y 195, well clear of the panel's unreliable bottom edge).
//
// Letters page (sym_page=false):
//   Row 0: q w e r t y u i o p          (shift → upper)
//   Row 1: a s d f g h j k l  [<-]
//   Row 2: z x c v b n m  [,] [.] [@]
//   Row 3: [?123] [  SPACE×5  ] [SFT] [EYE] [OK×2]
//
// Symbols page (sym_page=true):
//   Row 0: 1 2 3 4 5 6 7 8 9 0
//   Row 1: ! @ # $ % ^ & * ( )
//   Row 2: - _ = + / \ ; : ' "
//   Row 3: [ABC]  [  SPACE×5  ] [,]  [.]  [OK×2]

static const char _wc_L0[] = "qwertyuiop";
static const char _wc_L1[] = "asdfghjkl";   // 9 letters, col 9 = backspace
static const char _wc_L2[] = "zxcvbnm";     // 7 letters, cols 7-9 = , . @
static const char _wc_S0[] = "1234567890";
static const char _wc_S1[] = "!@#$%^&*()";
static const char _wc_S2[] = "-_=+/\\;:'\""; // 10 symbols

// Redraws both input fields.  active: 0=SSID, 1=PASS
static inline void _wc_fields(const char* ssid, const char* pass,
                               int active, bool show_pass) {
  for (int f = 0; f < 2; f++) {
    int16_t     fy  = f * _WC_FIELD_H;
    bool        act = (f == active);
    uint16_t    bg  = act ? DARKBLUE : DARKGRAY;
    const char* lbl = (f == 0) ? "SSID: " : "PASS: ";
    const char* val = (f == 0) ? ssid : pass;
    bool        sec = (f == 1 && !show_pass);

    gfx->fillRect(0, fy, SCREEN_W, _WC_FIELD_H, bg);
    gfx->setTextColor(WHITE, bg);
    gfx->setTextSize(1);
    gfx->setCursor(4, fy + (_WC_FIELD_H - 8) / 2);
    gfx->print(lbl);

    int16_t lbl_px = (int16_t)(strlen(lbl) * 6);
    int16_t avail  = (SCREEN_W - 4 - lbl_px - 6) / 6;
    size_t  len    = strlen(val);
    size_t  start  = (len > (size_t)avail) ? len - avail : 0;
    for (size_t i = start; i < len; i++) gfx->print(sec ? '*' : val[i]);
    if (act) gfx->print('_');
  }
}

static inline void _wc_keyboard(bool shifted, bool sym_page, bool show_pass) {
  int16_t ky = _WC_HDR_H;
  gfx->fillRect(0, ky, SCREEN_W, _WC_ROWS * _WC_ROW_H, BLACK);

  if (!sym_page) {
    // ── Letters page ──────────────────────────────────────────────────────
    for (int i = 0; i < 10; i++) {
      char c = shifted ? (char)toupper(_wc_L0[i]) : _wc_L0[i];
      char s[2] = { c, '\0' };
      _wc_key(i * _WC_KEY_W, ky, _WC_KEY_W, _WC_ROW_H, s, GRAY, WHITE);
    }
    ky += _WC_ROW_H;

    for (int i = 0; i < 9; i++) {
      char c = shifted ? (char)toupper(_wc_L1[i]) : _wc_L1[i];
      char s[2] = { c, '\0' };
      _wc_key(i * _WC_KEY_W, ky, _WC_KEY_W, _WC_ROW_H, s, GRAY, WHITE);
    }
    _wc_key(9 * _WC_KEY_W, ky, _WC_KEY_W, _WC_ROW_H, "<-", DARKGRAY, WHITE);
    ky += _WC_ROW_H;

    for (int i = 0; i < 7; i++) {
      char c = shifted ? (char)toupper(_wc_L2[i]) : _wc_L2[i];
      char s[2] = { c, '\0' };
      _wc_key(i * _WC_KEY_W, ky, _WC_KEY_W, _WC_ROW_H, s, GRAY, WHITE);
    }
    _wc_key(7 * _WC_KEY_W, ky, _WC_KEY_W, _WC_ROW_H, ",",  GRAY, WHITE);
    _wc_key(8 * _WC_KEY_W, ky, _WC_KEY_W, _WC_ROW_H, ".",  GRAY, WHITE);
    _wc_key(9 * _WC_KEY_W, ky, _WC_KEY_W, _WC_ROW_H, "@",  GRAY, WHITE);
    ky += _WC_ROW_H;

    // Row 3: [?123][SPACE×5][SFT][EYE][OK×2]
    _wc_key(0,               ky, _WC_KEY_W,     _WC_ROW_H, "?123", DARKGRAY, WHITE);
    _wc_key(_WC_KEY_W,       ky, _WC_KEY_W * 5, _WC_ROW_H, "SPACE", GRAY,   WHITE);
    _wc_key(_WC_KEY_W * 6,   ky, _WC_KEY_W,     _WC_ROW_H, shifted ? "ABC" : "abc",
                                                              shifted ? BLUE : DARKGRAY, WHITE);
    _wc_key(_WC_KEY_W * 7,   ky, _WC_KEY_W,     _WC_ROW_H, show_pass ? "***" : "...", DARKGRAY, WHITE);
    _wc_key(_WC_KEY_W * 8,   ky, _WC_KEY_W * 2, _WC_ROW_H, "OK",   GREEN,   BLACK);

  } else {
    // ── Symbols page ──────────────────────────────────────────────────────
    for (int i = 0; i < 10; i++) {
      char s[2] = { _wc_S0[i], '\0' };
      _wc_key(i * _WC_KEY_W, ky, _WC_KEY_W, _WC_ROW_H, s, GRAY, WHITE);
    }
    ky += _WC_ROW_H;

    for (int i = 0; i < 10; i++) {
      char s[2] = { _wc_S1[i], '\0' };
      _wc_key(i * _WC_KEY_W, ky, _WC_KEY_W, _WC_ROW_H, s, GRAY, WHITE);
    }
    ky += _WC_ROW_H;

    for (int i = 0; i < 10; i++) {
      char s[2] = { _wc_S2[i], '\0' };
      _wc_key(i * _WC_KEY_W, ky, _WC_KEY_W, _WC_ROW_H, s, GRAY, WHITE);
    }
    ky += _WC_ROW_H;

    // Row 3: [ABC][SPACE×5][,][.][OK×2]
    _wc_key(0,               ky, _WC_KEY_W,     _WC_ROW_H, "ABC",  DARKGRAY, WHITE);
    _wc_key(_WC_KEY_W,       ky, _WC_KEY_W * 5, _WC_ROW_H, "SPACE", GRAY,   WHITE);
    _wc_key(_WC_KEY_W * 6,   ky, _WC_KEY_W,     _WC_ROW_H, ",",    GRAY,    WHITE);
    _wc_key(_WC_KEY_W * 7,   ky, _WC_KEY_W,     _WC_ROW_H, ".",    GRAY,    WHITE);
    _wc_key(_WC_KEY_W * 8,   ky, _WC_KEY_W * 2, _WC_ROW_H, "OK",   GREEN,   BLACK);
  }
}

// Shows keyboard for ssid+pass.  ssid_ro=true means SSID came from scan (read-only).
// Returns true on OK, false on Back (return to AP list).
static inline bool _wc_pass_screen(char* ssid, char* pass, bool ssid_ro) {
  bool shifted   = false;
  bool sym_page  = false;
  bool show_pass = false;
  int  active    = ssid_ro ? 1 : 0;

  gfx->fillScreen(BLACK);
  _wc_fields(ssid, pass, active, show_pass);
  _wc_keyboard(shifted, sym_page, show_pass);

  int16_t  tx, ty;
  uint32_t last_tap = 0;

  while (true) {
    if (!touch_read(&tx, &ty)) continue;

    uint32_t now = millis();
    if (now - last_tap < 200) continue;
    last_tap = now;

    // ── Header: tap to switch active field ──────────────────────────────────
    if (ty < _WC_HDR_H) {
      int f = ty / _WC_FIELD_H;
      if (f == 0 && ssid_ro) continue;
      active = f;
      _wc_fields(ssid, pass, active, show_pass);
      continue;
    }

    // ── Keyboard hit ────────────────────────────────────────────────────────
    int row = constrain((ty - _WC_HDR_H) / _WC_ROW_H, 0, _WC_ROWS - 1);
    int col = constrain(tx / _WC_KEY_W,                0, 9);

    char* buf = (active == 0) ? ssid : pass;
    size_t len = strlen(buf);
    char ch = '\0';

    if (!sym_page) {
      // ── Letters page ────────────────────────────────────────────────────
      if (row == 0) {
        ch = shifted ? (char)toupper(_wc_L0[col]) : _wc_L0[col];

      } else if (row == 1) {
        if (col == 9) {
          if (len > 0) { buf[--len] = '\0'; _wc_fields(ssid, pass, active, show_pass); }
          continue;
        }
        ch = shifted ? (char)toupper(_wc_L1[col]) : _wc_L1[col];

      } else if (row == 2) {
        if      (col < 7) { ch = shifted ? (char)toupper(_wc_L2[col]) : _wc_L2[col]; }
        else if (col == 7) { ch = ','; }
        else if (col == 8) { ch = '.'; }
        else               { ch = '@'; }

      } else {
        // Row 3: [?123=0][SPACE=1-5][SFT=6][EYE=7][OK=8-9]
        if (col == 0) {
          sym_page = true;
          _wc_keyboard(shifted, sym_page, show_pass);
          continue;
        } else if (col >= 1 && col <= 5) {
          ch = ' ';
        } else if (col == 6) {
          shifted = !shifted;
          _wc_keyboard(shifted, sym_page, show_pass);
          continue;
        } else if (col == 7) {
          show_pass = !show_pass;
          _wc_fields(ssid, pass, active, show_pass);
          _wc_keyboard(shifted, sym_page, show_pass);
          continue;
        } else {
          if (ssid[0] == '\0') continue;
          return true;
        }
      }

    } else {
      // ── Symbols page ────────────────────────────────────────────────────
      if      (row == 0) { ch = _wc_S0[col]; }
      else if (row == 1) { ch = _wc_S1[col]; }
      else if (row == 2) { ch = _wc_S2[col]; }
      else {
        // Row 3: [ABC=0][SPACE=1-5][,=6][.=7][OK=8-9]
        if (col == 0) {
          sym_page = false;
          _wc_keyboard(shifted, sym_page, show_pass);
          continue;
        } else if (col >= 1 && col <= 5) {
          ch = ' ';
        } else if (col == 6) {
          ch = ',';
        } else if (col == 7) {
          ch = '.';
        } else {
          if (ssid[0] == '\0') continue;
          return true;
        }
      }
    }

    if (ch && len < _WC_MAXLEN) {
      buf[len]     = ch;
      buf[len + 1] = '\0';
      if (shifted && !sym_page) { shifted = false; _wc_keyboard(shifted, sym_page, show_pass); }
      _wc_fields(ssid, pass, active, show_pass);
    }
  }
}

// ─── Main entry point ─────────────────────────────────────────────────────────

static inline void wifi_config_screen() {
  char ssid[_WC_MAXLEN + 1] = {};
  char pass[_WC_MAXLEN + 1] = {};
  wifi_config_load(ssid, sizeof ssid, pass, sizeof pass);

  while (true) {
    // Step 1: scan & pick an AP (or "Enter custom...")
    char chosen[_WC_MAXLEN + 1] = {};
    strncpy(chosen, ssid, _WC_MAXLEN);

    bool from_scan = _wc_ap_select(chosen);

    // Step 2: enter/confirm password
    // Reuse stored password when the same SSID is re-selected
    char pw[_WC_MAXLEN + 1] = {};
    if (from_scan && strcmp(chosen, ssid) == 0) strncpy(pw, pass, _WC_MAXLEN);

    if (_wc_pass_screen(chosen, pw, from_scan)) {
      wifi_config_save(chosen, pw);
      return;
    }
    // Back → return to AP list
  }
}
