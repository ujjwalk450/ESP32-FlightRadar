#include "Astronomy.h"
#include "AppState.h"
#include "GeoMath.h"
#include "TimeService.h"
#include "UserConfig.h"
#include <math.h>
#include <time.h>

bool getSunData(SunData &sun) {
  struct tm timeinfo;
  if (!getLocalClock(timeinfo)) return false;
  int dayOfYear = timeinfo.tm_yday + 1;
  float hourDecimal = (float)timeinfo.tm_hour + ((float)timeinfo.tm_min / 60.0f) + ((float)timeinfo.tm_sec / 3600.0f);
  float gamma = (2.0f * PI / 365.0f) * ((float)dayOfYear - 1.0f + ((hourDecimal - 12.0f) / 24.0f));
  float eqTime = 229.18f * (0.000075f + 0.001868f * cos(gamma) - 0.032077f * sin(gamma) - 0.014615f * cos(2.0f * gamma) - 0.040849f * sin(2.0f * gamma));
  float decl = 0.006918f - 0.399912f * cos(gamma) + 0.070257f * sin(gamma) - 0.006758f * cos(2.0f * gamma) + 0.000907f * sin(2.0f * gamma) - 0.002697f * cos(3.0f * gamma) + 0.001480f * sin(3.0f * gamma);
  float timeOffset = eqTime + (4.0f * (float)currentUserLon) - (60.0f * DEFAULT_LOCAL_TIMEZONE_HOURS);
  float trueSolarTime = (hourDecimal * 60.0f) + timeOffset;
  while (trueSolarTime < 0.0f) trueSolarTime += 1440.0f;
  while (trueSolarTime >= 1440.0f) trueSolarTime -= 1440.0f;
  float hourAngleDeg = (trueSolarTime / 4.0f) - 180.0f;
  float hourAngleRad = degToRad(hourAngleDeg);
  float latRad = degToRad(currentUserLat);
  float cosZenith = sin(latRad) * sin(decl) + cos(latRad) * cos(decl) * cos(hourAngleRad);
  if (cosZenith > 1.0f) cosZenith = 1.0f;
  if (cosZenith < -1.0f) cosZenith = -1.0f;
  float zenithRad = acos(cosZenith);
  sun.elevationDeg = 90.0f - (float)radToDeg(zenithRad);
  float azRad = atan2(sin(hourAngleRad), (cos(hourAngleRad) * sin(latRad)) - (tan(decl) * cos(latRad)));
  sun.azimuthDeg = normalizeAngle((float)radToDeg(azRad) + 180.0f);
  return true;
}

bool getSolarEventsForDayOffset(int dayOffset, float &sunriseMin, float &sunsetMin) {
  struct tm timeinfo;
  if (!getLocalClock(timeinfo)) return false;
  int dayOfYear = timeinfo.tm_yday + 1 + dayOffset;
  while (dayOfYear < 1) dayOfYear += 365;
  while (dayOfYear > 365) dayOfYear -= 365;
  float gamma = (2.0f * PI / 365.0f) * ((float)dayOfYear - 1.0f);
  float eqTime = 229.18f * (0.000075f + 0.001868f * cos(gamma) - 0.032077f * sin(gamma) - 0.014615f * cos(2.0f * gamma) - 0.040849f * sin(2.0f * gamma));
  float decl = 0.006918f - 0.399912f * cos(gamma) + 0.070257f * sin(gamma) - 0.006758f * cos(2.0f * gamma) + 0.000907f * sin(2.0f * gamma) - 0.002697f * cos(3.0f * gamma) + 0.001480f * sin(3.0f * gamma);
  float latRad = degToRad(currentUserLat);
  float zenithRad = degToRad(90.833f);
  float haArg = (cos(zenithRad) / (cos(latRad) * cos(decl))) - (tan(latRad) * tan(decl));
  if (haArg < -1.0f) haArg = -1.0f;
  if (haArg > 1.0f) haArg = 1.0f;
  float hourAngleDeg = (float)radToDeg(acos(haArg));
  float solarNoonMin = 720.0f - (4.0f * (float)currentUserLon) - eqTime + (60.0f * DEFAULT_LOCAL_TIMEZONE_HOURS);
  sunriseMin = solarNoonMin - (4.0f * hourAngleDeg);
  sunsetMin  = solarNoonMin + (4.0f * hourAngleDeg);
  while (sunriseMin < 0.0f) sunriseMin += 1440.0f;
  while (sunriseMin >= 1440.0f) sunriseMin -= 1440.0f;
  while (sunsetMin < 0.0f) sunsetMin += 1440.0f;
  while (sunsetMin >= 1440.0f) sunsetMin -= 1440.0f;
  return true;
}

bool getNightProgress01(float &progress) {
  struct tm timeinfo;
  if (!getLocalClock(timeinfo)) return false;
  float nowMin = ((float)timeinfo.tm_hour * 60.0f) + (float)timeinfo.tm_min + ((float)timeinfo.tm_sec / 60.0f);
  float sunriseToday, sunsetToday;
  if (!getSolarEventsForDayOffset(0, sunriseToday, sunsetToday)) return false;
  if (nowMin >= sunsetToday) {
    float sunriseTomorrow, sunsetTomorrow;
    if (!getSolarEventsForDayOffset(1, sunriseTomorrow, sunsetTomorrow)) return false;
    float nightLength = (1440.0f - sunsetToday) + sunriseTomorrow;
    if (nightLength <= 1.0f) return false;
    progress = (nowMin - sunsetToday) / nightLength;
    return true;
  }
  if (nowMin < sunriseToday) {
    float sunriseYesterday, sunsetYesterday;
    if (!getSolarEventsForDayOffset(-1, sunriseYesterday, sunsetYesterday)) return false;
    float nightLength = (1440.0f - sunsetYesterday) + sunriseToday;
    if (nightLength <= 1.0f) return false;
    progress = ((1440.0f - sunsetYesterday) + nowMin) / nightLength;
    return true;
  }
  return false;
}

float getMoonPhase01() {
  time_t nowUtc = time(nullptr);
  if (nowUtc < 100000) return 0.0f;
  const double SYNODIC_MONTH = 29.53058867;
  const time_t KNOWN_NEW_MOON_UTC = 947182440;
  double days = (double)(nowUtc - KNOWN_NEW_MOON_UTC) / 86400.0;
  double phase = fmod(days, SYNODIC_MONTH);
  if (phase < 0.0) phase += SYNODIC_MONTH;
  return (float)(phase / SYNODIC_MONTH);
}

float getEstimatedMoonAzimuthFromNightPath() {
  float progress = 0.0f;
  if (!getNightProgress01(progress)) return 180.0f;
  if (progress < 0.0f) progress = 0.0f;
  if (progress > 1.0f) progress = 1.0f;
  float phase = getMoonPhase01();
  float phaseOffset = (phase - 0.5f) * 24.0f;
  return normalizeAngle(90.0f + (progress * 180.0f) + phaseOffset);
}
