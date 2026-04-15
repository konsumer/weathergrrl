#pragma once
// touch.h — board-agnostic touch input.
// Include after canvas.h (needs SCREEN_W / SCREEN_H).
// Call touch_begin() in setup() after canvas_begin().
//
// API:
//   touch_begin()                  — initialise touch hardware
//   touch_read(int16_t* x, *y)     — returns true while screen is pressed,
//                                    sets *x/*y to screen-space coordinates

// ─── CYD / CYD-35: XPT2046 resistive touch ───────────────────────────────────
#if defined(BOARD_CYD) || defined(BOARD_CYD_35)
  #include <SPI.h>
  #include <XPT2046_Touchscreen.h>

  // Pins (touch shares SPI bus with display, different CS)
  #define _TOUCH_CS   33
  #define _TOUCH_IRQ  36
  #define _TOUCH_SCK  14
  #define _TOUCH_MISO 12
  #define _TOUCH_MOSI 13

  // Raw ADC calibration — adjust if tap targets feel off
  #define _TOUCH_X_MIN  340
  #define _TOUCH_X_MAX  3860
  #define _TOUCH_Y_MIN  240
  #define _TOUCH_Y_MAX  3860

  static SPIClass            _touch_spi(VSPI);
  static XPT2046_Touchscreen _touch(_TOUCH_CS, _TOUCH_IRQ);

  static inline void touch_begin() {
    _touch_spi.begin(_TOUCH_SCK, _TOUCH_MISO, _TOUCH_MOSI, _TOUCH_CS);
    _touch.begin(_touch_spi);
    _touch.setRotation(1);
  }

  // Returns true while the screen is being pressed; maps raw to screen coords.
  static inline bool touch_read(int16_t* x, int16_t* y) {
    if (!_touch.touched()) return false;
    TS_Point p = _touch.getPoint();
    *x = (int16_t)map(p.x, _TOUCH_X_MIN, _TOUCH_X_MAX, 0, SCREEN_W - 1);
    *y = (int16_t)map(p.y, _TOUCH_Y_MIN, _TOUCH_Y_MAX, 0, SCREEN_H - 1);
    *x = constrain(*x, 0, SCREEN_W - 1);
    *y = constrain(*y, 0, SCREEN_H - 1);
    return true;
  }

// ─── Fallback stub for boards without touch ───────────────────────────────────
#else
  static inline void touch_begin() {}
  static inline bool touch_read(int16_t*, int16_t*) { return false; }
#endif
