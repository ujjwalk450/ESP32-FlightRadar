#pragma once
#include <Arduino.h>
#include <TFT_eSPI.h>
#include <WiFiManager.h>
#include "Types.h"
#include "UserConfig.h"

extern TFT_eSPI tft;
extern TFT_eSprite sprite;
extern WiFiManager wifiManager;
extern SemaphoreHandle_t aircraftMutex;

extern Aircraft aircraft[MAX_AIRCRAFT];
extern Aircraft fetchAircraft[MAX_AIRCRAFT];
extern Aircraft renderAircraft[MAX_AIRCRAFT];

extern int aircraftCount;
extern int renderAircraftCount;
extern int renderVisibleAircraftCount;
extern int selectedIndex;
extern int renderSelectedIndex;

extern float radarRangeKm;
extern float renderRadarRangeKm;
extern int mapZoomIndex;

extern volatile bool usingOpenSky;
extern volatile bool apiHasEverSucceeded;

extern uint32_t lastDisplayMs;
extern uint32_t lastTrailMs;
extern uint32_t lastSelectionSwitchMs;
extern uint32_t lastMapZoomMs;
extern float sweepAngleDeg;

extern bool timeConfigured;
extern double currentUserLat;
extern double currentUserLon;
extern bool currentLocationValid;

void initializeAppState();
