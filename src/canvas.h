#pragma once
// canvas.h — pre-configured Arduino_GFX display for your board.
//
// Usage:
//   #include "canvas.h"
//   void setup() { canvas_begin(); }
//   void loop()  { Display.fillScreen(BLACK); }
//
// The board is selected by a build flag in platformio.ini, e.g.:
//   build_flags = -DBOARD_M5CARDPUTER

#include <Arduino_GFX_Library.h>

// ─── Colour constants (RGB565) ────────────────────────────────────────────────
// Raylib palette converted to RGB565: ((R&0xF8)<<8)|((G&0xFC)<<3)|(B>>3)
#ifndef RGB565
  #define RGB565(r, g, b) (uint16_t)(((r & 0xF8u) << 8) | ((g & 0xFCu) << 3) | (b >> 3))
#endif
#ifndef LIGHTGRAY
  #define LIGHTGRAY   RGB565(200, 200, 200)
#endif
#ifndef GRAY
  #define GRAY        RGB565(130, 130, 130)
#endif
#ifndef DARKGRAY
  #define DARKGRAY    RGB565(80,  80,  80)
#endif
#ifndef YELLOW
  #define YELLOW      RGB565(253, 249, 0)
#endif
#ifndef GOLD
  #define GOLD        RGB565(255, 203, 0)
#endif
#ifndef ORANGE
  #define ORANGE      RGB565(255, 161, 0)
#endif
#ifndef PINK
  #define PINK        RGB565(255, 109, 194)
#endif
#ifndef RED
  #define RED         RGB565(230, 41,  55)
#endif
#ifndef MAROON
  #define MAROON      RGB565(190, 33,  55)
#endif
#ifndef GREEN
  #define GREEN       RGB565(0,   228, 48)
#endif
#ifndef LIME
  #define LIME        RGB565(0,   158, 47)
#endif
#ifndef DARKGREEN
  #define DARKGREEN   RGB565(0,   117, 44)
#endif
#ifndef SKYBLUE
  #define SKYBLUE     RGB565(102, 191, 255)
#endif
#ifndef BLUE
  #define BLUE        RGB565(0,   121, 241)
#endif
#ifndef DARKBLUE
  #define DARKBLUE    RGB565(0,   82,  172)
#endif
#ifndef PURPLE
  #define PURPLE      RGB565(200, 122, 255)
#endif
#ifndef VIOLET
  #define VIOLET      RGB565(135, 60,  190)
#endif
#ifndef DARKPURPLE
  #define DARKPURPLE  RGB565(112, 31,  126)
#endif
#ifndef BEIGE
  #define BEIGE       RGB565(211, 176, 131)
#endif
#ifndef BROWN
  #define BROWN       RGB565(127, 106, 79)
#endif
#ifndef DARKBROWN
  #define DARKBROWN   RGB565(76,  63,  47)
#endif
#ifndef WHITE
  #define WHITE       RGB565(255, 255, 255)
#endif
#ifndef BLACK
  #define BLACK       RGB565(0,   0,   0)
#endif
#ifndef MAGENTA
  #define MAGENTA     RGB565(255, 0,   255)
#endif
#ifndef CYAN
  #define CYAN        RGB565(0,   255, 255)
#endif
#ifndef RAYWHITE
  #define RAYWHITE    RGB565(245, 245, 245)
#endif

// ─── Board pin definitions ────────────────────────────────────────────────────

#if defined(BOARD_M5CARDPUTER) || defined(BOARD_M5CARDPUTER_ADV)
  // M5Cardputer (both revisions share the same display wiring)
  #define _CANVAS_DC        34
  #define _CANVAS_CS        37
  #define _CANVAS_SCK       36
  #define _CANVAS_MOSI      35
  #define _CANVAS_RST       33
  #define _CANVAS_BL        38
  #define _CANVAS_BL_ACTIVE HIGH  // HIGH turns backlight on
  #define _CANVAS_W         240   // post-rotation (landscape)
  #define _CANVAS_H         135
  #define _CANVAS_PHYS_W    135   // physical panel (portrait)
  #define _CANVAS_PHYS_H    240
  #define _CANVAS_ROTATION  1     // landscape
  #define _CANVAS_IPS       true
  #define _CANVAS_DRIVER    ST7789
  #define _CANVAS_COL_OFF1  52
  #define _CANVAS_ROW_OFF1  40
  #define _CANVAS_COL_OFF2  53
  #define _CANVAS_ROW_OFF2  40

#elif defined(BOARD_CYD)
  // Cheap Yellow Display — ESP32-2432S028 (2.8" 240×320 ILI9341)
  #define _CANVAS_DC        2
  #define _CANVAS_CS        15
  #define _CANVAS_SCK       14
  #define _CANVAS_MOSI      13
  #define _CANVAS_MISO      12
  #define _CANVAS_RST       -1
  #define _CANVAS_BL        21
  #define _CANVAS_BL_ACTIVE HIGH
  #define _CANVAS_W         320
  #define _CANVAS_H         240
  #define _CANVAS_ROTATION  1
  #define _CANVAS_IPS       false
  #define _CANVAS_DRIVER    ILI9341

#elif defined(BOARD_CYD_35)
  // Cheap Yellow Display — ESP32-3248S035 (3.5" 320×480 ST7796)
  #define _CANVAS_DC        2
  #define _CANVAS_CS        15
  #define _CANVAS_SCK       14
  #define _CANVAS_MOSI      13
  #define _CANVAS_MISO      12
  #define _CANVAS_RST       -1
  #define _CANVAS_BL        27
  #define _CANVAS_BL_ACTIVE HIGH
  #define _CANVAS_W         480
  #define _CANVAS_H         320
  #define _CANVAS_ROTATION  1
  #define _CANVAS_IPS       false
  #define _CANVAS_DRIVER    ST7796

#elif defined(BOARD_TDISPLAY_S3)
  // LilyGO T-Display S3
  #define _CANVAS_DC        9
  #define _CANVAS_CS        6
  #define _CANVAS_SCK       17
  #define _CANVAS_MOSI      15
  #define _CANVAS_RST       5
  #define _CANVAS_BL        38
  #define _CANVAS_BL_ACTIVE HIGH
  #define _CANVAS_W         320
  #define _CANVAS_H         170
  #define _CANVAS_ROTATION  1
  #define _CANVAS_IPS       true
  #define _CANVAS_DRIVER    ST7789

#else
  // Generic / custom — override these via -D build flags:
  //   -D_CANVAS_DC=x -D_CANVAS_CS=x -D_CANVAS_SCK=x -D_CANVAS_MOSI=x
  //   -D_CANVAS_RST=x -D_CANVAS_W=240 -D_CANVAS_H=135 -D_CANVAS_ROTATION=1
  #ifndef _CANVAS_DC
    #error "No board selected. Set BOARD_xxx in your platformio.ini build_flags, or define _CANVAS_DC etc. manually."
  #endif
  #ifndef _CANVAS_IPS
    #define _CANVAS_IPS false
  #endif
  #ifndef _CANVAS_BL_ACTIVE
    #define _CANVAS_BL_ACTIVE HIGH
  #endif
#endif

#ifndef _CANVAS_COL_OFF1
  #define _CANVAS_COL_OFF1 0
#endif
#ifndef _CANVAS_ROW_OFF1
  #define _CANVAS_ROW_OFF1 0
#endif
#ifndef _CANVAS_COL_OFF2
  #define _CANVAS_COL_OFF2 0
#endif
#ifndef _CANVAS_ROW_OFF2
  #define _CANVAS_ROW_OFF2 0
#endif
#ifndef _CANVAS_PHYS_W
  #define _CANVAS_PHYS_W _CANVAS_W
#endif
#ifndef _CANVAS_PHYS_H
  #define _CANVAS_PHYS_H _CANVAS_H
#endif

// ─── Public constants ─────────────────────────────────────────────────────────
#define SCREEN_W  _CANVAS_W
#define SCREEN_H  _CANVAS_H

// ─── Global display pointer (defined in canvas.h, safe with #pragma once) ────
static Arduino_DataBus* cnv_bus = nullptr;
static Arduino_GFX*     gfx    = nullptr;

// Convenience reference macro — use Display.xxx() just like M5Cardputer.Display
#define Display (*gfx)

// ─── canvas_begin() ───────────────────────────────────────────────────────────
// Call once in setup(). Initialises the bus, display, and backlight.
static inline void canvas_begin() {
#ifdef _CANVAS_MISO
  cnv_bus = new Arduino_ESP32SPI(
    _CANVAS_DC,
    _CANVAS_CS,
    _CANVAS_SCK,
    _CANVAS_MOSI,
    _CANVAS_MISO
  );
#else
  cnv_bus = new Arduino_ESP32SPI(
    _CANVAS_DC,
    _CANVAS_CS,
    _CANVAS_SCK,
    _CANVAS_MOSI
  );
#endif

#if defined(_CANVAS_DRIVER) && (_CANVAS_DRIVER == ILI9341)
  gfx = new Arduino_ILI9341(
    cnv_bus,
    _CANVAS_RST,
    _CANVAS_ROTATION
  );
#elif defined(_CANVAS_DRIVER) && (_CANVAS_DRIVER == ST7796)
  gfx = new Arduino_ST7796(
    cnv_bus,
    _CANVAS_RST,
    _CANVAS_ROTATION
  );
#else
  gfx = new Arduino_ST7789(
    cnv_bus,
    _CANVAS_RST,
    _CANVAS_ROTATION,
    _CANVAS_IPS,
    _CANVAS_PHYS_W,
    _CANVAS_PHYS_H,
    _CANVAS_COL_OFF1,
    _CANVAS_ROW_OFF1,
    _CANVAS_COL_OFF2,
    _CANVAS_ROW_OFF2
  );
#endif

  Serial.begin(115200);
  delay(500);
  if (!gfx->begin()) {
    Serial.println("canvas: display begin() FAILED");
  } else {
    Serial.println("canvas: display begin() OK");
    gfx->fillScreen(RED);
    delay(500);
    gfx->fillScreen(BLACK);
  }

#ifdef _CANVAS_BL
  pinMode(_CANVAS_BL, OUTPUT);
  digitalWrite(_CANVAS_BL, _CANVAS_BL_ACTIVE);
#endif
}
