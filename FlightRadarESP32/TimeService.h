#pragma once
#include <Arduino.h>
#include <time.h>

void setupTimeSync();
bool getLocalClock(struct tm &timeinfo);
bool isNightTime();
int getFiveMinuteSlot();
float getFastSweepAngle();
