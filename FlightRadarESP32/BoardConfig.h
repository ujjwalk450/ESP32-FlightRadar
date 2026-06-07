#pragma once
#include <TFT_eSPI.h>

//====================================================
// BOARD SELECTION
//====================================================
#define BOARD_ESP32_DEVKIT_V1 1
#define BOARD_ESP32S3_CUSTOM  2

#define SELECTED_BOARD BOARD_ESP32_DEVKIT_V1

#if SELECTED_BOARD == BOARD_ESP32_DEVKIT_V1
  #define TFT_BL 22
#elif SELECTED_BOARD == BOARD_ESP32S3_CUSTOM
  #define TFT_BL 15
#else
  #define TFT_BL 22
#endif

//====================================================
// DISPLAY GEOMETRY
//====================================================
static const int SCREEN_W = 240;
static const int SCREEN_H = 240;

static const int ROUND_CX = 120;
static const int ROUND_CY = 120;
static const int ROUND_R  = 119;

static const int RADAR_CX = 120;
static const int RADAR_CY = 120;
static const int RADAR_R  = 108;

static const int INFO_CX = 120;
static const int INFO_Y1 = 162;
static const int INFO_Y2 = 178;
static const int INFO_Y3 = 192;

//====================================================
// COLORS
//====================================================
static const uint16_t C_BG              = TFT_BLACK;
static const uint16_t C_GRID            = 0x0320;
static const uint16_t C_GRID_DIM        = 0x0180;
static const uint16_t C_SWEEP           = TFT_GREEN;
static const uint16_t C_ACTIVE          = TFT_CYAN;
static const uint16_t C_INACTIVE        = TFT_GREEN;
static const uint16_t C_TEXT            = TFT_WHITE;
static const uint16_t C_TEXT_DIM        = 0xBDF7;
static const uint16_t C_WARN            = TFT_ORANGE;
static const uint16_t C_OUTER_RING      = 0x03E0;
static const uint16_t C_AIRPORT_MAJOR   = TFT_RED;
static const uint16_t C_AIRPORT_OTHER   = 0xFD20;
static const uint16_t C_AIRPORT_LABEL   = 0xA3C0;
static const uint16_t C_INFO_TEXT_DIM   = 0x9CD3;

static const uint16_t C_TIME_HOUR       = TFT_MAGENTA;
static const uint16_t C_TIME_MIN        = 0xB81F;

static const uint16_t C_STAR            = TFT_WHITE;
static const uint16_t C_STAR_MAJOR      = TFT_WHITE;
static const uint16_t C_STAR_LINE       = 0x632D;

static const uint16_t C_SUN             = 0xFDC0;
static const uint16_t C_MOON            = 0xE71C;
static const uint16_t C_MOON_OUTLINE    = 0xBDF7;
