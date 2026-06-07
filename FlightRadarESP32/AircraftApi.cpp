#include "AircraftApi.h"
#include "AppState.h"
#include "UserConfig.h"
#include "AircraftStore.h"
#include "TextUtils.h"
#include "RadarRenderer.h"
#include "OperatorLookup.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

void setupWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);
  WiFi.setSleep(false);
  wifiManager.setConfigPortalTimeout(WIFI_PORTAL_TIMEOUT_SEC);
  wifiManager.setConnectTimeout(WIFI_CONNECT_TIMEOUT_SEC);
  wifiManager.setWiFiAutoReconnect(true);
  bool connected = wifiManager.autoConnect(WIFI_PORTAL_SSID);
  if (!connected) {
    delay(500);
    ESP.restart();
  }
#if DEBUG_SERIAL
  Serial.print("WiFi connected. IP: ");
  Serial.print(WiFi.localIP());
  Serial.print(" RSSI: ");
  Serial.println(WiFi.RSSI());
#endif
}

bool ensureWiFiConnected() {
  if (WiFi.status() == WL_CONNECTED) return true;
  WiFi.reconnect();
  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 8000) delay(100);
  return WiFi.status() == WL_CONNECTED;
}

static void debugDnsTest(const char *host) {
#if DEBUG_SERIAL
  IPAddress ip;
  Serial.print("DNS ");
  Serial.print(host);
  Serial.print(": ");
  if (WiFi.hostByName(host, ip)) {
    Serial.println(ip);
  } else {
    Serial.println("FAILED");
  }
#else
  (void)host;
#endif
}

static void debugTlsConnectTest(const char *host) {
#if DEBUG_SERIAL
  releaseDisplaySpriteForNetwork("TLS test");

  Serial.print("Heap before TLS test: ");
  Serial.println(ESP.getFreeHeap());
  Serial.print("Largest block before TLS test: ");
  Serial.println(ESP.getMaxAllocHeap());

  WiFiClientSecure client;
  client.setInsecure();
  client.setTimeout(15000);
  client.setHandshakeTimeout(15000);
  Serial.print("TLS connect ");
  Serial.print(host);
  Serial.print(": ");
  if (client.connect(host, 443)) {
    Serial.println("OK");
    client.stop();
  } else {
    Serial.println("FAILED");
  }

  restoreDisplaySpriteAfterNetwork();
#else
  (void)host;
#endif
}

static bool httpGetJson(const char *url, DynamicJsonDocument &doc) {
  if (!ensureWiFiConnected()) {
#if DEBUG_SERIAL
    Serial.println("WiFi not connected before HTTP request");
#endif
    return false;
  }

#if DEBUG_SERIAL
  Serial.print("Free heap before HTTP: ");
  Serial.println(ESP.getFreeHeap());
  Serial.print("Largest free block before HTTP: ");
  Serial.println(ESP.getMaxAllocHeap());
#endif

  WiFiClientSecure client;
  client.setInsecure();
  client.setTimeout(15000);
  client.setHandshakeTimeout(15000);

  HTTPClient http;
  http.setTimeout(15000);
  http.setConnectTimeout(15000);
  http.setReuse(false);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

  if (!http.begin(client, url)) {
#if DEBUG_SERIAL
    Serial.println("HTTP begin failed");
#endif
    return false;
  }

  http.addHeader("User-Agent", "ESP32-FlightRadar/1.0");
  http.addHeader("Accept", "application/json");
  http.addHeader("Accept-Encoding", "identity");
  http.addHeader("Connection", "close");

  int code = http.GET();

#if DEBUG_SERIAL
  Serial.print("HTTP ");
  Serial.print(code);
  Serial.print(" ");
  Serial.println(url);
#endif

  if (code <= 0) {
#if DEBUG_SERIAL
    Serial.print("HTTPClient error: ");
    Serial.println(http.errorToString(code));
    Serial.print("Free heap after HTTP fail: ");
    Serial.println(ESP.getFreeHeap());
#endif
    http.end();
    return false;
  }

  if (code != HTTP_CODE_OK) {
#if DEBUG_SERIAL
    Serial.print("Unexpected HTTP response: ");
    Serial.println(code);
#endif
    http.end();
    return false;
  }

  String payload = http.getString();
  http.end();

#if DEBUG_SERIAL
  Serial.print("Payload bytes: ");
  Serial.println(payload.length());
  if (payload.length() > 0) {
    Serial.print("Payload head: ");
    Serial.println(payload.substring(0, min((unsigned int)80, (unsigned int)payload.length())));
  }
#endif

  if (payload.length() < 2) {
#if DEBUG_SERIAL
    Serial.println("Payload too short");
#endif
    return false;
  }

  doc.clear();
  DeserializationError error = deserializeJson(doc, payload);

  payload = String();

  if (error) {
#if DEBUG_SERIAL
    Serial.print("JSON parse failed: ");
    Serial.println(error.c_str());
    Serial.print("Free heap after JSON fail: ");
    Serial.println(ESP.getFreeHeap());
#endif
    return false;
  }

#if DEBUG_SERIAL
  Serial.print("Free heap after HTTP OK: ");
  Serial.println(ESP.getFreeHeap());
#endif

  return true;
}

bool fetchADSB() {
  char url[180];
  snprintf(url, sizeof(url), "%s%.6f/%.6f/%d", ADSB_BASE_URL, currentUserLat, currentUserLon, ADSB_RADIUS_NM);

#if DEBUG_SERIAL
  Serial.println("Fetching ADSB.lol...");
#endif

  bool ok = false;
  int count = 0;

  releaseDisplaySpriteForNetwork("HTTPS fetch");

  {
    DynamicJsonDocument doc(ADSB_JSON_DOC_BYTES);

    if (!httpGetJson(url, doc)) {
#if DEBUG_SERIAL
      Serial.println("ADSB.lol fetch failed");
#endif
    } else {
      JsonArray ac = doc["ac"].as<JsonArray>();

      if (ac.isNull()) {
#if DEBUG_SERIAL
        Serial.println("ADSB.lol: missing ac array");
#endif
      } else {
        for (int i = 0; i < MAX_AIRCRAFT; i++) clearAircraft(fetchAircraft[i]);

        for (JsonObject item : ac) {
          if (count >= MAX_AIRCRAFT) break;
          if (item["lat"].isNull() || item["lon"].isNull()) continue;

          float lat = item["lat"].as<float>();
          float lon = item["lon"].as<float>();

          char icao[9], callsign[12], registration[12], typeCode[8], model[22], operatorName[18], operatorCountry[18], routeFrom[6], routeTo[6];

          safeCopy(icao, sizeof(icao), item["hex"] | "");
          safeCopy(callsign, sizeof(callsign), firstJsonText(item, "flight", "call", "callsign", "squawk"));
          safeCopy(registration, sizeof(registration), firstJsonText(item, "r", "reg", "registration", "tail"));
          safeCopy(typeCode, sizeof(typeCode), firstJsonText(item, "t", "type", "aircraft_type", "typeDesignator"));
          friendlyTypeFromCode(typeCode, model, sizeof(model));
          if (textIsEmpty(model)) buildModelText(item, model, sizeof(model));
          inferOperatorFromCallsign(callsign, operatorName, sizeof(operatorName), operatorCountry, sizeof(operatorCountry));

          safeCopy(routeFrom, sizeof(routeFrom), firstJsonText(item, "from", "origin", "orig_iata", "orig_icao"));
          safeCopy(routeTo, sizeof(routeTo), firstJsonText(item, "to", "destination", "dest_iata", "dest_icao"));

          if (textIsEmpty(routeFrom) || textIsEmpty(routeTo)) {
            char parsedFrom[6], parsedTo[6];
            parseRouteCodes(
              firstJsonText(item, "route", "airport_codes_iata", "airport_codes_icao", "airport_codes"),
              parsedFrom,
              sizeof(parsedFrom),
              parsedTo,
              sizeof(parsedTo)
            );

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

          if (fillAircraft(fetchAircraft[count], icao, callsign, registration, typeCode, model, operatorName, operatorCountry, routeFrom, routeTo, lat, lon, heading, speed, altitude)) {
            count++;
          }
        }

        sortAircraftArray(fetchAircraft, count);
        commitFetchedAircraft(count);
        ok = (count > 0);
      }
    }
  }

#if DEBUG_SERIAL
  Serial.print("Free heap before sprite restore: ");
  Serial.println(ESP.getFreeHeap());
  Serial.print("Largest block before sprite restore: ");
  Serial.println(ESP.getMaxAllocHeap());
#endif

  restoreDisplaySpriteAfterNetwork();

#if DEBUG_SERIAL
  Serial.print("ADSB aircraft count: ");
  Serial.println(count);
#endif

  return ok;
}

bool fetchOpenSky() {
  double lamin = currentUserLat - OPENSKY_BOX_DEG;
  double lamax = currentUserLat + OPENSKY_BOX_DEG;
  double lomin = currentUserLon - OPENSKY_BOX_DEG;
  double lomax = currentUserLon + OPENSKY_BOX_DEG;

  char url[240];
  snprintf(
    url,
    sizeof(url),
    "%s?lamin=%.6f&lomin=%.6f&lamax=%.6f&lomax=%.6f",
    OPENSKY_BASE_URL,
    lamin,
    lomin,
    lamax,
    lomax
  );

#if DEBUG_SERIAL
  Serial.println("Fetching OpenSky...");
#endif

  bool ok = false;
  int count = 0;

  releaseDisplaySpriteForNetwork("HTTPS fetch");

  {
    DynamicJsonDocument doc(OPENSKY_JSON_DOC_BYTES);

    if (!httpGetJson(url, doc)) {
#if DEBUG_SERIAL
      Serial.println("OpenSky fetch failed");
#endif
    } else {
      JsonArray states = doc["states"].as<JsonArray>();

      if (states.isNull()) {
#if DEBUG_SERIAL
        Serial.println("OpenSky: missing states array");
#endif
      } else {
        for (int i = 0; i < MAX_AIRCRAFT; i++) clearAircraft(fetchAircraft[i]);

        for (JsonArray s : states) {
          if (count >= MAX_AIRCRAFT) break;
          if (s.size() < 17) continue;
          if (s[5].isNull() || s[6].isNull()) continue;

          char icao[9], callsign[12], operatorName[18], operatorCountry[18];

          safeCopy(icao, sizeof(icao), s[0] | "");
          safeCopy(callsign, sizeof(callsign), s[1] | "");
          inferOperatorFromCallsign(callsign, operatorName, sizeof(operatorName), operatorCountry, sizeof(operatorCountry));
          if (textIsEmpty(operatorCountry) && s.size() > 2 && !s[2].isNull()) {
            safeCopy(operatorCountry, sizeof(operatorCountry), s[2] | "");
          }

          float lon = s[5].as<float>();
          float lat = s[6].as<float>();

          float velocityMs = 0.0f;
          float heading = 0.0f;

          if (!s[9].isNull()) velocityMs = s[9].as<float>();
          if (!s[10].isNull()) heading = s[10].as<float>();

          float speedKts = velocityMs * 1.943844f;

          int altitudeFt = 0;
          if (!s[7].isNull()) altitudeFt = (int)(s[7].as<float>() * 3.28084f);
          else if (!s[13].isNull()) altitudeFt = (int)(s[13].as<float>() * 3.28084f);

          if (fillAircraft(fetchAircraft[count], icao, callsign, "", "", "", operatorName, operatorCountry, "--", "--", lat, lon, heading, speedKts, altitudeFt)) {
            count++;
          }
        }

        sortAircraftArray(fetchAircraft, count);
        commitFetchedAircraft(count);
        ok = (count > 0);
      }
    }
  }

#if DEBUG_SERIAL
  Serial.print("Free heap before sprite restore: ");
  Serial.println(ESP.getFreeHeap());
  Serial.print("Largest block before sprite restore: ");
  Serial.println(ESP.getMaxAllocHeap());
#endif

  restoreDisplaySpriteAfterNetwork();

#if DEBUG_SERIAL
  Serial.print("OpenSky aircraft count: ");
  Serial.println(count);
#endif

  return ok;
}

void aircraftUpdateTask(void *parameter) {
  uint32_t lastAdsbMs = 0;
  uint32_t lastOpenSkyMs = 0;
  uint32_t lastAdsbRetryMs = 0;
  bool localUsingOpenSky = false;
  bool diagnosticsDone = false;
  for (;;) {
    if (!diagnosticsDone && ensureWiFiConnected()) {
      diagnosticsDone = true;
      debugDnsTest("api.adsb.lol");
      debugDnsTest("opensky-network.org");
      debugTlsConnectTest("api.adsb.lol");
      debugTlsConnectTest("opensky-network.org");
    }
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
