#include "GeoMath.h"
#include "BoardConfig.h"
#include <math.h>

double degToRad(double deg) {
  return deg * PI / 180.0;
}

double radToDeg(double rad) {
  return rad * 180.0 / PI;
}

float normalizeAngle(float deg) {
  while (deg < 0.0f) deg += 360.0f;
  while (deg >= 360.0f) deg -= 360.0f;
  return deg;
}

double haversineKm(double lat1, double lon1, double lat2, double lon2) {
  static const double EARTH_RADIUS_KM = 6371.0;
  double dLat = degToRad(lat2 - lat1);
  double dLon = degToRad(lon2 - lon1);
  double a = sin(dLat * 0.5) * sin(dLat * 0.5) +
             cos(degToRad(lat1)) * cos(degToRad(lat2)) *
             sin(dLon * 0.5) * sin(dLon * 0.5);
  double c = 2.0 * atan2(sqrt(a), sqrt(1.0 - a));
  return EARTH_RADIUS_KM * c;
}

float bearingDeg(double lat1, double lon1, double lat2, double lon2) {
  double p1 = degToRad(lat1);
  double p2 = degToRad(lat2);
  double dLon = degToRad(lon2 - lon1);
  double y = sin(dLon) * cos(p2);
  double x = cos(p1) * sin(p2) - sin(p1) * cos(p2) * cos(dLon);
  return normalizeAngle((float)radToDeg(atan2(y, x)));
}

uint16_t blend565(uint16_t c1, uint16_t c2, uint8_t amount) {
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

bool polarToScreen(float distanceKm, float bearingDegIn, float rangeKm, int &x, int &y) {
  if (rangeKm <= 0.1f) return false;
  if (distanceKm > rangeKm) return false;
  float r = (distanceKm / rangeKm) * RADAR_R;
  float a = degToRad(bearingDegIn - 90.0f);
  x = RADAR_CX + (int)roundf(cos(a) * r);
  y = RADAR_CY + (int)roundf(sin(a) * r);
  return true;
}
