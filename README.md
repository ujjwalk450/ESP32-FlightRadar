# ESP32 FlightRadar Round Display

A small FlightRadar24-style radar display for ESP32 + 1.28 inch GC9A01 round TFT.

The project started as a single Arduino `.ino` sketch and is now split into small Arduino IDE friendly modules so different users can change location, timezone, airports, GPS, board pins, and display setup without touching the radar logic.

## Features

- ESP32 DevKit V1 default target
- Waveshare 1.28 inch GC9A01 round TFT, 240 x 240
- TFT_eSPI rendering with 8-bit `TFT_eSprite` framebuffer
- WiFiManager auto portal
- ADSB.lol primary aircraft source
- OpenSky fallback
- Nearest-aircraft selection and rotating aircraft info
- Auto zoom cycling from 25 km to 250 km
- Aircraft trails
- Aircraft icon keep-out/deconfliction to reduce blobs around airports
- Airport markers from `UserConfig.h`
- Day/night decorative sky layer
- Sun position from local solar math
- Moon phase and estimated night-path position
- Optional GPS location support through TinyGPSPlus

## Hardware

Default tested hardware:

- ESP32 DevKit V1
- Waveshare 1.28 inch GC9A01 round LCD
- No touch

Default wiring:

| GC9A01 | ESP32 DevKit V1 |
|---|---|
| VCC | 3V3 |
| GND | GND |
| DIN | GPIO23 |
| CLK | GPIO18 |
| CS | GPIO5 |
| DC | GPIO27 |
| RST | GPIO33 |
| BL | GPIO22 |

## Required Arduino libraries

Install these from Arduino Library Manager:

- TFT_eSPI
- WiFiManager
- ArduinoJson

Optional GPS mode:

- TinyGPSPlus

## TFT_eSPI setup

TFT_eSPI must be configured for GC9A01.

Copy this file:

```text
TFT_eSPI_Setups/User_Setup_GC9A01_ESP32_DevKitV1.h
```

into your TFT_eSPI library as:

```text
TFT_eSPI/User_Setup.h
```

For ESP32-S3, use the S3 example setup and edit the pins.

## Quick start

1. Open `FlightRadarESP32/FlightRadarESP32.ino` in Arduino IDE.
2. Install required libraries.
3. Copy the correct TFT_eSPI setup file.
4. Select your ESP32 board.
5. Upload.
6. On first boot, connect to WiFi portal `FlightRadarESP32`.

## Configure your city

Edit:

```text
FlightRadarESP32/UserConfig.h
```

Important fields:

```cpp
#define LOCATION_MODE LOCATION_MODE_FIXED

static const double DEFAULT_USER_LAT = 28.494106080147343;
static const double DEFAULT_USER_LON = 77.13798165111254;

static const long DEFAULT_GMT_OFFSET_SEC = 19800;
static const int DEFAULT_DAYLIGHT_OFFSET_SEC = 0;
static const float DEFAULT_LOCAL_TIMEZONE_HOURS = 5.5f;
```

Examples:

| Offset | Seconds |
|---|---:|
| UTC | 0 |
| India UTC+5:30 | 19800 |
| UTC+1 | 3600 |
| UTC-5 | -18000 |

## Configure airports

Edit this array in `UserConfig.h`:

```cpp
static const AirportMarker USER_AIRPORTS[] = {
  { "IGI", 28.556162f, 77.100281f, true, 0, 0 },
  { "AGR", 27.155800f, 77.960900f, false, 20, 10 },
};
```

Format:

```cpp
{ "CODE", latitude, longitude, primaryAirport, xOffsetPixels, yOffsetPixels }
```

Keep 3 to 8 airports for readability.

The `xOffsetPixels` and `yOffsetPixels` fields are visual offsets only. Use them when a label overlaps aircraft text. They do not change actual distance/bearing calculations.

## Optional GPS

Set in `UserConfig.h`:

```cpp
#define ENABLE_GPS 1
#define LOCATION_MODE LOCATION_MODE_GPS
#define GPS_RX_PIN 16
#define GPS_TX_PIN 17
#define GPS_BAUD 9600
```

GPS can provide live latitude/longitude. You should still configure timezone manually because GPS gives UTC time, not local timezone rules.

## File layout

```text
FlightRadarESP32/
├── FlightRadarESP32.ino     Main Arduino entry point
├── UserConfig.h             Location, timezone, airports, update rates
├── BoardConfig.h            Board/display constants and colors
├── Types.h                  Shared structs
├── AppState.*               Global state and shared buffers
├── GeoMath.*                Bearing, distance, projection, color blend
├── TextUtils.*              Fixed char buffer and JSON helpers
├── TimeService.*            NTP/local time helpers
├── Astronomy.*              Sun/moon calculations
├── Constellations.*         Decorative night sky data
├── AircraftStore.*          Aircraft arrays, trails, selection, zoom
├── AircraftApi.*            WiFi, ADSB.lol, OpenSky
├── GPSProvider.*            Optional GPS support
└── RadarRenderer.*          TFT drawing code
```

## Memory profile

This build uses `MAX_TRAIL_POINTS = 16` and `TRAIL_INTERVAL_MS = 1000`, giving about 16 seconds of trail history while keeping enough contiguous heap for HTTPS on classic ESP32 boards. `FREE_DISPLAY_SPRITE_DURING_HTTPS` is disabled by default to avoid the screen switching to the FETCHING page every 10 seconds. If HTTPS fails on a very memory-constrained board, enable it in `FlightRadarESP32/UserConfig.h`.

## Troubleshooting

### Display is black

- Confirm TFT_eSPI setup has `GC9A01_DRIVER`.
- Confirm backlight pin matches `TFT_BL` in `BoardConfig.h`.
- Confirm display wiring.
- Test with a simple TFT_eSPI GC9A01 fill-screen sketch first.

### `SPRITE FAIL`

The framebuffer could not be allocated.

Try:

- Keep sprite color depth at 8-bit.
- Use ESP32-S3 with PSRAM if adding more graphics.
- Reduce `MAX_TRAIL_POINTS`.
- Reduce `MAX_AIRCRAFT`.

### No aircraft

- Check WiFi is connected.
- ADSB.lol may have temporary outages.
- OpenSky fallback is slower and may return less data.
- Increase range/zoom or wait near a busier airspace.

### Wrong sun/moon position

- Check latitude/longitude.
- Check timezone values.
- GPS mode still needs timezone configuration.

## Notes on ESP32-S3

ESP32-S3 should work, but edit:

- TFT_eSPI setup pins
- `TFT_BL` in `BoardConfig.h`
- GPS UART pins if used

If using PSRAM, keep the sprite 8-bit unless you know you have enough memory for 16-bit.

## Roadmap

- Web config portal fields for latitude/longitude/timezone
- Store config in NVS/LittleFS
- PC-side nearest-airport generator
- PlatformIO support
- Better true moon azimuth/elevation calculation
- More display themes

## Aircraft type and operator display

This build extracts extra aircraft metadata from ADSB.lol when available:

- `t` is stored as the aircraft type code, for example `A20N`, `A21N`, `B38M`, `B752`.
- `r` is stored as the aircraft registration, for example `VT-TQQ` or `9K-AQA`.
- Common callsign prefixes are mapped locally to operator names, for example `IGO` → `INDIGO`, `AIC` → `AIR INDIA`, `KAC` → `KUWAIT AIR`.

OpenSky fallback does not provide aircraft type or registration in the normal states API response, but it does provide `origin_country`, which is displayed when ADSB.lol is not available.

To add more airline/operator mappings, edit:

```text
FlightRadarESP32/OperatorLookup.cpp
```
