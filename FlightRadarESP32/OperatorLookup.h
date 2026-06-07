#pragma once
#include <Arduino.h>

void inferOperatorFromCallsign(const char *callsign, char *operatorName, size_t operatorNameSize, char *operatorCountry, size_t operatorCountrySize);
void friendlyTypeFromCode(const char *typeCode, char *out, size_t outSize);
