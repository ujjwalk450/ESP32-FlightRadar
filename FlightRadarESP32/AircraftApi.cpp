#include "AircraftApi.h"
#include "AppState.h"
#include "UserConfig.h"
#include "AircraftStore.h"
#include "TextUtils.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

void setupWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);
  wifiManager.setConfigPortalTimeout(WIFI_PORTAL_TIMEOUT_SEC);
  wifiManager.setConnectTimeout(WIFI_CONNECT_TIMEOUT_SEC);
  wifiManager.setWiFiAutoReconnect(true);
  bool connected = wifiManager.autoConnect(WIFI_PORTAL_SSID);
  if (!connected) {
    delay(500);
    ESP.restart();
  }
}

bool ensureWiFiConnected() {
  if (WiFi.status() == WL_CONNECTED) return true;
  WiFi.reconnect();
  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 8000) delay(100);
  return WiFi.status() == WL_CONNECTED;
}

static bool httpGetJson(const char *url, DynamicJsonDocument &doc) {
  if (!ensureWiFiConnected()) return false;
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.setTimeout(12000);
  http.setReuse(false);
  http.useHTTP10(true);
  if (!http.begin(client, url)) {
#if DEBUG_SERIAL
    Serial.println("HTTP begin failed");
#endif
    return false;
  }
  http.addHeader("User-Agent", "ESP32-FlightRadar/1.0");
  http.addHeader("Accept", "application/json");
  int code = http.GET();
#if DEBUG_SERIAL
  Serial.print("HTTP ");
  Serial.print(code);
  Serial.print(" ");
  Serial.println(url);
#endif
  if (code != HTTP_CODE_OK) {
    http.end();
    return false;
  }
  doc.clear();
  DeserializationError error = deserializeJson(doc, http.getStream());
  http.end();
  if (error) {
#if DEBUG_SERIAL
    Serial.print("JSON parse failed: ");
    Serial.println(error.c_str());
#endif
    return false;
  }
  return true;
}

bool fetchADSB() {
  char url[180];
  snprintf(url, sizeof(url), "%s%.6f/%.6f/%d", ADSB_BASE_URL, currentUserLat, currentUserLon, ADSB_RADIUS_NM);
  static DynamicJsonDocument doc(ADSB_JSON_DOC_BYTES);
#if DEBUG_SERIAL
  Serial.println("Fetching ADSB.lol...");
#endif
  if (!httpGetJson(url, doc)) {
#if DEBUG_SERIAL
    Serial.println("ADSB.lol fetch failed");
#endif
    return false;
  }
  JsonArray ac = doc["ac"].as<JsonArray>();
  if (ac.isNull()) {
#if DEBUG_SERIAL
    Serial.println("ADSB.lol: missing ac array");
#endif
    return false;
  }
  for (int i = 0; i < MAX_AIRCRAFT; i++) clearAircraft(fetchAircraft[i]);
  int count = 0;
  for (JsonObject item : ac) {
    if (count >= MAX_AIRCRAFT) break;
    if (item["lat"].isNull() || item["lon"].isNull()) continue;
    float lat = item["lat"].as<float>();
    float lon = item["lon"].as<float>();
    char icao[9], callsign[12], model[22], routeFrom[6], routeTo[6];
    safeCopy(icao, sizeof(icao), item["hex"] | "");
    safeCopy(callsign, sizeof(callsign), firstJsonText(item, "flight", "call", "callsign", "squawk"));
    buildModelText(item, model, sizeof(model));
    safeCopy(routeFrom, sizeof(routeFrom), firstJsonText(item, "from", "origin", "orig_iata", "orig_icao"));
    safeCopy(routeTo, sizeof(routeTo), firstJsonText(item, "to", "destination", "dest_iata", "dest_icao"));
    if (textIsEmpty(routeFrom) || textIsEmpty(routeTo)) {
      char parsedFrom[6], parsedTo[6];
      parseRouteCodes(firstJsonText(item, "route", "airport_codes_iata", "airport_codes_icao", "airport_codes"), parsedFrom, sizeof(parsedFrom), parsedTo, sizeof(parsedTo));
      if (textIsEmpty(routeFrom)) safeCopy(routeFrom, sizeof(routeFrom), parsedFrom);
      if (textIsEmpty(routeTo)) safeCopy(routeTo, sizeof(routeTo), parsedTo);
    }
    float heading = 0.0f;
    if (!item["track"].isNull()) heading = item["track"].as<float>();
    else if (!item["true_heading"].isNull()) heading = item["true_heading"].as<float>();
    else if (!item["mag_heading"].isNull()) heading = item["mag_heading"].as<float>();
    float speed = 0.0f;
    if (!item["gs"].isNull()) speed = item["gs"].as<float>();
    int altitude = 0;
    if (!item["alt_baro"].isNull() && item["alt_baro"].is<int>()) altitude = item["alt_baro"].as<int>();
    else if (!item["alt_geom"].isNull() && item["alt_geom"].is<int>()) altitude = item["alt_geom"].as<int>();
    if (fillAircraft(fetchAircraft[count], icao, callsign, model, routeFrom, routeTo, lat, lon, heading, speed, altitude)) count++;
  }
  sortAircraftArray(fetchAircraft, count);
  commitFetchedAircraft(count);
#if DEBUG_SERIAL
  Serial.print("ADSB aircraft count: ");
  Serial.println(count);
#endif
  return count > 0;
}

bool fetchOpenSky() {
  double lamin = currentUserLat - OPENSKY_BOX_DEG;
  double lamax = currentUserLat + OPENSKY_BOX_DEG;
  double lomin = currentUserLon - OPENSKY_BOX_DEG;
  double lomax = currentUserLon + OPENSKY_BOX_DEG;
  char url[240];
  snprintf(url, sizeof(url), "%s?lamin=%.6f&lomin=%.6f&lamax=%.6f&lomax=%.6f", OPENSKY_BASE_URL, lamin, lomin, lamax, lomax);
  static DynamicJsonDocument doc(OPENSKY_JSON_DOC_BYTES);
#if DEBUG_SERIAL
  Serial.println("Fetching OpenSky...");
#endif
  if (!httpGetJson(url, doc)) {
#if DEBUG_SERIAL
    Serial.println("OpenSky fetch failed");
#endif
    return false;
  }
  JsonArray states = doc["states"].as<JsonArray>();
  if (states.isNull()) {
#if DEBUG_SERIAL
    Serial.println("OpenSky: missing states array");
#endif
    return false;
  }
  for (int i = 0; i < MAX_AIRCRAFT; i++) clearAircraft(fetchAircraft[i]);
  int count = 0;
  for (JsonArray s : states) {
    if (count >= MAX_AIRCRAFT) break;
    if (s.size() < 17) continue;
    if (s[5].isNull() || s[6].isNull()) continue;
    char icao[9], callsign[12];
    safeCopy(icao, sizeof(icao), s[0] | "");
    safeCopy(callsign, sizeof(callsign), s[1] | "");
    float lon = s[5].as<float>();
    float lat = s[6].as<float>();
    float velocityMs = 0.0f, heading = 0.0f;
    if (!s[9].isNull()) velocityMs = s[9].as<float>();
    if (!s[10].isNull()) heading = s[10].as<float>();
    float speedKts = velocityMs * 1.943844f;
    int altitudeFt = 0;
    if (!s[7].isNull()) altitudeFt = (int)(s[7].as<float>() * 3.28084f);
    else if (!s[13].isNull()) altitudeFt = (int)(s[13].as<float>() * 3.28084f);
    if (fillAircraft(fetchAircraft[count], icao, callsign, "", "--", "--", lat, lon, heading, speedKts, altitudeFt)) count++;
  }
  sortAircraftArray(fetchAircraft, count);
  commitFetchedAircraft(count);
#if DEBUG_SERIAL
  Serial.print("OpenSky aircraft count: ");
  Serial.println(count);
#endif
  return count > 0;
}

void aircraftUpdateTask(void *parameter) {
  uint32_t lastAdsbMs = 0;
  uint32_t lastOpenSkyMs = 0;
  uint32_t lastAdsbRetryMs = 0;
  bool localUsingOpenSky = false;
  for (;;) {
    uint32_t now = millis();
    if (!ensureWiFiConnected()) {
      vTaskDelay(pdMS_TO_TICKS(3000));
      continue;
    }
    if (!localUsingOpenSky) {
      if (lastAdsbMs == 0 || now - lastAdsbMs >= ADSB_UPDATE_INTERVAL_MS) {
        lastAdsbMs = now;
        bool ok = fetchADSB();
        if (ok) { apiHasEverSucceeded = true; usingOpenSky = false; }
        else { localUsingOpenSky = true; usingOpenSky = true; lastOpenSkyMs = 0; lastAdsbRetryMs = now; }
      }
    } else {
      if (lastOpenSkyMs == 0 || now - lastOpenSkyMs >= OPENSKY_UPDATE_INTERVAL_MS) {
        lastOpenSkyMs = now;
        if (fetchOpenSky()) apiHasEverSucceeded = true;
      }
      if (now - lastAdsbRetryMs >= ADSB_RETRY_INTERVAL_MS) {
        lastAdsbRetryMs = now;
        bool ok = fetchADSB();
        if (ok) { apiHasEverSucceeded = true; localUsingOpenSky = false; usingOpenSky = false; lastAdsbMs = now; }
      }
    }
    vTaskDelay(pdMS_TO_TICKS(250));
  }
}
