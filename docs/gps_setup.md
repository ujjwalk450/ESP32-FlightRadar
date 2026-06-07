# GPS Setup

GPS support is optional.

Enable it in `UserConfig.h`:

```cpp
#define ENABLE_GPS 1
#define LOCATION_MODE LOCATION_MODE_GPS
#define GPS_RX_PIN 16
#define GPS_TX_PIN 17
#define GPS_BAUD 9600
```

Install `TinyGPSPlus` from Arduino Library Manager.

GPS can provide latitude, longitude, and UTC time. This project still asks you to manually set timezone because GPS does not know local civil timezone or daylight-saving rules.
