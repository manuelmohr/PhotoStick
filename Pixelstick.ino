#include <Adafruit_GFX.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_ILI9341.h>
#include <Adafruit_STMPE610.h>
#include "SD.h"
#include "FastLED.h"

// This is calibration data for the raw touch data to the screen coordinates
#define TS_MINX 150
#define TS_MINY 130
#define TS_MAXX 3800
#define TS_MAXY 4000

// TFT display and SD card will share the hardware SPI interface.
// Hardware SPI pins are specific to the Arduino board type and
// cannot be remapped to alternate pins.  For Arduino Uno,
// Duemilanove, etc., pin 11 = MOSI, pin 12 = MISO, pin 13 = SCK.
// The display also uses hardware SPI, plus #9 & #10
#define TFT_CS 10
#define TFT_DC 9
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

// The STMPE610 uses hardware SPI on the shield, and #8
#define STMPE_CS 8
Adafruit_STMPE610 ts = Adafruit_STMPE610(STMPE_CS);

// SD card chip select
#define SD_CS 4

// How many leds in your strip?
#define NUM_LEDS 288

#define BMP_WIDTH 341

// For led chips like Neopixels, which have a data line, ground, and power, you just
// need to define DATA_PIN.  For led chipsets that are SPI based (four wires - data, clock,
// ground, and power), like the LPD8806 define both DATA_PIN and CLOCK_PIN
#define DATA_PIN 2

CRGB *leds = nullptr;
CRGB testLed;

struct BMPFile
{
  File     file;
  boolean  flip;
  uint32_t height;
  uint32_t imageOffset;
} bmpFile;

__attribute__((noreturn)) void panic(const __FlashStringHelper *pgstr)
{
  Serial.println(pgstr);
  while (1) {
  }
}

void setup(void)
{
  Serial.begin(9600);
  Serial.println(F("Pixelstick\n"));

  Serial.print(F("Initializing touchscreen..."));
  tft.begin();
  if (!ts.begin()) {
    panic(F("failed!"));
  }
  Serial.println(F("OK!"));

  Serial.print(F("Initializing SD card..."));
  if (!SD.begin(SD_CS)) {
    panic(F("failed!"));
  }
  Serial.println(F("OK!"));

  tft.fillScreen(ILI9341_BLACK);

//  FastLED.addLeds<WS2812B, DATA_PIN, RGB>(leds, NUM_LEDS);
//  FastLED.setBrightness(1);

  bmpOpen("TestPad.bmp");
  bmpLoadRow(0);
}

void loop()
{
}

void bmpOpen(const char *filename)
{
  Serial.println();
  Serial.print(F("Loading image "));
  Serial.println(filename);

  bmpFile.file = SD.open(filename);
  if (!bmpFile.file) {
    panic(F("File not found"));
  }

  // Parse BMP header
  if (bmpRead16() != 0x4D42) {
    panic(F("Invalid header"));
  }
  Serial.print(F("File size: "));
  Serial.println(bmpRead32());
  (void)bmpRead32(); // Ignore reserved word
  bmpFile.imageOffset = bmpRead32();
  Serial.print(F("Image Offset: "));
  Serial.println(bmpFile.imageOffset, DEC);

  // Read BMP Info header
  Serial.print(F("Header size: "));
  Serial.println(bmpRead32());

  if (bmpRead32() != BMP_WIDTH) {
    panic(F("Width must be 341"));
  }

  uint32_t height = bmpRead32();
  boolean  flip   = true;
  if (height < 0) {
    height = -height;
    flip   = false;
  }
  bmpFile.height = height;
  bmpFile.flip   = flip;

  if (bmpRead16() != 1) {
    panic(F("Planes must be 1"));
  }
  if (bmpRead16() != 16) {
    panic(F("Depth must be 16"));
  }
  const uint32_t depth = bmpRead32();
  if (depth != 0 && depth != 3) {
    panic(F("Must be uncompressed"));
  }

  // BMP rows are padded (if needed) to 4-byte boundary
  const uint32_t rowSize = (BMP_WIDTH * 3U + 3U) & ~3U;

  // If bmpHeight is negative, image is in top-down order.
  // This is not canon but has been observed in the wild.
  Serial.print(F("Image size: 341"));
  Serial.print('x');
  Serial.println(bmpFile.height);
  Serial.print(F("Row size: "));
  Serial.println(rowSize);
}

void bmpLoadRow(uint32_t row)
{
  const uint32_t rowSize = (BMP_WIDTH * 3U + 3U) & ~3U;

  // Seek to start of scan line.  It might seem labor-
  // intensive to be doing this on every line, but this
  // method covers a lot of gritty details like cropping
  // and scanline padding.  Also, the seek only takes
  // place if the file position actually needs to change
  // (avoids a lot of cluster math in SD library).
  uint32_t pos;
  if (bmpFile.flip) { // Normal case
    pos = bmpFile.imageOffset + (bmpFile.height - 1 - row) * rowSize;
  } else {
    pos = bmpFile.imageOffset + row * rowSize;
  }

  File& file = bmpFile.file;
  if (file.position() != pos) {
    file.seek(pos);
  }
  uint8_t *rawData = &SdVolume::cacheBuffer_.data[0];
  Serial.println(F("Values from SD card:"));
  for (uint16_t i = 0; i < 32; i += 2) {
    uint16_t c;
    c  = rawData[i+0] << 8U;
    c |= rawData[i+1] << 0U;

    Serial.print(i, HEX);
    Serial.print(F(": "));
    Serial.println(c, HEX);
  }
  for (uint16_t i = 500; i < 532; i += 2) {
    uint16_t c;
    c  = rawData[i+0] << 8U;
    c |= rawData[i+1] << 0U;

    Serial.print(i, HEX);
    Serial.print(F(": "));
    Serial.println(c, HEX);
  }

  file.close();
}

// These read 16- and 32-bit types from the SD card file.
// BMP data is stored little-endian, Arduino is little-endian too.
// May need to reverse subscript order if porting elsewhere.
uint16_t bmpRead16()
{
  File& f = bmpFile.file;
  uint16_t result;
  ((uint8_t*)&result)[0] = f.read(); // LSB
  ((uint8_t*)&result)[1] = f.read(); // MSB
  return result;
}

uint32_t bmpRead32()
{
  File& f = bmpFile.file;
  uint32_t result;
  ((uint8_t*)&result)[0] = f.read(); // LSB
  ((uint8_t*)&result)[1] = f.read();
  ((uint8_t*)&result)[2] = f.read();
  ((uint8_t*)&result)[3] = f.read(); // MSB
  return result;
}

// vim: et ts=2
