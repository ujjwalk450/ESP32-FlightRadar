//====================================================
// FlightRadarESP32.ino
//====================================================
//
// TFT_eSPI User_Setup.h MUST contain:
//
// #define GC9A01_DRIVER
//
// #define TFT_WIDTH 240
// #define TFT_HEIGHT 240
//
// #define TFT_MOSI 23
// #define TFT_SCLK 18
//
// #define TFT_CS   5
// #define TFT_DC   27
// #define TFT_RST  33
//
// #define LOAD_GLCD
// #define LOAD_FONT2
// #define LOAD_FONT4
// #define LOAD_FONT6
// #define LOAD_FONT7
// #define LOAD_FONT8
//
// #define SPI_FREQUENCY 40000000
//
// Display wiring:
// GC9A01 VCC -> ESP32 3V3
// GC9A01 GND -> ESP32 GND
// GC9A01 DIN -> ESP32 GPIO23
// GC9A01 CLK -> ESP32 GPIO18
// GC9A01 CS  -> ESP32 GPIO5
// GC9A01 DC  -> ESP32 GPIO27
// GC9A01 RST -> ESP32 GPIO33
// GC9A01 BL  -> ESP32 GPIO22
//
//====================================================
// LIBRARIES
//====================================================
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include <math.h>
#include <ctype.h>
#include <time.h>

//====================================================
// HARDWARE CONSTANTS
//====================================================
#define TFT_BL 22

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
// USER LOCATION
//====================================================
static const double USER_LAT = 28.494106080147343;
static const double USER_LON = 77.13798165111254;

//====================================================
// TIME CONFIGURATION
//====================================================
static const long GMT_OFFSET_SEC = 19800;
static const int DAYLIGHT_OFFSET_SEC = 0;
static const float LOCAL_TIMEZONE_HOURS = 5.5f;

static bool timeConfigured = false;

//====================================================
// AIRCRAFT CONSTANTS
//====================================================
static const int MAX_AIRCRAFT = 25;
static const int MAX_TRAIL_POINTS = 40;

static const uint32_t DISPLAY_INTERVAL_MS = 33;
static const uint32_t TRAIL_INTERVAL_MS = 500;

static const uint32_t INFO_TEMPLATE_INTERVAL_MS = 5000;
static const uint32_t SELECT_SWITCH_INTERVAL_MS = 10000;
static const uint32_t MAP_ZOOM_INTERVAL_MS = 40000;

static const uint32_t ADSB_UPDATE_INTERVAL_MS = 10000;
static const uint32_t OPENSKY_UPDATE_INTERVAL_MS = 30000;
static const uint32_t ADSB_RETRY_INTERVAL_MS = 60000;

static const int ADSB_RADIUS_NM = 135;
static const double OPENSKY_BOX_DEG = 2.35;

//====================================================
// ZOOM LEVELS
//====================================================
static const int ZOOM_LEVEL_COUNT = 10;

static const float ZOOM_LEVELS_KM[ZOOM_LEVEL_COUNT] = {
  25.0f,
  50.0f,
  100.0f,
  150.0f,
  200.0f,
  250.0f,
  200.0f,
  150.0f,
  100.0f,
  50.0f
};

//====================================================
// API CONSTANTS
//====================================================
static const char ADSB_BASE_URL[] = "https://api.adsb.lol/v2/point/";
static const char OPENSKY_BASE_URL[] = "https://opensky-network.org/api/states/all";

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
static const uint16_t C_STAR_LINE       = 0x632d;

static const uint16_t C_SUN             = 0xFDC0;
static const uint16_t C_MOON            = 0xE71C;
static const uint16_t C_MOON_OUTLINE    = 0xBDF7;

//====================================================
// AIRPORT MARKERS
//====================================================
struct AirportMarker {
  const char *code;
  float lat;
  float lon;
  bool primary;
  int8_t drawOffsetX;
  int8_t drawOffsetY;
};

static const AirportMarker AIRPORTS[] = {
  { "IGI", 28.556162f, 77.100281f, true,    0,   0 },
  { "AGR", 27.155800f, 77.960900f, false,  20,   10 },
  { "DED", 30.189700f, 78.180300f, false,   0,   0 },
  { "JAI", 26.824200f, 75.812200f, false,   0,   0 },
  { "IXC", 30.673500f, 76.788500f, false,   0,   0 }
};

static const int AIRPORT_COUNT = sizeof(AIRPORTS) / sizeof(AIRPORTS[0]);

//====================================================
// CONSTELLATION DATA
//====================================================
struct StarPoint {
  int8_t x;
  int8_t y;
  bool major;
};

struct ConstellationPattern {
  const char *name;
  const StarPoint *stars;
  uint8_t count;
};

static const StarPoint CONST_ORION[] = {
  { -18, -18, true  },
  {  18, -20, true  },
  {  -8,  -2, false },
  {   0,   0, true  },
  {   8,   2, false },
  { -15,  20, true  },
  {  16,  18, true  }
};

static const StarPoint CONST_DIPPER[] = {
  { -24,   8, true  },
  { -12,   1, true  },
  {   0,   3, true  },
  {  11,  11, true  },
  {  21,   2, true  },
  {  13, -11, true  },
  {   2,  -9, true  }
};

static const StarPoint CONST_CASS[] = {
  { -24,   4, true  },
  { -12, -10, true  },
  {   0,   0, true  },
  {  13, -13, true  },
  {  25,   1, true  }
};

static const StarPoint CONST_CYGNUS[] = {
  {   0, -28, true  },
  {   0, -12, false },
  {   0,   4, true  },
  {   0,  22, true  },
  { -19,   1, true  },
  {  20,   1, true  }
};

static const ConstellationPattern CONSTELLATIONS[] = {
  { "ORION",  CONST_ORION,  sizeof(CONST_ORION)  / sizeof(CONST_ORION[0])  },
  { "DIPPER", CONST_DIPPER, sizeof(CONST_DIPPER) / sizeof(CONST_DIPPER[0]) },
  { "CASS",   CONST_CASS,   sizeof(CONST_CASS)   / sizeof(CONST_CASS[0])   },
  { "CYGNUS", CONST_CYGNUS, sizeof(CONST_CYGNUS) / sizeof(CONST_CYGNUS[0]) }
};

static const int CONSTELLATION_COUNT = sizeof(CONSTELLATIONS) / sizeof(CONSTELLATIONS[0]);

static const int CONST_POSITIONS[][2] = {
  { 58,  68  },
  { 176, 72  },
  { 62,  166 },
  { 178, 158 },
  { 120, 70  },
  { 120, 176 }
};

static const int CONST_POSITION_COUNT = sizeof(CONST_POSITIONS) / sizeof(CONST_POSITIONS[0]);

//====================================================
// DATA STRUCTURES
//====================================================
struct TrailPoint {
  float lat;
  float lon;
  float distanceKm;
  float bearingDeg;
  bool valid;
};

struct Aircraft {
  char icao[9];
  char callsign[12];
  char model[22];
  char routeFrom[6];
  char routeTo[6];

  float lat;
  float lon;

  float headingDeg;
  float speedKts;
  int altitudeFt;

  float distanceKm;
  float bearingDeg;

  bool valid;

  TrailPoint trail[MAX_TRAIL_POINTS];
  int trailHead;
  int trailCount;
};

struct SunData {
  float azimuthDeg;
  float elevationDeg;
};

struct DrawZone {
  int x;
  int y;
  int r;
};

//====================================================
// GLOBAL OBJECTS
//====================================================
TFT_eSPI tft = TFT_eSPI();
TFT_eSprite sprite = TFT_eSprite(&tft);
WiFiManager wifiManager;

SemaphoreHandle_t aircraftMutex = NULL;

//====================================================
// GLOBAL AIRCRAFT BUFFERS
//====================================================
static Aircraft aircraft[MAX_AIRCRAFT];
static Aircraft fetchAircraft[MAX_AIRCRAFT];
static Aircraft renderAircraft[MAX_AIRCRAFT];

static int aircraftCount = 0;
static int renderAircraftCount = 0;
static int renderVisibleAircraftCount = 0;

static int selectedIndex = -1;
static int renderSelectedIndex = -1;

static float radarRangeKm = 25.0f;
static float renderRadarRangeKm = 25.0f;

static int mapZoomIndex = 0;

static volatile bool usingOpenSky = false;
static volatile bool apiHasEverSucceeded = false;

//====================================================
// TIMING STATE
//====================================================
static uint32_t lastDisplayMs = 0;
static uint32_t lastTrailMs = 0;
static uint32_t lastSelectionSwitchMs = 0;
static uint32_t lastMapZoomMs = 0;

static float sweepAngleDeg = 0.0f;

//====================================================
// UTILITY FUNCTIONS
//====================================================
static void safeCopy(char *dst, size_t dstSize, const char *src) {
  if (!dst || dstSize == 0) return;

  if (!src) {
    dst[0] = '\0';
    return;
  }

  strncpy(dst, src, dstSize - 1);
  dst[dstSize - 1] = '\0';
}

static void trimText(char *s) {
  if (!s) return;

  int len = strlen(s);

  while (len > 0) {
    char c = s[len - 1];

    if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
      s[len - 1] = '\0';
      len--;
    } else {
      break;
    }
  }

  int start = 0;

  while (s[start] == ' ' || s[start] == '\t') {
    start++;
  }

  if (start > 0) {
    memmove(s, s + start, strlen(s + start) + 1);
  }
}

static bool textIsEmpty(const char *s) {
  if (!s) return true;

  while (*s) {
    if (*s != ' ' && *s != '\t' && *s != '\r' && *s != '\n') {
      return false;
    }

    s++;
  }

  return true;
}

static double degToRad(double deg) {
  return deg * PI / 180.0;
}

static double radToDeg(double rad) {
  return rad * 180.0 / PI;
}

static float normalizeAngle(float deg) {
  while (deg < 0.0f) deg += 360.0f;
  while (deg >= 360.0f) deg -= 360.0f;
  return deg;
}

static double haversineKm(double lat1, double lon1, double lat2, double lon2) {
  static const double EARTH_RADIUS_KM = 6371.0;

  double dLat = degToRad(lat2 - lat1);
  double dLon = degToRad(lon2 - lon1);

  double a =
    sin(dLat * 0.5) * sin(dLat * 0.5) +
    cos(degToRad(lat1)) *
    cos(degToRad(lat2)) *
    sin(dLon * 0.5) *
    sin(dLon * 0.5);

  double c = 2.0 * atan2(sqrt(a), sqrt(1.0 - a));

  return EARTH_RADIUS_KM * c;
}

static float bearingDeg(double lat1, double lon1, double lat2, double lon2) {
  double p1 = degToRad(lat1);
  double p2 = degToRad(lat2);
  double dLon = degToRad(lon2 - lon1);

  double y = sin(dLon) * cos(p2);
  double x =
    cos(p1) * sin(p2) -
    sin(p1) * cos(p2) * cos(dLon);

  return normalizeAngle((float)radToDeg(atan2(y, x)));
}

static uint16_t blend565(uint16_t c1, uint16_t c2, uint8_t amount) {
  uint8_t r1 = (c1 >> 11) & 0x1F;
  uint8_t g1 = (c1 >> 5) & 0x3F;
  uint8_t b1 = c1 & 0x1F;

  uint8_t r2 = (c2 >> 11) & 0x1F;
  uint8_t g2 = (c2 >> 5) & 0x3F;
  uint8_t b2 = c2 & 0x1F;

  uint8_t r = (r1 * (255 - amount) + r2 * amount) / 255;
  uint8_t g = (g1 * (255 - amount) + g2 * amount) / 255;
  uint8_t b = (b1 * (255 - amount) + b2 * amount) / 255;

  return (r << 11) | (g << 5) | b;
}

static void formatAltitudeShort(int altitudeFt, char *out, size_t outSize) {
  if (!out || outSize == 0) return;

  if (altitudeFt <= 0) {
    snprintf(out, outSize, "--ft");
  } else if (altitudeFt >= 10000) {
    snprintf(out, outSize, "%.0fkft", altitudeFt / 1000.0f);
  } else {
    snprintf(out, outSize, "%dft", altitudeFt);
  }
}

static void uppercaseText(char *s) {
  if (!s) return;

  while (*s) {
    *s = (char)toupper((unsigned char)*s);
    s++;
  }
}

static bool hasUsefulText(const char *s) {
  if (textIsEmpty(s)) return false;
  if (strcmp(s, "--") == 0) return false;
  if (strcmp(s, "MODEL --") == 0) return false;
  if (strcmp(s, "TYPE --") == 0) return false;
  return true;
}

//====================================================
// TIME FUNCTIONS
//====================================================
static void setupTimeSync() {
  configTime(
    GMT_OFFSET_SEC,
    DAYLIGHT_OFFSET_SEC,
    "pool.ntp.org",
    "time.nist.gov",
    "time.google.com"
  );

  timeConfigured = true;
}

static bool getLocalClock(struct tm &timeinfo) {
  if (!timeConfigured) return false;
  return getLocalTime(&timeinfo, 10);
}

static bool isNightTime() {
  struct tm timeinfo;

  if (!getLocalClock(timeinfo)) {
    return false;
  }

  return timeinfo.tm_hour >= 19 || timeinfo.tm_hour < 5;
}

static int getFiveMinuteSlot() {
  struct tm timeinfo;

  if (!getLocalClock(timeinfo)) {
    return (millis() / 300000UL);
  }

  return (timeinfo.tm_hour * 12) + (timeinfo.tm_min / 5);
}

static float getFastSweepAngle() {
  uint32_t ms = millis() % 1000;
  return normalizeAngle((float)ms * 0.36f);
}

//====================================================
// SUN AND MOON FUNCTIONS
//====================================================
static bool getSunData(SunData &sun) {
  struct tm timeinfo;

  if (!getLocalClock(timeinfo)) {
    return false;
  }

  int dayOfYear = timeinfo.tm_yday + 1;

  float hourDecimal =
    (float)timeinfo.tm_hour +
    ((float)timeinfo.tm_min / 60.0f) +
    ((float)timeinfo.tm_sec / 3600.0f);

  float gamma =
    (2.0f * PI / 365.0f) *
    ((float)dayOfYear - 1.0f + ((hourDecimal - 12.0f) / 24.0f));

  float eqTime =
    229.18f *
    (
      0.000075f +
      0.001868f * cos(gamma) -
      0.032077f * sin(gamma) -
      0.014615f * cos(2.0f * gamma) -
      0.040849f * sin(2.0f * gamma)
    );

  float decl =
    0.006918f -
    0.399912f * cos(gamma) +
    0.070257f * sin(gamma) -
    0.006758f * cos(2.0f * gamma) +
    0.000907f * sin(2.0f * gamma) -
    0.002697f * cos(3.0f * gamma) +
    0.001480f * sin(3.0f * gamma);

  float timeOffset = eqTime + (4.0f * (float)USER_LON) - (60.0f * LOCAL_TIMEZONE_HOURS);
  float trueSolarTime = (hourDecimal * 60.0f) + timeOffset;

  while (trueSolarTime < 0.0f) trueSolarTime += 1440.0f;
  while (trueSolarTime >= 1440.0f) trueSolarTime -= 1440.0f;

  float hourAngleDeg = (trueSolarTime / 4.0f) - 180.0f;
  float hourAngleRad = degToRad(hourAngleDeg);

  float latRad = degToRad(USER_LAT);

  float cosZenith =
    sin(latRad) * sin(decl) +
    cos(latRad) * cos(decl) * cos(hourAngleRad);

  if (cosZenith > 1.0f) cosZenith = 1.0f;
  if (cosZenith < -1.0f) cosZenith = -1.0f;

  float zenithRad = acos(cosZenith);

  sun.elevationDeg = 90.0f - (float)radToDeg(zenithRad);

  float azRad =
    atan2(
      sin(hourAngleRad),
      (cos(hourAngleRad) * sin(latRad)) - (tan(decl) * cos(latRad))
    );

  sun.azimuthDeg = normalizeAngle((float)radToDeg(azRad) + 180.0f);

  return true;
}

static bool getSolarEventsForDayOffset(int dayOffset, float &sunriseMin, float &sunsetMin) {
  struct tm timeinfo;

  if (!getLocalClock(timeinfo)) {
    return false;
  }

  int dayOfYear = timeinfo.tm_yday + 1 + dayOffset;

  while (dayOfYear < 1) {
    dayOfYear += 365;
  }

  while (dayOfYear > 365) {
    dayOfYear -= 365;
  }

  float gamma =
    (2.0f * PI / 365.0f) *
    ((float)dayOfYear - 1.0f);

  float eqTime =
    229.18f *
    (
      0.000075f +
      0.001868f * cos(gamma) -
      0.032077f * sin(gamma) -
      0.014615f * cos(2.0f * gamma) -
      0.040849f * sin(2.0f * gamma)
    );

  float decl =
    0.006918f -
    0.399912f * cos(gamma) +
    0.070257f * sin(gamma) -
    0.006758f * cos(2.0f * gamma) +
    0.000907f * sin(2.0f * gamma) -
    0.002697f * cos(3.0f * gamma) +
    0.001480f * sin(3.0f * gamma);

  float latRad = degToRad(USER_LAT);
  float zenithRad = degToRad(90.833f);

  float haArg =
    (cos(zenithRad) / (cos(latRad) * cos(decl))) -
    (tan(latRad) * tan(decl));

  if (haArg < -1.0f) haArg = -1.0f;
  if (haArg > 1.0f) haArg = 1.0f;

  float hourAngleDeg = (float)radToDeg(acos(haArg));

  float solarNoonMin =
    720.0f -
    (4.0f * (float)USER_LON) -
    eqTime +
    (60.0f * LOCAL_TIMEZONE_HOURS);

  sunriseMin = solarNoonMin - (4.0f * hourAngleDeg);
  sunsetMin  = solarNoonMin + (4.0f * hourAngleDeg);

  while (sunriseMin < 0.0f) sunriseMin += 1440.0f;
  while (sunriseMin >= 1440.0f) sunriseMin -= 1440.0f;

  while (sunsetMin < 0.0f) sunsetMin += 1440.0f;
  while (sunsetMin >= 1440.0f) sunsetMin -= 1440.0f;

  return true;
}

static bool getNightProgress01(float &progress) {
  struct tm timeinfo;

  if (!getLocalClock(timeinfo)) {
    return false;
  }

  float nowMin =
    ((float)timeinfo.tm_hour * 60.0f) +
    (float)timeinfo.tm_min +
    ((float)timeinfo.tm_sec / 60.0f);

  float sunriseToday;
  float sunsetToday;

  if (!getSolarEventsForDayOffset(0, sunriseToday, sunsetToday)) {
    return false;
  }

  if (nowMin >= sunsetToday) {
    float sunriseTomorrow;
    float sunsetTomorrow;

    if (!getSolarEventsForDayOffset(1, sunriseTomorrow, sunsetTomorrow)) {
      return false;
    }

    float nightLength = (1440.0f - sunsetToday) + sunriseTomorrow;

    if (nightLength <= 1.0f) {
      return false;
    }

    progress = (nowMin - sunsetToday) / nightLength;
    return true;
  }

  if (nowMin < sunriseToday) {
    float sunriseYesterday;
    float sunsetYesterday;

    if (!getSolarEventsForDayOffset(-1, sunriseYesterday, sunsetYesterday)) {
      return false;
    }

    float nightLength = (1440.0f - sunsetYesterday) + sunriseToday;

    if (nightLength <= 1.0f) {
      return false;
    }

    progress = ((1440.0f - sunsetYesterday) + nowMin) / nightLength;
    return true;
  }

  return false;
}

static float getMoonPhase01() {
  time_t nowUtc = time(nullptr);

  if (nowUtc < 100000) {
    return 0.0f;
  }

  const double SYNODIC_MONTH = 29.53058867;
  const time_t KNOWN_NEW_MOON_UTC = 947182440;

  double days = (double)(nowUtc - KNOWN_NEW_MOON_UTC) / 86400.0;
  double phase = fmod(days, SYNODIC_MONTH);

  if (phase < 0.0) {
    phase += SYNODIC_MONTH;
  }

  return (float)(phase / SYNODIC_MONTH);
}

static float getEstimatedMoonAzimuthFromNightPath() {
  float progress = 0.0f;

  if (!getNightProgress01(progress)) {
    return 180.0f;
  }

  if (progress < 0.0f) progress = 0.0f;
  if (progress > 1.0f) progress = 1.0f;

  float phase = getMoonPhase01();
  float phaseOffset = (phase - 0.5f) * 24.0f;
  float azimuth = 90.0f + (progress * 180.0f) + phaseOffset;

  return normalizeAngle(azimuth);
}

//====================================================
// JSON TEXT HELPERS
//====================================================
static const char *jsonTextOrEmpty(JsonObject obj, const char *key) {
  if (!key) return "";

  JsonVariant v = obj[key];

  if (v.isNull()) return "";

  const char *s = v.as<const char *>();

  if (!s) return "";

  return s;
}

static const char *firstJsonText(
  JsonObject obj,
  const char *k1,
  const char *k2,
  const char *k3,
  const char *k4
) {
  const char *s = jsonTextOrEmpty(obj, k1);
  if (!textIsEmpty(s)) return s;

  s = jsonTextOrEmpty(obj, k2);
  if (!textIsEmpty(s)) return s;

  s = jsonTextOrEmpty(obj, k3);
  if (!textIsEmpty(s)) return s;

  s = jsonTextOrEmpty(obj, k4);
  if (!textIsEmpty(s)) return s;

  return "";
}

static void parseRouteCodes(
  const char *route,
  char *from,
  size_t fromSize,
  char *to,
  size_t toSize
) {
  if (from && fromSize > 0) from[0] = '\0';
  if (to && toSize > 0) to[0] = '\0';

  if (!route || !route[0]) return;

  char token[8];
  int tokenLen = 0;
  int found = 0;

  for (int i = 0; ; i++) {
    char c = route[i];

    bool isCodeChar = isalnum((unsigned char)c);

    if (isCodeChar && tokenLen < 5) {
      token[tokenLen++] = (char)toupper((unsigned char)c);
    }

    if (!isCodeChar || c == '\0') {
      if (tokenLen >= 2) {
        token[tokenLen] = '\0';

        if (found == 0) {
          safeCopy(from, fromSize, token);
          found++;
        } else if (found == 1) {
          safeCopy(to, toSize, token);
          found++;
          break;
        }
      }

      tokenLen = 0;
    }

    if (c == '\0') {
      break;
    }
  }
}

static void buildModelText(JsonObject item, char *out, size_t outSize) {
  if (!out || outSize == 0) return;

  out[0] = '\0';

  const char *desc = firstJsonText(item, "desc", "description", "model", "aircraft");
  const char *type = firstJsonText(item, "t", "type", "aircraft_type", "typeDesignator");
  const char *reg  = firstJsonText(item, "r", "reg", "registration", "tail");

  if (!textIsEmpty(desc)) {
    safeCopy(out, outSize, desc);
  } else if (!textIsEmpty(type)) {
    safeCopy(out, outSize, type);
  } else if (!textIsEmpty(reg)) {
    safeCopy(out, outSize, reg);
  } else {
    safeCopy(out, outSize, "");
  }

  trimText(out);
  uppercaseText(out);
}

//====================================================
// AIRCRAFT MEMORY FUNCTIONS
//====================================================
static void clearAircraft(Aircraft &a) {
  memset(&a, 0, sizeof(Aircraft));
  a.valid = false;
  a.trailHead = 0;
  a.trailCount = 0;

  safeCopy(a.model, sizeof(a.model), "");
  safeCopy(a.routeFrom, sizeof(a.routeFrom), "--");
  safeCopy(a.routeTo, sizeof(a.routeTo), "--");

  for (int i = 0; i < MAX_TRAIL_POINTS; i++) {
    a.trail[i].valid = false;
  }
}

static void clearAllAircraftArrays() {
  for (int i = 0; i < MAX_AIRCRAFT; i++) {
    clearAircraft(aircraft[i]);
    clearAircraft(fetchAircraft[i]);
    clearAircraft(renderAircraft[i]);
  }

  aircraftCount = 0;
  renderAircraftCount = 0;
  renderVisibleAircraftCount = 0;
  selectedIndex = -1;
  renderSelectedIndex = -1;
  radarRangeKm = ZOOM_LEVELS_KM[0];
  renderRadarRangeKm = ZOOM_LEVELS_KM[0];
  mapZoomIndex = 0;
}

static int findAircraftByIcao(const Aircraft *arr, int count, const char *icao) {
  if (!icao || !icao[0]) return -1;

  for (int i = 0; i < count; i++) {
    if (arr[i].valid && strcmp(arr[i].icao, icao) == 0) {
      return i;
    }
  }

  return -1;
}

static bool isAircraftVisibleInRange(const Aircraft &a, float rangeKm) {
  return a.valid && a.distanceKm <= rangeKm;
}

static bool isAircraftInInnerClutterZone(const Aircraft &a, float rangeKm) {
  if (!a.valid) return false;
  if (rangeKm <= 50.0f) return false;

  float innerLimitKm = rangeKm / 5.0f;

  return a.distanceKm <= innerLimitKm;
}

static bool selectedAircraftIsInnerClutter() {
  if (renderSelectedIndex < 0 || renderSelectedIndex >= renderAircraftCount) {
    return false;
  }

  return isAircraftInInnerClutterZone(renderAircraft[renderSelectedIndex], renderRadarRangeKm);
}

static bool shouldDrawAircraftIndex(int index) {
  if (index < 0 || index >= renderAircraftCount) return false;

  Aircraft &a = renderAircraft[index];

  if (!isAircraftVisibleInRange(a, renderRadarRangeKm)) return false;

  if (!isAircraftInInnerClutterZone(a, renderRadarRangeKm)) {
    return true;
  }

  if (index == renderSelectedIndex) {
    return true;
  }

  bool selectedInner = selectedAircraftIsInnerClutter();

  int allowedNonSelectedInner = selectedInner ? 1 : 2;
  int nonSelectedInnerSeen = 0;

  for (int i = 0; i < renderAircraftCount; i++) {
    if (i == renderSelectedIndex) {
      continue;
    }

    if (!isAircraftVisibleInRange(renderAircraft[i], renderRadarRangeKm)) {
      continue;
    }

    if (!isAircraftInInnerClutterZone(renderAircraft[i], renderRadarRangeKm)) {
      continue;
    }

    nonSelectedInnerSeen++;

    if (i == index) {
      return nonSelectedInnerSeen <= allowedNonSelectedInner;
    }
  }

  return false;
}

static int countVisibleAircraftLocked() {
  int count = 0;

  for (int i = 0; i < aircraftCount; i++) {
    if (isAircraftVisibleInRange(aircraft[i], radarRangeKm)) {
      count++;
    }
  }

  return count;
}

static int visibleRankForIndex(int targetIndex) {
  if (targetIndex < 0 || targetIndex >= renderAircraftCount) return 0;

  int rank = 0;

  for (int i = 0; i < renderAircraftCount; i++) {
    if (isAircraftVisibleInRange(renderAircraft[i], renderRadarRangeKm)) {
      rank++;

      if (i == targetIndex) {
        return rank;
      }
    }
  }

  return 0;
}

static void getBestAircraftDisplayName(const Aircraft &a, char *out, size_t outSize) {
  if (!out || outSize == 0) return;

  if (hasUsefulText(a.model)) {
    safeCopy(out, outSize, a.model);
  } else if (hasUsefulText(a.callsign)) {
    safeCopy(out, outSize, a.callsign);
  } else if (hasUsefulText(a.icao)) {
    safeCopy(out, outSize, a.icao);
  } else {
    safeCopy(out, outSize, "AIRCRAFT");
  }
}

static int nextVisibleAircraftIndexLocked(int currentIndex) {
  if (aircraftCount <= 0) return -1;

  int start = currentIndex;

  if (start < 0 || start >= aircraftCount) {
    start = -1;
  }

  for (int step = 1; step <= aircraftCount; step++) {
    int idx = (start + step) % aircraftCount;

    if (isAircraftVisibleInRange(aircraft[idx], radarRangeKm)) {
      return idx;
    }
  }

  return -1;
}

static void copyTrail(Aircraft &dst, const Aircraft &src) {
  dst.trailHead = src.trailHead;
  dst.trailCount = src.trailCount;

  for (int i = 0; i < MAX_TRAIL_POINTS; i++) {
    dst.trail[i] = src.trail[i];
  }
}

static void addTrailPointLocked(Aircraft &a) {
  if (!a.valid) return;

  a.trailHead = (a.trailHead + 1) % MAX_TRAIL_POINTS;

  a.trail[a.trailHead].lat = a.lat;
  a.trail[a.trailHead].lon = a.lon;
  a.trail[a.trailHead].distanceKm = a.distanceKm;
  a.trail[a.trailHead].bearingDeg = a.bearingDeg;
  a.trail[a.trailHead].valid = true;

  if (a.trailCount < MAX_TRAIL_POINTS) {
    a.trailCount++;
  }
}

static bool fillAircraft(
  Aircraft &a,
  const char *icao,
  const char *callsign,
  const char *model,
  const char *routeFrom,
  const char *routeTo,
  float lat,
  float lon,
  float headingDeg,
  float speedKts,
  int altitudeFt
) {
  if (lat < -90.0f || lat > 90.0f || lon < -180.0f || lon > 180.0f) {
    return false;
  }

  clearAircraft(a);

  safeCopy(a.icao, sizeof(a.icao), icao);
  safeCopy(a.callsign, sizeof(a.callsign), callsign);
  safeCopy(a.model, sizeof(a.model), model);
  safeCopy(a.routeFrom, sizeof(a.routeFrom), routeFrom);
  safeCopy(a.routeTo, sizeof(a.routeTo), routeTo);

  trimText(a.icao);
  trimText(a.callsign);
  trimText(a.model);
  trimText(a.routeFrom);
  trimText(a.routeTo);

  uppercaseText(a.icao);
  uppercaseText(a.callsign);
  uppercaseText(a.model);
  uppercaseText(a.routeFrom);
  uppercaseText(a.routeTo);

  if (textIsEmpty(a.icao)) {
    safeCopy(a.icao, sizeof(a.icao), "UNKNOWN");
  }

  if (textIsEmpty(a.callsign)) {
    safeCopy(a.callsign, sizeof(a.callsign), "NO CALL");
  }

  if (textIsEmpty(a.routeFrom)) {
    safeCopy(a.routeFrom, sizeof(a.routeFrom), "--");
  }

  if (textIsEmpty(a.routeTo)) {
    safeCopy(a.routeTo, sizeof(a.routeTo), "--");
  }

  a.lat = lat;
  a.lon = lon;
  a.headingDeg = normalizeAngle(headingDeg);
  a.speedKts = speedKts;
  a.altitudeFt = altitudeFt;
  a.distanceKm = (float)haversineKm(USER_LAT, USER_LON, lat, lon);
  a.bearingDeg = bearingDeg(USER_LAT, USER_LON, lat, lon);
  a.valid = true;

  return true;
}

static void sortAircraftArray(Aircraft *arr, int count) {
  for (int i = 0; i < count - 1; i++) {
    for (int j = i + 1; j < count; j++) {
      if (arr[j].distanceKm < arr[i].distanceKm) {
        Aircraft temp = arr[i];
        arr[i] = arr[j];
        arr[j] = temp;
      }
    }
  }
}

static void commitFetchedAircraft(int count) {
  if (count < 0) count = 0;
  if (count > MAX_AIRCRAFT) count = MAX_AIRCRAFT;

  sortAircraftArray(fetchAircraft, count);

  if (aircraftMutex) {
    xSemaphoreTake(aircraftMutex, portMAX_DELAY);
  }

  char oldSelectedIcao[9];
  oldSelectedIcao[0] = '\0';

  if (selectedIndex >= 0 && selectedIndex < aircraftCount) {
    safeCopy(oldSelectedIcao, sizeof(oldSelectedIcao), aircraft[selectedIndex].icao);
  }

  for (int i = 0; i < count; i++) {
    int oldIndex = findAircraftByIcao(aircraft, aircraftCount, fetchAircraft[i].icao);

    if (oldIndex >= 0) {
      copyTrail(fetchAircraft[i], aircraft[oldIndex]);
    } else {
      addTrailPointLocked(fetchAircraft[i]);
    }
  }

  for (int i = 0; i < MAX_AIRCRAFT; i++) {
    clearAircraft(aircraft[i]);
  }

  for (int i = 0; i < count; i++) {
    aircraft[i] = fetchAircraft[i];
  }

  aircraftCount = count;

  if (aircraftCount <= 0) {
    selectedIndex = -1;
  } else {
    int newSelected = findAircraftByIcao(aircraft, aircraftCount, oldSelectedIcao);

    if (newSelected >= 0 && isAircraftVisibleInRange(aircraft[newSelected], radarRangeKm)) {
      selectedIndex = newSelected;
    } else {
      selectedIndex = nextVisibleAircraftIndexLocked(-1);
    }
  }

  if (aircraftMutex) {
    xSemaphoreGive(aircraftMutex);
  }
}

static void updateTrails() {
  if (!aircraftMutex) return;

  xSemaphoreTake(aircraftMutex, portMAX_DELAY);

  for (int i = 0; i < aircraftCount; i++) {
    addTrailPointLocked(aircraft[i]);
  }

  xSemaphoreGive(aircraftMutex);
}

static void updateMapZoomCycle() {
  if (!aircraftMutex) return;

  uint32_t now = millis();

  if (lastMapZoomMs == 0) {
    lastMapZoomMs = now;
    return;
  }

  if (now - lastMapZoomMs < MAP_ZOOM_INTERVAL_MS) {
    return;
  }

  lastMapZoomMs = now;

  xSemaphoreTake(aircraftMutex, portMAX_DELAY);

  mapZoomIndex++;

  if (mapZoomIndex >= ZOOM_LEVEL_COUNT) {
    mapZoomIndex = 0;
  }

  radarRangeKm = ZOOM_LEVELS_KM[mapZoomIndex];

  if (
    selectedIndex < 0 ||
    selectedIndex >= aircraftCount ||
    !isAircraftVisibleInRange(aircraft[selectedIndex], radarRangeKm)
  ) {
    selectedIndex = nextVisibleAircraftIndexLocked(-1);
    lastSelectionSwitchMs = now;
  }

  xSemaphoreGive(aircraftMutex);
}

static void updateSelectionCycle() {
  if (!aircraftMutex) return;

  uint32_t now = millis();

  xSemaphoreTake(aircraftMutex, portMAX_DELAY);

  if (aircraftCount <= 0) {
    selectedIndex = -1;
    lastSelectionSwitchMs = now;
  } else {
    if (
      selectedIndex < 0 ||
      selectedIndex >= aircraftCount ||
      !isAircraftVisibleInRange(aircraft[selectedIndex], radarRangeKm)
    ) {
      selectedIndex = nextVisibleAircraftIndexLocked(-1);
      lastSelectionSwitchMs = now;
    } else if (now - lastSelectionSwitchMs >= SELECT_SWITCH_INTERVAL_MS) {
      selectedIndex = nextVisibleAircraftIndexLocked(selectedIndex);
      lastSelectionSwitchMs = now;
    }
  }

  xSemaphoreGive(aircraftMutex);
}

static void snapshotAircraftForRender() {
  if (!aircraftMutex) return;

  xSemaphoreTake(aircraftMutex, portMAX_DELAY);

  renderAircraftCount = aircraftCount;
  renderSelectedIndex = selectedIndex;
  renderRadarRangeKm = radarRangeKm;
  renderVisibleAircraftCount = countVisibleAircraftLocked();

  for (int i = 0; i < MAX_AIRCRAFT; i++) {
    renderAircraft[i] = aircraft[i];
  }

  xSemaphoreGive(aircraftMutex);
}

//====================================================
// WIFI FUNCTIONS
//====================================================
static void setupWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);

  wifiManager.setConfigPortalTimeout(180);
  wifiManager.setConnectTimeout(30);
  wifiManager.setWiFiAutoReconnect(true);

  bool connected = wifiManager.autoConnect("FlightRadarESP32");

  if (!connected) {
    delay(500);
    ESP.restart();
  }
}

static bool ensureWiFiConnected() {
  if (WiFi.status() == WL_CONNECTED) {
    return true;
  }

  WiFi.reconnect();

  uint32_t start = millis();

  while (WiFi.status() != WL_CONNECTED && millis() - start < 8000) {
    delay(100);
  }

  return WiFi.status() == WL_CONNECTED;
}

//====================================================
// HTTP FUNCTIONS
//====================================================
static bool httpGetJson(const char *url, DynamicJsonDocument &doc) {
  if (!ensureWiFiConnected()) {
    return false;
  }

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  http.setTimeout(9000);
  http.setReuse(false);
  http.useHTTP10(true);

  if (!http.begin(client, url)) {
    return false;
  }

  int code = http.GET();

  if (code != HTTP_CODE_OK) {
    http.end();
    return false;
  }

  doc.clear();

  DeserializationError error = deserializeJson(doc, http.getStream());

  http.end();

  if (error) {
    return false;
  }

  return true;
}

//====================================================
// ADSB.LOL FETCH
//====================================================
static bool fetchADSB() {
  char url[180];

  snprintf(
    url,
    sizeof(url),
    "%s%.6f/%.6f/%d",
    ADSB_BASE_URL,
    USER_LAT,
    USER_LON,
    ADSB_RADIUS_NM
  );

  static DynamicJsonDocument doc(32768);

  if (!httpGetJson(url, doc)) {
    return false;
  }

  JsonArray ac = doc["ac"].as<JsonArray>();

  if (ac.isNull()) {
    return false;
  }

  for (int i = 0; i < MAX_AIRCRAFT; i++) {
    clearAircraft(fetchAircraft[i]);
  }

  int count = 0;

  for (JsonObject item : ac) {
    if (count >= MAX_AIRCRAFT) break;

    if (item["lat"].isNull() || item["lon"].isNull()) {
      continue;
    }

    float lat = item["lat"].as<float>();
    float lon = item["lon"].as<float>();

    char icao[9];
    char callsign[12];
    char model[22];
    char routeFrom[6];
    char routeTo[6];

    safeCopy(icao, sizeof(icao), item["hex"] | "");
    safeCopy(callsign, sizeof(callsign), firstJsonText(item, "flight", "call", "callsign", "squawk"));

    buildModelText(item, model, sizeof(model));

    safeCopy(routeFrom, sizeof(routeFrom), firstJsonText(item, "from", "origin", "orig_iata", "orig_icao"));
    safeCopy(routeTo, sizeof(routeTo), firstJsonText(item, "to", "destination", "dest_iata", "dest_icao"));

    if (textIsEmpty(routeFrom) || textIsEmpty(routeTo)) {
      char parsedFrom[6];
      char parsedTo[6];

      parseRouteCodes(
        firstJsonText(item, "route", "airport_codes_iata", "airport_codes_icao", "airport_codes"),
        parsedFrom,
        sizeof(parsedFrom),
        parsedTo,
        sizeof(parsedTo)
      );

      if (textIsEmpty(routeFrom)) {
        safeCopy(routeFrom, sizeof(routeFrom), parsedFrom);
      }

      if (textIsEmpty(routeTo)) {
        safeCopy(routeTo, sizeof(routeTo), parsedTo);
      }
    }

    float heading = 0.0f;

    if (!item["track"].isNull()) {
      heading = item["track"].as<float>();
    } else if (!item["true_heading"].isNull()) {
      heading = item["true_heading"].as<float>();
    } else if (!item["mag_heading"].isNull()) {
      heading = item["mag_heading"].as<float>();
    }

    float speed = 0.0f;

    if (!item["gs"].isNull()) {
      speed = item["gs"].as<float>();
    }

    int altitude = 0;

    if (!item["alt_baro"].isNull() && item["alt_baro"].is<int>()) {
      altitude = item["alt_baro"].as<int>();
    } else if (!item["alt_geom"].isNull() && item["alt_geom"].is<int>()) {
      altitude = item["alt_geom"].as<int>();
    }

    if (
      fillAircraft(
        fetchAircraft[count],
        icao,
        callsign,
        model,
        routeFrom,
        routeTo,
        lat,
        lon,
        heading,
        speed,
        altitude
      )
    ) {
      count++;
    }
  }

  sortAircraftArray(fetchAircraft, count);
  commitFetchedAircraft(count);

  return count > 0;
}

//====================================================
// OPENSKY FETCH
//====================================================
static bool fetchOpenSky() {
  double lamin = USER_LAT - OPENSKY_BOX_DEG;
  double lamax = USER_LAT + OPENSKY_BOX_DEG;
  double lomin = USER_LON - OPENSKY_BOX_DEG;
  double lomax = USER_LON + OPENSKY_BOX_DEG;

  char url[240];

  snprintf(
    url,
    sizeof(url),
    "%s?lamin=%.6f&lomin=%.6f&lamax=%.6f&lomax=%.6f",
    OPENSKY_BASE_URL,
    lamin,
    lomin,
    lamax,
    lomax
  );

  static DynamicJsonDocument doc(32768);

  if (!httpGetJson(url, doc)) {
    return false;
  }

  JsonArray states = doc["states"].as<JsonArray>();

  if (states.isNull()) {
    return false;
  }

  for (int i = 0; i < MAX_AIRCRAFT; i++) {
    clearAircraft(fetchAircraft[i]);
  }

  int count = 0;

  for (JsonArray s : states) {
    if (count >= MAX_AIRCRAFT) break;
    if (s.size() < 17) continue;

    if (s[5].isNull() || s[6].isNull()) {
      continue;
    }

    char icao[9];
    char callsign[12];

    safeCopy(icao, sizeof(icao), s[0] | "");
    safeCopy(callsign, sizeof(callsign), s[1] | "");

    float lon = s[5].as<float>();
    float lat = s[6].as<float>();

    float velocityMs = 0.0f;
    float heading = 0.0f;

    if (!s[9].isNull()) {
      velocityMs = s[9].as<float>();
    }

    if (!s[10].isNull()) {
      heading = s[10].as<float>();
    }

    float speedKts = velocityMs * 1.943844f;

    int altitudeFt = 0;

    if (!s[7].isNull()) {
      altitudeFt = (int)(s[7].as<float>() * 3.28084f);
    } else if (!s[13].isNull()) {
      altitudeFt = (int)(s[13].as<float>() * 3.28084f);
    }

    if (
      fillAircraft(
        fetchAircraft[count],
        icao,
        callsign,
        "",
        "--",
        "--",
        lat,
        lon,
        heading,
        speedKts,
        altitudeFt
      )
    ) {
      count++;
    }
  }

  sortAircraftArray(fetchAircraft, count);
  commitFetchedAircraft(count);

  return count > 0;
}

//====================================================
// AIRCRAFT BACKGROUND TASK
//====================================================
void aircraftUpdateTask(void *parameter) {
  uint32_t lastAdsbMs = 0;
  uint32_t lastOpenSkyMs = 0;
  uint32_t lastAdsbRetryMs = 0;

  bool localUsingOpenSky = false;

  for (;;) {
    uint32_t now = millis();

    if (!ensureWiFiConnected()) {
      vTaskDelay(pdMS_TO_TICKS(3000));
      continue;
    }

    if (!localUsingOpenSky) {
      if (lastAdsbMs == 0 || now - lastAdsbMs >= ADSB_UPDATE_INTERVAL_MS) {
        lastAdsbMs = now;

        bool ok = fetchADSB();

        if (ok) {
          apiHasEverSucceeded = true;
          usingOpenSky = false;
        } else {
          localUsingOpenSky = true;
          usingOpenSky = true;
          lastOpenSkyMs = 0;
          lastAdsbRetryMs = now;
        }
      }
    } else {
      if (lastOpenSkyMs == 0 || now - lastOpenSkyMs >= OPENSKY_UPDATE_INTERVAL_MS) {
        lastOpenSkyMs = now;

        bool ok = fetchOpenSky();

        if (ok) {
          apiHasEverSucceeded = true;
        }
      }

      if (now - lastAdsbRetryMs >= ADSB_RETRY_INTERVAL_MS) {
        lastAdsbRetryMs = now;

        bool ok = fetchADSB();

        if (ok) {
          apiHasEverSucceeded = true;
          localUsingOpenSky = false;
          usingOpenSky = false;
          lastAdsbMs = now;
        }
      }
    }

    vTaskDelay(pdMS_TO_TICKS(250));
  }
}

//====================================================
// DISPLAY POSITION FUNCTIONS
//====================================================
static bool polarToScreen(float distanceKm, float bearingDegIn, float rangeKm, int &x, int &y) {
  if (rangeKm <= 0.1f) return false;
  if (distanceKm > rangeKm) return false;

  float r = (distanceKm / rangeKm) * RADAR_R;
  float a = degToRad(bearingDegIn - 90.0f);

  x = RADAR_CX + (int)roundf(cos(a) * r);
  y = RADAR_CY + (int)roundf(sin(a) * r);

  return true;
}

static bool aircraftToScreen(const Aircraft &a, int &x, int &y) {
  if (!a.valid) return false;

  return polarToScreen(a.distanceKm, a.bearingDeg, renderRadarRangeKm, x, y);
}

//====================================================
// DISPLAY BASIC DRAWING
//====================================================
static void drawThickPixel(int x, int y, uint16_t color) {
  sprite.drawPixel(x, y, color);
  sprite.drawPixel(x + 1, y, color);
  sprite.drawPixel(x - 1, y, color);
  sprite.drawPixel(x, y + 1, color);
  sprite.drawPixel(x, y - 1, color);
}

static void drawStartupScreen(const char *message) {
  sprite.fillSprite(C_BG);

  sprite.drawCircle(120, 120, 108, C_OUTER_RING);
  sprite.drawCircle(120, 120, 82, C_GRID_DIM);
  sprite.drawCircle(120, 120, 56, C_GRID_DIM);
  sprite.drawCircle(120, 120, 30, C_GRID_DIM);

  sprite.drawLine(120, 12, 120, 228, C_GRID_DIM);
  sprite.drawLine(12, 120, 228, 120, C_GRID_DIM);

  sprite.setTextDatum(MC_DATUM);
  sprite.setTextColor(C_ACTIVE, C_BG);
  sprite.drawString("FlightRadar", 120, 88, 4);

  sprite.setTextColor(C_TEXT, C_BG);
  sprite.drawString("ESP32", 120, 120, 2);

  sprite.setTextColor(C_TEXT_DIM, C_BG);
  sprite.drawString(message, 120, 148, 2);

  sprite.pushSprite(0, 0);
}

static void drawRoundBorder() {
  sprite.drawCircle(ROUND_CX, ROUND_CY, ROUND_R, C_OUTER_RING);
}

static void drawLineThickSoft(int x1, int y1, int x2, int y2, uint16_t color) {
  sprite.drawLine(x1, y1, x2, y2, color);

  if (abs(x2 - x1) >= abs(y2 - y1)) {
    sprite.drawLine(x1, y1 + 1, x2, y2 + 1, color);
  } else {
    sprite.drawLine(x1 + 1, y1, x2 + 1, y2, color);
  }
}

//====================================================
// NIGHT CONSTELLATION DRAWING
//====================================================
static void drawOneConstellation(int constellationIndex, int positionIndex, int offsetJitter) {
  constellationIndex %= CONSTELLATION_COUNT;
  positionIndex %= CONST_POSITION_COUNT;

  const ConstellationPattern &cp = CONSTELLATIONS[constellationIndex];

  int ox = CONST_POSITIONS[positionIndex][0] + ((offsetJitter % 3) - 1) * 4;
  int oy = CONST_POSITIONS[positionIndex][1] + (((offsetJitter / 3) % 3) - 1) * 4;

  for (int i = 0; i < cp.count - 1; i++) {
    int x1 = ox + cp.stars[i].x;
    int y1 = oy + cp.stars[i].y;
    int x2 = ox + cp.stars[i + 1].x;
    int y2 = oy + cp.stars[i + 1].y;

    drawLineThickSoft(x1, y1, x2, y2, C_STAR_LINE);
  }

  for (int i = 0; i < cp.count; i++) {
    int x = ox + cp.stars[i].x;
    int y = oy + cp.stars[i].y;

    sprite.fillCircle(x, y, 1, C_STAR);
  }
}

static void drawNightConstellationBackground() {
  if (!isNightTime()) {
    return;
  }

  int slot = getFiveMinuteSlot();

  drawOneConstellation(slot + 0, slot + 1, slot + 2);
  drawOneConstellation(slot + 1, slot + 3, slot + 5);

  if (renderRadarRangeKm >= 100.0f) {
    drawOneConstellation(slot + 2, slot + 5, slot + 8);
  }
}

//====================================================
// RADAR GRID DRAWING
//====================================================
static void drawRadarGrid() {
  sprite.fillSprite(C_BG);

  drawNightConstellationBackground();

  for (int i = 1; i <= 5; i++) {
    int r = (RADAR_R * i) / 5;
    sprite.drawCircle(RADAR_CX, RADAR_CY, r, (i == 5) ? C_OUTER_RING : C_GRID);
  }

  sprite.drawLine(RADAR_CX - RADAR_R, RADAR_CY, RADAR_CX + RADAR_R, RADAR_CY, C_GRID_DIM);
  sprite.drawLine(RADAR_CX, RADAR_CY - RADAR_R, RADAR_CX, RADAR_CY + RADAR_R, C_GRID_DIM);

  for (int deg = 30; deg < 360; deg += 30) {
    float a = degToRad(deg - 90.0f);

    int x1 = RADAR_CX + (int)(cos(a) * (RADAR_R - 5));
    int y1 = RADAR_CY + (int)(sin(a) * (RADAR_R - 5));
    int x2 = RADAR_CX + (int)(cos(a) * RADAR_R);
    int y2 = RADAR_CY + (int)(sin(a) * RADAR_R);

    sprite.drawLine(x1, y1, x2, y2, C_GRID_DIM);
  }

  sprite.setTextDatum(MC_DATUM);
  sprite.setTextColor(C_TEXT_DIM);

  sprite.drawString("N", RADAR_CX, 12, 1);
  sprite.drawString("S", RADAR_CX, 228, 1);
  sprite.drawString("E", 228, RADAR_CY, 1);
  sprite.drawString("W", 12, RADAR_CY, 1);

  char rangeText[14];
  snprintf(rangeText, sizeof(rangeText), "%.0fkm", renderRadarRangeKm);

  sprite.setTextColor(C_WARN);
  sprite.drawString(rangeText, 120, 27, 1);

  char countText[18];
  snprintf(countText, sizeof(countText), "%d/%d", renderVisibleAircraftCount, renderAircraftCount);

  sprite.setTextColor(C_TEXT_DIM);
  sprite.setTextDatum(TR_DATUM);
  sprite.drawString(countText, 204, 48, 1);
}

//====================================================
// CLOCK DOT DRAWING
//====================================================
static void drawTimeDots() {
  struct tm timeinfo;

  if (!getLocalClock(timeinfo)) {
    return;
  }

  float minuteAngle = ((float)timeinfo.tm_min + ((float)timeinfo.tm_sec / 60.0f)) * 6.0f;
  float hourAngle = (((float)(timeinfo.tm_hour % 12)) + ((float)timeinfo.tm_min / 60.0f)) * 30.0f;

  float minuteRad = degToRad(minuteAngle - 90.0f);
  float hourRad = degToRad(hourAngle - 90.0f);

  int minuteRadius = RADAR_R - 3;
  int hourRadius = (RADAR_R * 4) / 5;

  int mx = RADAR_CX + (int)roundf(cos(minuteRad) * minuteRadius);
  int my = RADAR_CY + (int)roundf(sin(minuteRad) * minuteRadius);

  int hx = RADAR_CX + (int)roundf(cos(hourRad) * hourRadius);
  int hy = RADAR_CY + (int)roundf(sin(hourRad) * hourRadius);

  sprite.fillCircle(mx, my, 2, C_TIME_MIN);
  sprite.drawCircle(mx, my, 3, blend565(C_BG, C_TIME_MIN, 150));

  sprite.fillCircle(hx, hy, 3, C_TIME_HOUR);
  sprite.drawCircle(hx, hy, 4, blend565(C_BG, C_TIME_HOUR, 150));
}

//====================================================
// SUN / MOON DRAWING
//====================================================
static void drawSunIcon(int x, int y) {
  const int r = 8;

  sprite.fillCircle(x, y, r, C_SUN);
  sprite.drawCircle(x, y, r + 1, blend565(C_BG, C_SUN, 180));

  sprite.drawPixel(x + 11, y, C_SUN);
  sprite.drawPixel(x - 11, y, C_SUN);
  sprite.drawPixel(x, y + 11, C_SUN);
  sprite.drawPixel(x, y - 11, C_SUN);

  sprite.drawPixel(x + 8, y + 8, C_SUN);
  sprite.drawPixel(x - 8, y - 8, C_SUN);
  sprite.drawPixel(x + 8, y - 8, C_SUN);
  sprite.drawPixel(x - 8, y + 8, C_SUN);

  sprite.drawPixel(x + 10, y + 4, C_SUN);
  sprite.drawPixel(x - 10, y - 4, C_SUN);
  sprite.drawPixel(x + 10, y - 4, C_SUN);
  sprite.drawPixel(x - 10, y + 4, C_SUN);
}

static void drawMoonPhaseIcon(int x, int y) {
  const int r = 10;

  float phase = getMoonPhase01();
  float illumination = 0.5f * (1.0f - cosf(2.0f * PI * phase));

  if (illumination < 0.03f) {
    sprite.drawCircle(x, y, r, C_MOON_OUTLINE);
    sprite.drawCircle(x, y, r - 2, blend565(C_BG, C_MOON_OUTLINE, 120));
    return;
  }

  if (illumination > 0.97f) {
    sprite.fillCircle(x, y, r, C_MOON);
    sprite.drawCircle(x, y, r + 1, C_MOON_OUTLINE);
    return;
  }

  sprite.fillCircle(x, y, r, C_MOON);
  sprite.drawCircle(x, y, r + 1, C_MOON_OUTLINE);

  int shift = (int)roundf(illumination * (2.0f * r));

  if (shift < 1) shift = 1;
  if (shift > 2 * r) shift = 2 * r;

  bool waxing = (phase < 0.5f);
  int maskX = waxing ? (x - shift) : (x + shift);

  sprite.fillCircle(maskX, y, r, C_BG);

  if (illumination < 0.35f) {
    int rimX = waxing ? (x + 4) : (x - 4);
    int side = waxing ? 1 : -1;

    for (int yy = -7; yy <= 7; yy++) {
      sprite.drawPixel(rimX, y + yy, C_MOON_OUTLINE);
    }

    for (int yy = -5; yy <= 5; yy++) {
      sprite.drawPixel(rimX + side, y + yy, C_MOON);
    }
  }
}

static void drawSunMoonTracker() {
  SunData sun;

  if (!getSunData(sun)) {
    return;
  }

  int radius = RADAR_R - 24;

  if (sun.elevationDeg > 0.0f) {
    float a = degToRad(sun.azimuthDeg - 90.0f);

    int x = RADAR_CX + (int)roundf(cos(a) * radius);
    int y = RADAR_CY + (int)roundf(sin(a) * radius);

    drawSunIcon(x, y);
    return;
  }

  float nightProgress = 0.0f;

  if (getNightProgress01(nightProgress)) {
    float moonAz = getEstimatedMoonAzimuthFromNightPath();
    float a = degToRad(moonAz - 90.0f);

    int x = RADAR_CX + (int)roundf(cos(a) * radius);
    int y = RADAR_CY + (int)roundf(sin(a) * radius);

    drawMoonPhaseIcon(x, y);
  }
}

//====================================================
// RADAR SWEEP DRAWING
//====================================================
static void drawRadarSweep() {
  sweepAngleDeg = getFastSweepAngle();

  for (int i = 0; i < 18; i++) {
    float aDeg = sweepAngleDeg - (float)i * 3.0f;

    while (aDeg < 0.0f) {
      aDeg += 360.0f;
    }

    int amountInt = 165 - i * 8;

    if (amountInt < 10) {
      amountInt = 10;
    }

    uint16_t c = blend565(C_BG, C_SWEEP, (uint8_t)amountInt);

    float a = degToRad(aDeg - 90.0f);

    int x = RADAR_CX + (int)(cos(a) * RADAR_R);
    int y = RADAR_CY + (int)(sin(a) * RADAR_R);

    sprite.drawLine(RADAR_CX, RADAR_CY, x, y, c);

    if (i < 4) {
      int x2 = RADAR_CX + (int)(cos(a) * (RADAR_R - 14));
      int y2 = RADAR_CY + (int)(sin(a) * (RADAR_R - 14));

      sprite.drawLine(RADAR_CX - 1, RADAR_CY, x2, y2, c);
      sprite.drawLine(RADAR_CX + 1, RADAR_CY, x2, y2, c);
    }
  }

  drawThickPixel(RADAR_CX, RADAR_CY, C_SWEEP);
}

//====================================================
// AIRPORT MARKER DRAWING
//====================================================
static bool airportHasNearbyAircraft(const AirportMarker &ap) {
  for (int i = 0; i < renderAircraftCount; i++) {
    if (!isAircraftVisibleInRange(renderAircraft[i], renderRadarRangeKm)) {
      continue;
    }

    float d = (float)haversineKm(ap.lat, ap.lon, renderAircraft[i].lat, renderAircraft[i].lon);

    if (d <= 18.0f) {
      return true;
    }
  }

  return false;
}

static void drawAirportMarkers() {
  for (int i = 0; i < AIRPORT_COUNT; i++) {
    const AirportMarker &ap = AIRPORTS[i];

    float d = (float)haversineKm(USER_LAT, USER_LON, ap.lat, ap.lon);
    float b = bearingDeg(USER_LAT, USER_LON, ap.lat, ap.lon);

    int x;
    int y;

    if (!polarToScreen(d, b, renderRadarRangeKm, x, y)) {
      continue;
    }

    x += ap.drawOffsetX;
    y += ap.drawOffsetY;

    bool busy = airportHasNearbyAircraft(ap);
    uint16_t color = ap.primary ? C_AIRPORT_MAJOR : C_AIRPORT_OTHER;

    if (busy) {
      sprite.drawPixel(x, y, color);
      sprite.drawCircle(x, y, 2, color);
    } else {
      if (ap.primary) {
        sprite.fillCircle(x, y, 3, color);
        sprite.drawCircle(x, y, 5, color);
      } else {
        sprite.fillCircle(x, y, 2, color);
        sprite.drawCircle(x, y, 4, color);
      }

      if (renderRadarRangeKm >= 50.0f || ap.primary) {
        sprite.setTextDatum(MC_DATUM);
        sprite.setTextColor(ap.primary ? C_AIRPORT_MAJOR : C_AIRPORT_LABEL);
        sprite.drawString(ap.code, x, y - 11, 1);
      }
    }
  }
}

//====================================================
// AIRCRAFT ICON DRAWING
//====================================================
static void rotatePoint(float x, float y, float scale, float angleRad, int cx, int cy, int &ox, int &oy) {
  x *= scale;
  y *= scale;

  float ca = cos(angleRad);
  float sa = sin(angleRad);

  ox = cx + (int)roundf(x * ca - y * sa);
  oy = cy + (int)roundf(x * sa + y * ca);
}

static void fillRotatedTriangle(
  int cx,
  int cy,
  float headingRad,
  float scale,
  float x1,
  float y1,
  float x2,
  float y2,
  float x3,
  float y3,
  uint16_t color
) {
  int ax;
  int ay;
  int bx;
  int by;
  int cx2;
  int cy2;

  rotatePoint(x1, y1, scale, headingRad, cx, cy, ax, ay);
  rotatePoint(x2, y2, scale, headingRad, cx, cy, bx, by);
  rotatePoint(x3, y3, scale, headingRad, cx, cy, cx2, cy2);

  sprite.fillTriangle(ax, ay, bx, by, cx2, cy2, color);
}

static void drawAircraftIcon(int cx, int cy, float headingDegIn, uint16_t color, bool active) {
  float a = degToRad(headingDegIn);
  float scale = active ? 0.52f : 0.46f;

  fillRotatedTriangle(cx, cy, a, scale, 0, -14, -3, 5, 3, 5, color);
  fillRotatedTriangle(cx, cy, a, scale, -3, 5, 3, 5, 0, 14, color);

  fillRotatedTriangle(cx, cy, a, scale, -2, -2, -18, 5, -2, 4, color);
  fillRotatedTriangle(cx, cy, a, scale, 2, -2, 18, 5, 2, 4, color);

  fillRotatedTriangle(cx, cy, a, scale, -2, 9, -9, 13, -1, 12, color);
  fillRotatedTriangle(cx, cy, a, scale, 2, 9, 9, 13, 1, 12, color);

  fillRotatedTriangle(cx, cy, a, scale, -1, -11, 1, -11, 0, -16, color);

  if (active) {
    drawThickPixel(cx, cy, color);
  } else {
    sprite.drawPixel(cx, cy, color);
  }
}

//====================================================
// TRAIL DRAWING
//====================================================
static void drawAircraftTrails() {
  for (int i = 0; i < renderAircraftCount; i++) {
    Aircraft &a = renderAircraft[i];

    if (!a.valid || a.trailCount < 2) {
      continue;
    }

    if (!shouldDrawAircraftIndex(i)) {
      continue;
    }

    for (int n = a.trailCount - 1; n >= 1; n--) {
      int idx1 = a.trailHead - n;
      int idx2 = a.trailHead - n + 1;

      while (idx1 < 0) idx1 += MAX_TRAIL_POINTS;
      while (idx2 < 0) idx2 += MAX_TRAIL_POINTS;

      idx1 %= MAX_TRAIL_POINTS;
      idx2 %= MAX_TRAIL_POINTS;

      TrailPoint &p1 = a.trail[idx1];
      TrailPoint &p2 = a.trail[idx2];

      if (!p1.valid || !p2.valid) {
        continue;
      }

      int x1;
      int y1;
      int x2;
      int y2;

      if (!polarToScreen(p1.distanceKm, p1.bearingDeg, renderRadarRangeKm, x1, y1)) {
        continue;
      }

      if (!polarToScreen(p2.distanceKm, p2.bearingDeg, renderRadarRangeKm, x2, y2)) {
        continue;
      }

      uint8_t amount = 22 + (uint8_t)((MAX_TRAIL_POINTS - n) * 7);

      if (amount > 210) {
        amount = 210;
      }

      uint16_t base = (i == renderSelectedIndex) ? C_ACTIVE : C_INACTIVE;
      uint16_t color = blend565(C_BG, base, amount);

      sprite.drawLine(x1, y1, x2, y2, color);
    }
  }
}

//====================================================
// AIRCRAFT KEEP-OUT HELPERS
//====================================================
static bool zoneOverlaps(int x, int y, const DrawZone *zones, int zoneCount, int r) {
  for (int i = 0; i < zoneCount; i++) {
    int dx = x - zones[i].x;
    int dy = y - zones[i].y;

    int rr = r + zones[i].r;

    if ((dx * dx + dy * dy) <= (rr * rr)) {
      return true;
    }
  }

  return false;
}

static void addDrawZone(DrawZone *zones, int &zoneCount, int maxZones, int x, int y, int r) {
  if (zoneCount >= maxZones) {
    return;
  }

  zones[zoneCount].x = x;
  zones[zoneCount].y = y;
  zones[zoneCount].r = r;

  zoneCount++;
}

//====================================================
// AIRCRAFT DRAWING
//====================================================
static void drawAircraft() {
  static DrawZone zones[MAX_AIRCRAFT];

  int zoneCount = 0;

  const int activeKeepout = 20;
  const int inactiveKeepout = 18;

  if (
    renderSelectedIndex >= 0 &&
    renderSelectedIndex < renderAircraftCount &&
    shouldDrawAircraftIndex(renderSelectedIndex)
  ) {
    Aircraft &a = renderAircraft[renderSelectedIndex];

    int x;
    int y;

    if (aircraftToScreen(a, x, y)) {
      drawAircraftIcon(x, y, a.headingDeg, C_ACTIVE, true);
      addDrawZone(zones, zoneCount, MAX_AIRCRAFT, x, y, activeKeepout);
    }
  }

  for (int i = 0; i < renderAircraftCount; i++) {
    if (i == renderSelectedIndex) {
      continue;
    }

    Aircraft &a = renderAircraft[i];

    if (!a.valid) {
      continue;
    }

    if (!shouldDrawAircraftIndex(i)) {
      continue;
    }

    int x;
    int y;

    if (!aircraftToScreen(a, x, y)) {
      continue;
    }

    if (zoneOverlaps(x, y, zones, zoneCount, inactiveKeepout)) {
      continue;
    }

    drawAircraftIcon(x, y, a.headingDeg, C_INACTIVE, false);
    addDrawZone(zones, zoneCount, MAX_AIRCRAFT, x, y, inactiveKeepout);
  }
}

//====================================================
// SELECTED INFO DRAWING
//====================================================
static void drawSelectedAircraftInfo() {
  sprite.setTextDatum(MC_DATUM);

  if (
    renderSelectedIndex < 0 ||
    renderSelectedIndex >= renderAircraftCount ||
    !isAircraftVisibleInRange(renderAircraft[renderSelectedIndex], renderRadarRangeKm)
  ) {
    sprite.setTextColor(C_TEXT);
    sprite.drawString("NO AIRCRAFT", INFO_CX, INFO_Y1, 2);

    sprite.setTextColor(C_INFO_TEXT_DIM);

    if (apiHasEverSucceeded) {
      sprite.drawString("NO TRAFFIC IN RANGE", INFO_CX, INFO_Y2, 1);
    } else {
      sprite.drawString("WAITING FOR DATA", INFO_CX, INFO_Y2, 1);
    }

    return;
  }

  Aircraft &a = renderAircraft[renderSelectedIndex];

  bool showFlightTemplate = ((millis() / INFO_TEMPLATE_INTERVAL_MS) % 2) == 0;

  int visibleRank = visibleRankForIndex(renderSelectedIndex);

  if (showFlightTemplate) {
    char line1[20];
    char line2[28];
    char line3[18];

    snprintf(line1, sizeof(line1), "%s", a.callsign);

    if (hasUsefulText(a.routeFrom) && hasUsefulText(a.routeTo)) {
      snprintf(line2, sizeof(line2), "%s - %s", a.routeFrom, a.routeTo);
    } else {
      snprintf(line2, sizeof(line2), "%s %.1fkm", a.icao, a.distanceKm);
    }

    snprintf(line3, sizeof(line3), "%d/%d", visibleRank, renderVisibleAircraftCount);

    sprite.setTextColor(C_ACTIVE);
    sprite.drawString(line1, INFO_CX, INFO_Y1, 2);

    sprite.setTextColor(C_TEXT);
    sprite.drawString(line2, INFO_CX, INFO_Y2, 2);

    sprite.setTextColor(C_INFO_TEXT_DIM);
    sprite.drawString(line3, INFO_CX, INFO_Y3, 1);
  } else {
    char altText[12];
    char bestName[24];
    char line2[34];
    char line3[30];

    formatAltitudeShort(a.altitudeFt, altText, sizeof(altText));
    getBestAircraftDisplayName(a, bestName, sizeof(bestName));

    snprintf(line2, sizeof(line2), "%.1fkm  %s", a.distanceKm, altText);
    snprintf(line3, sizeof(line3), "%.0fkt  HDG %.0f", a.speedKts, a.headingDeg);

    sprite.setTextColor(C_ACTIVE);
    sprite.drawString(bestName, INFO_CX, INFO_Y1, 2);

    sprite.setTextColor(C_TEXT);
    sprite.drawString(line2, INFO_CX, INFO_Y2, 2);

    sprite.setTextColor(C_INFO_TEXT_DIM);
    sprite.drawString(line3, INFO_CX, INFO_Y3, 1);
  }
}

//====================================================
// MAIN DISPLAY DRAWING
//====================================================
static void drawDisplayFrame() {
  snapshotAircraftForRender();

  drawRadarGrid();
  drawSunMoonTracker();
  drawRadarSweep();
  drawTimeDots();
  drawAircraftTrails();
  drawAirportMarkers();
  drawSelectedAircraftInfo();
  drawAircraft();
  drawRoundBorder();

  sprite.pushSprite(0, 0);
}

static void updateDisplay() {
  uint32_t now = millis();

  if (lastDisplayMs == 0 || now - lastDisplayMs >= DISPLAY_INTERVAL_MS) {
    lastDisplayMs = now;
    drawDisplayFrame();
  }
}

//====================================================
// SETUP
//====================================================
void setup() {
  Serial.begin(115200);
  delay(100);

  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);
  delay(100);

  tft.init();
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);

  sprite.setColorDepth(8);

  void *spritePtr = sprite.createSprite(SCREEN_W, SCREEN_H);

  if (spritePtr == NULL) {
    tft.fillScreen(TFT_BLACK);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.drawString("SPRITE FAIL", 120, 105, 2);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString("RAM LOW", 120, 130, 2);

    while (true) {
      delay(1000);
    }
  }

  sprite.setSwapBytes(false);

  aircraftMutex = xSemaphoreCreateMutex();

  if (aircraftMutex == NULL) {
    tft.fillScreen(TFT_BLACK);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.drawString("MUTEX FAIL", 120, 120, 4);

    while (true) {
      delay(1000);
    }
  }

  clearAllAircraftArrays();

  drawStartupScreen("WiFi setup");
  setupWiFi();

  setupTimeSync();

  drawStartupScreen("Fetching data");

  xTaskCreatePinnedToCore(
    aircraftUpdateTask,
    "aircraftTask",
    12288,
    NULL,
    1,
    NULL,
    0
  );

  lastDisplayMs = 0;
  lastTrailMs = millis();
  lastSelectionSwitchMs = millis();
  lastMapZoomMs = millis();
}

//====================================================
// LOOP
//====================================================
void loop() {
  uint32_t now = millis();

  if (now - lastTrailMs >= TRAIL_INTERVAL_MS) {
    lastTrailMs = now;
    updateTrails();
  }

  updateMapZoomCycle();
  updateSelectionCycle();
  updateDisplay();

  delay(1);
}