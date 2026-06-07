#include "GPSProvider.h"
#include "UserConfig.h"
#include "AppState.h"

#if ENABLE_GPS
#include <TinyGPSPlus.h>
static TinyGPSPlus gps;
static HardwareSerial gpsSerial(1);
#endif

void gpsBegin() {
#if ENABLE_GPS
  gpsSerial.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
#endif
}

void gpsUpdate() {
#if ENABLE_GPS
  while (gpsSerial.available()) {
    gps.encode(gpsSerial.read());
  }
#endif
}

bool gpsHasFix() {
#if ENABLE_GPS
  return gps.location.isValid() && gps.location.age() < 10000;
#else
  return false;
#endif
}

bool gpsApplyLocationIfAvailable() {
#if ENABLE_GPS
  if (!gpsHasFix()) return false;
  currentUserLat = gps.location.lat();
  currentUserLon = gps.location.lng();
  currentLocationValid = true;
  return true;
#else
  return false;
#endif
}
