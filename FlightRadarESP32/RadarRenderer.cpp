#include "RadarRenderer.h"
#include "AppState.h"
#include "BoardConfig.h"
#include "UserConfig.h"
#include "GeoMath.h"
#include "TimeService.h"
#include "Astronomy.h"
#include "AircraftStore.h"
#include "TextUtils.h"
#include "Constellations.h"
#include <math.h>

static void drawThickPixel(int x, int y, uint16_t color) {
  sprite.drawPixel(x, y, color);
  sprite.drawPixel(x + 1, y, color);
  sprite.drawPixel(x - 1, y, color);
  sprite.drawPixel(x, y + 1, color);
  sprite.drawPixel(x, y - 1, color);
}

static void drawRoundBorder() {
  sprite.drawCircle(ROUND_CX, ROUND_CY, ROUND_R, C_OUTER_RING);
}

static void drawLineThickSoft(int x1, int y1, int x2, int y2, uint16_t color) {
  sprite.drawLine(x1, y1, x2, y2, color);
  if (abs(x2 - x1) >= abs(y2 - y1)) sprite.drawLine(x1, y1 + 1, x2, y2 + 1, color);
  else sprite.drawLine(x1 + 1, y1, x2 + 1, y2, color);
}

void initDisplay() {
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
    while (true) delay(1000);
  }
  sprite.setSwapBytes(false);
}

void drawStartupScreen(const char *message) {
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

static bool shouldShowConstellations() {
#if CONSTELLATION_MODE == CONSTELLATION_OFF
  return false;
#elif CONSTELLATION_MODE == CONSTELLATION_ALWAYS
  return true;
#else
  return isNightTime();
#endif
}

static void drawNightConstellationBackground() {
  if (!shouldShowConstellations()) return;
  int slot = getFiveMinuteSlot();
  drawOneConstellation(slot + 0, slot + 1, slot + 2);
  drawOneConstellation(slot + 1, slot + 3, slot + 5);
  if (renderRadarRangeKm >= 100.0f) drawOneConstellation(slot + 2, slot + 5, slot + 8);
}

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

static void drawTimeDots() {
  struct tm timeinfo;
  if (!getLocalClock(timeinfo)) return;
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

static void drawSunIcon(int x, int y) {
  const int r = 8;
  sprite.fillCircle(x, y, r, C_SUN);
  sprite.drawCircle(x, y, r + 1, blend565(C_BG, C_SUN, 180));
  sprite.drawPixel(x + 11, y, C_SUN); sprite.drawPixel(x - 11, y, C_SUN);
  sprite.drawPixel(x, y + 11, C_SUN); sprite.drawPixel(x, y - 11, C_SUN);
  sprite.drawPixel(x + 8, y + 8, C_SUN); sprite.drawPixel(x - 8, y - 8, C_SUN);
  sprite.drawPixel(x + 8, y - 8, C_SUN); sprite.drawPixel(x - 8, y + 8, C_SUN);
  sprite.drawPixel(x + 10, y + 4, C_SUN); sprite.drawPixel(x - 10, y - 4, C_SUN);
  sprite.drawPixel(x + 10, y - 4, C_SUN); sprite.drawPixel(x - 10, y + 4, C_SUN);
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
    for (int yy = -7; yy <= 7; yy++) sprite.drawPixel(rimX, y + yy, C_MOON_OUTLINE);
    for (int yy = -5; yy <= 5; yy++) sprite.drawPixel(rimX + side, y + yy, C_MOON);
  }
}

static void drawSunMoonTracker() {
  SunData sun;
  if (!getSunData(sun)) return;
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

static void drawRadarSweep() {
  sweepAngleDeg = getFastSweepAngle();
  for (int i = 0; i < 18; i++) {
    float aDeg = sweepAngleDeg - (float)i * 3.0f;
    while (aDeg < 0.0f) aDeg += 360.0f;
    int amountInt = 165 - i * 8;
    if (amountInt < 10) amountInt = 10;
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

static bool airportHasNearbyAircraft(const AirportMarker &ap) {
  for (int i = 0; i < renderAircraftCount; i++) {
    if (!isAircraftVisibleInRange(renderAircraft[i], renderRadarRangeKm)) continue;
    float d = (float)haversineKm(ap.lat, ap.lon, renderAircraft[i].lat, renderAircraft[i].lon);
    if (d <= 18.0f) return true;
  }
  return false;
}

static void drawAirportMarkers() {
  for (int i = 0; i < USER_AIRPORT_COUNT; i++) {
    const AirportMarker &ap = USER_AIRPORTS[i];
    float d = (float)haversineKm(currentUserLat, currentUserLon, ap.lat, ap.lon);
    float b = bearingDeg(currentUserLat, currentUserLon, ap.lat, ap.lon);
    int x, y;
    if (!polarToScreen(d, b, renderRadarRangeKm, x, y)) continue;
    x += ap.drawOffsetX;
    y += ap.drawOffsetY;
    bool busy = airportHasNearbyAircraft(ap);
    uint16_t color = ap.primary ? C_AIRPORT_MAJOR : C_AIRPORT_OTHER;
    if (busy) {
      sprite.drawPixel(x, y, color);
      sprite.drawCircle(x, y, 2, color);
    } else {
      if (ap.primary) { sprite.fillCircle(x, y, 3, color); sprite.drawCircle(x, y, 5, color); }
      else { sprite.fillCircle(x, y, 2, color); sprite.drawCircle(x, y, 4, color); }
      if (renderRadarRangeKm >= 50.0f || ap.primary) {
        sprite.setTextDatum(MC_DATUM);
        sprite.setTextColor(ap.primary ? C_AIRPORT_MAJOR : C_AIRPORT_LABEL);
        sprite.drawString(ap.code, x, y - 11, 1);
      }
    }
  }
}

static void rotatePoint(float x, float y, float scale, float angleRad, int cx, int cy, int &ox, int &oy) {
  x *= scale; y *= scale;
  float ca = cos(angleRad);
  float sa = sin(angleRad);
  ox = cx + (int)roundf(x * ca - y * sa);
  oy = cy + (int)roundf(x * sa + y * ca);
}

static void fillRotatedTriangle(int cx, int cy, float headingRad, float scale, float x1, float y1, float x2, float y2, float x3, float y3, uint16_t color) {
  int ax, ay, bx, by, cx2, cy2;
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
  if (active) drawThickPixel(cx, cy, color);
  else sprite.drawPixel(cx, cy, color);
}

static bool aircraftToScreen(const Aircraft &a, int &x, int &y) {
  if (!a.valid) return false;
  return polarToScreen(a.distanceKm, a.bearingDeg, renderRadarRangeKm, x, y);
}

static void drawAircraftTrails() {
  for (int i = 0; i < renderAircraftCount; i++) {
    Aircraft &a = renderAircraft[i];
    if (!a.valid || a.trailCount < 2) continue;
    if (!shouldDrawAircraftIndex(i)) continue;
    for (int n = a.trailCount - 1; n >= 1; n--) {
      int idx1 = a.trailHead - n;
      int idx2 = a.trailHead - n + 1;
      while (idx1 < 0) idx1 += MAX_TRAIL_POINTS;
      while (idx2 < 0) idx2 += MAX_TRAIL_POINTS;
      idx1 %= MAX_TRAIL_POINTS; idx2 %= MAX_TRAIL_POINTS;
      TrailPoint &p1 = a.trail[idx1];
      TrailPoint &p2 = a.trail[idx2];
      if (!p1.valid || !p2.valid) continue;
      int x1, y1, x2, y2;
      if (!polarToScreen(p1.distanceKm, p1.bearingDeg, renderRadarRangeKm, x1, y1)) continue;
      if (!polarToScreen(p2.distanceKm, p2.bearingDeg, renderRadarRangeKm, x2, y2)) continue;
      uint8_t amount = 22 + (uint8_t)((MAX_TRAIL_POINTS - n) * 7);
      if (amount > 210) amount = 210;
      uint16_t base = (i == renderSelectedIndex) ? C_ACTIVE : C_INACTIVE;
      uint16_t color = blend565(C_BG, base, amount);
      sprite.drawLine(x1, y1, x2, y2, color);
    }
  }
}

static bool zoneOverlaps(int x, int y, const DrawZone *zones, int zoneCount, int r) {
  for (int i = 0; i < zoneCount; i++) {
    int dx = x - zones[i].x;
    int dy = y - zones[i].y;
    int rr = r + zones[i].r;
    if ((dx * dx + dy * dy) <= (rr * rr)) return true;
  }
  return false;
}

static void addDrawZone(DrawZone *zones, int &zoneCount, int maxZones, int x, int y, int r) {
  if (zoneCount >= maxZones) return;
  zones[zoneCount].x = x;
  zones[zoneCount].y = y;
  zones[zoneCount].r = r;
  zoneCount++;
}

static void drawAircraft() {
  static DrawZone zones[MAX_AIRCRAFT];
  int zoneCount = 0;
  const int activeKeepout = 20;
  const int inactiveKeepout = 18;
  if (renderSelectedIndex >= 0 && renderSelectedIndex < renderAircraftCount && shouldDrawAircraftIndex(renderSelectedIndex)) {
    Aircraft &a = renderAircraft[renderSelectedIndex];
    int x, y;
    if (aircraftToScreen(a, x, y)) {
      drawAircraftIcon(x, y, a.headingDeg, C_ACTIVE, true);
      addDrawZone(zones, zoneCount, MAX_AIRCRAFT, x, y, activeKeepout);
    }
  }
  for (int i = 0; i < renderAircraftCount; i++) {
    if (i == renderSelectedIndex) continue;
    Aircraft &a = renderAircraft[i];
    if (!a.valid) continue;
    if (!shouldDrawAircraftIndex(i)) continue;
    int x, y;
    if (!aircraftToScreen(a, x, y)) continue;
    if (zoneOverlaps(x, y, zones, zoneCount, inactiveKeepout)) continue;
    drawAircraftIcon(x, y, a.headingDeg, C_INACTIVE, false);
    addDrawZone(zones, zoneCount, MAX_AIRCRAFT, x, y, inactiveKeepout);
  }
}

static void drawSelectedAircraftInfo() {
  sprite.setTextDatum(MC_DATUM);
  if (renderSelectedIndex < 0 || renderSelectedIndex >= renderAircraftCount || !isAircraftVisibleInRange(renderAircraft[renderSelectedIndex], renderRadarRangeKm)) {
    sprite.setTextColor(C_TEXT);
    sprite.drawString("NO AIRCRAFT", INFO_CX, INFO_Y1, 2);
    sprite.setTextColor(C_INFO_TEXT_DIM);
    sprite.drawString(apiHasEverSucceeded ? "NO TRAFFIC IN RANGE" : "WAITING FOR DATA", INFO_CX, INFO_Y2, 1);
    return;
  }
  Aircraft &a = renderAircraft[renderSelectedIndex];
  bool showFlightTemplate = ((millis() / INFO_TEMPLATE_INTERVAL_MS) % 2) == 0;
  int visibleRank = visibleRankForIndex(renderSelectedIndex);
  if (showFlightTemplate) {
    char line1[20], line2[28], line3[18];
    snprintf(line1, sizeof(line1), "%s", a.callsign);
    if (hasUsefulText(a.routeFrom) && hasUsefulText(a.routeTo)) snprintf(line2, sizeof(line2), "%s - %s", a.routeFrom, a.routeTo);
    else snprintf(line2, sizeof(line2), "%s %.1fkm", a.icao, a.distanceKm);
    snprintf(line3, sizeof(line3), "%d/%d", visibleRank, renderVisibleAircraftCount);
    sprite.setTextColor(C_ACTIVE); sprite.drawString(line1, INFO_CX, INFO_Y1, 2);
    sprite.setTextColor(C_TEXT); sprite.drawString(line2, INFO_CX, INFO_Y2, 2);
    sprite.setTextColor(C_INFO_TEXT_DIM); sprite.drawString(line3, INFO_CX, INFO_Y3, 1);
  } else {
    char altText[12], bestName[24], line2[34], line3[30];
    formatAltitudeShort(a.altitudeFt, altText, sizeof(altText));
    getBestAircraftDisplayName(a, bestName, sizeof(bestName));
    snprintf(line2, sizeof(line2), "%.1fkm  %s", a.distanceKm, altText);
    snprintf(line3, sizeof(line3), "%.0fkt  HDG %.0f", a.speedKts, a.headingDeg);
    sprite.setTextColor(C_ACTIVE); sprite.drawString(bestName, INFO_CX, INFO_Y1, 2);
    sprite.setTextColor(C_TEXT); sprite.drawString(line2, INFO_CX, INFO_Y2, 2);
    sprite.setTextColor(C_INFO_TEXT_DIM); sprite.drawString(line3, INFO_CX, INFO_Y3, 1);
  }
}

void drawDisplayFrame() {
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

void updateDisplay() {
  uint32_t now = millis();
  if (lastDisplayMs == 0 || now - lastDisplayMs >= DISPLAY_INTERVAL_MS) {
    lastDisplayMs = now;
    drawDisplayFrame();
  }
}
