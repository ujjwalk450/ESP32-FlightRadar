#include "TextUtils.h"
#include <ctype.h>
#include <string.h>

void safeCopy(char *dst, size_t dstSize, const char *src) {
  if (!dst || dstSize == 0) return;
  if (!src) {
    dst[0] = '\0';
    return;
  }
  strncpy(dst, src, dstSize - 1);
  dst[dstSize - 1] = '\0';
}

void trimText(char *s) {
  if (!s) return;
  int len = strlen(s);
  while (len > 0) {
    char c = s[len - 1];
    if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
      s[len - 1] = '\0';
      len--;
    } else {
      break;
    }
  }
  int start = 0;
  while (s[start] == ' ' || s[start] == '\t') start++;
  if (start > 0) memmove(s, s + start, strlen(s + start) + 1);
}

bool textIsEmpty(const char *s) {
  if (!s) return true;
  while (*s) {
    if (*s != ' ' && *s != '\t' && *s != '\r' && *s != '\n') return false;
    s++;
  }
  return true;
}

void uppercaseText(char *s) {
  if (!s) return;
  while (*s) {
    *s = (char)toupper((unsigned char)*s);
    s++;
  }
}

bool hasUsefulText(const char *s) {
  if (textIsEmpty(s)) return false;
  if (strcmp(s, "--") == 0) return false;
  if (strcmp(s, "MODEL --") == 0) return false;
  if (strcmp(s, "TYPE --") == 0) return false;
  return true;
}

void formatAltitudeShort(int altitudeFt, char *out, size_t outSize) {
  if (!out || outSize == 0) return;
  if (altitudeFt <= 0) snprintf(out, outSize, "--ft");
  else if (altitudeFt >= 10000) snprintf(out, outSize, "%.0fkft", altitudeFt / 1000.0f);
  else snprintf(out, outSize, "%dft", altitudeFt);
}

const char *jsonTextOrEmpty(JsonObject obj, const char *key) {
  if (!key) return "";
  JsonVariant v = obj[key];
  if (v.isNull()) return "";
  const char *s = v.as<const char *>();
  if (!s) return "";
  return s;
}

const char *firstJsonText(JsonObject obj, const char *k1, const char *k2, const char *k3, const char *k4) {
  const char *s = jsonTextOrEmpty(obj, k1); if (!textIsEmpty(s)) return s;
  s = jsonTextOrEmpty(obj, k2); if (!textIsEmpty(s)) return s;
  s = jsonTextOrEmpty(obj, k3); if (!textIsEmpty(s)) return s;
  s = jsonTextOrEmpty(obj, k4); if (!textIsEmpty(s)) return s;
  return "";
}

void parseRouteCodes(const char *route, char *from, size_t fromSize, char *to, size_t toSize) {
  if (from && fromSize > 0) from[0] = '\0';
  if (to && toSize > 0) to[0] = '\0';
  if (!route || !route[0]) return;
  char token[8];
  int tokenLen = 0;
  int found = 0;
  for (int i = 0; ; i++) {
    char c = route[i];
    bool isCodeChar = isalnum((unsigned char)c);
    if (isCodeChar && tokenLen < 5) token[tokenLen++] = (char)toupper((unsigned char)c);
    if (!isCodeChar || c == '\0') {
      if (tokenLen >= 2) {
        token[tokenLen] = '\0';
        if (found == 0) { safeCopy(from, fromSize, token); found++; }
        else if (found == 1) { safeCopy(to, toSize, token); found++; break; }
      }
      tokenLen = 0;
    }
    if (c == '\0') break;
  }
}

void buildModelText(JsonObject item, char *out, size_t outSize) {
  if (!out || outSize == 0) return;
  out[0] = '\0';
  const char *desc = firstJsonText(item, "desc", "description", "model", "aircraft");
  const char *type = firstJsonText(item, "t", "type", "aircraft_type", "typeDesignator");
  const char *reg  = firstJsonText(item, "r", "reg", "registration", "tail");
  if (!textIsEmpty(desc)) safeCopy(out, outSize, desc);
  else if (!textIsEmpty(type)) safeCopy(out, outSize, type);
  else if (!textIsEmpty(reg)) safeCopy(out, outSize, reg);
  else safeCopy(out, outSize, "");
  trimText(out);
  uppercaseText(out);
}
