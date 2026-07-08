// OpenDrift — Waveshare ESP32-S3-Touch-LCD-1.28 (GC9A01A)
#define USER_SETUP_ID 302
#define USER_SETUP_INFO "OpenDrift ESP32-S3-Touch-LCD-1.28"

#define GC9A01_DRIVER

#define TFT_WIDTH 240
#define TFT_HEIGHT 240

#define TFT_MISO 12
#define TFT_MOSI 11
#define TFT_SCLK 10
#define TFT_CS 9
#define TFT_DC 8
#define TFT_RST 14
#define TFT_BL 2
#define TFT_BACKLIGHT_ON HIGH

#define USE_HSPI_PORT
#define SPI_FREQUENCY 80000000

#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_GFXFF
#define SMOOTH_FONT
