#pragma once
#include <Arduino.h>

void gpsBegin();
void gpsUpdate();
bool gpsHasFix();
bool gpsApplyLocationIfAvailable();
