#pragma once
#include "Types.h"

void clearAircraft(Aircraft &a);
void clearAllAircraftArrays();
int findAircraftByIcao(const Aircraft *arr, int count, const char *icao);
bool isAircraftVisibleInRange(const Aircraft &a, float rangeKm);
bool isAircraftInInnerClutterZone(const Aircraft &a, float rangeKm);
bool shouldDrawAircraftIndex(int index);
int countVisibleAircraftLocked();
int visibleRankForIndex(int targetIndex);
void getBestAircraftDisplayName(const Aircraft &a, char *out, size_t outSize);
int nextVisibleAircraftIndexLocked(int currentIndex);
void copyTrail(Aircraft &dst, const Aircraft &src);
void addTrailPointLocked(Aircraft &a);
bool fillAircraft(
  Aircraft &a,
  const char *icao,
  const char *callsign,
  const char *registration,
  const char *typeCode,
  const char *model,
  const char *operatorName,
  const char *operatorCountry,
  const char *routeFrom,
  const char *routeTo,
  float lat,
  float lon,
  float headingDeg,
  float speedKts,
  int altitudeFt
);
void sortAircraftArray(Aircraft *arr, int count);
void commitFetchedAircraft(int count);
void updateTrails();
void updateMapZoomCycle();
void updateSelectionCycle();
void snapshotAircraftForRender();
