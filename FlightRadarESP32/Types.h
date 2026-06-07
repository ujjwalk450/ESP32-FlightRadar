#pragma once
#include <Arduino.h>

struct AirportMarker {
  const char *code;
  float lat;
  float lon;
  bool primary;
  int8_t drawOffsetX;
  int8_t drawOffsetY;
};

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

struct TrailPoint {
  float distanceKm;
  float bearingDeg;
  bool valid;
};

#ifndef MAX_TRAIL_POINTS
#define MAX_TRAIL_POINTS 16
#endif

struct Aircraft {
  char icao[9];
  char callsign[12];
  char registration[12];
  char typeCode[8];
  char model[22];
  char operatorName[18];
  char operatorCountry[18];
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
