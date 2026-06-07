# Configuration Guide

All normal user configuration lives in `FlightRadarESP32/UserConfig.h`.

## Fixed location

Use this when the device stays in one city.

```cpp
#define LOCATION_MODE LOCATION_MODE_FIXED
static const double DEFAULT_USER_LAT = 28.494106080147343;
static const double DEFAULT_USER_LON = 77.13798165111254;
```

## Timezone

Set these manually.

```cpp
static const long DEFAULT_GMT_OFFSET_SEC = 19800;
static const int DEFAULT_DAYLIGHT_OFFSET_SEC = 0;
static const float DEFAULT_LOCAL_TIMEZONE_HOURS = 5.5f;
```

The seconds value is used by NTP. The hour value is used by sun/sunrise/sunset math.

## Airport markers

Use nearest commercial airports. Keep the list short.

```cpp
static const AirportMarker USER_AIRPORTS[] = {
  { "AAA", 12.345f, 67.890f, true, 0, 0 },
  { "BBB", 12.500f, 67.100f, false, 0, 0 }
};
```

The last two values are label/dot pixel offsets. Use them only if labels overlap aircraft info.
