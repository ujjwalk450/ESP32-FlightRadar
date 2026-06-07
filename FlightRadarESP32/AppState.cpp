#include "AppState.h"
#include "BoardConfig.h"

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite sprite = TFT_eSprite(&tft);
WiFiManager wifiManager;
SemaphoreHandle_t aircraftMutex = NULL;

Aircraft aircraft[MAX_AIRCRAFT];
Aircraft fetchAircraft[MAX_AIRCRAFT];
Aircraft renderAircraft[MAX_AIRCRAFT];

int aircraftCount = 0;
int renderAircraftCount = 0;
int renderVisibleAircraftCount = 0;
int selectedIndex = -1;
int renderSelectedIndex = -1;

float radarRangeKm = ZOOM_LEVELS_KM[0];
float renderRadarRangeKm = ZOOM_LEVELS_KM[0];
int mapZoomIndex = 0;

volatile bool usingOpenSky = false;
volatile bool apiHasEverSucceeded = false;

uint32_t lastDisplayMs = 0;
uint32_t lastTrailMs = 0;
uint32_t lastSelectionSwitchMs = 0;
uint32_t lastMapZoomMs = 0;
float sweepAngleDeg = 0.0f;

bool timeConfigured = false;
double currentUserLat = DEFAULT_USER_LAT;
double currentUserLon = DEFAULT_USER_LON;
bool currentLocationValid = true;

void initializeAppState() {
  currentUserLat = DEFAULT_USER_LAT;
  currentUserLon = DEFAULT_USER_LON;
  currentLocationValid = true;
  radarRangeKm = ZOOM_LEVELS_KM[0];
  renderRadarRangeKm = ZOOM_LEVELS_KM[0];
  mapZoomIndex = 0;
  aircraftCount = 0;
  renderAircraftCount = 0;
  renderVisibleAircraftCount = 0;
  selectedIndex = -1;
  renderSelectedIndex = -1;
}
