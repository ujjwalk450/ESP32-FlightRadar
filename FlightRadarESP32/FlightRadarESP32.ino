//====================================================
// FlightRadarESP32.ino
//====================================================
// Main Arduino sketch entry point.
// Edit UserConfig.h for location, timezone, GPS, airports, and update rates.
// Edit BoardConfig.h for board/backlight/display geometry.

#include <Arduino.h>
#include "AppState.h"
#include "BoardConfig.h"
#include "UserConfig.h"
#include "AircraftApi.h"
#include "AircraftStore.h"
#include "GPSProvider.h"
#include "RadarRenderer.h"
#include "TimeService.h"

void setup() {
  Serial.begin(115200);
  delay(100);

  initializeAppState();
  initDisplay();

  aircraftMutex = xSemaphoreCreateMutex();
  if (aircraftMutex == NULL) {
    tft.fillScreen(TFT_BLACK);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.drawString("MUTEX FAIL", 120, 120, 4);
    while (true) delay(1000);
  }

  clearAllAircraftArrays();
  gpsBegin();

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

void loop() {
  uint32_t now = millis();

  gpsUpdate();
#if ENABLE_GPS
  gpsApplyLocationIfAvailable();
#endif

  if (now - lastTrailMs >= TRAIL_INTERVAL_MS) {
    lastTrailMs = now;
    updateTrails();
  }

  updateMapZoomCycle();
  updateSelectionCycle();
  updateDisplay();

  delay(1);
}
