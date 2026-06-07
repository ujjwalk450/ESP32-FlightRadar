#define USER_SETUP_INFO "GC9A01 ESP32-S3 FlightRadar Example"

#define GC9A01_DRIVER

#define TFT_WIDTH  240
#define TFT_HEIGHT 240

// Edit these for your ESP32-S3 board wiring.
#define TFT_MOSI 11
#define TFT_SCLK 12
#define TFT_CS   10
#define TFT_DC   9
#define TFT_RST  8

#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8

#define SPI_FREQUENCY 40000000
