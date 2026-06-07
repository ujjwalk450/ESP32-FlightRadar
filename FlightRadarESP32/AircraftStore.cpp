#include "AircraftStore.h"
#include "AppState.h"
#include "UserConfig.h"
#include "GeoMath.h"
#include "TextUtils.h"
#include <string.h>

void clearAircraft(Aircraft &a) {
  memset(&a, 0, sizeof(Aircraft));
  a.valid = false;
  a.trailHead = 0;
  a.trailCount = 0;
  safeCopy(a.model, sizeof(a.model), "");
  safeCopy(a.routeFrom, sizeof(a.routeFrom), "--");
  safeCopy(a.routeTo, sizeof(a.routeTo), "--");
  for (int i = 0; i < MAX_TRAIL_POINTS; i++) a.trail[i].valid = false;
}

void clearAllAircraftArrays() {
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

int findAircraftByIcao(const Aircraft *arr, int count, const char *icao) {
  if (!icao || !icao[0]) return -1;
  for (int i = 0; i < count; i++) {
    if (arr[i].valid && strcmp(arr[i].icao, icao) == 0) return i;
  }
  return -1;
}

bool isAircraftVisibleInRange(const Aircraft &a, float rangeKm) {
  return a.valid && a.distanceKm <= rangeKm;
}

bool isAircraftInInnerClutterZone(const Aircraft &a, float rangeKm) {
  if (!a.valid) return false;
  if (rangeKm <= 50.0f) return false;
  return a.distanceKm <= (rangeKm / 5.0f);
}

static bool selectedAircraftIsInnerClutter() {
  if (renderSelectedIndex < 0 || renderSelectedIndex >= renderAircraftCount) return false;
  return isAircraftInInnerClutterZone(renderAircraft[renderSelectedIndex], renderRadarRangeKm);
}

bool shouldDrawAircraftIndex(int index) {
  if (index < 0 || index >= renderAircraftCount) return false;
  Aircraft &a = renderAircraft[index];
  if (!isAircraftVisibleInRange(a, renderRadarRangeKm)) return false;
  if (!isAircraftInInnerClutterZone(a, renderRadarRangeKm)) return true;
  if (index == renderSelectedIndex) return true;

  bool selectedInner = selectedAircraftIsInnerClutter();
  int allowedNonSelectedInner = selectedInner ? 1 : 2;
  int nonSelectedInnerSeen = 0;
  for (int i = 0; i < renderAircraftCount; i++) {
    if (i == renderSelectedIndex) continue;
    if (!isAircraftVisibleInRange(renderAircraft[i], renderRadarRangeKm)) continue;
    if (!isAircraftInInnerClutterZone(renderAircraft[i], renderRadarRangeKm)) continue;
    nonSelectedInnerSeen++;
    if (i == index) return nonSelectedInnerSeen <= allowedNonSelectedInner;
  }
  return false;
}

int countVisibleAircraftLocked() {
  int count = 0;
  for (int i = 0; i < aircraftCount; i++) {
    if (isAircraftVisibleInRange(aircraft[i], radarRangeKm)) count++;
  }
  return count;
}

int visibleRankForIndex(int targetIndex) {
  if (targetIndex < 0 || targetIndex >= renderAircraftCount) return 0;
  int rank = 0;
  for (int i = 0; i < renderAircraftCount; i++) {
    if (isAircraftVisibleInRange(renderAircraft[i], renderRadarRangeKm)) {
      rank++;
      if (i == targetIndex) return rank;
    }
  }
  return 0;
}

void getBestAircraftDisplayName(const Aircraft &a, char *out, size_t outSize) {
  if (!out || outSize == 0) return;
  if (hasUsefulText(a.model)) safeCopy(out, outSize, a.model);
  else if (hasUsefulText(a.callsign)) safeCopy(out, outSize, a.callsign);
  else if (hasUsefulText(a.icao)) safeCopy(out, outSize, a.icao);
  else safeCopy(out, outSize, "AIRCRAFT");
}

int nextVisibleAircraftIndexLocked(int currentIndex) {
  if (aircraftCount <= 0) return -1;
  int start = currentIndex;
  if (start < 0 || start >= aircraftCount) start = -1;
  for (int step = 1; step <= aircraftCount; step++) {
    int idx = (start + step) % aircraftCount;
    if (isAircraftVisibleInRange(aircraft[idx], radarRangeKm)) return idx;
  }
  return -1;
}

void copyTrail(Aircraft &dst, const Aircraft &src) {
  dst.trailHead = src.trailHead;
  dst.trailCount = src.trailCount;
  for (int i = 0; i < MAX_TRAIL_POINTS; i++) dst.trail[i] = src.trail[i];
}

void addTrailPointLocked(Aircraft &a) {
  if (!a.valid) return;
  a.trailHead = (a.trailHead + 1) % MAX_TRAIL_POINTS;
  a.trail[a.trailHead].lat = a.lat;
  a.trail[a.trailHead].lon = a.lon;
  a.trail[a.trailHead].distanceKm = a.distanceKm;
  a.trail[a.trailHead].bearingDeg = a.bearingDeg;
  a.trail[a.trailHead].valid = true;
  if (a.trailCount < MAX_TRAIL_POINTS) a.trailCount++;
}

bool fillAircraft(Aircraft &a, const char *icao, const char *callsign, const char *model, const char *routeFrom, const char *routeTo, float lat, float lon, float headingDeg, float speedKts, int altitudeFt) {
  if (lat < -90.0f || lat > 90.0f || lon < -180.0f || lon > 180.0f) return false;
  clearAircraft(a);
  safeCopy(a.icao, sizeof(a.icao), icao);
  safeCopy(a.callsign, sizeof(a.callsign), callsign);
  safeCopy(a.model, sizeof(a.model), model);
  safeCopy(a.routeFrom, sizeof(a.routeFrom), routeFrom);
  safeCopy(a.routeTo, sizeof(a.routeTo), routeTo);
  trimText(a.icao); trimText(a.callsign); trimText(a.model); trimText(a.routeFrom); trimText(a.routeTo);
  uppercaseText(a.icao); uppercaseText(a.callsign); uppercaseText(a.model); uppercaseText(a.routeFrom); uppercaseText(a.routeTo);
  if (textIsEmpty(a.icao)) safeCopy(a.icao, sizeof(a.icao), "UNKNOWN");
  if (textIsEmpty(a.callsign)) safeCopy(a.callsign, sizeof(a.callsign), "NO CALL");
  if (textIsEmpty(a.routeFrom)) safeCopy(a.routeFrom, sizeof(a.routeFrom), "--");
  if (textIsEmpty(a.routeTo)) safeCopy(a.routeTo, sizeof(a.routeTo), "--");
  a.lat = lat;
  a.lon = lon;
  a.headingDeg = normalizeAngle(headingDeg);
  a.speedKts = speedKts;
  a.altitudeFt = altitudeFt;
  a.distanceKm = (float)haversineKm(currentUserLat, currentUserLon, lat, lon);
  a.bearingDeg = bearingDeg(currentUserLat, currentUserLon, lat, lon);
  a.valid = true;
  return true;
}

void sortAircraftArray(Aircraft *arr, int count) {
  for (int i = 0; i < count - 1; i++) {
    for (int j = i + 1; j < count; j++) {
      if (arr[j].distanceKm < arr[i].distanceKm) {
        Aircraft temp = arr[i]; arr[i] = arr[j]; arr[j] = temp;
      }
    }
  }
}

void commitFetchedAircraft(int count) {
  if (count < 0) count = 0;
  if (count > MAX_AIRCRAFT) count = MAX_AIRCRAFT;
  sortAircraftArray(fetchAircraft, count);
  if (aircraftMutex) xSemaphoreTake(aircraftMutex, portMAX_DELAY);
  char oldSelectedIcao[9]; oldSelectedIcao[0] = '\0';
  if (selectedIndex >= 0 && selectedIndex < aircraftCount) safeCopy(oldSelectedIcao, sizeof(oldSelectedIcao), aircraft[selectedIndex].icao);
  for (int i = 0; i < count; i++) {
    int oldIndex = findAircraftByIcao(aircraft, aircraftCount, fetchAircraft[i].icao);
    if (oldIndex >= 0) copyTrail(fetchAircraft[i], aircraft[oldIndex]);
    else addTrailPointLocked(fetchAircraft[i]);
  }
  for (int i = 0; i < MAX_AIRCRAFT; i++) clearAircraft(aircraft[i]);
  for (int i = 0; i < count; i++) aircraft[i] = fetchAircraft[i];
  aircraftCount = count;
  if (aircraftCount <= 0) selectedIndex = -1;
  else {
    int newSelected = findAircraftByIcao(aircraft, aircraftCount, oldSelectedIcao);
    if (newSelected >= 0 && isAircraftVisibleInRange(aircraft[newSelected], radarRangeKm)) selectedIndex = newSelected;
    else selectedIndex = nextVisibleAircraftIndexLocked(-1);
  }
  if (aircraftMutex) xSemaphoreGive(aircraftMutex);
}

void updateTrails() {
  if (!aircraftMutex) return;
  xSemaphoreTake(aircraftMutex, portMAX_DELAY);
  for (int i = 0; i < aircraftCount; i++) addTrailPointLocked(aircraft[i]);
  xSemaphoreGive(aircraftMutex);
}

void updateMapZoomCycle() {
  if (!aircraftMutex) return;
  uint32_t now = millis();
  if (lastMapZoomMs == 0) { lastMapZoomMs = now; return; }
  if (now - lastMapZoomMs < MAP_ZOOM_INTERVAL_MS) return;
  lastMapZoomMs = now;
  xSemaphoreTake(aircraftMutex, portMAX_DELAY);
  mapZoomIndex++;
  if (mapZoomIndex >= ZOOM_LEVEL_COUNT) mapZoomIndex = 0;
  radarRangeKm = ZOOM_LEVELS_KM[mapZoomIndex];
  if (selectedIndex < 0 || selectedIndex >= aircraftCount || !isAircraftVisibleInRange(aircraft[selectedIndex], radarRangeKm)) {
    selectedIndex = nextVisibleAircraftIndexLocked(-1);
    lastSelectionSwitchMs = now;
  }
  xSemaphoreGive(aircraftMutex);
}

void updateSelectionCycle() {
  if (!aircraftMutex) return;
  uint32_t now = millis();
  xSemaphoreTake(aircraftMutex, portMAX_DELAY);
  if (aircraftCount <= 0) {
    selectedIndex = -1;
    lastSelectionSwitchMs = now;
  } else {
    if (selectedIndex < 0 || selectedIndex >= aircraftCount || !isAircraftVisibleInRange(aircraft[selectedIndex], radarRangeKm)) {
      selectedIndex = nextVisibleAircraftIndexLocked(-1);
      lastSelectionSwitchMs = now;
    } else if (now - lastSelectionSwitchMs >= SELECT_SWITCH_INTERVAL_MS) {
      selectedIndex = nextVisibleAircraftIndexLocked(selectedIndex);
      lastSelectionSwitchMs = now;
    }
  }
  xSemaphoreGive(aircraftMutex);
}

void snapshotAircraftForRender() {
  if (!aircraftMutex) return;
  xSemaphoreTake(aircraftMutex, portMAX_DELAY);
  renderAircraftCount = aircraftCount;
  renderSelectedIndex = selectedIndex;
  renderRadarRangeKm = radarRangeKm;
  renderVisibleAircraftCount = countVisibleAircraftLocked();
  for (int i = 0; i < MAX_AIRCRAFT; i++) renderAircraft[i] = aircraft[i];
  xSemaphoreGive(aircraftMutex);
}
