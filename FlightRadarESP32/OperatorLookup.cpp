#include "OperatorLookup.h"
#include <string.h>
#include <ctype.h>

struct OperatorPrefixEntry {
  const char *prefix;
  const char *name;
  const char *country;
};

struct AircraftTypeEntry {
  const char *code;
  const char *name;
};

static const OperatorPrefixEntry OPERATOR_TABLE[] = {
  { "AIC", "AIR INDIA", "INDIA" },
  { "IGO", "INDIGO", "INDIA" },
  { "SEJ", "SPICEJET", "INDIA" },
  { "AXB", "AIR INDIA X", "INDIA" },
  { "BDA", "BLUE DART", "INDIA" },
  { "VTI", "INDIAN GA", "INDIA" },
  { "VTF", "FLIGHT SCHOOL", "INDIA" },
  { "VTR", "INDIAN GA", "INDIA" },
  { "QTR", "QATAR", "QATAR" },
  { "KAC", "KUWAIT AIR", "KUWAIT" },
  { "FDB", "FLYDUBAI", "UAE" },
  { "ABY", "AIR ARABIA", "UAE" },
  { "GFA", "GULF AIR", "BAHRAIN" },
  { "ETD", "ETIHAD", "UAE" },
  { "UAE", "EMIRATES", "UAE" },
  { "BBC", "BIMAN", "BANGLADESH" },
  { "SIA", "SINGAPORE", "SINGAPORE" },
  { "THA", "THAI", "THAILAND" },
  { "AFL", "AEROFLOT", "RUSSIA" },
  { "CSS", "CHINA SOUTH", "CHINA" },
  { "CPA", "CATHAY", "HONG KONG" },
  { "BAW", "BRITISH AIR", "UK" },
  { "DLH", "LUFTHANSA", "GERMANY" },
  { "AFR", "AIR FRANCE", "FRANCE" },
  { "KLM", "KLM", "NETHERLANDS" }
};

static const AircraftTypeEntry AIRCRAFT_TYPE_TABLE[] = {
  { "A20N", "A320neo" },
  { "A21N", "A321neo" },
  { "A19N", "A319neo" },
  { "A320", "A320" },
  { "A321", "A321" },
  { "A319", "A319" },
  { "A359", "A350-900" },
  { "A35K", "A350-1000" },
  { "A388", "A380" },
  { "B38M", "737 MAX 8" },
  { "B39M", "737 MAX 9" },
  { "B737", "737-700" },
  { "B738", "737-800" },
  { "B739", "737-900" },
  { "B752", "757-200" },
  { "B763", "767-300" },
  { "B772", "777-200" },
  { "B77W", "777-300ER" },
  { "B788", "787-8" },
  { "B789", "787-9" },
  { "B78X", "787-10" },
  { "AT72", "ATR 72" },
  { "AT75", "ATR 72-500" },
  { "AT76", "ATR 72-600" },
  { "DH8D", "Dash 8 Q400" },
  { "E190", "E190" },
  { "E195", "E195" },
  { "C172", "Cessna 172" },
  { "C152", "Cessna 152" },
  { "P28A", "PA-28" }
};

static void copyText(char *dst, size_t dstSize, const char *src) {
  if (!dst || dstSize == 0) return;
  if (!src) src = "";
  strncpy(dst, src, dstSize - 1);
  dst[dstSize - 1] = '\0';
}

static void makePrefix(const char *callsign, char *prefix, size_t prefixSize) {
  if (!prefix || prefixSize == 0) return;
  prefix[0] = '\0';
  if (!callsign) return;

  size_t out = 0;
  for (size_t i = 0; callsign[i] && out < prefixSize - 1; i++) {
    char c = callsign[i];
    if (isalpha((unsigned char)c)) {
      prefix[out++] = (char)toupper((unsigned char)c);
    } else if (out > 0) {
      break;
    }
  }
  prefix[out] = '\0';
}

void inferOperatorFromCallsign(const char *callsign, char *operatorName, size_t operatorNameSize, char *operatorCountry, size_t operatorCountrySize) {
  copyText(operatorName, operatorNameSize, "");
  copyText(operatorCountry, operatorCountrySize, "");

  char prefix[8];
  makePrefix(callsign, prefix, sizeof(prefix));
  if (prefix[0] == '\0') return;

  for (size_t i = 0; i < sizeof(OPERATOR_TABLE) / sizeof(OPERATOR_TABLE[0]); i++) {
    if (strcmp(prefix, OPERATOR_TABLE[i].prefix) == 0) {
      copyText(operatorName, operatorNameSize, OPERATOR_TABLE[i].name);
      copyText(operatorCountry, operatorCountrySize, OPERATOR_TABLE[i].country);
      return;
    }
  }
}

void friendlyTypeFromCode(const char *typeCode, char *out, size_t outSize) {
  copyText(out, outSize, "");
  if (!typeCode || !typeCode[0]) return;

  char code[8];
  copyText(code, sizeof(code), typeCode);
  for (size_t i = 0; code[i]; i++) code[i] = (char)toupper((unsigned char)code[i]);

  for (size_t i = 0; i < sizeof(AIRCRAFT_TYPE_TABLE) / sizeof(AIRCRAFT_TYPE_TABLE[0]); i++) {
    if (strcmp(code, AIRCRAFT_TYPE_TABLE[i].code) == 0) {
      copyText(out, outSize, AIRCRAFT_TYPE_TABLE[i].name);
      return;
    }
  }

  copyText(out, outSize, code);
}
