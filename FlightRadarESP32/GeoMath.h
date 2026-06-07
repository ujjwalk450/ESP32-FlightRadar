#pragma once
#include <Arduino.h>

double degToRad(double deg);
double radToDeg(double rad);
float normalizeAngle(float deg);
double haversineKm(double lat1, double lon1, double lat2, double lon2);
float bearingDeg(double lat1, double lon1, double lat2, double lon2);
uint16_t blend565(uint16_t c1, uint16_t c2, uint8_t amount);
bool polarToScreen(float distanceKm, float bearingDegIn, float rangeKm, int &x, int &y);
