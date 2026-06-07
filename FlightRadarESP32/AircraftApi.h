#pragma once
#include <Arduino.h>

void setupWiFi();
bool ensureWiFiConnected();
bool fetchADSB();
bool fetchOpenSky();
void aircraftUpdateTask(void *parameter);
