#pragma once
#include "Types.h"

//====================================================
// LOCATION MODE
//====================================================
#define LOCATION_MODE_FIXED 1
#define LOCATION_MODE_GPS   2

#define LOCATION_MODE LOCATION_MODE_FIXED

//====================================================
// DEFAULT / FIXED LOCATION
//====================================================
static const double DEFAULT_USER_LAT = 28.494106080147343;
static const double DEFAULT_USER_LON = 77.13798165111254;

// India = UTC +05:30 = 19800 seconds
static const long DEFAULT_GMT_OFFSET_SEC = 19800;
static const int DEFAULT_DAYLIGHT_OFFSET_SEC = 0;
static const float DEFAULT_LOCAL_TIMEZONE_HOURS = 5.5f;

//====================================================
// GPS CONFIGURATION
//====================================================
#define ENABLE_GPS 0
#define GPS_RX_PIN 16
#define GPS_TX_PIN 17
#define GPS_BAUD   9600

// If enabled, GPS can update location and UTC time.
// You should still set DEFAULT_GMT_OFFSET_SEC / DEFAULT_LOCAL_TIMEZONE_HOURS manually.
#define GPS_REQUIRE_FIX_BEFORE_FETCH 0

//====================================================
// WIFI PORTAL
//====================================================
#define WIFI_PORTAL_SSID "FlightRadarESP32"
#define WIFI_PORTAL_TIMEOUT_SEC 180
#define WIFI_CONNECT_TIMEOUT_SEC 30

//====================================================
// DEBUG
//====================================================
#define DEBUG_SERIAL 0

//====================================================
// CONSTELLATION DISPLAY
//====================================================
#define CONSTELLATION_AUTO_NIGHT 0
#define CONSTELLATION_ALWAYS     1
#define CONSTELLATION_OFF        2

// Default is ALWAYS so users can immediately verify the layer works.
// Change to CONSTELLATION_AUTO_NIGHT if you only want it after local sunset.
#define CONSTELLATION_MODE CONSTELLATION_AUTO_NIGHT


//====================================================
// NETWORK / MEMORY CONFIGURATION
//====================================================
// Classic ESP32 may need this enabled if HTTPS fails.
// This low-memory build keeps it disabled so the display does not flicker to FETCHING every update.
#define FREE_DISPLAY_SPRITE_DURING_HTTPS 0

//====================================================
// AIRCRAFT / RADAR CONFIGURATION
//====================================================
#ifndef MAX_AIRCRAFT
#define MAX_AIRCRAFT 25
#endif
#ifndef MAX_TRAIL_POINTS
#define MAX_TRAIL_POINTS 16
#endif

static const uint32_t DISPLAY_INTERVAL_MS = 33;
static const uint32_t TRAIL_INTERVAL_MS = 1000;    // 16 points x 1000 ms = around 16 seconds trail history
static const uint32_t INFO_TEMPLATE_INTERVAL_MS = 5000;
static const uint32_t SELECT_SWITCH_INTERVAL_MS = 10000;
static const uint32_t MAP_ZOOM_INTERVAL_MS = 40000;

static const uint32_t ADSB_UPDATE_INTERVAL_MS = 10000;
static const uint32_t OPENSKY_UPDATE_INTERVAL_MS = 30000;
static const uint32_t ADSB_RETRY_INTERVAL_MS = 60000;

static const int ADSB_RADIUS_NM = 135;
static const double OPENSKY_BOX_DEG = 2.35;

static const size_t ADSB_JSON_DOC_BYTES = 32768;
static const size_t OPENSKY_JSON_DOC_BYTES = 49152;

static const int ZOOM_LEVEL_COUNT = 10;
static const float ZOOM_LEVELS_KM[ZOOM_LEVEL_COUNT] = {
  25.0f, 50.0f, 100.0f, 150.0f, 200.0f,
  250.0f, 200.0f, 150.0f, 100.0f, 50.0f
};

//====================================================
// API CONFIGURATION
//====================================================
static const char ADSB_BASE_URL[] = "https://api.adsb.lol/v2/point/";
static const char OPENSKY_BASE_URL[] = "https://opensky-network.org/api/states/all";

//====================================================
// USER AIRPORT MARKERS
//====================================================
// Edit these for your own city. Keep around 3-8 markers for readability.
static const AirportMarker USER_AIRPORTS[] = {
  { "IGI", 28.556162f, 77.100281f, true,    0,   0 },
  { "AGR", 27.155800f, 77.960900f, false,  20,  10 },
  { "DED", 30.189700f, 78.180300f, false,   0,   0 },
  { "JAI", 26.824200f, 75.812200f, false,   0,   0 },
  { "IXC", 30.673500f, 76.788500f, false,   0,   0 }
};

static const int USER_AIRPORT_COUNT = sizeof(USER_AIRPORTS) / sizeof(USER_AIRPORTS[0]);
