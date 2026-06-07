#pragma once
#include <Arduino.h>
#include "Types.h"

bool getSunData(SunData &sun);
bool getSolarEventsForDayOffset(int dayOffset, float &sunriseMin, float &sunsetMin);
bool getNightProgress01(float &progress);
float getMoonPhase01();
float getEstimatedMoonAzimuthFromNightPath();
