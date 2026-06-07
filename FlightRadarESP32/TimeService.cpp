#include "TimeService.h"
#include "UserConfig.h"
#include "AppState.h"
#include "GeoMath.h"

void setupTimeSync() {
  configTime(DEFAULT_GMT_OFFSET_SEC, DEFAULT_DAYLIGHT_OFFSET_SEC, "pool.ntp.org", "time.nist.gov", "time.google.com");
  timeConfigured = true;
}

bool getLocalClock(struct tm &timeinfo) {
  if (!timeConfigured) return false;
  return getLocalTime(&timeinfo, 10);
}

bool isNightTime() {
  struct tm timeinfo;
  if (!getLocalClock(timeinfo)) return false;
  return timeinfo.tm_hour >= 19 || timeinfo.tm_hour < 5;
}

int getFiveMinuteSlot() {
  struct tm timeinfo;
  if (!getLocalClock(timeinfo)) return (millis() / 300000UL);
  return (timeinfo.tm_hour * 12) + (timeinfo.tm_min / 5);
}

float getFastSweepAngle() {
  uint32_t ms = millis() % 1000;
  return normalizeAngle((float)ms * 0.36f);
}
