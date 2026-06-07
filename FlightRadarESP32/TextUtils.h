#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>

void safeCopy(char *dst, size_t dstSize, const char *src);
void trimText(char *s);
bool textIsEmpty(const char *s);
void uppercaseText(char *s);
bool hasUsefulText(const char *s);
void formatAltitudeShort(int altitudeFt, char *out, size_t outSize);
const char *jsonTextOrEmpty(JsonObject obj, const char *key);
const char *firstJsonText(JsonObject obj, const char *k1, const char *k2, const char *k3, const char *k4);
void parseRouteCodes(const char *route, char *from, size_t fromSize, char *to, size_t toSize);
void buildModelText(JsonObject item, char *out, size_t outSize);
